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

package com.twitter.heron.spi.common;

import java.util.Map;

import com.twitter.heron.common.basics.ByteAmount;
import com.twitter.heron.common.basics.DryRunFormatType;
import com.twitter.heron.common.basics.PackageType;

public class Context {

  protected Context() {
  }

  public static String cluster(Config cfg) {
    return cfg.getStringValue(Key.CLUSTER);
  }

  public static String role(Config cfg) {
    return cfg.getStringValue(Key.ROLE);
  }

  public static String environ(Config cfg) {
    return cfg.getStringValue(Key.ENVIRON);
  }

  public static Boolean dryRun(Config cfg) {
    return cfg.getBooleanValue(Key.DRY_RUN);
  }

  public static DryRunFormatType dryRunFormatType(Config cfg) {
    return cfg.getDryRunFormatType(Key.DRY_RUN_FORMAT_TYPE);
  }

  public static Boolean verbose(Config cfg) {
    return cfg.getBooleanValue(Key.VERBOSE);
  }

  public static String buildVersion(Config cfg) {
    return cfg.getStringValue(Key.BUILD_VERSION);
  }

  public static String buildTime(Config cfg) {
    return cfg.getStringValue(Key.BUILD_TIME);
  }

  public static Long buildTimeStamp(Config cfg) {
    return cfg.getLongValue(Key.BUILD_TIMESTAMP);
  }

  public static String buildHost(Config cfg) {
    return cfg.getStringValue(Key.BUILD_HOST);
  }

  public static String buildUser(Config cfg) {
    return cfg.getStringValue(Key.BUILD_USER);
  }

  public static String topologyName(Config cfg) {
    return cfg.getStringValue(Key.TOPOLOGY_NAME);
  }

  public static int topologyContainerId(Config cfg) {
    return cfg.getIntegerValue(Key.TOPOLOGY_CONTAINER_ID);
  }

  public static String uploaderClass(Config cfg) {
    return cfg.getStringValue(Key.UPLOADER_CLASS);
  }

  public static String launcherClass(Config cfg) {
    return cfg.getStringValue(Key.LAUNCHER_CLASS);
  }

  public static String schedulerClass(Config cfg) {
    return cfg.getStringValue(Key.SCHEDULER_CLASS);
  }

  public static String packingClass(Config cfg) {
    return cfg.getStringValue(Key.PACKING_CLASS);
  }

  public static String repackingClass(Config cfg) {
    return cfg.getStringValue(Key.REPACKING_CLASS);
  }

  public static String stateManagerClass(Config cfg) {
    return cfg.getStringValue(Key.STATE_MANAGER_CLASS);
  }

  public static Boolean schedulerService(Config cfg) {
    return cfg.getBooleanValue(Key.SCHEDULER_IS_SERVICE);
  }

  public static String clusterFile(Config cfg) {
    return cfg.getStringValue(Key.CLUSTER_YAML);
  }

  public static String statefulFile(Config cfg) {
    return cfg.getStringValue(Key.STATEFUL_YAML);
  }

  public static String clientFile(Config cfg) {
    return cfg.getStringValue(Key.CLIENT_YAML);
  }

  public static String metricsSinksFile(Config cfg) {
    return cfg.getStringValue(Key.METRICS_YAML);
  }

  public static String packingFile(Config cfg) {
    return cfg.getStringValue(Key.PACKING_YAML);
  }

  public static String schedulerFile(Config cfg) {
    return cfg.getStringValue(Key.SCHEDULER_YAML);
  }

  public static String stateManagerFile(Config cfg) {
    return cfg.getStringValue(Key.STATEMGR_YAML);
  }

  public static String systemFile(Config cfg) {
    return cfg.getStringValue(Key.SYSTEM_YAML);
  }

  public static String uploaderFile(Config cfg) {
    return cfg.getStringValue(Key.UPLOADER_YAML);
  }

  public static String schedulerJar(Config cfg) {
    return cfg.getStringValue(Key.SCHEDULER_JAR);
  }

  public static String schedulerProxyConnectionString(Config cfg) {
    return cfg.getStringValue(Key.SCHEDULER_PROXY_CONNECTION_STRING);
  }

  public static String schedulerProxyConnectionType(Config cfg) {
    return cfg.getStringValue(Key.SCHEDULER_PROXY_CONNECTION_TYPE);
  }

  public static String stateManagerConnectionString(Config cfg) {
    return cfg.getStringValue(Key.STATEMGR_CONNECTION_STRING);
  }

  public static String stateManagerRootPath(Config cfg) {
    return cfg.getStringValue(Key.STATEMGR_ROOT_PATH);
  }

  public static String corePackageUri(Config cfg) {
    return cfg.getStringValue(Key.CORE_PACKAGE_URI);
  }

  public static String systemConfigFile(Config cfg) {
    return cfg.getStringValue(Key.SYSTEM_YAML);
  }

  public static String topologyDefinitionFile(Config cfg) {
    return cfg.getStringValue(Key.TOPOLOGY_DEFINITION_FILE);
  }

  public static String topologyBinaryFile(Config cfg) {
    return cfg.getStringValue(Key.TOPOLOGY_BINARY_FILE);
  }

  public static String topologyPackageFile(Config cfg) {
    return cfg.getStringValue(Key.TOPOLOGY_PACKAGE_FILE);
  }

  public static PackageType topologyPackageType(Config cfg) {
    return cfg.getPackageType(Key.TOPOLOGY_PACKAGE_TYPE);
  }

  public static ByteAmount stmgrRam(Config cfg) {
    return cfg.getByteAmountValue(Key.STMGR_RAM);
  }

  public static ByteAmount instanceRam(Config cfg) {
    return cfg.getByteAmountValue(Key.INSTANCE_RAM);
  }

  public static Double instanceCpu(Config cfg) {
    return cfg.getDoubleValue(Key.INSTANCE_CPU);
  }

  public static ByteAmount instanceDisk(Config cfg) {
    return cfg.getByteAmountValue(Key.INSTANCE_DISK);
  }

  public static String heronHome(Config cfg) {
    return cfg.getStringValue(Key.HERON_HOME);
  }

  public static String heronBin(Config cfg) {
    return cfg.getStringValue(Key.HERON_BIN);
  }

  public static String heronConf(Config cfg) {
    return cfg.getStringValue(Key.HERON_CONF);
  }

  public static String heronLib(Config cfg) {
    return cfg.getStringValue(Key.HERON_LIB);
  }

  public static String heronDist(Config cfg) {
    return cfg.getStringValue(Key.HERON_DIST);
  }

  public static String heronEtc(Config cfg) {
    return cfg.getStringValue(Key.HERON_ETC);
  }

  public static String instanceClassPath(Config cfg) {
    return cfg.getStringValue(Key.INSTANCE_CLASSPATH);
  }

  public static String metricsManagerClassPath(Config cfg) {
    return cfg.getStringValue(Key.METRICSMGR_CLASSPATH);
  }

  public static String packingClassPath(Config cfg) {
    return cfg.getStringValue(Key.PACKING_CLASSPATH);
  }

  public static String schedulerClassPath(Config cfg) {
    return cfg.getStringValue(Key.SCHEDULER_CLASSPATH);
  }

  public static String ckptmgrClassPath(Config cfg) {
    return cfg.getStringValue(Key.CKPTMGR_CLASSPATH);
  }

  public static Boolean isStateful(Config cfg) {
    return cfg.getBooleanValue(Key.IS_STATEFUL);
  }

  public static String stateManagerClassPath(Config cfg) {
    return cfg.getStringValue(Key.STATEMGR_CLASSPATH);
  }

  public static String uploaderClassPath(Config cfg) {
    return cfg.getStringValue(Key.UPLOADER_CLASSPATH);
  }

  public static String javaHome(Config cfg) {
    return cfg.getStringValue(Key.JAVA_HOME);
  }

  public static String clusterJavaHome(Config cfg) {
    return cfg.getStringValue(Key.HERON_CLUSTER_JAVA_HOME);
  }

  public static String overrideFile(Config cfg) {
    return cfg.getStringValue(Key.OVERRIDE_YAML);
  }

  public static String executorBinary(Config cfg) {
    return cfg.getStringValue(Key.EXECUTOR_BINARY);
  }

  public static String stmgrBinary(Config cfg) {
    return cfg.getStringValue(Key.STMGR_BINARY);
  }

  public static String tmasterBinary(Config cfg) {
    return cfg.getStringValue(Key.TMASTER_BINARY);
  }

  public static String shellBinary(Config cfg) {
    return cfg.getStringValue(Key.SHELL_BINARY);
  }

  public static String pythonInstanceBinary(Config cfg) {
    return cfg.getStringValue(Key.PYTHON_INSTANCE_BINARY);
  }

  public static final Boolean isCleanStateCheckpoints(Config cfg) {
    return cfg.getBooleanValue(Key.IS_CLEAN_STATEFUL_CHECKPOINTS);
  }

  @SuppressWarnings("unchecked")
  public static final String stateBackendSandboxClassPath(Config config) {
    String stateBackend = config.getStringValue(Key.CHECKPOINT_MGR_STATEFUL_BACKEND_ID);
    Map<String, Object> backendConfig = (Map<String, Object>) config.get(stateBackend);
    Object o = backendConfig.get("classpath");
    return o == null ? "" : (String) o;
  }
}
