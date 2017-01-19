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

package com.twitter.heron.instance;

import java.io.Serializable;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.google.protobuf.Message;

import com.twitter.heron.api.Config;
import com.twitter.heron.api.generated.TopologyAPI;
import com.twitter.heron.api.state.HashMapState;
import com.twitter.heron.api.state.State;
import com.twitter.heron.api.utils.Utils;
import com.twitter.heron.common.basics.Communicator;
import com.twitter.heron.common.basics.SingletonRegistry;
import com.twitter.heron.common.basics.SlaveLooper;
import com.twitter.heron.common.config.SystemConfig;
import com.twitter.heron.common.utils.metrics.MetricsCollector;
import com.twitter.heron.common.utils.misc.PhysicalPlanHelper;
import com.twitter.heron.common.utils.misc.ThreadNames;
import com.twitter.heron.instance.bolt.BoltInstance;
import com.twitter.heron.instance.spout.SpoutInstance;
import com.twitter.heron.proto.ckptmgr.CheckpointManager;
import com.twitter.heron.proto.system.Metrics;

/**
 * The slave, which in fact is a InstanceFactory, creates a new spout or bolt according to the PhysicalPlan.
 * First, if the instance is null, it will wait for the PhysicalPlan from inQueue and, if it receives one,
 * will instantiate a new instance (spout or bolt) according to the PhysicalPlanHelper in SingletonRegistry.
 * It is a Runnable so it could be executed in a Thread. During run(), it will begin the SlaveLooper's loop().
 */

public class Slave implements Runnable, AutoCloseable {
  private static final Logger LOG = Logger.getLogger(Slave.class.getName());

  private final SlaveLooper slaveLooper;
  private final MetricsCollector metricsCollector;
  // Communicator
  private final Communicator<Message> streamInCommunicator;
  private final Communicator<Message> streamOutCommunicator;
  private final Communicator<InstanceControlMsg> inControlQueue;
  private IInstance instance;
  private PhysicalPlanHelper helper;
  private SystemConfig systemConfig;

  private boolean isInstanceStarted = false;

  private final State instanceState;
  private boolean isStatefulProcessingStarted;

  public Slave(SlaveLooper slaveLooper,
               final Communicator<Message> streamInCommunicator,
               final Communicator<Message> streamOutCommunicator,
               final Communicator<InstanceControlMsg> inControlQueue,
               final Communicator<Metrics.MetricPublisherPublishMessage> metricsOutCommunicator) {
    this.slaveLooper = slaveLooper;
    this.streamInCommunicator = streamInCommunicator;
    this.streamOutCommunicator = streamOutCommunicator;
    this.inControlQueue = inControlQueue;

    // TODO(mfu): Create the state with corresponding type according to config
    instanceState = new HashMapState();
    isStatefulProcessingStarted = false;

    this.systemConfig =
        (SystemConfig) SingletonRegistry.INSTANCE.getSingleton(SystemConfig.HERON_SYSTEM_CONFIG);

    this.metricsCollector = new MetricsCollector(slaveLooper, metricsOutCommunicator);

    handleControlMessage();
  }

  private void handleControlMessage() {
    Runnable handleControlMessageTask = new Runnable() {
      @Override
      public void run() {
        while (!inControlQueue.isEmpty()) {
          InstanceControlMsg instanceControlMsg = inControlQueue.poll();

          // Handle start stateful processing request
          // Pre-condition: This message is received after RestoreInstanceStateRequest
          if (instanceControlMsg.isStartInstanceStatefulProcessing()) {
            CheckpointManager.StartInstanceStatefulProcessing startRequest =
                instanceControlMsg.getStartInstanceStatefulProcessing();
            LOG.info("Starting the stateful processing: " + startRequest.getCheckpointId());
            isStatefulProcessingStarted = true;

            // At this point, the pre-condition is we have already create the actual instance
            // Check if we can start the topology
            startInstanceIfNeeded();
          }

          // Handle restore instance state request
          // It can happen in 2 cases:
          // 1. Startup -- there will always be at least one physical plan coming
          // before or after the RestoreInstanceStateRequest
          // 2. Normal running -- there may not be any new physical plan
          if (instanceControlMsg.isRestoreInstanceStateRequest()) {
            CheckpointManager.RestoreInstanceStateRequest request =
                instanceControlMsg.getRestoreInstanceStateRequest();
            LOG.info("Restoring state to checkpoint id: " + request.getState().getCheckpointId());
            isStatefulProcessingStarted = false;
            // Reset the in stream queue
            streamInCommunicator.clear();

            // Stop current working instance if there is one
            if (instance != null) {
              instance.stop();
            }

            // Reset the state
            instanceState.clear();
            // TODO(mfu): Add interface to allow customized serialization instead of java default
            @SuppressWarnings("unchecked")
            Map<Serializable, Serializable> stateToRestore =
                (Map<Serializable, Serializable>) Utils.deserialize(
                    request.getState().getState().toByteArray());
            instanceState.putAll(stateToRestore);

            // 1. If during the startup time. Nothing need to be done here:
            // - We would create new instance when new "PhysicalPlan" comes
            // - Instance will start either within "StartStatefulProcessing" or new "PhysicalPlan"

            // 2. If during the normal running, we need to restart the instance
            if (isInstanceStarted && helper != null) {
              LOG.info("Restarting instance to restore state");
              resetCurrentAssignment();
            }

          }

          // Handle New Physical Plan
          if (instanceControlMsg.isNewPhysicalPlanHelper()) {
            PhysicalPlanHelper newHelper = instanceControlMsg.getNewPhysicalPlanHelper();

            // Bind the MetricsCollector with topologyContext
            newHelper.setTopologyContext(metricsCollector);

            if (helper == null) {
              helper = newHelper;
              handleNewAssignment();
            } else {

              instance.update(newHelper);

              // Handle the state changing
              if (!helper.getTopologyState().equals(newHelper.getTopologyState())) {
                switch (newHelper.getTopologyState()) {
                  case RUNNING:
                    if (!isInstanceStarted) {
                      // Start the instance if it has not yet started
                      startInstanceIfNeeded();
                    }
                    instance.activate();
                    break;
                  case PAUSED:
                    instance.deactivate();
                    break;
                  default:
                    throw new RuntimeException("Unexpected TopologyState is updated for spout: "
                        + newHelper.getTopologyState());
                }
              } else {
                LOG.info("Topology state remains the same in Slave: " + helper.getTopologyState());
              }

              // update the PhysicalPlanHelper in Slave
              helper = newHelper;
            }
          }


          // TODO:- We might handle more control Message in future
        }
      }
    };

    slaveLooper.addTasksOnWakeup(handleControlMessageTask);
  }

  private void resetCurrentAssignment() {
    instance = helper.getMySpout() != null
        ? new SpoutInstance(helper, streamInCommunicator, streamOutCommunicator, slaveLooper)
        : new BoltInstance(helper, streamInCommunicator, streamOutCommunicator, slaveLooper);

    startInstanceIfNeeded();
  }

  private void handleNewAssignment() {
    LOG.log(Level.INFO,
        "Incarnating ourselves as {0} with task id {1}",
        new Object[]{helper.getMyComponent(), helper.getMyTaskId()});

    // During the initiation of instance,
    // we would add a bunch of tasks to slaveLooper's tasksOnWakeup
    if (helper.getMySpout() != null) {
      instance =
          new SpoutInstance(helper, streamInCommunicator, streamOutCommunicator, slaveLooper);

      streamInCommunicator.init(systemConfig.getInstanceInternalSpoutReadQueueCapacity(),
          systemConfig.getInstanceTuningExpectedSpoutReadQueueSize(),
          systemConfig.getInstanceTuningCurrentSampleWeight());
      streamOutCommunicator.init(systemConfig.getInstanceInternalSpoutWriteQueueCapacity(),
          systemConfig.getInstanceTuningExpectedSpoutWriteQueueSize(),
          systemConfig.getInstanceTuningCurrentSampleWeight());
    } else {
      instance =
          new BoltInstance(helper, streamInCommunicator, streamOutCommunicator, slaveLooper);

      streamInCommunicator.init(systemConfig.getInstanceInternalBoltReadQueueCapacity(),
          systemConfig.getInstanceTuningExpectedBoltReadQueueSize(),
          systemConfig.getInstanceTuningCurrentSampleWeight());
      streamOutCommunicator.init(systemConfig.getInstanceInternalBoltWriteQueueCapacity(),
          systemConfig.getInstanceTuningExpectedBoltWriteQueueSize(),
          systemConfig.getInstanceTuningCurrentSampleWeight());
    }

    if (!helper.getTopologyState().equals(TopologyAPI.TopologyState.RUNNING)) {
      LOG.info("The instance is deployed in deactivated state");
    }

    startInstanceIfNeeded();
  }

  @Override
  public void run() {
    Thread.currentThread().setName(ThreadNames.THREAD_SLAVE_NAME);

    slaveLooper.loop();
  }

  private void startInstanceIfNeeded() {
    // To start the instance when:
    // 1. We got the PhysicalPlan
    // 2. The TopologyState == RUNNING
    // 3. If the topology is stateful
    //    - We got the stateful processing start signal
    //    If the topology is non-stateful
    if (helper != null && helper.getTopologyState().equals(TopologyAPI.TopologyState.RUNNING)) {
      Map<String, Object> config = helper.getTopologyContext().getTopologyConfig();
      boolean isTopologyStateful =
          Boolean.parseBoolean((String) config.get(Config.TOPOLOGY_STATEFUL));

      if (!isTopologyStateful || (isTopologyStateful && isStatefulProcessingStarted)) {
        instance.start(instanceState);
        isInstanceStarted = true;
        LOG.info("Started instance.");
      }
    }
  }

  public void close() {
    LOG.info("Closing the Slave Thread");
    this.metricsCollector.forceGatherAllMetrics();
    LOG.info("Cleaning up the instance");
    if (instance != null) {
      instance.stop();
    }

    // Clean the resources we own
    slaveLooper.exitLoop();
    streamInCommunicator.clear();
    // The clean of out stream communicator will be handled by instance itself
  }
}
