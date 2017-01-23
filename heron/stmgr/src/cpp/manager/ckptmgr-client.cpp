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
#include "config/heron-internals-config-reader.h"

namespace heron {
namespace ckptmgr {

CkptMgrClient::CkptMgrClient(EventLoop* eventloop, const NetworkOptions& _options,
                             const sp_string& _topology_name, const sp_string& _topology_id,
                             const sp_string& _ckptmgr_id, const sp_string& _stmgr_id,
                             std::function<void(const proto::system::Instance&,
                                                const std::string&)> _ckpt_saved_watcher,
                             std::function<void(sp_int32,
                               const proto::ckptmgr::InstanceStateCheckpoint&)> _ckpt_get_watcher,
                             std::function<void()> _register_watcher)
    : Client(eventloop, _options),
      topology_name_(_topology_name),
      topology_id_(_topology_id),
      ckptmgr_id_(_ckptmgr_id),
      stmgr_id_(_stmgr_id),
      quit_(false),
      ckpt_saved_watcher_(_ckpt_saved_watcher),
      ckpt_get_watcher_(_ckpt_get_watcher),
      register_watcher_(_register_watcher) {

  reconnect_cpktmgr_interval_sec_ =
    config::HeronInternalsConfigReader::Instance()->GetHeronStreammgrClientReconnectIntervalSec();

  InstallResponseHandler(new proto::ckptmgr::RegisterStMgrRequest(),
                         &CkptMgrClient::HandleStMgrRegisterResponse);
  InstallResponseHandler(new proto::ckptmgr::SaveInstanceStateRequest(),
                         &CkptMgrClient::HandleSaveInstanceStateResponse);
  InstallResponseHandler(new proto::ckptmgr::GetInstanceStateRequest(),
                         &CkptMgrClient::HandleGetInstanceStateResponse);
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
    LOG(INFO) << "Connected to ckptmgr " << ckptmgr_id_ << " running at "
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
    LOG(INFO) << "We closed our server connection with cpktmgr " << ckptmgr_id_ << " running at "
              << get_clientoptions().get_host() << ":" << get_clientoptions().get_port()
              << std::endl;
  } else {
    LOG(INFO) << "Ckptmgr" << ckptmgr_id_ << " running at "
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

void CkptMgrClient::HandleStMgrRegisterResponse(void*,
                                                proto::ckptmgr::RegisterStMgrResponse* _response,
                                                NetworkErrorCode _status) {
  if (_status != OK) {
    LOG(ERROR) << "NonOk network code " << _status << " for register response from ckptmgr "
               << ckptmgr_id_ << "running at " << get_clientoptions().get_host() << ":"
               << get_clientoptions().get_port();
    delete _response;
    Stop();
    return;
  }
  proto::system::StatusCode status = _response->status().status();
  if (status != proto::system::OK) {
    LOG(ERROR) << "NonOK register response " << status << " from ckptmgr " << ckptmgr_id_
               << " running at " << get_clientoptions().get_host() << ":"
               << get_clientoptions().get_port();
    Stop();
  } else {
    LOG(INFO) << "Register request got response OK from ckptmgr " << ckptmgr_id_
              << " running at " << get_clientoptions().get_host() << ":"
              << get_clientoptions().get_port();
    register_watcher_();
  }
  delete _response;
}

void CkptMgrClient::OnReconnectTimer() { Start(); }

void CkptMgrClient::SendRegisterRequest() {
  auto request = new proto::ckptmgr::RegisterStMgrRequest();
  request->set_topology_name(topology_name_);
  request->set_topology_id(topology_id_);
  request->set_stmgr(stmgr_id_);
  SendRequest(request, NULL);
  return;
}


void CkptMgrClient::SaveInstanceState(proto::ckptmgr::SaveInstanceStateRequest* _request) {
  SendRequest(_request, NULL);
}

void CkptMgrClient::GetInstanceState(const proto::system::Instance& _instance,
                                     const std::string& _checkpoint_id) {
  auto request = new proto::ckptmgr::GetInstanceStateRequest();
  request->mutable_instance()->CopyFrom(_instance);
  request->set_checkpoint_id(_checkpoint_id);
  SendRequest(request, NULL);
}

void CkptMgrClient::HandleSaveInstanceStateResponse(void*,
                             proto::ckptmgr::SaveInstanceStateResponse* _response,
                             NetworkErrorCode _status) {
  if (_status != OK) {
    LOG(ERROR) << "NonOK response message for SaveInstanceStateResponse";
    delete _response;
    Stop();
    return;
  }
  if (_response->status().status() != proto::system::OK) {
    LOG(ERROR) << "CkptMgr could not save " << _response->status().status();
    delete _response;
    return;
  }
  ckpt_saved_watcher_(_response->instance(), _response->checkpoint_id());
  delete _response;
}

void CkptMgrClient::HandleGetInstanceStateResponse(void*,
                             proto::ckptmgr::GetInstanceStateResponse* _response,
                             NetworkErrorCode _status) {
  if (_status != OK) {
    LOG(ERROR) << "NonOK response message for GetInstanceStateResponse";
    delete _response;
    Stop();
    return;
  }
  if (_response->status().status() != proto::system::OK ||
      !_response->has_checkpoint()) {
    LOG(ERROR) << "CkptMgr could not get " << _response->status().status();
    delete _response;
    return;
  }
  ckpt_get_watcher_(_response->instance().info().task_id(), _response->checkpoint());
  delete _response;
}
}  // namespace ckptmgr
}  // namespace heron

