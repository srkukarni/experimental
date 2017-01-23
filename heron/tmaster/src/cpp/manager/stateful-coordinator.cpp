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

#include "tmaster/src/cpp/manager/stateful-coordinator.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <string>
#include <vector>
#include "config/physical-plan-helper.h"
#include "manager/tmaster.h"
#include "manager/stmgrstate.h"
#include "basics/basics.h"
#include "errors/errors.h"
#include "proto/tmaster.pb.h"

namespace heron {
namespace tmaster {

StatefulCoordinator::StatefulCoordinator(
  const std::string& _topology_name,
  const std::string& _latest_consistent_checkpoint,
  heron::common::HeronStateMgr* _state_mgr,
  std::chrono::high_resolution_clock::time_point _tmaster_start_time)
  : topology_name_(_topology_name),
    tmaster_start_time_(_tmaster_start_time),
    state_mgr_(_state_mgr),
    latest_consistent_checkpoint_(_latest_consistent_checkpoint) {
  // nothing really
}

StatefulCoordinator::~StatefulCoordinator() { }

sp_string StatefulCoordinator::GenerateCheckpointId() {
  // TODO(skukarni) Should we append any topology name/id stuff?
  std::ostringstream tag;
  tag << tmaster_start_time_.time_since_epoch().count()
      << "-" << time(NULL);
  return tag.str();
}

void StatefulCoordinator::DoCheckpoint(const StMgrMap& _stmgrs) {
  // Generate the checkpoint id
  sp_string checkpoint_id = GenerateCheckpointId();

  // Send the checkpoint message to all active stmgrs
  LOG(INFO) << "Sending checkpoint tag " << checkpoint_id
            << " to all strmgrs";
  StMgrMapConstIter iter;
  for (iter = _stmgrs.begin(); iter != _stmgrs.end(); ++iter) {
    proto::ckptmgr::StartStatefulCheckpoint request;
    request.set_checkpoint_id(checkpoint_id);
    iter->second->StatefulNewCheckpoint(request);
  }
}

void StatefulCoordinator::RegisterNewPplan(const proto::system::PhysicalPlan& _pplan) {
  config::PhysicalPlanHelper::GetAllTasks(_pplan, all_tasks_);
}

void StatefulCoordinator::HandleInstanceStateStored(const std::string& _checkpoint_id,
                                          const proto::system::Instance& _instance) {
  LOG(INFO) << "Handling InstanceStateStored for checkpoint:- " << _checkpoint_id
            << " and instance " << _instance.info().task_id();
  if (current_partial_checkpoint_.empty()) {
    LOG(INFO) << "Seeing the checkpoint id for the first time";
    partial_checkpoint_remaining_tasks_ = all_tasks_;
    current_partial_checkpoint_ = _checkpoint_id;
    partial_checkpoint_remaining_tasks_.erase(_instance.info().task_id());
  } else if (_checkpoint_id > current_partial_checkpoint_) {
    LOG(INFO) << "This new checkpoint id is newer than old partial one "
              << current_partial_checkpoint_;
    partial_checkpoint_remaining_tasks_ = all_tasks_;
    current_partial_checkpoint_ = _checkpoint_id;
    partial_checkpoint_remaining_tasks_.erase(_instance.info().task_id());
  } else if (_checkpoint_id == current_partial_checkpoint_) {
    partial_checkpoint_remaining_tasks_.erase(_instance.info().task_id());
  } else {
    LOG(INFO) << "This checkpoint id is older than partial one "
              << current_partial_checkpoint_;
  }
  if (partial_checkpoint_remaining_tasks_.empty()) {
    LOG(INFO) << "All task ids have their state stored for "
              << current_partial_checkpoint_;
    proto::ckptmgr::StatefulMostRecentCheckpoint ckpt;
    ckpt.set_checkpoint_id(current_partial_checkpoint_);
    state_mgr_->SetStatefulCheckpoint(topology_name_, ckpt,
       std::bind(&StatefulCoordinator::HandleCkptSave, this,
                 current_partial_checkpoint_, std::placeholders::_1));
    current_partial_checkpoint_ = "";
  }
}

void StatefulCoordinator::HandleCkptSave(std::string _checkpoint_id,
                                         proto::system::StatusCode _status) {
  if (_status == proto::system::OK) {
    LOG(INFO) << "Successfully saved " << _checkpoint_id
              << " as the new globally consistent checkpoint";
    latest_consistent_checkpoint_ = _checkpoint_id;
  } else {
    LOG(ERROR) << "Error saving " << _checkpoint_id
              << " as the new globally consistent checkpoint "
              << _status;
  }
}

}  // namespace tmaster
}  // namespace heron
