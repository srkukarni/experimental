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

#include "tmaster/src/cpp/manager/stateful-restorer.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <string>
#include <vector>
#include "manager/tmaster.h"
#include "manager/stmgrstate.h"
#include "basics/basics.h"
#include "errors/errors.h"

namespace heron {
namespace tmaster {

StatefulRestorer::StatefulRestorer()
  : in_progress_(false),
    restore_txid_(0) {
}

StatefulRestorer::~StatefulRestorer() { }

void StatefulRestorer::Start(const std::string& _checkpoint_id,
                             const StMgrMap& _stmgrs) {
  if (in_progress_) {
    // Hmm... It seems that we were in the middle of a
    LOG(WARNING) << "Starting a 2PC Restore for checkpoint "
                 << _checkpoint_id << " when we are already busy "
                 << " within 2PC of " << checkpoint_id_in_progress_;
  }
  in_progress_ = true;
  checkpoint_id_in_progress_ = _checkpoint_id;
  unreplied_stmgrs_.clear();
  ++restore_txid_;
  LOG(INFO) << "Starting a 2PC Restore for checkpoint "
            << _checkpoint_id << " and restore txid "
            << restore_txid_;
  proto::ckptmgr::RestoreTopologyStateRequest request;
  request.set_checkpoint_id(_checkpoint_id);
  request.set_restore_txid(restore_txid_);
  for (auto kv : _stmgrs) {
    kv.second->SendRestoreTopologyStateMessage(request);
    unreplied_stmgrs_.insert(kv.first);
  }
}

void StatefulRestorer::HandleRestored(const std::string& _stmgr_id,
                                      const std::string& _checkpoint_id,
                                      int64_t _restore_txid,
                                      const StMgrMap& _stmgrs) {
  if (!in_progress_) {
    LOG(WARNING) << "Got a Restored Topology State from stmgr "
                 << _stmgr_id << " for checkpoint " << _checkpoint_id
                 << " with txid " << _restore_txid << " when "
                 << " we are not in progress";
    return;
  } else if (restore_txid_ != _restore_txid ||
             checkpoint_id_in_progress_ != _checkpoint_id) {
    LOG(WARNING) << "Got a Restored Topology State from stmgr "
                 << _stmgr_id << " for checkpoint " << _checkpoint_id
                 << " with txid " << _restore_txid << " when "
                 << " we are in progress with checkpoint "
                 << checkpoint_id_in_progress_ << " and txid "
                 << restore_txid_;
    return;
  } else {
    LOG(INFO) << "Got a Restored Topology State from stmgr "
              << _stmgr_id << " for checkpoint " << _checkpoint_id
              << " with txid " << _restore_txid;
    unreplied_stmgrs_.erase(_stmgr_id);
    if (unreplied_stmgrs_.empty()) {
      Finish2PC(_stmgrs);
    }
  }
}

void StatefulRestorer::Finish2PC(const StMgrMap& _stmgrs) {
  LOG(INFO) << "Finishing Stateful 2PC since all stmgrs have replied back";
  CHECK(unreplied_stmgrs_.empty());
  for (auto kv : _stmgrs) {
    kv.second->SendStartStatefulProcessingMessage(checkpoint_id_in_progress_);
  }
  in_progress_ = false;
  checkpoint_id_in_progress_ = "";
}
}  // namespace tmaster
}  // namespace heron
