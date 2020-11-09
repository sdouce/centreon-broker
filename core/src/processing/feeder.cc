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

#include "com/centreon/broker/processing/feeder.hh"

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

/**
 *  Constructor.
 *
 *  @param[in] name           Name.
 *  @param[in] client         Client stream.
 *  @param[in] read_filters   Read filters.
 *  @param[in] write_filters  Write filters.
 */
feeder::feeder(std::string const& name,
               std::shared_ptr<io::stream> client,
               std::unordered_set<uint32_t> const& read_filters,
               std::unordered_set<uint32_t> const& write_filters)
    : _name(name),
      _state{feeder::stopped},
      _should_exit{false},
      _client(client),
      _subscriber(name, false, read_filters, write_filters),
      _stats(stats::center::instance().register_feeder(name)) {
  if (!client)
    throw exceptions::msg()
        << "could not process '" << _name << "' with no client stream";

  stats::center::instance().update(_stats->mutable_read_filters(),
                                    misc::dump_filters(read_filters));
  stats::center::instance().update(_stats->mutable_write_filters(),
                                    misc::dump_filters(write_filters));

  set_last_connection_attempt(timestamp::now());
  set_last_connection_success(timestamp::now());
  set_state("connecting");
    std::unique_lock<std::mutex> lck(_state_m);
  _thread = std::thread(&feeder::_callback, this);
  _state_cv.wait(lck,
                 [& state = this->_state] { return state != feeder::stopped; });
}

/**
 *  Destructor.
 */
feeder::~feeder() {
  std::unique_lock<std::mutex> lock(_state_m);
  switch (_state) {
    case stopped:
      _state = finished;
      break;
    case running:
      _should_exit = true;
      _state_cv.wait(lock, [this] { return _state == finished; });
      _thread.join();
      break;
    case finished:
      _thread.join();
      break;
  }
}

bool feeder::is_finished() const noexcept {
  std::lock_guard<std::mutex> lock(_state_m);
  return _state == finished && _should_exit;
}

/**
 *  Get the read filters used by the feeder.
 *
 *  @return  The read filters used by the feeder.
 */

/**
 *  Get the write filters used by the feeder.
 *
 *  @return  The write filters used by the feeder.
 */
//std::string const& feeder::_get_write_filters() const {
//  return _subscriber.get_muxer().get_write_filters_str();
//}

/**
 *  Forward to stream.
 *
 *  @param[in] tree  The statistic tree.
 */
void feeder::_forward_statistic(json11::Json::object& tree) {
  if (_client_m.try_lock_shared_for(300)) {
    if (_client)
      _client->statistics(tree);
    _client_m.unlock();
  }
  _subscriber.get_muxer().statistics(tree);
}

void feeder::_callback() noexcept {
  log_v2::processing()->info("feeder: thread of client '{}' is starting",
                             _name);
  time_t fill_stats_time = time(nullptr);
  std::unique_lock<std::mutex> lock(_state_m);

  try {
    set_state("connected");
    bool stream_can_read(true);
    bool muxer_can_read(true);
    std::shared_ptr<io::data> d;
    _state = feeder::running;
    _state_cv.notify_all();
    lock.unlock();
    while (!_should_exit) {
      // Read from stream.
      bool timed_out_stream(true);

      // Filling stats
      if (time(nullptr) >= fill_stats_time) {
        fill_stats_time += 5;
        set_queued_events(_subscriber.get_muxer().get_event_queue_size());
        set_event_processing_speed(_event_processing_speed.get_processing_speed());
        set_last_event_at(static_cast<double>(_event_processing_speed.get_last_event_time()));
        set_unacknowledged_events(_subscriber.get_muxer().get_unacknowledged_events() + 1);
        set_queue_file_enabled(!_subscriber.get_muxer().is_queue_file_enabled());
      }

      if (stream_can_read) {
        try {
          misc::read_lock lock(_client_m);
          timed_out_stream = !_client->read(d, 0);
        } catch (exceptions::shutdown const& e) {
          stream_can_read = false;
        }
        if (d) {
          log_v2::processing()->trace(
              "feeder '{}': sending 1 event from stream to muxer", _name);
          {
            misc::read_lock lock(_client_m);
            _subscriber.get_muxer().write(d);
          }
          tick();
          continue;  // Stream read bias.
        }
      }

      // Read from muxer.
      d.reset();
      bool timed_out_muxer(true);
      if (muxer_can_read)
        try {
          timed_out_muxer = !_subscriber.get_muxer().read(d, 0);
        } catch (exceptions::shutdown const& e) {
          muxer_can_read = false;
        }
      if (d) {
        log_v2::processing()->trace(
            "feeder '{}': sending 1 event from muxer to client", _name);
        {
          misc::read_lock lock(_client_m);
          _client->write(d);
        }
        _subscriber.get_muxer().ack_events(1);
        tick();
      }

      // If both timed out, sleep a while.
      d.reset();
      if (timed_out_stream && timed_out_muxer) {
        log_v2::processing()->trace(
            "feeder '{}': timeout on stream and muxer, waiting for 100000µs",
            _name);
        ::usleep(100000);
      }
    }
  } catch (exceptions::shutdown const& e) {
    // Normal termination.
    (void)e;
    log_v2::core()->info("feeder '{}' shut down", _name);
  } catch (std::exception const& e) {
    logging::error(logging::medium)
        << "feeder: error occured while processing client '" << _name
        << "': " << e.what();
    set_last_error(e.what());
  } catch (...) {
    logging::error(logging::high)
        << "feeder: unknown error occured while processing client '" << _name
        << "'";
  }

  /* If we are here, that is because the loop is finished, and if we want
   * is_finished() to return true, we have to set _should_exit to true. */
  _should_exit = true;
  std::unique_lock<std::mutex> lock_stop(_state_m);
  _state = feeder::finished;
  _state_cv.notify_all();
  lock_stop.unlock();

  {
    misc::read_lock lock(_client_m);
    _client.reset();
    set_state("disconnected");
    _subscriber.get_muxer().remove_queue_files();
  }
  log_v2::processing()->info("feeder: thread of client '{}' will exit", _name);
}

//uint32_t feeder::_get_queued_events() const {
//  return _subscriber.get_muxer().get_event_queue_size();
//}

/**
 * @brief Get the feeder state as a string. Interesting for logs.
 *
 * @return a const char* with the current state.
 */
const char* feeder::get_state() const {
  switch (_state) {
    case stopped:
      return "stopped";
    case running:
      return "running";
    case finished:
      return "finished";
  }
  return "unknown";
}

/**
 * @brief Fills the stats object with the given state. This function is just
 * here to simplify the developer work.
 *
 * @param state A string containing the current state.
 */
void feeder::set_state(const std::string& state) {
  stats::center::instance().update(_stats->mutable_state(), state);
}

void feeder::set_queued_events(uint32_t events) {
  stats::center::instance().update(&FeederStats::set_queued_events, _stats, events);
}

void feeder::set_last_connection_attempt(timestamp time) {
  stats::center::instance().update(_stats->mutable_last_connection_attempt(), time.get_time_t());
}

void feeder::set_last_connection_success(timestamp time) {
  stats::center::instance().update(_stats->mutable_last_connection_success(), time.get_time_t());
}

void feeder::set_last_error(const std::string& last_error) {
  stats::center::instance().update(_stats->mutable_last_error(), last_error);
}

void feeder::set_event_processing_speed(double value) {
  stats::center::instance().update(&FeederStats::set_event_processing_speed,
                                   _stats, value);
}

void feeder::set_last_event_at(timestamp last_event_at) {
  stats::center::instance().update(_stats->mutable_last_event_at(),
                                   last_event_at.get_time_t());
}

void feeder::set_queue_file_enabled(bool value) {
  stats::center::instance().update(&FeederStats::set_queue_file_enabled,
                                   _stats, value);
}

void feeder::set_unacknowledged_events(uint32_t value) {
  stats::center::instance().update(&FeederStats::set_unacknowledged_events,
                                   _stats, value);
}

/**
 *  Gather statistics on this thread.
 *
 *  @param[in] tree  Tree of information.
 */
void feeder::stats(json11::Json::object& tree) {
  std::lock_guard<std::mutex> lock(_stat_mutex);
  tree["state"] = ""; // FIXME DBR: std::string(_state); It is now is protobuf object
  tree["read_filters"] = "";  //FIXME DBR: _get_read_filters();
  tree["write_filters"]  = ""; //FIXME DBR: _get_write_filters();
  tree["event_processing_speed"] = _event_processing_speed.get_processing_speed();
  tree["last_connection_attempt"] = -1; //FIXME DBR: static_cast<double>(_last_connection_attempt);
  tree["last_connection_success"] = -1; //FIXME DBR: static_cast<double>(_last_connection_success);
  tree["last_event_at"] = static_cast<double>(_event_processing_speed.get_last_event_time());
  tree["queued_events"] = 0; //FIXME DBR: static_cast<int>(_get_queued_events());

  // Forward the stats.
  _forward_statistic(tree);
}

/**
 *  Tick the event processing computation.
 */
void feeder::tick(uint32_t events) {
  std::lock_guard<std::mutex> lock(_stat_mutex);
  _event_processing_speed.tick(events);
}

const std::string& feeder::get_name() const {
  return _name;
}
