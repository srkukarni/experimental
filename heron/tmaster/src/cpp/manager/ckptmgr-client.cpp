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

#include "manager/ckptmgr-client.h"
#include <iostream>
#include <string>
#include "basics/basics.h"
#include "proto/messages.h"
#include "errors/errors.h"
#include "threads/threads.h"
#include "network/network.h"

namespace heron {
namespace tmaster {

CkptMgrClient::CkptMgrClient(EventLoop* eventloop, const NetworkOptions& _options,
                             const sp_string& _topology_name, const sp_string& _topology_id,
                             std::function<void(proto::system::StatusCode)> _clean_response_watcher)
    : Client(eventloop, _options),
      topology_name_(_topology_name),
      topology_id_(_topology_id),
      quit_(false),
      clean_response_watcher_(_clean_response_watcher) {
  // TODO(sanjeev): Take this from config
  reconnect_cpktmgr_interval_sec_ = 10;

  InstallResponseHandler(new proto::ckptmgr::RegisterTMasterRequest(),
                         &CkptMgrClient::HandleTMasterRegisterResponse);
  InstallResponseHandler(new proto::ckptmgr::CleanStatefulCheckpointRequest(),
                         &CkptMgrClient::HandleCleanStatefulCheckpointResponse);
}

CkptMgrClient::~CkptMgrClient() {
  Stop();
}


void CkptMgrClient::Quit() {
  quit_ = true;
  Stop();
}

void CkptMgrClient::HandleConnect(NetworkErrorCode _status) {
  if (_status == OK) {
    LOG(INFO) << "Connected to ckptmgr running at "
              << get_clientoptions().get_host() << ":" << get_clientoptions().get_port()
              << std::endl;
    if (quit_) {
      Stop();
    } else {
      SendRegisterRequest();
    }
  } else {
    LOG(WARNING) << "Could not connect to cpktmgr" << " running at "
                 << get_clientoptions().get_host() << ":" << get_clientoptions().get_port()
                 << " due to: " << _status << std::endl;
    if (quit_) {
      LOG(ERROR) << "Quitting";
      delete this;
      return;
    } else {
      LOG(INFO) << "Retrying again..." << std::endl;
      AddTimer([this]() { this->OnReconnectTimer(); },
               reconnect_cpktmgr_interval_sec_ * 1000 * 1000);
    }
  }
}

void CkptMgrClient::HandleClose(NetworkErrorCode _code) {
  if (_code == OK) {
    LOG(INFO) << "We closed our server connection with cpktmgr running at "
              << get_clientoptions().get_host() << ":" << get_clientoptions().get_port()
              << std::endl;
  } else {
    LOG(INFO) << "Ckptmgr running at "
              << get_clientoptions().get_host() << ":" << get_clientoptions().get_port()
              << " closed connection with code " << _code << std::endl;
  }
  if (quit_) {
    delete this;
  } else {
    LOG(INFO) << "Will try to reconnect again..." << std::endl;
    AddTimer([this]() { this->OnReconnectTimer(); },
             reconnect_cpktmgr_interval_sec_ * 1000 * 1000);
  }
}

void CkptMgrClient::HandleTMasterRegisterResponse(void*,
                                                proto::ckptmgr::RegisterTMasterResponse* _response,
                                                NetworkErrorCode _status) {
  if (_status != OK) {
    LOG(ERROR) << "NonOk network code " << _status << " for register response from ckptmgr "
               << "running at " << get_clientoptions().get_host() << ":"
               << get_clientoptions().get_port();
    delete _response;
    Stop();
    return;
  }
  proto::system::StatusCode status = _response->status().status();
  if (status != proto::system::OK) {
    LOG(ERROR) << "NonOK register response " << status << " from ckptmgr"
               << " running at " << get_clientoptions().get_host() << ":"
               << get_clientoptions().get_port();
    Stop();
  } else {
    LOG(INFO) << "Register request got response OK from ckptmgr"
              << " running at " << get_clientoptions().get_host() << ":"
              << get_clientoptions().get_port();
  }
  delete _response;
}

void CkptMgrClient::OnReconnectTimer() { Start(); }

void CkptMgrClient::SendRegisterRequest() {
  auto request = new proto::ckptmgr::RegisterTMasterRequest();
  request->set_topology_name(topology_name_);
  request->set_topology_id(topology_id_);
  SendRequest(request, NULL);
  return;
}


void CkptMgrClient::SendCleanStatefulCheckpointRequest(const std::string& _oldest_ckpt,
                                                       bool _clean_all) {
  LOG(INFO) << "Sending CleanStatefulCheckpoint request to ckptmgr with oldest "
            << _oldest_ckpt << " and clean_all " << _clean_all;
  auto request = new proto::ckptmgr::CleanStatefulCheckpointRequest();
  request->set_oldest_checkpoint_preserved(_oldest_ckpt);
  request->set_clean_all_checkpoints(_clean_all);
  SendRequest(request, NULL);
}

void CkptMgrClient::HandleCleanStatefulCheckpointResponse(void*,
                                     proto::ckptmgr::CleanStatefulCheckpointResponse* _response,
                                     NetworkErrorCode status) {
  LOG(INFO) << "Got CleanStatefulCheckpoint response from ckptmgr";
  proto::system::StatusCode code = proto::system::OK;
  if (status != OK) {
    code = proto::system::NOTOK;
  } else {
    code = _response->status().status();
  }
  clean_response_watcher_(code);
  delete _response;
}
}  // namespace tmaster
}  // namespace heron

