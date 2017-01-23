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

#include <stdlib.h>

#include <iostream>
#include <string>

#include "basics/basics.h"
#include "config/config.h"
#include "network/network.h"
#include "proto/messages.h"
#include "localfs/localfs.h"
#include "manager/ckptmgr.h"

heron::ckptmgr::Storage*
GetStorageInstance(const heron::config::Config& config) {
  std::string storage_type = config.getstr(heron::config::StatefulConfigVars::STORAGE_TYPE);

  LOG(INFO) << "Storage type: " << storage_type;
  if (storage_type == heron::ckptmgr::LocalFS::storage_type()) {
    return new heron::ckptmgr::LocalFS(config);
  }

  LOG(FATAL) << "Unknown storage type " <<  storage_type;
}

int main(int argc, char* argv[]) {
  if (argc != 9) {
    std::cout << "Usage: " << argv[0] << " "
              << "<topname> <topid> <ckptmgr_id> <myport> <stateful_config_filename> "
              << "<cluster> <role> <environ>"
              << std::endl;
    ::exit(1);
  }

  std::string topology_name = argv[1];
  std::string topology_id = argv[2];
  std::string ckptmgr_id = argv[3];
  sp_int32 my_port = atoi(argv[4]);
  std::string stateful_config_filename = argv[5];
  std::string cluster = argv[6];
  std::string role = argv[7];
  std::string environ = argv[8];

  // initialize glog and other chores
  heron::common::Initialize(argv[0], ckptmgr_id.c_str());

  // Read stateful config from local file
  heron::config::StatefulConfigReader::Create(&ss, stateful_config_filename);

//  std::string home_dir(::getenv("HOME"));
//  home_dir.append("/").append(".herondata");
//  home_dir.append("/").append("topologies");
//  home_dir.append("/").append("local");
//  home_dir.append("/").append(::getenv("USER"));
//  home_dir.append("/").append(topology_name);
//  home_dir.append("/").append("state");

  // declare the event loop
  EventLoopImpl ss;

  // Read stateful config from local file
  heron::config::StatefulConfigReader::Create(&ss, stateful_config_filename);
  std::string storage_type =
    heron::config::StatefulConfigReader::Instance()->GetDefaultCheckpointStorageType();
  auto state_config =
    heron::config::StatefulConfigReader::Instance()->GetStorageConfig(storage_type);

  LOG(INFO) << "Successfully read config " << std::endl;

  // construct a full config that includes environment and expand, if necessary
  auto full_config = state_config
    .putstr(heron::config::EnvironVars::CLUSTER, cluster)
    .putstr(heron::config::EnvironVars::ROLE, role)
    .putstr(heron::config::EnvironVars::ENVIRON, environ)
    .putstr(heron::config::EnvironVars::TOPOLOGY_NAME, topology_name)
    .putstr(heron::config::StatefulConfigVars::STORAGE_TYPE, storage_type)
    .putall(state_config)
    .build()
    .expand();

  LOG(INFO) << "Successfully constructed full config ";

  // get an instance of the storage instance
  heron::ckptmgr::Storage* storage = ::GetStorageInstance(full_config);

  // start the check point manager
  heron::ckptmgr::CkptMgr mgr(&ss, my_port, topology_name, topology_id, ckptmgr_id, storage);
  mgr.Init();
  ss.loop();

  delete storage;
  return 0;
}
