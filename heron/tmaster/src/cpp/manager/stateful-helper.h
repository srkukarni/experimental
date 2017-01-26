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

#ifndef __TMASTER_STATEFUL_HELPER_H_
#define __TMASTER_STATEFUL_HELPER_H_

#include <set>
#include <string>
#include "network/network.h"
#include "manager/tmaster.h"
#include "basics/basics.h"

namespace heron {
namespace tmaster {

class StatefulRestorer;
class StatefulCheckpointer;

class StatefulHelper {
 public:
  explicit StatefulHelper(const std::string& _topology_name,
               proto::ckptmgr::StatefulConsistentCheckpoints* _ckpt,
               heron::common::HeronStateMgr* _state_mgr,
               std::chrono::high_resolution_clock::time_point _tmaster_start_time,
               std::function<void()> _after_2pc_cb);
  virtual ~StatefulHelper();
  // Start a new restore process
  void StartRestore(const StMgrMap& _stmgrs, bool _ignore_prev_checkpoints);
  // When a Stmgr responds back with a RestoreTopologyStateResponse
  void HandleStMgrRestored(const std::string& _stmgr_id,
                           const std::string& _checkpoint_id,
                           int64_t _restore_txid,
                           proto::system::StatusCode _status,
                           const StMgrMap& _stmgrs);
  // When a new pplan is made
  void RegisterNewPplan(const proto::system::PhysicalPlan& _pplan);

  // Called its time to send ckpt messages to ckptmgr
  void StartCheckpoint(const StMgrMap& _stmgrs);

  // Called when we receive a InstanceStateStored message
  void HandleInstanceStateStored(const std::string& _checkpoint_id,
                                 const proto::system::Instance& _instance);

 private:
  // Calculates the next in line checkpoint that must be used
  const std::string& GetNextInLineCheckpointId(const std::string& _ckpt_id);
  // Creates a new ckpt record adding the latest one
  proto::ckptmgr::StatefulConsistentCheckpoints*
    AddNewConsistentCheckpoint(const std::string& _new_checkpoint);
  // Handler when statemgr saves the new checkpoint record
  void HandleCkptSave(proto::ckptmgr::StatefulConsistentCheckpoints* _new_ckpt,
                      proto::system::StatusCode _status);

  std::string topology_name_;
  proto::ckptmgr::StatefulConsistentCheckpoints* ckpt_record_;
  heron::common::HeronStateMgr* state_mgr_;
  StatefulCheckpointer* checkpointer_;
  StatefulRestorer* restorer_;
};
}  // namespace tmaster
}  // namespace heron

#endif
