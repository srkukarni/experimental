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

#ifndef __TMASTER_STATEFUL_RESTORER_H_
#define __TMASTER_STATEFUL_RESTORER_H_

#include <set>
#include <string>
#include "network/network.h"
#include "manager/tmaster.h"
#include "basics/basics.h"

namespace heron {
namespace tmaster {

class StatefulRestorer {
 public:
  explicit StatefulRestorer(std::function<void()> _after_2pc_cb);
  virtual ~StatefulRestorer();
  // Start a new 2PC with this checkpoint_id
  void Start(const std::string& _checkpoint_id, const StMgrMap& _stmgrs);
  void HandleRestored(const std::string& _stmgr_id,
                      const std::string& _checkpoint_id,
                      int64_t _restore_txid,
                      const StMgrMap& _stmgrs);

 private:
  void Finish2PC(const StMgrMap& _stmgrs);
  bool in_progress_;
  int64_t restore_txid_;
  std::string checkpoint_id_in_progress_;
  std::set<std::string> unreplied_stmgrs_;
  std::function<void()> after_2pc_cb_;
};
}  // namespace tmaster
}  // namespace heron

#endif
