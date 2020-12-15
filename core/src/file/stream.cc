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

#include "com/centreon/broker/file/stream.hh"

#include <cstdio>
#include <limits>
#include <sstream>

#include "com/centreon/broker/io/raw.hh"
#include "com/centreon/broker/misc/string.hh"

using namespace com::centreon::broker;
using namespace com::centreon::broker::file;

/**************************************
 *                                     *
 *           Public Methods            *
 *                                     *
 **************************************/

/**
 *  Constructor.
 *
 *  @param[in] file  Splitted file on which the stream will operate.
 */
stream::stream(splitter* file)
    : io::stream("file"),
      _file(file),
      _last_read_offset(0),
      _last_time(0),
      _last_write_offset(0) {}

/**
 *  Destructor.
 */
stream::~stream() {}

/**
 *  Get peer name.
 *
 *  @return Peer name.
 */
std::string stream::peer() const {
  std::ostringstream oss;
  oss << "file://" << _file->get_file_path();
  return oss.str();
}

/**
 *  Read data from the file.
 *
 *  @param[out] d         Bunch of data.
 *  @param[in]  deadline  Timeout.
 *
 *  @return Always true as file never times out.
 */
bool stream::read(std::shared_ptr<io::data>& d, time_t deadline) {
  (void)deadline;

  d.reset();

  // Build data array.
  std::unique_ptr<io::raw> data(new io::raw);
  data->resize(BUFSIZ);

  // Read data.
  long rb(_file->read(data->data(), data->size()));
  if (rb) {
    data->resize(rb);
    d.reset(data.release());
  }

  return true;
}


/**
 *  Write data to the file.
 *
 *  @param[in] d  Data to write.
 *
 *  @return Number of events acknowledged (1).
 */
int stream::write(std::shared_ptr<io::data> const& d) {
  // Check that data exists.
  if (!validate(d, get_name()))
    return 1;

  if (d->type() == io::raw::static_type()) {
    // Get data.
    char const* memory;
    uint32_t size;
    {
      io::raw* data(static_cast<io::raw*>(d.get()));
      memory = data->data();
      size = data->size();
    }

    // Write data.
    while (size > 0) {
      long wb(_file->write(memory, size));
      size -= wb;
      memory += wb;
    }
  }

  return 1;
}

/**
 *  Remove all the files this stream in concerned by.
 */
void stream::remove_all_files() {
  _file->remove_all_files();
}
