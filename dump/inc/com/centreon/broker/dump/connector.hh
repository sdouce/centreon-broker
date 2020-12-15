/*
** Copyright 2015-2017 Centreon
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

#ifndef CCB_INFLUXDB_CONNECTOR_HH
#define CCB_INFLUXDB_CONNECTOR_HH

#include <ctime>
#include <memory>
#include <vector>
#include "com/centreon/broker/database_config.hh"
#include "com/centreon/broker/io/endpoint.hh"
#include "com/centreon/broker/namespace.hh"

CCB_BEGIN()

namespace dump {
/**
 *  @class connector connector.hh "com/centreon/broker/dump/connector.hh"
 *  @brief Connect to an dump stream.
 */
class connector : public io::endpoint {
 public:
  connector();
  ~connector();
  connector(connector const& other) = delete;
  connector& operator=(connector const& other) = delete;
  void connect_to(std::string const& filename);
  std::shared_ptr<io::stream> open() override;

 private:
  std::string _filename;
  std::shared_ptr<persistent_cache> _cache;
};
}  // namespace dump

CCB_END()

#endif  // !CCB_INFLUXDB_CONNECTOR_HH
