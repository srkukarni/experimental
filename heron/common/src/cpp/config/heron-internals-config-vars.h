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

////////////////////////////////////////////////////////////////
//
// topology_config_vars.h
//
// Essentially this file defines as the config variables
// heron internals system config, which should not be touched
// by users
///////////////////////////////////////////////////////////////

#ifndef HERON_INTERNALS_COFNIG_VARS_H_
#define HERON_INTERNALS_COFNIG_VARS_H_
#include "basics/basics.h"

namespace heron {
namespace config {

class HeronInternalsConfigVars {
 public:
  /**
  * HERON_ configs are general configurations over all componenets
  **/

  // The relative path to the logging directory
  static const sp_string HERON_LOGGING_DIRECTORY;

  // The maximum log file size in MB
  static const sp_string HERON_LOGGING_MAXIMUM_SIZE_MB;

  // The maximum number of log files
  static const sp_string HERON_LOGGING_MAXIMUM_FILES;

  // The interval in seconds after which to check if the tmaster location
  // has been fetched or not
  static const sp_string HERON_CHECK_TMASTER_LOCATION_INTERVAL_SEC;

  // The interval in seconds to prune logging files in C+++
  static const sp_string HERON_LOGGING_PRUNE_INTERVAL_SEC;

  // The interval in seconds to flush log files in C+++
  static const sp_string HERON_LOGGING_FLUSH_INTERVAL_SEC;

  // The threadhold level to log error
  static const sp_string HERON_LOGGING_ERR_THRESHOLD;

  // The interval in seconds for different components to export metrics to metrics manager
  static const sp_string HERON_METRICS_EXPORT_INTERVAL_SEC;

  /**
  * HERON_METRICSMGR_* configs are for the metrics manager
  **/

  // The host of scribe to be exported metrics to
  static const sp_string HERON_METRICSMGR_SCRIBE_HOST;

  // The port of scribe to be exported metrics to
  static const sp_string HERON_METRICSMGR_SCRIBE_PORT;

  // The category of the scribe to be exported metrics to
  static const sp_string HERON_METRICSMGR_SCRIBE_CATEGORY;

  // The service name of the metrics in cuckoo_json
  static const sp_string HERON_METRICSMGR_SCRIBE_SERVICE_NAMESPACE;

  // The maximum retry attempts to write metrics to scribe
  static const sp_string HERON_METRICSMGR_SCRIBE_WRITE_RETRY_TIMES;

  // The timeout in seconds for metrics manager to write metrics to scribe
  static const sp_string HERON_METRICSMGR_SCRIBE_WRITE_TIMEOUT_SEC;

  // The interval in seconds to flush cached metircs to scribe
  static const sp_string HERON_METRICSMGR_SCRIBE_PERIODIC_FLUSH_INTERVAL_SEC;

  // The interval in seconds to reconnect to tmaster if a connection failure happens
  static const sp_string HERON_METRICSMGR_RECONNECT_TMASTER_INTERVAL_SEC;

  // The maximum packet size in MB of metrics manager's network options
  static const sp_string HERON_METRICSMGR_NETWORK_OPTIONS_MAXIMUM_PACKET_MB;

  /**
  * HERON_TMASTER_* configs are for the metrics manager
  **/

  // The maximum interval in minutes of metrics to be kept in tmaster
  static const sp_string HERON_TMASTER_METRICS_COLLECTOR_MAXIMUM_INTERVAL_MIN;

  // The maximum time to retry to establish the tmaster
  static const sp_string HERON_TMASTER_ESTABLISH_RETRY_TIMES;

  // The interval to retry to establish the tmaster
  static const sp_string HERON_TMASTER_ESTABLISH_RETRY_INTERVAL_SEC;

  // The maximum packet size in MB of tmaster's network options for stmgrs to connect to
  static const sp_string HERON_TMASTER_NETWORK_MASTER_OPTIONS_MAXIMUM_PACKET_MB;

  // The maximum packet size in MB of tmaster's network options for scheduler to connect to
  static const sp_string HERON_TMASTER_NETWORK_CONTROLLER_OPTIONS_MAXIMUM_PACKET_MB;

  // The maximum packet size in MB of tmaster's network options for stat queries
  static const sp_string HERON_TMASTER_NETWORK_STATS_OPTIONS_MAXIMUM_PACKET_MB;

  // The inteval for tmaster to purge metrics from socket
  static const sp_string HERON_TMASTER_METRICS_COLLECTOR_PURGE_INTERVAL_SEC;

  // The maximum # of exception to be stored in tmetrics collector, to prevent potential OOM
  static const sp_string HERON_TMASTER_METRICS_COLLECTOR_MAXIMUM_EXCEPTION;

  // Whether tmaster's metrics server should bind on all interfaces
  static const sp_string HERON_TMASTER_METRICS_NETWORK_BINDALLINTERFACES;

  // The timeout in seconds for stream mgr, compared with (current time - last heartbeat time)
  static const sp_string HERON_TMASTER_STMGR_STATE_TIMEOUT_SEC;

  /**
  * HERON_STREAMMGR_* configs are for the stream manager
  **/

  // Maximum size in bytes of a packet to be send out from stream manager
  static const sp_string HERON_STREAMMGR_PACKET_MAXIMUM_SIZE_BYTES;

  // The tuple cache (used for batching) can be drained in two ways: (a) Time based (b) size based
  // The frequency in ms to drain the tuple cache in stream manager
  static const sp_string HERON_STREAMMGR_CACHE_DRAIN_FREQUENCY_MS;

  // The sized based threshold in MB for draining the tuple cache
  static const sp_string HERON_STREAMMGR_CACHE_DRAIN_SIZE_MB;

  // The sized based threshold in MB for draining the checkpoint buffering for stateful topologies
  static const sp_string HERON_STREAMMGR_CHECKPOINT_DRAIN_SIZE_MB;

  // For efficient acknowledgement
  static const sp_string HERON_STREAMMGR_XORMGR_ROTATINGMAP_NBUCKETS;

  // The reconnect interval to other stream managers in second for stream manager client
  static const sp_string HERON_STREAMMGR_CLIENT_RECONNECT_INTERVAL_SEC;

  // The reconnect interval to tamster in second for stream manager client
  static const sp_string HERON_STREAMMGR_CLIENT_RECONNECT_TMASTER_INTERVAL_SEC;

  // The maximum packet size in MB of stream manager's network options
  static const sp_string HERON_STREAMMGR_NETWORK_OPTIONS_MAXIMUM_PACKET_MB;

  // The interval in seconds to send heartbeat
  static const sp_string HERON_STREAMMGR_TMASTER_HEARTBEAT_INTERVAL_SEC;

  // Maximum batch size in MB to read by stream manager from socket
  static const sp_string HERON_STREAMMGR_CONNECTION_READ_BATCH_SIZE_MB;

  // Maximum batch size in MB to write by stream manager to socket
  static const sp_string HERON_STREAMMGR_CONNECTION_WRITE_BATCH_SIZE_MB;

  // Number of times we should wait to see a buffer full while enqueueing data before declaring
  // start of back pressure
  static const sp_string HERON_STREAMMGR_NETWORK_BACKPRESSURE_THRESHOLD;

  // High water mark on the num in MB that can be left outstanding on a connection
  static const sp_string HERON_STREAMMGR_NETWORK_BACKPRESSURE_HIGHWATERMARK_MB;

  // Low water mark on the num in MB that can be left outstanding on a connection
  static const sp_string HERON_STREAMMGR_NETWORK_BACKPRESSURE_LOWWATERMARK_MB;
};
}  // namespace config
}  // namespace heron

#endif
