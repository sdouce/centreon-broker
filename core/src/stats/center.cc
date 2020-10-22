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

using namespace com::centreon::broker;
using namespace com::centreon::broker::stats;

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
