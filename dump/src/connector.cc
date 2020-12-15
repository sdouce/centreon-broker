/*
** Copyright 2011-2017 Centreon
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

#include "com/centreon/broker/dump/connector.hh"
#include "com/centreon/broker/dump/stream.hh"
#include "com/centreon/broker/persistent_cache.hh"

using namespace com::centreon::broker;
using namespace com::centreon::broker::dump;

/**************************************
 *                                     *
 *           Public Methods            *
 *                                     *
 **************************************/

/**
 *  Default constructor.
 */
connector::connector() : io::endpoint(false) {}

/**
 *  Destructor.
 */
connector::~connector() {}

/**
 *  Set connection parameters.
 *
 */
void connector::connect_to(std::string const& filename) {
  _filename = filename;
}

/**
 * @brief Connect to an dump DB.
 *
 * @return An Influxdb connection object.
 */
std::shared_ptr<io::stream> connector::open() {
  return std::make_shared<stream>(_filename);
}
