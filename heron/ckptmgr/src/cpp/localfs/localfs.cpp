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

#include "localfs/localfs.h"
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>

#include "config/config.h"
#include "localfs/localfs-config-vars.h"

namespace heron {
namespace ckptmgr {

LocalFS::LocalFS(const heron::config::Config& _config) {
  std::string stype = _config.getstr(heron::config::StatefulConfigVars::STORAGE_TYPE);
  CHECK_EQ(storage_type(), stype);

  // get the root directory for storing checkpoints
  base_dir_ = _config.getstr(LocalfsConfigVars::ROOT_DIR);
  LOG_IF(FATAL, base_dir_.empty()) << "Local File System root directory not set";
}

std::string LocalFS::ckptDirectory(const Checkpoint& _ckpt) {
  std::string directory(base_dir_ + "/");
  directory.append(_ckpt.getCkptId()).append("/");
  directory.append(_ckpt.getComponent());
  return directory;
}

std::string LocalFS::ckptFile(const Checkpoint& _ckpt) {
  std::string directory(ckptDirectory(_ckpt) + "/");
  return directory.append(_ckpt.getTaskId());
}

std::string LocalFS::tempCkptFile(const Checkpoint& _ckpt) {
  std::string directory(ckptDirectory(_ckpt) + "/");
  return directory.append(".").append(_ckpt.getTaskId());
}

std::string LocalFS::logMessageFragment(const Checkpoint& _ckpt) {
  std::string message(_ckpt.getTopology() + " ");
  message.append(_ckpt.getCkptId()).append(" ");
  message.append(_ckpt.getComponent()).append(" ");
  message.append(_ckpt.getInstance()).append(" ");
  return message;
}

int LocalFS::createCkptDirectory(const Checkpoint& _ckpt) {
  std::string directory = ckptDirectory(_ckpt);
  if (FileUtils::makePath(directory) != SP_OK) {
    LOG(ERROR) << "Unable to create directory " << directory;
    return SP_NOTOK;
  }
  return SP_OK;
}

int LocalFS::store(const Checkpoint& _ckpt) {
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

int LocalFS::restore(Checkpoint& _ckpt) {
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
