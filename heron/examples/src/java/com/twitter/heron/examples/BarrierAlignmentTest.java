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

package com.twitter.heron.examples;

import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;

import com.twitter.heron.api.Config;
import com.twitter.heron.api.HeronSubmitter;
import com.twitter.heron.api.bolt.BaseRichBolt;
import com.twitter.heron.api.bolt.IStatefulBolt;
import com.twitter.heron.api.bolt.OutputCollector;
import com.twitter.heron.api.exception.AlreadyAliveException;
import com.twitter.heron.api.exception.InvalidTopologyException;
import com.twitter.heron.api.spout.BaseRichSpout;
import com.twitter.heron.api.spout.IStatefulSpout;
import com.twitter.heron.api.spout.SpoutOutputCollector;
import com.twitter.heron.api.state.State;
import com.twitter.heron.api.topology.OutputFieldsDeclarer;
import com.twitter.heron.api.topology.TopologyBuilder;
import com.twitter.heron.api.topology.TopologyContext;
import com.twitter.heron.api.tuple.Fields;
import com.twitter.heron.api.tuple.Tuple;
import com.twitter.heron.api.tuple.Values;
import com.twitter.heron.api.utils.Utils;
import com.twitter.heron.common.basics.ByteAmount;

public final class BarrierAlignmentTest {
  private BarrierAlignmentTest() {

  }

  public static class TestWordSpout extends BaseRichSpout implements IStatefulSpout {
    private static final long serialVersionUID = -4436767162744355744L;
    private SpoutOutputCollector collector;
    private String[] words;

    private static final int EMIT_INTERVAL_MS = 1;
    private static final long TOTAL_COUNT_TO_EMIT = 10;
    private static final long EXCEPTION_THROWING_INTERVAL_COUNT = TOTAL_COUNT_TO_EMIT / 10;
    private static final double EXCEPTION_PROBABILITY = 0.3;
    private static final String KEY_EMITTED = "tuples_emitted";
    // This value will be restored from the spoutState
    private long emitted;
    // This value is not persistent
    private long emittedThisSession;

    private State spoutState;

    private int taskId;


    @Override
    public void declareOutputFields(OutputFieldsDeclarer declarer) {
      declarer.declare(new Fields("word"));
    }

    @Override
    public void open(Map<String, Object> conf,
                     TopologyContext context, SpoutOutputCollector spoutOutputCollector) {
      collector = spoutOutputCollector;
      words = new String[]{"sky", "blue", "white", "jazz", "tango"};
      taskId = context.getThisTaskId();
    }

    @Override
    public void nextTuple() {
      // The spout would emit only finite # of tuples
      if (emitted >= TOTAL_COUNT_TO_EMIT) {
        System.out.println("Done the emit. Sleep for a while...");
        Utils.sleep(2000);
        return;
      }

//      // Randomly throw exceptions when # of emitted tuples is a multiple of
//      // EXCEPTION_THROWING_INTERVAL_COUNT
//      // The probability to throw exception will increase,
//      // and it will throw at least once
//      if (emitted % EXCEPTION_THROWING_INTERVAL_COUNT == 0) {
//        double p = EXCEPTION_PROBABILITY + (double) emittedThisSession / TOTAL_COUNT_TO_EMIT;
//        System.out.println("Probability to throw exception: " + p);
//        if (new Random().nextDouble() < p) {
//          throw new RuntimeException("Intentional exception for failure recovery testing. "
//              + "Emitted: " + emitted);
//        }
//      }

      emittedThisSession++;
      int index = (int) (emitted++ % this.words.length);
      final String sentence = emitted + ":" + taskId;
      System.out.println(sentence);
      collector.emit(new Values(sentence));

      // Sleep a while for rate control
      Utils.sleep(2000 * EMIT_INTERVAL_MS);
    }

    @Override
    public void initState(State state) {
      System.out.println("Initializing state...");
      spoutState = state;

      // Restore the value of emitted
      emitted = spoutState.containsKey(KEY_EMITTED)
          ? (long) spoutState.get(KEY_EMITTED) : 0;
      System.out.println("Recover from last state.. Have emitted tuples: " + emitted);
    }

    @Override
    public void preSave(String checkpointId) {
      System.out.println("Saving state...");
      System.out.println(checkpointId + " Current sentence emitted count: " + emitted);
      spoutState.put(KEY_EMITTED, emitted);
    }
  }

  /**
   * A bolt that counts the words that it receives
   */
  public static class ConsumerBolt extends BaseRichBolt implements IStatefulBolt {
    private static final long serialVersionUID = -2345073092065993460L;
    private OutputCollector collector;
    private Map<String, Integer> countMap;

    // State of word count
    private State countState;

    @SuppressWarnings("rawtypes")
    public void prepare(Map map, TopologyContext topologyContext, OutputCollector outputCollector) {
      System.out.println("Preparing...");
      collector = outputCollector;
      countMap = new HashMap<String, Integer>();

      // Initialize the value of word count
      for (Map.Entry<Serializable, Serializable> entry : countState.entrySet()) {
        if (entry.getKey() instanceof String && entry.getValue() instanceof Integer) {
          countMap.put((String) entry.getKey(), (Integer) entry.getValue());
        }
      }
      System.out.println("Word count recovered size: " + countMap.size());
    }

    @Override
    public void execute(Tuple tuple) {
      String key = tuple.getString(0);
      System.out.println(key);
      if (countMap.get(key) == null) {
        countMap.put(key, 1);
      } else {
        Integer val = countMap.get(key);
        countMap.put(key, ++val);
      }
    }

    @Override
    public void declareOutputFields(OutputFieldsDeclarer outputFieldsDeclarer) {
    }

    @Override
    public void initState(State state) {
      System.out.println("Initializing state...");
      countState = state;
    }

    @Override
    public void preSave(String checkpointId) {
      System.out.println("Saving state...");
      System.out.println(checkpointId + " Current word count size: " + countMap.size());
      for (Map.Entry<String, Integer> entry : countMap.entrySet()) {
        countState.put(entry.getKey(), entry.getValue());
      }
    }
  }

  /**
   * Main method
   */
  public static void main(String[] args) throws AlreadyAliveException, InvalidTopologyException {
    if (args.length < 1) {
      throw new RuntimeException("Specify topology name");
    }

    int parallelism = 1;
    if (args.length > 1) {
      parallelism = Integer.parseInt(args[1]);
    }
    TopologyBuilder builder = new TopologyBuilder();
    builder.setSpout("word", new TestWordSpout(), parallelism);
    builder.setBolt("consumer", new ConsumerBolt(), parallelism)
        .fieldsGrouping("word", new Fields("word"));
    Config conf = new Config();
    conf.setNumStmgrs(parallelism);

    /*
    Set config here
    */
    conf.setComponentRam("word", ByteAmount.fromGigabytes(1));
    conf.setComponentRam("consumer", ByteAmount.fromGigabytes(1));

    // For stateful processing
    conf.put(Config.TOPOLOGY_STATEFUL, true);
    conf.put(Config.TOPOLOGY_STATEFUL_CHECKPOINT_INTERVAL, 5);

    HeronSubmitter.submitTopology(args[0], conf, builder.createTopology());
  }
}
