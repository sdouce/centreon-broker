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

#include <gtest/gtest.h>

#include "com/centreon/broker/pool.hh"
#include "com/centreon/broker/stats/center.hh"

using namespace com::centreon::broker;
using namespace com::centreon::broker::stats;

class StatsCenterTest : public ::testing::Test {
 public:
  void SetUp() override {
    pool::set_size(4);
  }
  void TearDown() override {
  }

 protected:
  stats::center _stats;
};

TEST_F(StatsCenterTest, Simple) {
  auto ep = _stats.register_endpoint("foobar");
  _stats.update(ep->mutable_status(), std::string("OK"));
  _stats.update(ep->mutable_state(), std::string("Connected"));
  ASSERT_TRUE(ep);
  ASSERT_EQ(ep->name(), "foobar");
  ASSERT_EQ(ep->status(), "OK");
  ASSERT_EQ(ep->state(), "Connected");
}
