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
#include "com/centreon/broker/pool.hh"

#include "com/centreon/broker/log_v2.hh"
#include "com/centreon/broker/stats/center.hh"

using namespace com::centreon::broker;

size_t pool::_pool_size(0);
std::mutex pool::_init_m;
std::atomic_bool pool::_initialized(false);
pool pool::_instance{};

/**
 * @brief The way to access to the pool.
 *
 * @return a reference to the pool.
 */
pool& pool::instance() {
  assert(pool::_initialized);
  return _instance;
}

void pool::start(size_t size) {
  std::lock_guard<std::mutex> lck(_init_m);
  if (!_initialized) {
    _instance._start(size);
    _initialized = true;
  } else
    log_v2::core()->error("pool already started.");
}

/**
 * @brief A static method to access the IO context.
 *
 * @return the IO context.
 */
asio::io_context& pool::io_context() {
  return instance()._io_context;
}

/**
 * @brief Default constructor. Private, it is called throw the static instance()
 * method. While this object gathers statistics for the statistics engine,
 * is is not initialized as others. This is because, the stats engine is
 * heavily dependent on the pool. So the stats engine needs the pool and the
 * pool needs the stats engine.
 *
 * The idea here, is that when the pool is started, no stats are done. And when
 * the stats::center is well started, it asks the pool to start its stats.
 */
pool::pool()
    : _stats(nullptr),
      _io_context(_pool_size),
      _worker(_io_context),
      _closed(true),
      _timer(_io_context) {}

/**
 * @brief Start the stats of the pool. This method is called by the stats engine
 * when it is ready.
 *
 * @param stats The pointer used by the pool to set its data in the stats
 * engine.
 */
void pool::start_stats(ThreadPool* stats) {
  _stats = stats;
  /* The only time, we set a data directly to stats, this is because, this
   * method is called by the stats engine and the _check_latency has not started
   */
  _stats->set_size(get_current_size());
  _check_latency();
}

/**
 * @brief Start the thread pool used for the tcp connections.
 *
 */
void pool::_start(size_t size) {
  std::lock_guard<std::mutex> lock(_closed_m);
  _pool_size = size;
  if (_closed) {
    _closed = false;
    /* We fix the thread pool used by asio to hardware concurrency and at
     * least, we want 2 threads. So in case of two sockets, one in and one out,
     * they should be managed by those two threads. This is empirical, and maybe
     * will be changed later. */
    size_t count = _pool_size == 0
                       ? std::max(std::thread::hardware_concurrency(), 2u)
                       : _pool_size;

    log_v2::core()->info("Starting the TCP thread pool of {} threads", count);
    for (uint32_t i = 0; i < count; i++)
      _pool.emplace_back([this] { _io_context.run(); });
  }
}

/**
 * @brief Destructor
 */
pool::~pool() noexcept {
  _stop();
}

/**
 * @brief Stop the thread pool.
 */
void pool::_stop() {
  log_v2::core()->trace("Stopping the TCP thread pool");
  std::lock_guard<std::mutex> lock(_closed_m);
  if (!_closed) {
    _closed = true;
    _io_context.stop();
    for (auto& t : _pool)
      t.join();
  }
  log_v2::core()->trace("No remaining thread in the pool");
}

/**
 * @brief Returns the number of threads used in the pool.
 *
 * @return a size.
 */
uint32_t pool::get_current_size() const {
  std::lock_guard<std::mutex> lock(_closed_m);
  return _pool.size();
}

/**
 * @brief The function whose role is to compute the latency. It makes the
 * computation every 10s.
 *
 */
void pool::_check_latency() {
  auto start = std::chrono::system_clock::now();
  asio::post(_io_context, [start, this] {
    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start);
    stats::center::instance().update(_stats->mutable_latency(),
                                     fmt::format("{:.3f}ms", duration.count()));
    log_v2::core()->trace("Thread pool latency {:.3f}ms", duration.count());
  });
  _timer.expires_after(std::chrono::seconds(10));
  _timer.async_wait(std::bind(&pool::_check_latency, this));
}
