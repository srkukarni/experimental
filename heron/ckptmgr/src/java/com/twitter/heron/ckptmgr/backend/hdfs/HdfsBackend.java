// Copyright 2016 Twitter. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.twitter.heron.ckptmgr.backend.hdfs;

import java.io.IOException;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;

import com.twitter.heron.ckptmgr.backend.Checkpoint;
import com.twitter.heron.ckptmgr.backend.IBackend;
import com.twitter.heron.common.basics.SysUtils;
import com.twitter.heron.proto.ckptmgr.CheckpointManager;

public class HdfsBackend implements IBackend {
  private static final Logger LOG = Logger.getLogger(HdfsBackend.class.getName());

  private static final String ROOT_PATH_KEY = "root.path";
  private static final String HDFS_CONFIG_DIR_KEY = "hdfs.config.dir";

  // TODO(mfu): Also need to specify actual hadoop jar in config and append on classpath

  private String checkpointRootPath;
  private String hdfsConfigDir;

  private FileSystem fileSystem;

  @Override
  public void init(Map<String, Object> conf) {
    LOG.info("Initialing... Config: " + conf.toString());

    hdfsConfigDir = (String) conf.get(HDFS_CONFIG_DIR_KEY);
    checkpointRootPath = (String) conf.get(ROOT_PATH_KEY);

    // TODO(mfu): Make it work with passing the config folder from classpath
    Configuration hadoopConfig = new Configuration();
    hadoopConfig.addResource(new Path(hdfsConfigDir + "core-site.xml"));
    hadoopConfig.addResource(new Path(hdfsConfigDir + "hdfs-site.xml"));
    hadoopConfig.addResource(new Path(hdfsConfigDir + "mapred-site.xml"));

    try {
      fileSystem = FileSystem.get(hadoopConfig);
    } catch (IOException e) {
      throw new RuntimeException("Failed to get hadoop file system", e);
    }


    // For test
    if (!ensureDirExists(checkpointRootPath)) {
      LOG.severe("Failed to ensure root path: " + checkpointRootPath);
    }


    Path path = new Path("/user/mfu/test-data");


    // Trey to write something
    String content = "WO cao ni ma bi";
    try {
      FSDataOutputStream out = fileSystem.create(path);
      out.write(content.getBytes());
      out.flush();
      out.close();
    } catch (IOException e) {
      LOG.log(Level.SEVERE, "Failed to write: ", e);
    }

    // Try to read something
    try {
      FSDataInputStream in = fileSystem.open(path);
      byte[] b = new byte[content.getBytes().length];
      in.readFully(b);
      String s = new String(b);
      LOG.info(s);
    } catch (IOException e) {
      LOG.log(Level.SEVERE, "Failed to read: ", e);
    }

  }

  @Override
  public void close() {
    SysUtils.closeIgnoringExceptions(fileSystem);
  }

  @Override
  public boolean store(Checkpoint checkpoint) {
    Path path = new Path(getCheckpointPath(checkpoint));

    if (!ensureDirExists(getCheckpointDir(checkpoint))) {
      LOG.warning("Failed to ensure dir exists: " + getCheckpointDir(checkpoint));
      return false;
    }

    FSDataOutputStream out = null;
    try {
      out = fileSystem.create(path);
      checkpoint.checkpoint().writeTo(out);
    } catch (IOException e) {
      LOG.log(Level.SEVERE, "Failed to persist", e);
      return false;
    } finally {
      SysUtils.closeIgnoringExceptions(out);
    }

    return true;
  }

  @Override
  public boolean restore(Checkpoint checkpoint) {
    Path path = new Path(getCheckpointPath(checkpoint));

    FSDataInputStream in = null;
    CheckpointManager.SaveInstanceStateRequest state = null;
    try {
      in = fileSystem.open(path);
      state =
          CheckpointManager.SaveInstanceStateRequest.parseFrom(in);
    } catch (IOException e) {
      LOG.log(Level.SEVERE, "Failed to read", e);
      return false;
    } finally {
      SysUtils.closeIgnoringExceptions(in);
    }

    checkpoint.setCheckpoint(state);

    return true;
  }

  protected boolean ensureDirExists(String dir) {
    Path path = new Path(dir);

    try {
      fileSystem.mkdirs(path);
      if (!fileSystem.exists(path)) {
        return false;
      }
    } catch (IOException e) {
      LOG.log(Level.SEVERE, "Failed to mkdirs: " + dir, e);
      return false;
    }

    return true;
  }

  protected String getCheckpointDir(Checkpoint checkpoint) {
    return new StringBuilder()
        .append(checkpointRootPath).append("/")
        .append(checkpoint.getCheckpointId()).append("/")
        .append(checkpoint.getComponent())
        .toString();

  }

  protected String getCheckpointPath(Checkpoint checkpoint) {
    return new StringBuilder()
        .append(getCheckpointDir(checkpoint)).append("/")
        .append(checkpoint.getTaskId())
        .toString();
  }
}
