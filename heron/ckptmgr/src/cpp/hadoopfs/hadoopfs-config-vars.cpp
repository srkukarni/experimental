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

#include "hadoopfs/hadoopfs-config-vars.h"
#include <string>

namespace heron {
namespace ckptmgr {

const std::string HadoopfsConfigVars::ROOT_DIR = "heron.stateful.hdfs.root.path";
const std::string HadoopfsConfigVars::USER_NAME = "heron.stateful.hdfs.username";
const std::string HadoopfsConfigVars::NN_HOST = "heron.stateful.hdfs.namenode.host";
const std::string HadoopfsConfigVars::NN_PORT = "heron.stateful.hdfs.namenode.port";

}  // namespace ckptmgr
}  // namespace heron
