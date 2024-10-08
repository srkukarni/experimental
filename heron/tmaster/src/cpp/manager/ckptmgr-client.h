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

#ifndef SRC_CPP_SVCS_TMASTER_SRC_CKPTCLIENT_CLIENT_H_
#define SRC_CPP_SVCS_TMASTER_SRC_CKPTCLIENT_CLIENT_H_

#include <string>
#include "basics/basics.h"
#include "network/network.h"
#include "network/network_error.h"
#include "proto/messages.h"

namespace heron {
namespace tmaster {

class CkptMgrClient : public Client {
 public:
  CkptMgrClient(EventLoop* eventLoop, const NetworkOptions& _options,
                const sp_string& _topology_name, const sp_string& _topology_id,
                std::function<void(proto::system::StatusCode)> _clean_response_watcher);
  virtual ~CkptMgrClient();

  void Quit();

  void SendCleanStatefulCheckpointRequest(const std::string& _oldest_ckpt, bool _clean_all);

 protected:
  virtual void HandleCleanStatefulCheckpointResponse(void*,
                             proto::ckptmgr::CleanStatefulCheckpointResponse* _response,
                             NetworkErrorCode status);
  virtual void HandleConnect(NetworkErrorCode status);
  virtual void HandleClose(NetworkErrorCode status);

 private:
  void HandleTMasterRegisterResponse(void *, proto::ckptmgr::RegisterTMasterResponse *_response,
                                   NetworkErrorCode);
  void SendRegisterRequest();

  void OnReconnectTimer();

  sp_string topology_name_;
  sp_string topology_id_;
  bool quit_;
  std::function<void(proto::system::StatusCode)> clean_response_watcher_;

  // Config
  sp_int32 reconnect_cpktmgr_interval_sec_;
};

}  // namespace tmaster
}  // namespace heron

#endif  // SRC_CPP_SVCS_TMASTER_SRC_CKPTCLIENT_CLIENT_H_
