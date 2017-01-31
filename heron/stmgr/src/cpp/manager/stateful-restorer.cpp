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

#include "manager/stateful-restorer.h"
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "manager/stmgr-server.h"
#include "manager/ckptmgr-client.h"
#include "manager/stmgr-clientmgr.h"
#include "util/tuple-cache.h"
#include "proto/messages.h"
#include "basics/basics.h"
#include "errors/errors.h"
#include "threads/threads.h"
#include "network/network.h"

namespace heron {
namespace stmgr {

StatefulRestorer::StatefulRestorer(CkptMgrClient* _ckptmgr,
                             StMgrClientMgr* _clientmgr, TupleCache* _tuple_cache,
                             StMgrServer* _server,
                             std::function<void(proto::system::StatusCode,
                                                std::string, sp_int64)> _restore_done_watcher) {
  ckptmgr_ = _ckptmgr;
  clientmgr_ = _clientmgr;
  tuple_cache_ = _tuple_cache;
  server_ = _server;

  in_progress_ = false;
  restore_done_watcher_ = _restore_done_watcher;
}

StatefulRestorer::~StatefulRestorer() { }

void StatefulRestorer::StartRestore(const std::string& _checkpoint_id, sp_int64 _restore_txid,
                                    proto::system::PhysicalPlan* _pplan) {
  if (in_progress_) {
    LOG(WARNING) << "Got a RestoreTopologyState request for " << _checkpoint_id
                 << " " << _restore_txid << " while we were still in old one "
                 << checkpoint_id_ << " " << restore_txid_;
    if (_restore_txid <= restore_txid_) {
      LOG(FATAL) << "New restore txid is <= old!!! Dying!!!";
    }
  } else {
    LOG(INFO) << "Starting Restore for checkpoint_id " << _checkpoint_id
              << " and txid " << _restore_txid;
    tuple_cache_->clear();
    clientmgr_->CloseConnectionsAndClear();
    server_->ClearCache();
  }
  // This is a new one for this checkpoint
  in_progress_ = true;
  clients_connections_pending_ = true;
  instance_connections_pending_ = true;
  std::vector<proto::system::Instance*> instances;
  server_->GetInstanceInfo(instances);
  for (auto instance : instances) {
    local_taskids_.insert(instance->info().task_id());
  }
  restore_pending_ = local_taskids_;
  get_ckpt_pending_ = local_taskids_;
  checkpoint_id_ = _checkpoint_id;
  restore_txid_ = _restore_txid;

  if (checkpoint_id_.empty()) {
    LOG(INFO) << "Checkpoint id is empty meaning we are starting from scratch";
    get_ckpt_pending_.clear();
    for (auto task_id : restore_pending_) {
      proto::ckptmgr::InstanceStateCheckpoint dummy;
      dummy.set_checkpoint_id(checkpoint_id_);
      dummy.mutable_state();
      if (!server_->SendRestoreInstanceStateRequest(task_id, dummy)) {
        get_ckpt_pending_.insert(task_id);
      }
    }
  } else {
    // Send messages to ckpt
    GetCheckpoints();
  }
  clientmgr_->StartConnections(_pplan);
  if (clientmgr_->AllStMgrClientsConnected()) {
    // Its possible that this is really a restore while we were already in progress
    // and there was no change in pplan. In which case there would be no new
    // connections to restore
    clients_connections_pending_ = false;
  }
  if (server_->HaveAllInstancesConnectedToUs()) {
    instance_connections_pending_ = false;
  }
}

void StatefulRestorer::GetCheckpoints() {
  for (auto task_id : get_ckpt_pending_) {
    ckptmgr_->GetInstanceState(*(server_->GetInstanceInfo(task_id)), checkpoint_id_);
  }
}

void StatefulRestorer::HandleCheckpointState(proto::system::StatusCode _status, sp_int32 _task_id,
                                       const proto::ckptmgr::InstanceStateCheckpoint& _state) {
  LOG(INFO) << "Got InstanceState from checkpoint mgr for task " << _task_id
            << " and checkpoint " << _state.checkpoint_id();
  if (!in_progress_) {
    LOG(INFO) << "Ignoring it because we are not in restore";
    return;
  }
  if (_state.checkpoint_id() != checkpoint_id_) {
    LOG(WARNING) << "Discarding state retrieved from checkpoint mgr because the checkpoint"
                 << " id in the response does not match ours " << checkpoint_id_;
    return;
  }
  if (_status == proto::system::OK) {
    if (server_->SendRestoreInstanceStateRequest(_task_id, _state)) {
      get_ckpt_pending_.erase(_task_id);
    }
  } else {
    LOG(INFO) << "InstanceState from checkpont mgr contained non ok status " << _status;
    in_progress_ = false;
    restore_done_watcher_(_status, checkpoint_id_, restore_txid_);
  }
}

void StatefulRestorer::HandleInstanceRestoredState(sp_int32 _task_id,
                                                   const std::string& _checkpoint_id) {
  LOG(INFO) << "Instance " << _task_id << " restored its state for " << _checkpoint_id;
  if (!in_progress_) {
    LOG(INFO) << "Ignoring the message because we are not in Restore";
    return;
  }
  if (_checkpoint_id != checkpoint_id_) {
    LOG(WARNING) << "Ignoring it because we are operating on a different one " << checkpoint_id_;
    return;
  }
  restore_pending_.erase(_task_id);
  CheckAndFinishRestore();
}

void StatefulRestorer::HandleCkptMgrRestart() {
  LOG(INFO) << "Checkpoint ClientMgr restarted";
  if (in_progress_) {
    GetCheckpoints();
  }
}

void StatefulRestorer::HandleAllStMgrClientsConnected() {
  LOG(INFO) << "All StMgr Clients Connected";
  if (!in_progress_) {
    LOG(INFO) << "Ignoring it becuase we are not in restore";
    return;
  }
  clients_connections_pending_ = false;
  CheckAndFinishRestore();
}

void StatefulRestorer::HandleDeadStMgrConnection() {
  if (in_progress_) {
    clients_connections_pending_ = true;
  }
}

void StatefulRestorer::HandleAllInstancesConnected() {
  LOG(INFO) << "All Instances Connected";
  if (!in_progress_) {
    LOG(INFO) << "Ignoring it becuase we are not in restore";
    return;
  }
  instance_connections_pending_ = false;
  if (!get_ckpt_pending_.empty()) {
    GetCheckpoints();
  } else {
    CheckAndFinishRestore();
  }
}

void StatefulRestorer::HandleDeadInstanceConnection(sp_int32 _task_id) {
  if (in_progress_) {
    instance_connections_pending_ = true;
    CHECK(local_taskids_.find(_task_id) != local_taskids_.end());
    restore_pending_.insert(_task_id);
    get_ckpt_pending_.insert(_task_id);
  }
}

void StatefulRestorer::CheckAndFinishRestore() {
  if (!instance_connections_pending_ && !clients_connections_pending_ &&
      restore_pending_.empty()) {
    LOG(INFO) << "Restore Done Successfully for " << checkpoint_id_
              << " " << restore_txid_;
    in_progress_ = false;
    restore_done_watcher_(proto::system::OK, checkpoint_id_, restore_txid_);
  }
}

}  // namespace stmgr
}  // namespace heron
