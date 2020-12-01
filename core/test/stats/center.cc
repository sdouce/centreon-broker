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

#include <gtest/gtest.h>
#include "com/centreon/broker/config/applier/init.hh"
#include "com/centreon/broker/pool.hh"
#include "com/centreon/broker/version.hh"
#include <iostream>
#include <unistd.h>
#include <fmt/core.h>
#include <json11.hpp>

using namespace com::centreon::broker;
using namespace com::centreon::broker::stats;

class StatsCenterTest : public ::testing::Test {
 public:
  void SetUp() override {
    pool::start(0);
    config::applier::init();
  }
  void TearDown() override {}
};

TEST_F(StatsCenterTest, Simple) {
  auto& stats = stats::center::instance();
  auto ep = stats.register_endpoint("foobar");
  stats.update(ep->mutable_status(), std::string("OK"));
  stats.update(ep->mutable_state(), std::string("Connected"));
  stats.update(&EndpointStats::set_queued_events, ep, 18u);

  std::string stats_to_evaluate = stats.to_string();
  std::string err;
  json11::Json const& result{json11::Json::parse(stats_to_evaluate, err)};

  std::cout << stats_to_evaluate << std::endl;

  ASSERT_TRUE(result["now"].is_string());
  ASSERT_EQ(result["endpoint"][0]["name"].string_value(), "foobar");
  ASSERT_EQ(result["endpoint"][0]["state"].string_value(), "Connected");
  ASSERT_EQ(result["endpoint"][0]["status"].string_value(), "OK");
  ASSERT_EQ(result["endpoint"][0]["queuedEvents"].number_value(), 18u);
  ASSERT_TRUE(ep);
}
