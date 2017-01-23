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

#include "config/stateful-config-reader.h"
#include "config/stateful-config-vars.h"
#include <string>
#include "network/network.h"

namespace heron {
namespace config {

// Global initialization to facilitate singleton design pattern
StatefulConfigReader* StatefulConfigReader::stateful_config_reader_ = 0;

std::unordered_map<std::string, Config>* StatefulConfigReader::storage_configs_ = 0;

StatefulConfigReader::StatefulConfigReader(EventLoop* eventLoop,
                                           const sp_string& _defaults_file)
    : YamlFileReader(eventLoop, _defaults_file) {
  LoadConfig();

  for (auto it = config_.begin(); it != config_.end(); ++it) {
    if (it->second.IsMap()) {
      std::string storage_type = it->first.as<std::string>();
      Config conf = BuildStorageConfig(it->second);
      auto pair = std::make_pair(storage_type, BuildStorageConfig(it->second));
      LOG(INFO) << "trying to insert pair: " << pair.first << std::endl;

//      storage_configs_->emplace(storage_type, conf);

      storage_configs_->insert(pair);
// 33 PC: @        0x10a1bbc85 std::__1::__hash_table<>::__node_insert_unique()
// 34 *** SIGSEGV (@0x8) received by PID 26982 (TID 0x7fff73e9b300) stack trace: ***
// 35     @     0x7fff87849f1a _sigtramp
// 36     @     0x7f9161403250 (unknown)
// 37     @        0x10a1bad2b std::__1::__hash_table<>::__insert_unique<>()
// 38     @        0x10a1abfb1 heron::config::StatefulConfigReader::StatefulConfigReader()
// 39     @        0x10a1ac785 heron::config::StatefulConfigReader::StatefulConfigReader()
// 40     @        0x10a1aca05 heron::config::StatefulConfigReader::Create()
// 41     @        0x10a18aa73 main
// 42     @     0x7fff8d9545c9 start
// 43     @                0x6 (unknown)

      // (*storage_configs_)[storage_type] = conf;
//     64 PC: @        0x106be6ccd std::__1::__hash_table<>::find<>()
//     65 *** SIGSEGV (@0x8) received by PID 26774 (TID 0x7fff73e9b300) stack trace: ***
//     66     @     0x7fff87849f1a _sigtramp
//     67     @     0x7fdae1d02440 (unknown)
//     68     @        0x106bd6eb2 std::__1::unordered_map<>::operator[]()
//     69     @        0x106bd5a79 heron::config::StatefulConfigReader::StatefulConfigReader()
//     70     @        0x106bd61a5 heron::config::StatefulConfigReader::StatefulConfigReader()
//     71     @        0x106bd6425 heron::config::StatefulConfigReader::Create()
//     72     @        0x106bb45b3 main
//     73     @     0x7fff8d9545c9 start


      // storage_configs_->insert(std::make_pair(storage_type, BuildStorageConfig(it->second)));
    }
  }
}

StatefulConfigReader::~StatefulConfigReader() {
  delete storage_configs_;
  delete stateful_config_reader_;
}

StatefulConfigReader* StatefulConfigReader::Instance() {
  if (stateful_config_reader_ == 0) {
    LOG(FATAL) << "Singleton StatefulConfigReader has not been created";
  }

  return stateful_config_reader_;
}

bool StatefulConfigReader::Exists() {
  return (stateful_config_reader_ != NULL);
}

void StatefulConfigReader::Create(EventLoop* eventLoop, const sp_string& _defaults_file) {
  if (stateful_config_reader_) {
    LOG(FATAL) << "Singleton StatefulConfigReader has already been created";
  } else {
    stateful_config_reader_ = new StatefulConfigReader(eventLoop, _defaults_file);
    storage_configs_ = new std::unordered_map<std::string, Config>();
  }
}

void StatefulConfigReader::Create(const sp_string& _defaults_file) {
  Create(NULL, _defaults_file);
}

void StatefulConfigReader::OnConfigFileLoad() {
  // Do Nothing
}

sp_string StatefulConfigReader::GetDefaultCheckpointStorageType() {
  return config_[StatefulConfigVars::STORAGE_TYPE].as<std::string>();
}

Config StatefulConfigReader::GetStorageConfig(std::string storage_type) {
  return storage_configs_->at(storage_type);
}

Config StatefulConfigReader::BuildStorageConfig(YAML::Node map_node) {
  if (!map_node.IsMap()) {
    LOG(ERROR) << "configs for each storage type must be grouped into a map" << std::endl;
    exit(1);
  }

  auto config_map_builder = heron::config::Config::Builder();

  for (auto it = map_node.begin(); it != map_node.end(); ++it) {
    if (!it->second.IsScalar()) {
      LOG(ERROR) << "nested config value provided" << std::endl;
      exit(1);
    }

    config_map_builder.putstr(it->first.as<std::string>(), it->second.as<std::string>());
  }

  return config_map_builder.build();
}

}  // namespace config
}  // namespace heron
