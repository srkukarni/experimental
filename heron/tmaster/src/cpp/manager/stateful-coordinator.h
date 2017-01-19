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

#ifndef __TMASTER_STATEFUL_COORDINATOR_H_
#define __TMASTER_STATEFUL_COORDINATOR_H_

#include <set>
#include <string>
#include "network/network.h"
#include "proto/tmaster.pb.h"
#include "manager/tmaster.h"
#include "basics/basics.h"

namespace heron {
namespace common {
class HeronStateMgr;
}
namespace proto {
class PhysicalPlan;
}
}

namespace heron {
namespace tmaster {

class StatefulCoordinator {
 public:
  explicit StatefulCoordinator(const std::string& _topology_name,
               const std::string& _latest_consistent_checkpoint_,
               heron::common::HeronStateMgr* _state_mgr,
               std::chrono::high_resolution_clock::time_point _tmaster_start_time);
  virtual ~StatefulCoordinator();
  void RegisterNewPplan(const proto::system::PhysicalPlan& _pplan);

  void DoCheckpoint(const StMgrMap& _stmgrs);

  // Called when we receive a TopologyStateStored message
  void HandleTopologyStateStored(const std::string& _checkpoint_id,
                                 const proto::system::Instance& _instance);

 private:
  void HandleCkptSave(std::string checkpoint_id, proto::system::StatusCode _status);
  sp_string GenerateCheckpointId();

  std::string topology_name_;
  std::chrono::high_resolution_clock::time_point tmaster_start_time_;
  heron::common::HeronStateMgr* state_mgr_;
  // Cached copy of our latest consistent checkpoint
  std::string latest_consistent_checkpoint_;
  // Current partially consistent checkpoint
  // for which still some more state needs to be saved
  std::string current_partial_checkpoint_;
  // The set of all tasks
  std::set<sp_int32> all_tasks_;
  // The set of tasks from which we still need confirmation
  // of state storage before we can declare it a globally
  // consistent checkpoint
  std::set<sp_int32> partial_checkpoint_remaining_tasks_;
};
}  // namespace tmaster
}  // namespace heron

#endif
