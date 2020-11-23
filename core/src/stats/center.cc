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

#include "com/centreon/broker/stats/center.hh"

#include <fmt/format.h>
#include <google/protobuf/util/json_util.h>

#include "com/centreon/broker/config/applier/modules.hh"
#include "com/centreon/broker/config/applier/state.hh"
#include "com/centreon/broker/misc/filesystem.hh"
#include "com/centreon/broker/modules/loader.hh"
#include "com/centreon/broker/version.hh"

using namespace com::centreon::broker;
using namespace com::centreon::broker::stats;
using namespace google::protobuf::util;
using namespace com::centreon::broker::modules;

center& center::instance() {
  static center inst;
  return inst;
}

center::center() : _strand(pool::instance().io_context()) {
  *_stats.mutable_version() = version::string;
  *_stats.mutable_asio_version() =
      fmt::format("{}.{}.{}", ASIO_VERSION / 100000, ASIO_VERSION / 100 % 1000,
                  ASIO_VERSION % 100);
  _stats.set_pid(getpid());

  /* Bringing modules statistics */
  if (config::applier::modules::loaded()) {
    config::applier::modules& mod_applier(config::applier::modules::instance());
    for (config::applier::modules::iterator it(mod_applier.begin()),
         end(mod_applier.end());
         it != end; ++it) {
      auto m = _stats.add_modules();
      *m->mutable_name() = it->first;
      *m->mutable_size() =
          fmt::format("{}B", misc::filesystem::file_size(it->first));
      *m->mutable_state() = "loaded";
    }
  }

  /*Start the thread pool */
  pool::instance().start_stats(_stats.mutable_pool_stats());
}

/**
 * @brief When a feeder needs to write statistics, it primarily has to
 * call this function to be registered in the statistic center and to get
 * a pointer for its statistics. It is prohibited to directly write into this
 * pointer. We must use the center member functions for this purpose.
 *
 * @param name
 *
 * @return A pointer to the feeder statistics.
 */
FeederStats* center::register_feeder(EndpointStats* ep_stats,
                                     const std::string& name) {
  std::promise<FeederStats*> p;
  std::future<FeederStats*> retval = p.get_future();
  _strand.post([this, ep_stats, &p, &name] {
    auto ep = &(*ep_stats->mutable_feeder())[name];
    p.set_value(ep);
  });
  return retval.get();
}

/**
 * @brief When an endpoint needs to write statistics, it primarily has to
 * call this function to be registered in the statistic center and to get
 * a pointer to its statistics. It is prohibited to directly write into this
 * pointer. We must use the center member function for this purpose.
 *
 * @param name
 *
 * @return A pointer to the endpoint statistics.
 */
EndpointStats* center::register_endpoint(const std::string& name) {
  std::promise<EndpointStats*> p;
  std::future<EndpointStats*> retval = p.get_future();
  _strand.post([this, &p, &name] {
    auto ep = _stats.add_endpoint();
    ep->set_name(name);
    *ep->mutable_memory_file_path() = fmt::format(
        "{}.memory.{}", config::applier::state::instance().cache_dir(), name);
    *ep->mutable_queue_file_path() = fmt::format(
        "{}.queue.{}", config::applier::state::instance().cache_dir(), name);

    p.set_value(ep);
  });
  return retval.get();
}

/**
 * @brief To allow the conflict manager to send statistics, it has to call this
 * function to get a pointer to its statistics container.
 * It is prohibited to directly write into the returned pointer. We must use
 * the center member functions for this purpose.
 *
 * @return A pointer to the conflict_manager statistics.
 */
ConflictManagerStats* center::register_conflict_manager() {
  std::promise<ConflictManagerStats*> p;
  std::future<ConflictManagerStats*> retval = p.get_future();
  _strand.post([this, &p] {
    auto cm = _stats.mutable_conflict_manager();
    p.set_value(cm);
  });
  return retval.get();
}

/**
 * @brief To allow the conflict manager to send statistics, it has to call this
 * function to get a pointer to its statistics container.
 * It is prohibited to directly write into the returned pointer. We must use
 * the center member functions for this purpose.
 *
 * @return A pointer to the module statistics.
 */
ModuleStats* center::register_modules() {
  std::promise<ModuleStats*> p;
  std::future<ModuleStats*> retval = p.get_future();
  _strand.post([this, &p] {
    auto m = _stats.add_modules();
    p.set_value(m);
  });
  return retval.get();
}

/**
 * @brief Convert the protobuf statistics object to a json string.
 *
 * @return a string with the object in json format.
 */
std::string center::to_string() {
  std::promise<std::string> p;
  std::future<std::string> retval = p.get_future();
  _strand.post([& s = this->_stats, &p] {
    const JsonPrintOptions options;
    std::string retval;
    std::time_t now;
    time(&now);
    s.mutable_now()->set_seconds(now);
    MessageToJsonString(s, &retval, options);
    p.set_value(std::move(retval));
  });
  return retval.get();
}
