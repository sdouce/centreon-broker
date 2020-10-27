/*
** Copyright 2011-2012,2015,2017 Centreon
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** For more information : contact@centreon.com
*/

#include "com/centreon/broker/processing/endpoint.hh"

#include <unistd.h>

#include "com/centreon/broker/exceptions/msg.hh"
#include "com/centreon/broker/exceptions/shutdown.hh"
#include "com/centreon/broker/io/raw.hh"
#include "com/centreon/broker/io/stream.hh"
#include "com/centreon/broker/log_v2.hh"
#include "com/centreon/broker/logging/logging.hh"
#include "com/centreon/broker/misc/misc.hh"
#include "com/centreon/broker/multiplexing/muxer.hh"

using namespace com::centreon::broker;
using namespace com::centreon::broker::processing;

/**************************************
 *                                     *
 *           Public Methods            *
 *                                     *
 **************************************/
 
void endpoint::set_name(const std::string& name) {
    stats::center::instance().update(_stats->mutable_name(), name);
}

void endpoint::set_read_filers(const std::string& rf) {
    stats::center::instance().update(_stats->mutable_read_filters(), rf);
}

void endpoint::set_write_filters(const std::string& wf){
    stats::center::instance().update(_stats->mutable_write_filters(), wf);
}

void endpoint::set_memory_file_path(const std::string& file){
    stats::center::instance().update(_stats->mutable_memory_file_path(), file);
}

void endpoint::set_event_processing_speed(double value) {
  stats::center::instance().update(&EndpointStats::set_event_processing_speed,
                                  _stats, value);
}

void endpoint::set_one_peer_retention_mode(bool value) {
  stats::center::instance().update(&EndpointStats::set_one_peer_retention_mode,
                                  _stats, value);
}

void endpoint::set_pending_events(uint32_t value) {
  stats::center::instance().update(&EndpointStats::set_pending_events,
                                  _stats, value);
}

void endpoint::set_queue_file_enabled(bool value) {
  stats::center::instance().update(&EndpointStats::set_queue_file_enabled,
                                  _stats, value);
}

void endpoint::set_unacknowledged_events(uint32_t value) {
  stats::center::instance().update(&EndpointStats::set_unacknowledged_events,
                                  _stats, value);
}

void endpoint::set_sql_pending_events(uint32_t value) {
  stats::center::instance().update(&EndpointStats::set_sql_pending_events,
                                  _stats, value);
}

void endpoint::set_queue_file_path(const std::string& file){
    stats::center::instance().update(_stats->mutable_queue_file_path(), file);
}

void endpoint::set_bbdo_input_ack_limit(uint32_t value) {
  stats::center::instance().update(&EndpointStats::set_bbdo_input_ack_limit,
                                  _stats, value);
}

void endpoint::set_bbdo_unacknowledged_events(uint32_t value) {
  stats::center::instance().update(&EndpointStats::set_bbdo_unacknowledged_events,
                                  _stats, value);
}

void endpoint::set_peers(uint32_t value) {
  stats::center::instance().update(&EndpointStats::set_peers,
                                  _stats, value);
}
