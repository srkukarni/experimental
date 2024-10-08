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

#ifndef SRC_CPP_SVCS_STMGR_SRC_MANAGER_CHECKPOINT_GATEWAY_H_
#define SRC_CPP_SVCS_STMGR_SRC_MANAGER_CHECKPOINT_GATEWAY_H_

#include <map>
#include <set>
#include <deque>
#include <tuple>
#include <utility>
#include <typeinfo>   // operator typeid
#include "proto/messages.h"
#include "network/network.h"
#include "basics/basics.h"

namespace heron {
namespace common {
class MetricsMgrSt;
class AssignableMetric;
}
}  // namespace heron

namespace heron {
namespace stmgr {

class StatefulHelper;

class CheckpointGateway {
 public:
  explicit CheckpointGateway(sp_uint64 _drain_threshold,
        StatefulHelper* _stateful_helper,
        common::MetricsMgrSt* _metrics_manager_client,
        std::function<void(sp_int32, proto::system::HeronTupleSet2*)> drainer1,
        std::function<void(proto::stmgr::TupleStreamMessage2*)> drainer2,
        std::function<void(sp_int32, proto::ckptmgr::InitiateStatefulCheckpoint*)> drainer3);
  virtual ~CheckpointGateway();
  void SendToInstance(sp_int32 _task_id, proto::system::HeronTupleSet2* _message);
  void SendToInstance(proto::stmgr::TupleStreamMessage2* _message);
  void HandleUpstreamMarker(sp_int32 _src_task_id, sp_int32 _destination_task_id,
                            const sp_string& _checkpoint_id);

  // Clears all tuples
  void Clear();

 private:
  typedef std::tuple<proto::system::HeronTupleSet2*,
                     proto::stmgr::TupleStreamMessage2*,
                     proto::ckptmgr::InitiateStatefulCheckpoint*>
          Tuple;
  class CheckpointInfo {
   public:
    explicit CheckpointInfo(sp_int32 _this_task_id,
                            const std::set<sp_int32>& _all_upstream_dependencies);
    ~CheckpointInfo();
    proto::system::HeronTupleSet2*  SendToInstance(proto::system::HeronTupleSet2* _tuple,
                                                   sp_uint64 _size);
    proto::stmgr::TupleStreamMessage2* SendToInstance(proto::stmgr::TupleStreamMessage2* _tuple,
                                                      sp_uint64 _size);
    std::deque<Tuple> HandleUpstreamMarker(sp_int32 _src_task_id,
                                             const sp_string& _checkpoint_id, sp_uint64* _size);
    std::deque<Tuple> ForceDrain();
    void Clear();
   private:
    void add(Tuple _tuple, sp_uint64 _size);
    void add_front(Tuple _tuple, sp_uint64 _size);
    sp_string checkpoint_id_;
    std::set<sp_int32> all_upstream_dependencies_;
    std::set<sp_int32> pending_upstream_dependencies_;
    std::deque<Tuple> pending_tuples_;
    sp_uint64 current_size_;
    sp_int32 this_task_id_;
  };
  void ForceDrain();
  void DrainTuple(sp_int32 _dest, Tuple& _tuple);
  CheckpointInfo* get_info(sp_int32 _task_id);
  // The maximum buffering that we can do before we discard the marker
  sp_uint64 drain_threshold_;
  sp_uint64 current_size_;
  StatefulHelper* stateful_helper_;
  common::MetricsMgrSt* metrics_manager_client_;
  common::AssignableMetric* size_metric_;
  std::map<sp_int32, CheckpointInfo*> pending_tuples_;
  std::function<void(sp_int32, proto::system::HeronTupleSet2*)> drainer1_;
  std::function<void(proto::stmgr::TupleStreamMessage2*)> drainer2_;
  std::function<void(sp_int32, proto::ckptmgr::InitiateStatefulCheckpoint*)> drainer3_;
};

}  // namespace stmgr
}  // namespace heron

#endif  // SRC_CPP_SVCS_STMGR_SRC_MANAGER_CHECKPOINT_GATEWAY_H_
