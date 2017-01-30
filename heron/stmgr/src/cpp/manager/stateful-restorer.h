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
namespace proto {
namespace system {
class PhysicalPlan;
}
}
}

namespace heron {
namespace stmgr {

class StMgrServer;
class TupleCache;
class StMgrClientMgr;
class CkptMgrClient;

class StatefulRestorer {
 public:
  explicit StatefulRestorer(CkptMgrClient* _ckptmgr,
                            StMgrClientMgr* _clientmgr, TupleCache* _tuple_cache,
                            StMgrServer* _server,
                            std::function<void(proto::system::StatusCode,
                                               std::string, sp_int64)> _restore_done_watcher);
  virtual ~StatefulRestorer();
  // Called when stmgr receives a RestoreTopologyStateRequest message
  void StartRestore(const std::string& _checkpoint_id, sp_int64 _restore_txid,
                    proto::system::PhysicalPlan* _pplan);
  // Called when ckptmgr client restarts
  void HandleCkptMgrRestart();
  // Called when instance responds back with RestoredInstanceStateResponse
  void HandleInstanceRestoredState(sp_int32 _task_id, const std::string& _checkpoint_id);
  // called when ckptmgr returns with instance state
  void HandleCheckpointState(proto::system::StatusCode _status, sp_int32 _task_id,
                             const proto::ckptmgr::InstanceStateCheckpoint& _state);
  // called when a stmgr connection closes
  void HandleDeadStMgrConnection();
  // called when all clients get connected
  void HandleAllStMgrClientsConnected();
  // called when an instance is dead
  void HandleDeadInstanceConnection();
  // called when all instances are connected
  void HandleAllInstancesConnected();
  bool InProgress() const { return in_progress_; }

 private:
  void GetCheckpoints();
  void CheckAndFinishRestore();

  std::set<sp_int32> get_ckpt_pending_;
  std::set<sp_int32> restore_pending_;
  bool clients_connections_pending_;
  bool instance_connections_pending_;
  std::string checkpoint_id_;
  sp_int64 restore_txid_;
  std::set<sp_int32> local_taskids_;

  CkptMgrClient* ckptmgr_;
  StMgrClientMgr* clientmgr_;
  TupleCache* tuple_cache_;
  StMgrServer* server_;

  bool in_progress_;
  std::function<void(proto::system::StatusCode, std::string, sp_int64)> restore_done_watcher_;
};
}  // namespace stmgr
}  // namespace heron

#endif  // SRC_CPP_SVCS_STMGR_SRC_MANAGER_STATEFUL_RESTORER_H_
