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

#ifndef CCB_STATS_CENTER_HH
#define CCB_STATS_CENTER_HH

#include <asio.hpp>
#include <functional>

#include "com/centreon/broker/pool.hh"
#include "broker.pb.h"

CCB_BEGIN()

namespace stats {
class center {
  BrokerStats _stats;

 public:
  center() = default;
  std::string to_string() {
    return "";
  }

  EndpointStats* register_endpoint(const std::string& name);
  template <typename T>
  void update(T* ptr, T value) {
    auto& ctx = pool::instance().io_context();
    asio::post(ctx, [ptr, &value] { *ptr = value; });
  }
};

}

CCB_END()

#endif /* !CCB_STATS_CENTER_HH */
