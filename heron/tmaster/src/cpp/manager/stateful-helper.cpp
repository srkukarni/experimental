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

#include "tmaster/src/cpp/manager/stateful-helper.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <string>
#include <vector>
#include "manager/stateful-checkpointer.h"
#include "manager/stateful-restorer.h"
#include "manager/tmaster.h"
#include "manager/stmgrstate.h"
#include "basics/basics.h"
#include "errors/errors.h"

namespace heron {
namespace tmaster {

StatefulHelper::StatefulHelper(const std::string& _topology_name,
               proto::ckptmgr::StatefulConsistentCheckpoints* _ckpt,
               heron::common::HeronStateMgr* _state_mgr,
               std::chrono::high_resolution_clock::time_point _tmaster_start_time)
  : topology_name_(_topology_name), ckpt_record_(_ckpt), state_mgr_(_state_mgr) {
  checkpointer_ = new StatefulCheckpointer(_tmaster_start_time);
  restorer_ = new StatefulRestorer();
}

StatefulHelper::~StatefulHelper() {
  delete ckpt_record_;
  delete checkpointer_;
  delete restorer_;
}

void StatefulHelper::StartRestore(const StMgrMap& _stmgrs, bool _ignore_prev_state) {
  // TODO(sanjeev): Do we really need to start from most_recent_checkpoint?
  if (_ignore_prev_state) {
    restorer_->StartRestore("", _stmgrs);
  } else {
    restorer_->StartRestore(ckpt_record_->most_recent_checkpoint_id(), _stmgrs);
  }
}

void StatefulHelper::HandleStMgrRestored(const std::string& _stmgr_id,
                                         const std::string& _checkpoint_id,
                                         int64_t _restore_txid,
                                         proto::system::StatusCode _status,
                                         const StMgrMap& _stmgrs) {
  if (!restorer_->InProgress()) {
    LOG(WARNING) << "Got a Restored Topology State from stmgr "
                 << _stmgr_id << " for checkpoint " << _checkpoint_id
                 << " with txid " << _restore_txid << " when "
                 << " we are not in restore";
    return;
  } else if (restorer_->RestoreTxid() != _restore_txid ||
             restorer_->CheckpointIdInProgress() != _checkpoint_id) {
    LOG(WARNING) << "Got a Restored Topology State from stmgr "
                 << _stmgr_id << " for checkpoint " << _checkpoint_id
                 << " with txid " << _restore_txid << " when "
                 << " we are in progress with checkpoint "
                 << restorer_->CheckpointIdInProgress() << " and txid "
                 << restorer_->RestoreTxid();
    return;
  } else if (_status != proto::system::OK) {
    LOG(INFO) << "Got a Cannot Restore Topology State from stmgr "
              << _stmgr_id << " for checkpoint " << _checkpoint_id
              << " with txid " << _restore_txid << " because of "
              << _status;
    const std::string& new_ckpt_id = GetNextInLineCheckpointId(_checkpoint_id);
    if (new_ckpt_id.empty()) {
      LOG(INFO) << "Next viable checkpoint id is empty";
    }
    restorer_->StartRestore(new_ckpt_id, _stmgrs);
  } else {
    LOG(INFO) << "Got a Restored Topology State from stmgr "
              << _stmgr_id << " for checkpoint " << _checkpoint_id
              << " with txid " << _restore_txid;
    restorer_->HandleStMgrRestored(_stmgr_id, _checkpoint_id, _restore_txid, _stmgrs);
  }
}

void StatefulHelper::RegisterNewPplan(const proto::system::PhysicalPlan& _pplan) {
  checkpointer_->RegisterNewPplan(_pplan);
}

void StatefulHelper::StartCheckpoint(const StMgrMap& _stmgrs) {
  if (restorer_->InProgress()) {
    LOG(INFO) << "Will not send checkpoint messages to stmgr because "
              << "we are in restore";
    return;
  }
  checkpointer_->StartCheckpoint(_stmgrs);
}

void StatefulHelper::HandleInstanceStateStored(const std::string& _checkpoint_id,
                                               const proto::system::Instance& _instance) {
  if (restorer_->InProgress()) {
    LOG(INFO) << "Ignoring the Instance State because we are in Restore";
    return;
  }
  if (checkpointer_->HandleInstanceStateStored(_checkpoint_id, _instance)) {
    // This is now a globally consistent checkpoint
    auto new_ckpt_record = AddNewConsistentCheckpoint(_checkpoint_id);
    state_mgr_->SetStatefulCheckpoint(topology_name_, *new_ckpt_record,
           std::bind(&StatefulHelper::HandleCkptSave, this,
                    new_ckpt_record, std::placeholders::_1));
  }
}

void StatefulHelper::HandleCkptSave(proto::ckptmgr::StatefulConsistentCheckpoints* _new_ckpt,
                                    proto::system::StatusCode _status) {
  if (_status == proto::system::OK) {
    LOG(INFO) << "Successfully saved " << _new_ckpt->most_recent_checkpoint_id()
              << " as the new globally consistent checkpoint";
    delete ckpt_record_;
    ckpt_record_ = _new_ckpt;
  } else {
    LOG(ERROR) << "Error saving " << _new_ckpt->most_recent_checkpoint_id()
              << " as the new globally consistent checkpoint "
              << _status;
    delete _new_ckpt;
  }
}

const std::string& StatefulHelper::GetNextInLineCheckpointId(const std::string& _ckpt_id) {
  if (_ckpt_id.empty()) {
    // There cannot be any checkpoints that are older than empty checkpoint
    LOG(FATAL) << "Could not recover even from the empty state";
  }
  if (ckpt_record_->most_recent_checkpoint_id() == _ckpt_id) {
    // The first backup will be the next in line
    if (ckpt_record_->backup_checkpoint_ids_size() > 0) {
      return ckpt_record_->backup_checkpoint_ids(0);
    } else {
      // There are no backups
      return EMPTY_STRING;
    }
  } else {
    for (int32_t i = 0; i < ckpt_record_->backup_checkpoint_ids_size(); ++i) {
      if (ckpt_record_->backup_checkpoint_ids(i) == _ckpt_id) {
        if (ckpt_record_->backup_checkpoint_ids_size() >= i) {
          return ckpt_record_->backup_checkpoint_ids(i + 1);
        } else {
          return EMPTY_STRING;
        }
      }
    }
    return EMPTY_STRING;
  }
}

proto::ckptmgr::StatefulConsistentCheckpoints*
StatefulHelper::AddNewConsistentCheckpoint(const std::string& _new_checkpoint) {
  auto new_record = new proto::ckptmgr::StatefulConsistentCheckpoints();
  new_record->set_most_recent_checkpoint_id(_new_checkpoint);
  if (!ckpt_record_->most_recent_checkpoint_id().empty()) {
    new_record->add_backup_checkpoint_ids(ckpt_record_->most_recent_checkpoint_id());
    for (int32_t i = 0; i < ckpt_record_->backup_checkpoint_ids_size() &&
                        new_record->backup_checkpoint_ids_size() < 5; ++i) {
      new_record->add_backup_checkpoint_ids(ckpt_record_->backup_checkpoint_ids(i));
    }
  }
  return new_record;
}

bool StatefulHelper::GotRestoreResponse(const std::string& _stmgr) const {
  CHECK(restorer_->InProgress());
  return restorer_->GotResponse(_stmgr);
}

bool StatefulHelper::RestoreInProgress() const {
  return restorer_->InProgress();
}
}  // namespace tmaster
}  // namespace heron
