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

#include "hadoopfs/hadoopfs.h"
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>

#include "config/config.h"
#include "hadoopfs/hadoopfs-config-vars.h"

namespace heron {
namespace ckptmgr {

HadoopFS::HadoopFS(const heron::config::Config& _config) {
  std::string stype = _config.getstr(heron::config::StatefulConfigVars::STORAGE_TYPE);
  CHECK_EQ(storage_type(), stype);

  // get the root directory for storing checkpoints
  base_dir_ = _config.getstr(HadoopfsConfigVars::ROOT_DIR);
  LOG_IF(FATAL, base_dir_.empty()) << "Hadoop File System root directory not set";

  // get the user name to store the checkpoints
  username_ = _config.getstr(HadoopfsConfigVars::USER_NAME, UserUtils::getUserName());
  LOG(INFO) << "Using username '" << UserUtils::getUserName() << "'";

  // get the name node of the hadoop host
  nnhost_ = _config.getstr(HadoopfsConfigVars::NN_HOST);
  LOG_IF(FATAL, nnhost_.empty()) << "Hadoop name node address not set";

  // get the name node port for hadoop, if not set default
  nnport_ = _config.getint16(HadoopfsConfigVars::NN_PORT, 50070);

  builder_ = hdfsNewBuilder();
  CHECK(builder_ != nullptr);
  hdfsBuilderSetNameNode(builder_, nnhost_.c_str());
  hdfsBuilderSetNameNodePort(builder_, nnport_);
  hdfsBuilderSetUserName(builder_, username_.c_str());
}

HadoopFS::~HadoopFS() {
  if (builder_ != nullptr) {
    hdfsFreeBuilder(builder_);
  }
}

std::string HadoopFS::ckptDirectory(const Checkpoint& _ckpt) {
  std::string directory(base_dir_ + "/");
  directory.append(_ckpt.getCkptId()).append("/");
  directory.append(_ckpt.getComponent());
  return directory;
}

std::string HadoopFS::ckptFile(const Checkpoint& _ckpt) {
  std::string directory(ckptDirectory(_ckpt) + "/");
  return directory.append(_ckpt.getTaskId());
}

std::string HadoopFS::tempCkptFile(const Checkpoint& _ckpt) {
  std::string directory(ckptDirectory(_ckpt) + "/");
  return directory.append(".").append(_ckpt.getTaskId());
}

std::string HadoopFS::logMessageFragment(const Checkpoint& _ckpt) {
  std::string message(_ckpt.getTopology() + " ");
  message.append(_ckpt.getCkptId()).append(" ");
  message.append(_ckpt.getComponent()).append(" ");
  message.append(_ckpt.getInstance()).append(" ");
  return message;
}

int HadoopFS::createCkptDirectory(const Checkpoint& _ckpt) {
  std::string directory = ckptDirectory(_ckpt);

  if (hdfsCreateDirectory(filesystem_, directory.c_str()) == -1) {
    LOG(ERROR) << "Unable to create directory " << directory << " " << hdfsGetLastError();
    return SP_NOTOK;
  }
  return SP_OK;
}

int HadoopFS::initialize() {
  filesystem_ = hdfsBuilderConnect(builder_);
  if (filesystem_ == nullptr) {
    LOG(INFO) << "Unable to connect to Hadoop fs";
    return SP_NOTOK;
  }
  LOG(INFO) << "Successfully connected to Hadoop fs";

  hdfsCreateDirectory(filesystem_, base_dir_.c_str());
  LOG(INFO) << "Successfully created base directory " << base_dir_;

  return SP_OK;
}

int HadoopFS::cleanup() {
  hdfsDisconnect(filesystem_);
  return SP_OK;
}

int HadoopFS::store(const Checkpoint& _ckpt) {
  std::string path = ckptFile(_ckpt);
  // create the checkpoint directory, if not there
  if (createCkptDirectory(_ckpt) == SP_NOTOK) {
    LOG(ERROR) << "Failed to create dir "
      << ckptDirectory(_ckpt) << " for " << logMessageFragment(_ckpt);
    return SP_NOTOK;
  }

  // write the contents atomically to file
  size_t len = _ckpt.nbytes();
  std::string buf;
  _ckpt.checkpoint()->SerializeToString(&buf);

  if (!FileUtils::writeAtomicAll(path, buf.c_str(), len)) {
    LOG(ERROR) << "Failed to checkpoint " << path << " for " << logMessageFragment(_ckpt);
    return SP_NOTOK;
  }

  return SP_OK;
}

int HadoopFS::restore(Checkpoint& _ckpt) {
  std::string path = ckptFile(_ckpt);

  // open the checkpoint file
  std::ifstream ifile(path, std::ifstream::in | std::ifstream::binary);
  if (!ifile.is_open()) {
    PLOG(ERROR) << "Failed to open checkpoint file " << path;
    LOG(ERROR) << "Failed to restore checkpoint for " << logMessageFragment(_ckpt);
    return SP_NOTOK;
  }

  // read the protobuf from checkpoint file
  auto savedbytes = new ::heron::proto::ckptmgr::SaveInstanceStateRequest;
  if (!savedbytes->ParseFromIstream(&ifile)) {
    LOG(ERROR) << "Failed to restore checkpoint from " << path
      << " for "<< logMessageFragment(_ckpt);
    return SP_NOTOK;
  }

  // pass the retrieved bytes to checkpoint
  _ckpt.set_checkpoint(savedbytes);

  // close the checkpoint file
  ifile.close();
  return SP_OK;
}

}  // namespace ckptmgr
}  // namespace heron
