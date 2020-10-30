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

#include <google/protobuf/util/json_util.h>

#include "com/centreon/broker/version.hh"

using namespace com::centreon::broker;
using namespace com::centreon::broker::stats;
using namespace google::protobuf::util;

center& center::instance() {
  static center inst;
  return inst;
}

center::center() : _strand(pool::instance().io_context()) {
  *_stats.mutable_version() = version::string;
  _stats.set_pid(getpid());
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
FeederStats* center::register_feeder(const std::string& name) {
  std::promise<FeederStats*> p;
  std::future<FeederStats*> retval = p.get_future();
  _strand.post([this, &p, &name] {
    auto ep = _stats.add_feeder();
    ep->set_name(name);
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
    p.set_value(ep);
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
