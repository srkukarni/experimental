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

#include "basics/fileutils.h"
#include <dirent.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "glog/logging.h"

#include "basics/sprcodes.h"
#include "basics/spconsts.h"

std::string FileUtils::baseName(const std::string& path) {
  return path.substr(path.find_last_of(constPathSeparator) + 1);
}

sp_int32 FileUtils::makeDirectory(const std::string& directory, mode_t mode) {
  struct stat st;

  // get the directory name stats
  auto retcode = ::stat(directory.c_str(), &st);

  // check for any errors
  if (retcode == -1) {
    if (errno == ENOENT) {
      if (::mkdir(directory.c_str(), mode) != 0) {
        PLOG(ERROR) << "Unable to create directory " << directory;
        return SP_NOTOK;
      }
      return SP_OK;
    }

    PLOG(ERROR) << "Unable to create directory " << directory;
    return SP_NOTOK;
  }

  return S_ISDIR(st.st_mode) ? SP_OK : SP_NOTOK;
}

sp_int32 FileUtils::existsDirectory(const std::string& directory) {
  struct stat info;
  if (::stat(directory.c_str(), &info) != 0) {
    PLOG(ERROR) << "Unable to stat directory " << directory;
    return SP_NOTOK;
  }

  return S_ISDIR(info.st_mode) ? SP_OK : SP_NOTOK;
}

sp_int32 FileUtils::makePath(const std::string& path) {
  mode_t mode = 0755;
  auto retcode = ::mkdir(path.c_str(), mode);
  if (retcode == 0)
    return SP_OK;

  switch (errno) {
    case ENOENT: {
      // parent directory does not exist, try to create it
      auto pos = path.find_last_of('/');
      if (pos == std::string::npos)
        return SP_NOTOK;

      auto retcode = makePath(path.substr(0, pos));
      if (retcode == SP_NOTOK)
        return SP_NOTOK;

      // now, try to create again
      return makePath(path);
    }
    case EEXIST:
    // done!
    return existsDirectory(path);

    default:
    return SP_NOTOK;
  }
  return SP_NOTOK;
}


sp_int32 FileUtils::removeFile(const std::string& filepath) {
  if (::unlink(filepath.c_str()) != 0) {
    PLOG(ERROR) << "Unable to delete file " << filepath;
    return SP_NOTOK;
  }
  return SP_OK;
}

sp_int32 FileUtils::removeRecursive(const std::string& directory, bool delete_self) {
  auto dir = ::opendir(directory.c_str());
  if (dir == nullptr) {
    PLOG(ERROR) << "opendir failed for " << directory;
    return SP_NOTOK;
  }

  // list all dir contents
  struct dirent* entry = nullptr;
  while ((entry = ::readdir(dir)) != nullptr) {
    auto current_dir = std::string(entry->d_name).compare(constCurrentDirectory);
    auto parent_dir = std::string(entry->d_name).compare(constParentDirectory);

    if (current_dir != 0 && parent_dir != 0) {
      auto path = directory + constPathSeparator + std::string(entry->d_name);
      if (entry->d_type == DT_DIR) {
        if (removeRecursive(path, true) == SP_NOTOK) return SP_NOTOK;
      } else {
        if (::unlink(path.c_str()) != 0) {
          PLOG(ERROR) << "Unable to delete " << path;
          return SP_NOTOK;
        }
      }
    }
  }

  ::closedir(dir);

  // now delete the dir
  if (delete_self) {
    if (::rmdir(directory.c_str()) != 0) {
      PLOG(ERROR) << "Unable to delete directory " << directory;
      return SP_NOTOK;
    }
  }

  return SP_OK;
}

sp_int32 FileUtils::listFiles(const std::string& directory, std::vector<std::string>& files) {
  auto dir = ::opendir(directory.c_str());
  if (dir != nullptr) {
    struct dirent* ent;
    while ((ent = ::readdir(dir)) != nullptr) {
      auto current_dir = std::string(ent->d_name).compare(constCurrentDirectory);
      auto parent_dir = std::string(ent->d_name).compare(constParentDirectory);
      if (current_dir != 0 && parent_dir != 0) {
        files.push_back(ent->d_name);
      }
    }

    ::closedir(dir);
    return SP_OK;
  }

  // log errors that we could not open the dir
  PLOG(ERROR) << "Unable to open directory " << directory;
  return SP_NOTOK;
}

std::string FileUtils::readAll(const std::string& file) {
  std::ifstream in(file.c_str(), std::ifstream::in | std::ifstream::binary);
  std::stringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

time_t FileUtils::getModifiedTime(const std::string& file) {
  struct stat attrib;
  if (::stat(file.c_str(), &attrib) == 0) {
    return attrib.st_mtime;
  }

  PLOG(ERROR) << "Unable to get file modified time for " << file;
  return SP_NOTOK;
}

bool FileUtils::is_symlink(const std::string& filepath) {
  struct stat attrib;
  if (::lstat(filepath.c_str(), &attrib) == 0) {
    return S_ISLNK(attrib.st_mode);
  }

  PLOG(ERROR) << "Unable to check if file is a symlink " << filepath;
  return false;
}

bool FileUtils::writeAll(const std::string& filename, const char* data, size_t len) {
  std::ofstream ot(filename.c_str(), std::ios::out | std::ios::binary);
  if (!ot) return false;
  ot.write(reinterpret_cast<const char*>(data), len);
  ot.close();
  return true;
}

bool FileUtils::writeSyncAll(const std::string& filename, const char* data, size_t len) {
  // open the file for creation and write only mode
  auto fd = ::open(filename.c_str(), O_CREAT | O_WRONLY, 0644);
  if (fd < 0) {
    PLOG(ERROR) << "Unable to open file " << filename;
    return false;
  }

  // write the contents of the file
  size_t count = 0;
  while (count < len) {
    int i = ::write(fd, data + count, len - count);
    if (i < 0) {
      PLOG(ERROR) << "Unable to write contents to file " << filename;
      return false;
    }
    count += i;
  }

  // force flush the file contents to persistent store
  auto code = ::fsync(fd);
  if (code < 0) {
    PLOG(ERROR) << "Unable to sync file " << filename;
    return false;
  }

  // close the file descriptor
  code = ::close(fd);
  if (code < 0) {
    PLOG(ERROR) << "Unable to close file " << filename;
    return false;
  }
  return true;
}

bool FileUtils::writeAtomicAll(const std::string& filename, const char* data, size_t len) {
  // form a temporary file name
  size_t pos = filename.find_last_of("/");
  if (pos == filename.size() - 1) {
    LOG(ERROR) << "Specified filename " << filename << " is a directory" << std::endl;
    return false;
  }

  std::string newfile;
  if (pos == std::string::npos) {
    newfile.append(".").append(filename);
  } else {
     newfile.append(filename.substr(0, pos + 1));
     newfile.append(".").append(filename.substr(pos+1));
  }

  // Write and flush the contents of the file
  if (!FileUtils::writeSyncAll(newfile, data, len))
    return false;

  // rename the file for atomic write
  return FileUtils::rename(newfile, filename);
}

bool FileUtils::rename(const std::string& from, const std::string& to) {
  auto code = ::rename(from.c_str(), to.c_str());
  if (code < 0) {
    PLOG(ERROR) << "Unable to rename file " << from << " to " << to;
    return false;
  }
  return true;
}

sp_int32 FileUtils::getCwd(std::string& path) {
  char maxpath[MAXPATHLEN];
  if (::getcwd(maxpath, MAXPATHLEN) == nullptr) {
    PLOG(ERROR) << "Could not get the current working directory";
    return SP_NOTOK;
  }

  path = maxpath;
  return SP_OK;
}

std::string FileUtils::getHomeDirectory() {
  const char *homedir = ::getenv("HOME");
  if (homedir != nullptr) {
    return std::string(homedir);
  }

  auto bufsize = ::sysconf(_SC_GETPW_R_SIZE_MAX);
  if (bufsize == -1)
    bufsize = 16384;

  char* buffer = new char[bufsize];
  struct passwd pwd, *result = NULL;

  auto code = ::getpwuid_r(::getuid(), &pwd, buffer, bufsize, &result);
  if (code != 0 || !result) {
    PLOG(ERROR) << "Unable to get home directory ";
    return std::string();
  }

  std::string directory(pwd.pw_dir);
  delete [] buffer;
  return directory;
}
