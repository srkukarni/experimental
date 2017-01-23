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

#ifndef SRC_CPP_SVCS_STMGR_SRC_MANAGER_STATEFUL_RESTORER_H_
#define SRC_CPP_SVCS_STMGR_SRC_MANAGER_STATEFUL_RESTORER_H_

#include <ostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <typeinfo>   // operator typeid
#include "proto/messages.h"
#include "network/network.h"
#include "basics/basics.h"
#include "grouping/shuffle-grouping.h"

namespace heron {
namespace ckptmgr {
class CkptMgrClient;
}
}

namespace heron {
namespace stmgr {

class StMgrServer;

class StatefulRestorer {
 public:
  explicit StatefulRestorer(ckptmgr::CkptMgrClient* _ckptmgr,
                            std::function<void(std::string, sp_int64)> _restore_done_watcher);
  virtual ~StatefulRestorer();
  void SetStMgrServer(StMgrServer* _server);
  // Called when stmgr receives a RestoreTopologyStateRequest message
  void StartRestore(const std::string& _checkpoint_id, sp_int64 _restore_txid);
  // Called when ckptmgr client restarts
  void HandleCkptMgrRestart();
  // Called when instance responds back with RestoredInstanceStateResponse
  void HandleInstanceRestoredState(sp_int32 _task_id, const std::string& _checkpoint_id);
  // called when ckptmgr returns with instance state
  void HandleCheckpointState(sp_int32 _task_id,
                             const proto::ckptmgr::InstanceStateCheckpoint& _state);
  bool InProgress() const { return in_progress_; }

 private:
  void GetCheckpoints();

  bool in_progress_;
  std::set<sp_int32> get_ckpt_pending_;
  std::set<sp_int32> restore_pending_;
  std::string checkpoint_id_;
  sp_int64 restore_txid_;
  std::set<sp_int32> local_taskids_;
  std::function<void(std::string, sp_int64)> restore_done_watcher_;
  StMgrServer* server_;
  ckptmgr::CkptMgrClient* ckptmgr_;
};
}  // namespace stmgr
}  // namespace heron

#endif  // SRC_CPP_SVCS_STMGR_SRC_MANAGER_STATEFUL_RESTORER_H_
