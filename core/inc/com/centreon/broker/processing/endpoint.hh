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

#include <atomic>
#include <json11.hpp>
#include <mutex>
#include <string>
#include <unordered_set>
#include <climits>
#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

#include "com/centreon/broker/namespace.hh"
#include "com/centreon/broker/stats/center.hh"
#include "com/centreon/broker/misc/shared_mutex.hh"
#include "com/centreon/broker/multiplexing/subscriber.hh"
#include "com/centreon/broker/misc/processing_speed_computer.hh"



CCB_BEGIN()

namespace processing {
class endpoint {
   std::atomic<char const*> _state;
   timestamp _last_connection_attempt;
   timestamp _last_connection_success;

 protected:
  misc::processing_speed_computer _event_processing_speed;
  EndpointStats* _stats;
  const std::string _name;
  mutable std::mutex _stat_mutex;

  virtual uint32_t _get_queued_events() const = 0;
  virtual std::string const& _get_write_filters() const = 0;
  virtual void _forward_statistic(json11::Json::object& tree);

 public:
  endpoint(const std::string& name);
  virtual ~endpoint() noexcept;
  virtual void update() {}
  virtual void start() = 0;
  virtual void exit() = 0;
  void set_read_filters(const std::string& rf);
  void set_write_filters(const std::string& wf);
  void set_event_processing_speed(double value);
  void set_one_peer_retention_mode(bool value);
  void set_pending_events(uint32_t value);
  void set_queue_file_enabled(bool value);
  void set_unacknowledged_events(uint32_t value);
  void set_sql_pending_events(uint32_t value);
  void set_bbdo_input_ack_limit(uint32_t value);
  void set_bbdo_unacknowledged_events(uint32_t value);
  void set_peers(uint32_t value);
  void set_state(const std::string& state);
  void set_status(const std::string& status);
  void set_last_connection_attempt(timestamp last_connection_attempt);
  void set_last_connection_success(timestamp last_connection_success);
  void set_last_event_at(timestamp last_event_at);
  void set_queued_events(uint32_t value);
  void tick(uint32_t events = 1);
  virtual void stats(json11::Json::object& tree);

  std::string const& get_name() const;
};
}  // namespace processing

CCB_END()

#endif /* CCB_PROCESSING_ENDPOINT_HH */
