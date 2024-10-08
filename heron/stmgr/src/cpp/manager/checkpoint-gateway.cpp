/*
 * Copyright 2015 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "manager/checkpoint-gateway.h"
#include <functional>
#include <iostream>
#include <deque>
#include <map>
#include <set>
#include <vector>
#include "manager/stateful-helper.h"
#include "metrics/metrics.h"
#include "proto/messages.h"
#include "basics/basics.h"
#include "errors/errors.h"
#include "threads/threads.h"
#include "network/network.h"

namespace heron {
namespace stmgr {

CheckpointGateway::CheckpointGateway(sp_uint64 _drain_threshold,
             StatefulHelper* _stateful_helper,
             common::MetricsMgrSt* _metrics_manager_client,
             std::function<void(sp_int32, proto::system::HeronTupleSet2*)> _drainer1,
             std::function<void(proto::stmgr::TupleStreamMessage2*)> _drainer2,
             std::function<void(sp_int32, proto::ckptmgr::InitiateStatefulCheckpoint*)> _drainer3) {
  drain_threshold_ = _drain_threshold;
  current_size_ = 0;
  stateful_helper_ = _stateful_helper;
  drainer1_ = _drainer1;
  drainer2_ = _drainer2;
  drainer3_ = _drainer3;
  metrics_manager_client_ = _metrics_manager_client;
  size_metric_ = new common::AssignableMetric(current_size_);
  metrics_manager_client_->register_metric("__stateful_gateway_size", size_metric_);
}

CheckpointGateway::~CheckpointGateway() {
  for (auto kv : pending_tuples_) {
    delete kv.second;
  }
  metrics_manager_client_->unregister_metric("__stateful_gateway_size");
  delete size_metric_;
}

void CheckpointGateway::SendToInstance(sp_int32 _task_id,
                                       proto::system::HeronTupleSet2* _message) {
  if (current_size_ > drain_threshold_) {
    ForceDrain();
  }
  CheckpointInfo* info = get_info(_task_id);
  sp_uint64 size = _message->GetCachedSize();
  _message = info->SendToInstance(_message, size);
  if (!_message) {
    current_size_ += size;
  } else {
    drainer1_(_task_id, _message);
  }
  size_metric_->SetValue(current_size_);
}

void CheckpointGateway::SendToInstance(proto::stmgr::TupleStreamMessage2* _message) {
  if (current_size_ > drain_threshold_) {
    ForceDrain();
  }
  sp_int32 task_id = _message->task_id();
  sp_uint64 size = _message->set().size();
  CheckpointInfo* info = get_info(task_id);
  _message = info->SendToInstance(_message, size);
  if (!_message) {
    current_size_ += size;
  } else {
    drainer2_(_message);
  }
  size_metric_->SetValue(current_size_);
}

void CheckpointGateway::HandleUpstreamMarker(sp_int32 _src_task_id, sp_int32 _destination_task_id,
                                             const sp_string& _checkpoint_id) {
  LOG(INFO) << "Got checkpoint marker for triplet "
            << _checkpoint_id << " " << _src_task_id << " " << _destination_task_id;
  CheckpointInfo* info = get_info(_destination_task_id);
  sp_uint64 size = 0;
  std::deque<Tuple> tuples = info->HandleUpstreamMarker(_src_task_id, _checkpoint_id, &size);
  for (auto tupl : tuples) {
    DrainTuple(_destination_task_id, tupl);
  }
  current_size_ -= size;
  size_metric_->SetValue(current_size_);
}

void CheckpointGateway::DrainTuple(sp_int32 _dest, Tuple& _tuple) {
  if (std::get<0>(_tuple)) {
    drainer1_(_dest, std::get<0>(_tuple));
  } else if (std::get<1>(_tuple)) {
    drainer2_(std::get<1>(_tuple));
  } else {
    drainer3_(_dest, std::get<2>(_tuple));
  }
}

void CheckpointGateway::ForceDrain() {
  for (auto kv : pending_tuples_) {
    std::deque<Tuple> tuples = kv.second->ForceDrain();
    for (auto tupl : tuples) {
      DrainTuple(kv.first, tupl);
    }
  }
  current_size_ = 0;
  size_metric_->SetValue(current_size_);
}

CheckpointGateway::CheckpointInfo*
CheckpointGateway::get_info(sp_int32 _task_id) {
  std::map<sp_int32, CheckpointInfo*>::iterator iter;
  iter = pending_tuples_.find(_task_id);
  if (iter == pending_tuples_.end()) {
    CheckpointInfo* info =
         new CheckpointInfo(_task_id, stateful_helper_->get_upstreamers(_task_id));
    pending_tuples_[_task_id] = info;
    return info;
  } else {
    return iter->second;
  }
}

void CheckpointGateway::Clear() {
  for (auto kv : pending_tuples_) {
    kv.second->Clear();
    delete kv.second;
  }
  pending_tuples_.clear();
  current_size_ = 0;
  size_metric_->SetValue(current_size_);
}

CheckpointGateway::CheckpointInfo::CheckpointInfo(sp_int32 _this_task_id,
               const std::set<sp_int32>& _all_upstream_dependencies) {
  checkpoint_id_ = "";
  all_upstream_dependencies_ = _all_upstream_dependencies;
  pending_upstream_dependencies_ = all_upstream_dependencies_;
  current_size_ = 0;
  this_task_id_ = _this_task_id;
}

CheckpointGateway::CheckpointInfo::~CheckpointInfo() {
  CHECK(pending_tuples_.empty());
  CHECK_EQ(current_size_, 0);
}

proto::system::HeronTupleSet2*
CheckpointGateway::CheckpointInfo::SendToInstance(proto::system::HeronTupleSet2* _tuple,
                                                  sp_uint64 _size) {
  if (checkpoint_id_.empty()) {
    return _tuple;
  } else {
    if (pending_upstream_dependencies_.find(_tuple->src_task_id()) !=
        pending_upstream_dependencies_.end()) {
      // This means that we still are expecting a checkpoint marker from this src task id
      return _tuple;
    } else {
      add(std::make_tuple(_tuple, (proto::stmgr::TupleStreamMessage2*)NULL,
                         (proto::ckptmgr::InitiateStatefulCheckpoint*)NULL), _size);
      return NULL;
    }
  }
}

proto::stmgr::TupleStreamMessage2*
CheckpointGateway::CheckpointInfo::SendToInstance(proto::stmgr::TupleStreamMessage2* _tuple,
                                                  sp_uint64 _size) {
  if (checkpoint_id_.empty()) {
    return _tuple;
  } else {
    if (pending_upstream_dependencies_.find(_tuple->src_task_id()) !=
        pending_upstream_dependencies_.end()) {
      // This means that we still are expecting a checkpoint marker from this src task id
      return _tuple;
    } else {
      add(std::make_tuple((proto::system::HeronTupleSet2*)NULL, _tuple,
                         (proto::ckptmgr::InitiateStatefulCheckpoint*)NULL), _size);
      return NULL;
    }
  }
}

std::deque<CheckpointGateway::Tuple>
CheckpointGateway::CheckpointInfo::HandleUpstreamMarker(sp_int32 _src_task_id,
                                                        const sp_string& _checkpoint_id,
                                                        sp_uint64* _size) {
  if (_checkpoint_id == checkpoint_id_) {
    pending_upstream_dependencies_.erase(_src_task_id);
  } else if (checkpoint_id_.empty()) {
    LOG(INFO) << "TaskId: " << this_task_id_
              << " Seeing the checkpoint marker " << _checkpoint_id
              << " for the first time";
    checkpoint_id_ = _checkpoint_id;
    pending_upstream_dependencies_.erase(_src_task_id);
  } else if (_checkpoint_id > checkpoint_id_) {
    LOG(INFO) << "TaskId: " << this_task_id_
              << " Seeing the checkpoint marker " << _checkpoint_id
              << " while we were already amidst " << checkpoint_id_
              << " ..resetting";
    checkpoint_id_ = _checkpoint_id;
    pending_upstream_dependencies_ = all_upstream_dependencies_;
    pending_upstream_dependencies_.erase(_src_task_id);
  } else {
    LOG(WARNING) << "TaskId: " << this_task_id_
                 << " Discarding older checkpoint_id message "
                 << _checkpoint_id << " from upstream task "
                 << _src_task_id;
  }
  if (pending_upstream_dependencies_.empty()) {
    LOG(INFO) << "TaskId: " << this_task_id_
              << " All checkpoint markers received for checkpoint "
                 << _checkpoint_id;
    // We need to add Initiate Checkpoint message before the current set
    auto message = new proto::ckptmgr::InitiateStatefulCheckpoint();
    message->set_checkpoint_id(_checkpoint_id);
    add_front(std::make_tuple((proto::system::HeronTupleSet2*)NULL,
                              (proto::stmgr::TupleStreamMessage2*)NULL, message),
                              message->GetCachedSize());
    return ForceDrain();
  } else {
    std::deque<Tuple> dummy;
    return dummy;
  }
}

std::deque<CheckpointGateway::Tuple>
CheckpointGateway::CheckpointInfo::ForceDrain() {
  checkpoint_id_ = "";
  current_size_ = 0;
  std::deque<Tuple> tmp = pending_tuples_;
  pending_tuples_.clear();
  pending_upstream_dependencies_ = all_upstream_dependencies_;
  return tmp;
}

void CheckpointGateway::CheckpointInfo::add(Tuple _tuple, sp_uint64 _size) {
  pending_tuples_.push_back(_tuple);
  current_size_ += _size;
}

void CheckpointGateway::CheckpointInfo::add_front(Tuple _tuple, sp_uint64 _size) {
  pending_tuples_.push_front(_tuple);
  current_size_ += _size;
}

void CheckpointGateway::CheckpointInfo::Clear() {
  for (auto tupl : pending_tuples_) {
    if (std::get<0>(tupl)) {
      __global_protobuf_pool_release__(std::get<0>(tupl));
    } else if (std::get<1>(tupl)) {
      __global_protobuf_pool_release__(std::get<1>(tupl));
    } else {
      delete std::get<2>(tupl);
    }
  }
  pending_tuples_.clear();
  current_size_ = 0;
  checkpoint_id_ = "";
}
}  // namespace stmgr
}  // namespace heron
