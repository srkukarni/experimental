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

#include "manager/ckptmgr-server.h"
#include <iostream>

namespace heron {
namespace ckptmgr {

CkptMgrServer::CkptMgrServer(EventLoop* eventloop, const NetworkOptions& _options,
                             const sp_string& _topology_name, const sp_string& _topology_id,
                             const sp_string& _ckptmgr_id, CkptMgr* _ckptmgr)
    : Server(eventloop, _options),
      topology_name_(_topology_name),
      topology_id_(_topology_id),
      ckptmgr_id_(_ckptmgr_id),
      ckptmgr_(_ckptmgr),
      stmgr_conn_(NULL) {

    // handlers
    InstallRequestHandler(&CkptMgrServer::HandleStMgrRegisterRequest);
    InstallRequestHandler(&CkptMgrServer::HandleSaveInstanceStateRequest);
}

CkptMgrServer::~CkptMgrServer() {
  Stop();
}

void CkptMgrServer::HandleNewConnection(Connection* _conn) {
  // Do nothing here, wait for the hello from stmgr
  LOG(INFO) << "Got new connection" << _conn << " from " << _conn->getIPAddress() << ":"
            << _conn->getPort();
}

void CkptMgrServer::HandleConnectionClose(Connection* _conn, NetworkErrorCode) {
  LOG(INFO) << "Got connection close of " << _conn << " from " << _conn->getIPAddress() << ":"
            << _conn->getPort();
}

void CkptMgrServer::HandleStMgrRegisterRequest(REQID _id, Connection* _conn,
                                            proto::ckptmgr::RegisterStMgrRequest* _request) {
  LOG(INFO) << "Got a register message from " << _request->stmgr() << " on connection" << _conn;
  proto::ckptmgr::RegisterStMgrResponse response;
  // Some basic checks
  if (_request->topology_name() != topology_name_) {
    LOG(ERROR) << "The register message was from a different topology " << _request->topology_name()
               << std::endl;
    response.mutable_status()->set_status(proto::system::NOTOK);
  } else if (_request->topology_id() != topology_id_) {
    LOG(ERROR) << "The register message was from a different topology id" << _request->topology_id()
               << std::endl;
    response.mutable_status()->set_status(proto::system::NOTOK);
  } else if (stmgr_conn_ != NULL) {
    LOG(WARNING) << "We already have an active connection from the stmgr " << _request->stmgr()
                 << ". Closing existing connection...";
    stmgr_conn_->closeConnection();
    response.mutable_status()->set_status(proto::system::NOTOK);
  } else {
    stmgr_conn_ = _conn;
    response.mutable_status()->set_status(proto::system::OK);
  }

  SendResponse(_id, _conn, response);
  delete _request;
}

void CkptMgrServer::HandleSaveInstanceStateRequest(REQID _id, Connection* _conn,
                                        heron::proto::ckptmgr::SaveInstanceStateRequest* _req) {
  Checkpoint checkpoint(topology_name_, _req);
  LOG(INFO) << "Got a save checkpoint for " << checkpoint.getCkptId() << " "
            << checkpoint.getComponent() << " " << checkpoint.getInstance() << " "
            << " on connection" << _conn;

  auto ret = ckptmgr_->storage()->store(checkpoint);
  proto::system::StatusCode status;
  if (ret != SP_OK) {
    LOG(ERROR) << "Checkpoint failed for " << checkpoint.getCkptId() << " "
            << checkpoint.getComponent() << " " << checkpoint.getInstance();
    status = proto::system::NOTOK;
  } else {
    status = proto::system::OK;
  }

  heron::proto::ckptmgr::SaveInstanceStateResponse response;
  response.mutable_status()->set_status(status);
  response.set_checkpoint_id(_req->checkpoint().checkpoint_id());
  response.mutable_instance()->CopyFrom(_req->instance());

  LOG(INFO) << "Checkpoint successful for " << checkpoint.getCkptId() << " "
            << checkpoint.getComponent() << " " << checkpoint.getInstance();

  SendResponse(_id, _conn, response);
}

}  // namespace ckptmgr
}  // namespace heron

