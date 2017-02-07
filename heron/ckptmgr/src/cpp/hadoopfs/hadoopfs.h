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

#if !defined(HADOOPFS_FILE_SYSTEM_H)
#define HADOOPFS_FILE_SYSTEM_H

#include <unistd.h>
#include <string>

#include "common/checkpoint.h"
#include "common/storage.h"
#include "config/config.h"
#include "hdfs/hdfs.h"

namespace heron {
namespace ckptmgr {

class HadoopFS : public Storage {
 public:
  // constructor
  explicit HadoopFS(const heron::config::Config& _config);

  // destructor
  virtual ~HadoopFS();

  static std::string storage_type() {
    return "HadoopFS";
  }

  // initialze - create hadoop connection/base directory
  virtual int initialize();

  // store the checkpoint
  virtual int store(const Checkpoint& _ckpt);

  // retrieve the checkpoint
  virtual int restore(Checkpoint& _ckpt);

  // cleanup - close hadoop connection/release resources
  virtual int cleanup();

 private:
  // get the name of the checkpoint directory
  std::string ckptDirectory(const Checkpoint& _ckpt);

  // get the name of the checkpoint file
  std::string ckptFile(const Checkpoint& _ckpt);

  // get the name of the temporary checkpoint file
  std::string tempCkptFile(const Checkpoint& _ckpt);

  // create the checkpoint directory
  int createCkptDirectory(const Checkpoint& _ckpt);

  // write the hadoop file and flush it
  bool writeSyncAll(const std::string& filename, const char* data, size_t len);

  // write the hadoop file into a temporary file and then move to the actual file
  bool writeAtomicAll(const std::string& filename, const char* data, size_t len);

 private:
  // generate the log message prefix/suffix for printing
  std::string logMessageFragment(const Checkpoint& _ckpt);

 private:
  std::string         base_dir_;
  std::string         username_;
  std::string         nnhost_;
  int16_t             nnport_;
  struct hdfsBuilder* builder_;
  hdfsFS              filesystem_;
};

}  // namespace ckptmgr
}  // namespace heron

#endif  // hadoopfs.h
