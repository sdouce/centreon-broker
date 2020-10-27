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

#ifndef CCB_PROCESSING_ENDPOINT_HH
#define CCB_PROCESSING_ENDPOINT_HH

#include "com/centreon/broker/namespace.hh"
#include "com/centreon/broker/processing/stat_visitable.hh"
#include "com/centreon/broker/stats/center.hh"

CCB_BEGIN()

namespace processing {
class endpoint : public stat_visitable {
 protected:
  EndpointStats* _stats;

 public:
  endpoint(const std::string& name)
      : stat_visitable(name),
        _stats(stats::center::instance().register_endpoint(name)) {}
  virtual ~endpoint() noexcept {}
  virtual void update() {}
  virtual void start() = 0;
  virtual void exit() = 0;
  void set_name(const std::string& name); 
  void set_read_filers(const std::string& rf);
  void set_write_filters(const std::string& wf);
  void set_memory_file_path(const std::string& file);
  void set_event_processing_speed(double value);
  void set_one_peer_retention_mode(bool value);
  void set_pending_events(uint32_t value);
  void set_queue_file_enabled(bool value);
  void set_unacknowledged_events(uint32_t value);
  void set_sql_pending_events(uint32_t value);
  void set_queue_file_path(const std::string& file);
  void set_bbdo_input_ack_limit(uint32_t value);
  void set_bbdo_unacknowledged_events(uint32_t value);
  void set_peers(uint32_t value);

  void set_state(const std::string& state) {
    stats::center::instance().update(_stats->mutable_state(), state);
  }
  void set_status(const std::string& status) {
    stats::center::instance().update(_stats->mutable_status(), status);
  }
  void set_last_connection_attempt(timestamp last_connection_attempt) {
    stats::center::instance().update(_stats->mutable_last_connection_attempt(),
                                     last_connection_attempt.get_time_t());
  }
  void set_last_connection_success(timestamp last_connection_success) {
    stats::center::instance().update(_stats->mutable_last_connection_success(),
                                     last_connection_success.get_time_t());
  }
  void set_last_event_at(timestamp last_event_at) {
    stats::center::instance().update(_stats->mutable_last_event_at(),
                                     last_event_at.get_time_t());
  }
  void set_queued_events(uint32_t value) {
    stats::center::instance().update(&EndpointStats::set_queued_events, _stats,
                                     value);
  }
};
}  // namespace processing

CCB_END()

#endif /* CCB_PROCESSING_ENDPOINT_HH */
