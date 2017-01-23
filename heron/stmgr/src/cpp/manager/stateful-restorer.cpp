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
#include "proto/messages.h"
#include "basics/basics.h"
#include "errors/errors.h"
#include "threads/threads.h"
#include "network/network.h"

namespace heron {
namespace stmgr {

StatefulRestorer::StatefulRestorer(ckptmgr::CkptMgrClient* _ckptmgr,
                             std::function<void(std::string, sp_int64)> _restore_done_watcher) {
  in_progress_ = false;
  restore_done_watcher_ = _restore_done_watcher;
  server_ = NULL;
  ckptmgr_ = _ckptmgr;
}

StatefulRestorer::~StatefulRestorer() { }

void StatefulRestorer::SetStMgrServer(StMgrServer* _server) {
  server_ = _server;
}

void StatefulRestorer::StartRestore(const std::string& _checkpoint_id, sp_int64 _restore_txid) {
  if (in_progress_) {
    LOG(WARNING) << "Got a RestoreTopologyState request for " << _checkpoint_id
                 << " " << _restore_txid << " while we were still in old one "
                 << checkpoint_id_ << " " << restore_txid_;
    if (_restore_txid <= restore_txid_) {
      LOG(FATAL) << "New restore txid is <= old!!! Dying!!!";
    }
    // This is a valid restore_txid
    if (_checkpoint_id == checkpoint_id_) {
      LOG(WARNING) << "New RestoreTopologyState has the same checkpoint id as previous "
                   << "continuing with the old one";
      restore_txid_ = _restore_txid;
      return;
    }
  } else {
    LOG(INFO) << "Starting Restore for checkpoint_id " << _checkpoint_id
              << " and txid " << _restore_txid;
  }
  // This is a new one for this checkpoint
  in_progress_ = true;
  std::vector<proto::system::Instance*> instances;
  server_->GetInstanceInfo(instances);
  for (auto instance : instances) {
    local_taskids_.insert(instance->info().task_id());
  }
  restore_pending_ = local_taskids_;
  get_ckpt_pending_ = local_taskids_;
  checkpoint_id_ = _checkpoint_id;
  restore_txid_ = _restore_txid;

  // Send messages to ckpt
  GetCheckpoints();
}

void StatefulRestorer::GetCheckpoints() {
  for (auto task_id : get_ckpt_pending_) {
    ckptmgr_->GetInstanceState(*(server_->GetInstanceInfo(task_id)), checkpoint_id_);
  }
}

void StatefulRestorer::HandleCheckpointState(sp_int32 _task_id,
                                       const proto::ckptmgr::InstanceStateCheckpoint& _state) {
  LOG(INFO) << "Got InstanceState from checkpoint mgr for task " << _task_id
            << " and checkpoint " << _state.checkpoint_id();
  if (_state.checkpoint_id() != checkpoint_id_) {
    LOG(WARNING) << "Discarding state retrieved from checkpoint mgr because the checkpoint"
                 << " id in the response does not match ours " << checkpoint_id_;
    return;
  }
  get_ckpt_pending_.erase(_task_id);
  server_->SendRestoreInstanceStateRequest(_task_id, _state);
}

void StatefulRestorer::HandleInstanceRestoredState(sp_int32 _task_id,
                                                   const std::string& _checkpoint_id) {
  LOG(INFO) << "Instance " << _task_id << " restored its state for " << _checkpoint_id;
  if (_checkpoint_id != checkpoint_id_) {
    LOG(WARNING) << "Ignoring it because we are operating on a different one " << checkpoint_id_;
    return;
  }
  restore_pending_.erase(_task_id);
  if (restore_pending_.empty()) {
    LOG(INFO) << "All instances restored state for " << checkpoint_id_
              << " " << restore_txid_;
    in_progress_ = false;
    restore_done_watcher_(checkpoint_id_, restore_txid_);
  }
}

void StatefulRestorer::HandleCkptMgrRestart() {
  LOG(INFO) << "Checkpoint ClientMgr restarted";
  if (in_progress_) {
    GetCheckpoints();
  }
}
}  // namespace stmgr
}  // namespace heron
