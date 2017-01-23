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

StatefulConfigReader::StatefulConfigReader(EventLoop* eventLoop,
                                           const sp_string& _defaults_file)
    : YamlFileReader(eventLoop, _defaults_file) {
  LoadConfig();
}

StatefulConfigReader::~StatefulConfigReader() { delete stateful_config_reader_; }

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
  }
}

void StatefulConfigReader::Create(const sp_string& _defaults_file) {
  Create(NULL, _defaults_file);
}

void StatefulConfigReader::OnConfigFileLoad() {
  // Do Nothing
}

sp_string StatefulConfigReader::GetCheckpointStorageType() {
  return config_[StatefulConfigVars::STORAGE_TYPE].as<std::string>();
}

Config StatefulConfigReader::GetConfigMap() {
  auto config_map_builder = heron::config::Config::Builder();

  for (auto it = config_.begin(); it != config_.end(); ++it) {
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
