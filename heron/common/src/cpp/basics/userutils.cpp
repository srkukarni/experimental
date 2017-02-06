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

#include "basics/userutils.h"

#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>

#include <string>
#include "glog/logging.h"

std::string
UserUtils::getUserHome() {
  int bufsize;
  if ((bufsize = ::sysconf(_SC_GETPW_R_SIZE_MAX)) < 0) {
    PLOG(FATAL) << "Unable to get passwd size max ";
  }

  std::unique_ptr<char> buffer(new char[bufsize]);
  struct passwd pwd, *result = NULL;
  int error = ::getpwuid_r(getuid(), &pwd, buffer.get(), bufsize, &result);
  if (error != 0 || !result) {
    LOG(FATAL) << "Unable to get user password structure";
  }

  return std::string(pwd.pw_dir);
}

std::string
UserUtils::getUserName() {
  int bufsize;
  if ((bufsize = ::sysconf(_SC_GETPW_R_SIZE_MAX)) < 0) {
    PLOG(FATAL) << "Unable to get passwd size max ";
  }

  std::unique_ptr<char> buffer(new char[bufsize]);
  struct passwd pwd, *result = NULL;
  int error = ::getpwuid_r(getuid(), &pwd, buffer.get(), bufsize, &result);
  if (error != 0 || !result) {
    LOG(FATAL) << "Unable to get user password structure";
  }

  return std::string(pwd.pw_name);
}
