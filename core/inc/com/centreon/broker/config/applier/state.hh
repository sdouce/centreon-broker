/*
** Copyright 2011-2012 Centreon
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

#ifndef CCB_CONFIG_APPLIER_STATE_HH
#define CCB_CONFIG_APPLIER_STATE_HH

#include <string>
#include "com/centreon/broker/config/state.hh"
#include "com/centreon/broker/namespace.hh"

CCB_BEGIN()

namespace config {
namespace applier {
/**
 *  @class state state.hh "com/centreon/broker/config/applier/state.hh"
 *  @brief Apply a configuration.
 *
 *  Apply some configuration state.
 */
class state {
  state();
  state(state const& other);
  state& operator=(state const& other);

  std::string _cache_dir;
  uint32_t _poller_id;
  uint32_t _rpc_port;
  std::string _poller_name;
  size_t _pool_size;

 public:
  ~state();
  void apply(config::state const& s, bool run_mux = true);
  std::string const& cache_dir() const noexcept;
  static state& instance();
  static void load();
  uint32_t rpc_port() const noexcept;
  uint32_t poller_id() const noexcept;
  size_t pool_size() const noexcept;
  std::string const& poller_name() const noexcept;
  static void unload();
};
}  // namespace applier
}  // namespace config

CCB_END()

#endif  // !CCB_CONFIG_APPLIER_STATE_HH
