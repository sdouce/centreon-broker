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

#include "com/centreon/broker/dump/stream.hh"
#include <sstream>
#include <fstream>
#include "com/centreon/broker/exceptions/msg.hh"
#include "com/centreon/broker/exceptions/shutdown.hh"
#include "com/centreon/broker/io/events.hh"
#include "com/centreon/broker/logging/logging.hh"
#include "com/centreon/broker/misc/global_lock.hh"
#include "com/centreon/broker/multiplexing/engine.hh"
#include "com/centreon/broker/multiplexing/publisher.hh"
#include "com/centreon/broker/storage/internal.hh"
#include "com/centreon/broker/storage/metric.hh"

using namespace com::centreon::broker;
using namespace com::centreon::broker::dump;

/**
 *  Constructor.
 *
 */
stream::stream(std::string const& filename)
    : io::stream("dump"),
      _filename(filename),
      _user(""),
      _password(""),
      _address(""),
      _db(""),
      _queries_per_transaction(0), 
      _pending_queries(0),
      _actual_query(0),
      _commit(false) {
  _fp = fopen(_filename.c_str(), "a");
  if (_fp == NULL)
    throw exceptions::msg() << "cannot open file " << _filename;
}

/**
 *  Destructor.
 */
stream::~stream() {
  fclose(_fp);
}

/**
 *  Flush the stream.
 *
 *  @return Number of events acknowledged.
 */
int stream::flush() {
  int ret(_pending_queries);
  fflush(_fp);
  _pending_queries = 0;
  return ret;
}

/**
 *  Read from the datbase.
 *
 *  @param[out] d         Cleared.
 *  @param[in]  deadline  Timeout.
 *
 *  @return This method will throw.
 */
bool stream::read(std::shared_ptr<io::data>& d, time_t deadline) {
  (void)deadline;
  d.reset();
  throw exceptions::shutdown() << "cannot read from Dump ";
  return true;
}

/**
 *  Write an event.
 *
 *  @param[in] data Event pointer.
 *
 *  @return Number of events acknowledged.
 */

int stream::write(std::shared_ptr<io::data> const& data) {
  int ret = 0;
  if (fprintf(_fp, "%u\n", data->type()) < 0)
    throw exceptions::msg() << "fprintf() error: cannot write into the file";
  //force flush stream
  _pending_queries++;
  if (_pending_queries == 20) {
    if (fflush(_fp) != 0)
      throw exceptions::msg() << "fflush() error: cannot clean the buffer";
    ret = _pending_queries;
    _pending_queries = 0;
  }
  return ret;
}
