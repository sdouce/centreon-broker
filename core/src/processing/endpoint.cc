/*
** Copyright 2020 Centreon
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


using namespace com::centreon::broker;
using namespace com::centreon::broker::processing;

endpoint::endpoint(const std::string& name)
      : stat_visitable(name),
        _stats(stats::center::instance().register_endpoint(name)) {}


void endpoint::set_read_filers(const std::string& rf) {
  stats::center::instance().update(_stats->mutable_read_filters(), rf);
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
  stats::center::instance().update(&EndpointStats::set_pending_events, _stats,
                                   value);
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

void endpoint::set_bbdo_input_ack_limit(uint32_t value) {
  stats::center::instance().update(&EndpointStats::set_bbdo_input_ack_limit,
                                   _stats, value);
}

void endpoint::set_bbdo_unacknowledged_events(uint32_t value) {
  stats::center::instance().update(
      &EndpointStats::set_bbdo_unacknowledged_events, _stats, value);
}

void endpoint::set_peers(uint32_t value) {
  stats::center::instance().update(&EndpointStats::set_peers, _stats, value);
}

void endpoint::set_state(const std::string& state) {
  stats::center::instance().update(_stats->mutable_state(), state);
}

void endpoint::set_status(const std::string& status) {
  stats::center::instance().update(_stats->mutable_status(), status);
}

void endpoint::set_last_connection_attempt(timestamp last_connection_attempt) {
  stats::center::instance().update(_stats->mutable_last_connection_attempt(),
                                   last_connection_attempt.get_time_t());
}

void endpoint::set_last_connection_success(timestamp last_connection_success) {
  stats::center::instance().update(_stats->mutable_last_connection_success(),
                                   last_connection_success.get_time_t());
}

void endpoint::set_last_event_at(timestamp last_event_at) {
  stats::center::instance().update(_stats->mutable_last_event_at(),
                                   last_event_at.get_time_t());
}

void endpoint::set_queued_events(uint32_t value) {
  stats::center::instance().update(&EndpointStats::set_queued_events, _stats,
                                   value);
}
