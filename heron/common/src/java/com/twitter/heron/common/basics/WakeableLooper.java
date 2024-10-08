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

package com.twitter.heron.common.basics;

import java.util.ArrayList;
import java.util.List;
import java.util.PriorityQueue;

/**
 * A WakeableLooper is a class that could:
 * Block the thread when doWait() is called and unblock
 * when the wakeUp() is called or the waiting time exceeds the timeout.
 * It could execute timer event
 * <p>
 * The WakeableLooper will execute in a while loop unless the exitLoop() is called. And in every
 * execution, it will execute runOnce(), which will:
 * 1. doWait(), which in fact is implemented by selector.select(timeout), and it will be wake up if other threads
 * wake it up, it meets the timeout, one channel is selected, or the current thread is interrupted.
 * 2. run executeTasksOnWakeup(), which is a list of Runnable, e.g. tasks added to run every time. We could add tasks
 * even during executionTasksOnWakeup, but the task added will be executed next time we run executeTasksOnwakeup().
 * Notice: you could just add tasks into it but not remove tasks from it.
 * 3. trigger the timers, which is a priority queue of {@code TimerTask}, the {@code TimerTask}
 * will be removed after execution.
 * <p>
 * So to use this class, user could add the persistent tasks, one time tasks and timer tasks as many
 * as they want.
 */
public abstract class WakeableLooper {
  // The tasks could only be added but not removed
  private final List<Runnable> tasksOnWakeup;
  private final PriorityQueue<TimerTask> timers;

  // The tasks would be invoked before exit
  private final ArrayList<Runnable> exitTasks;

  // For selector since there is bug in selector.select(timeout): we could not
  // use a timeout > 10 * Integer.MAX_VALUE
  // So here we set Integer.MAX_VALUE as the infinite future
  // We will also multiple 1000*1000 to convert mill-seconds to nano-seconds
  private static final long INFINITE_FUTURE = Integer.MAX_VALUE;
  private volatile boolean exitLoop;

  public WakeableLooper() {
    exitLoop = false;
    tasksOnWakeup = new ArrayList<Runnable>();
    timers = new PriorityQueue<TimerTask>();
    exitTasks = new ArrayList<>();
  }

  public void clear() {
    tasksOnWakeup.clear();
    timers.clear();
    exitTasks.clear();
  }

  public void clearTasksOnWakeup() {
    tasksOnWakeup.clear();
  }

  public void clearTimers() {
    timers.clear();
  }

  public void loop() {
    while (!exitLoop) {
      runOnce();
    }

    // Invoke the exit tasks
    onExit();
  }

  private void runOnce() {
    doWait();

    executeTasksOnWakeup();

    triggerExpiredTimers(System.nanoTime());
  }

  private void onExit() {
    for (Runnable r : exitTasks) {
      r.run();
    }
  }

  protected abstract void doWait();

  public abstract void wakeUp();

  public void addTasksOnWakeup(Runnable task) {
    tasksOnWakeup.add(task);
    // We need to wake up the looper itself when we add a new task, otherwise, it is possible
    // this task will never be executed due to the looper will never be wake up.
    wakeUp();
  }

  public void addTasksOnExit(Runnable task) {
    exitTasks.add(task);
  }

  public void registerTimerEventInSeconds(long timerInSeconds, Runnable task) {
    registerTimerEventInNanoSeconds(timerInSeconds * Constants.SECONDS_TO_NANOSECONDS, task);
  }

  public void registerTimerEventInNanoSeconds(long timerInNanoSecnods, Runnable task) {
    assert timerInNanoSecnods >= 0;
    assert task != null;
    long expirationNs = System.nanoTime() + timerInNanoSecnods;
    timers.add(new TimerTask(expirationNs, task));
  }

  public void exitLoop() {
    exitLoop = true;
    wakeUp();
  }

  /**
   * Get the timeout in milli-seconds which should be used in doWait().
   * We use milli-second here since the select.select(timeout) accepts only timeout in milli-seconds
   *
   * @return INFINITE_FUTURE : if there are no timer events
   * or the time to next timer event in milli-second
   */
  protected long getNextTimeoutIntervalMs() {
    long nextTimeoutIntervalMs = INFINITE_FUTURE;
    if (!timers.isEmpty()) {
      // The time recorded in timer is in nano-seconds. We have to convert it to milli-seconds
      // We need to ceil the result to avoid early wake up
      nextTimeoutIntervalMs =
          (timers.peek().getExpirationNs() - System.nanoTime()
              + Constants.MILLISECONDS_TO_NANOSECONDS) / Constants.MILLISECONDS_TO_NANOSECONDS;
    }
    return nextTimeoutIntervalMs;
  }

  private void executeTasksOnWakeup() {
    // Be careful here we could not use iterator, since it is possible that we may
    // add some items into this list during the iteration, which may cause
    // ConcurrentModificationException
    // We pre-get the size to avoid execute the tasks added during execution
    // We also need to consider case tasks was cleared during execution
    int s = tasksOnWakeup.size();
    for (int i = 0; i < s && i < tasksOnWakeup.size(); i++) {
      tasksOnWakeup.get(i).run();
    }
  }

  private void triggerExpiredTimers(long currentTime) {
    // Executes the task should be executed no later than current time
    while (!timers.isEmpty()) {
      long nextExpiredTime = timers.peek().getExpirationNs();
      if (nextExpiredTime - currentTime <= 0) {
        timers.poll().handler.run();
      } else {
        return;
      }
    }
  }

  /**
   * A TimerTask will has the runnable, and expirationNs to indicate when it will be executed.
   * The expirationNs is the time in nano-seconds
   */
  private static class TimerTask implements Comparable<TimerTask> {
    public final long expirationNs;
    public final Runnable handler;

    TimerTask(long expirationNs, Runnable handler) {
      this.expirationNs = expirationNs;
      this.handler = handler;
    }

    @Override
    public int compareTo(TimerTask other) {
      // We could not use t0 < t1, which may has over-flow issue
      if (this.expirationNs - other.expirationNs < 0) {
        return -1;
      }
      if (this.expirationNs - other.expirationNs > 0) {
        return 1;
      }
      return 0;
    }

    @Override
    public boolean equals(Object other) {
      throw new RuntimeException("TODO: implement");
    }

    @Override
    public int hashCode() {
      throw new RuntimeException("TODO: implement");
    }

    public long getExpirationNs() {
      return expirationNs;
    }
  }
}
