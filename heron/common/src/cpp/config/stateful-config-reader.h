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

#ifndef STATEFUL_CONFIG_READER_H
#define STATEFUL_CONFIG_READER_H

#include <unordered_map>
#include <string>
#include "basics/sptypes.h"
#include "config/yaml-file-reader.h"
#include "config/config-map.h"

class EventLoop;

namespace heron {
namespace config {

class StatefulConfigReader : public YamlFileReader {
 public:
  static StatefulConfigReader* Instance();
  static bool Exists();
  static void Create(EventLoop* eventLoop, const sp_string& _defaults_file);
  static void Create(const sp_string& _defaults_file);

  virtual void OnConfigFileLoad();

  sp_string GetDefaultCheckpointStorageType();
  Config GetStorageConfig(const std::string& storage_type);

 protected:
  StatefulConfigReader(EventLoop* eventLoop, const sp_string& _defaults_file);
  virtual ~StatefulConfigReader();

  Config BuildStorageConfig(const YAML::Node& map_node);

  std::unordered_map<std::string, Config> storage_configs_;

  static StatefulConfigReader* stateful_config_reader_;
};

}  // namespace config
}  // namespace heron

#endif  // STATEFUL_CONFIG_READER_H
