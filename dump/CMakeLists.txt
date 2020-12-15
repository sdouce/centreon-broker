##
## Copyright 2020 Centreon
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##
## For more information : contact@centreon.com
##

# Global options.
set(INC_DIR "${PROJECT_SOURCE_DIR}/dump/inc")
set(SRC_DIR "${PROJECT_SOURCE_DIR}/dump/src")
set(TEST_DIR "${PROJECT_SOURCE_DIR}/dump/test")
include_directories("${INC_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/neb/inc")
include_directories("${PROJECT_SOURCE_DIR}/storage/inc")
set(INC_DIR "${INC_DIR}/com/centreon/broker/dump")

# DUMP module.
set(DUMP "70-dump")
set(DUMP "${DUMP}" PARENT_SCOPE)
add_library("${DUMP}" SHARED
  # Sources.
  "${SRC_DIR}/connector.cc"
  "${SRC_DIR}/factory.cc"
  "${SRC_DIR}/main.cc"
  "${SRC_DIR}/stream.cc"
  # Headers.
  "${INC_DIR}/connector.hh"
  "${INC_DIR}/factory.hh"
  "${INC_DIR}/stream.hh"
)
set_target_properties("${DUMP}" PROPERTIES PREFIX "")

# Testing.
#if (WITH_TESTING)
#  set(
#    TESTS_SOURCES
#    ${TESTS_SOURCES}
#    ${TEST_DIR}/column.cc
#    ${TEST_DIR}/factory.cc
#    ${TEST_DIR}/dump12.cc
#    ${TEST_DIR}/line_protocol_query.cc
#    ${TEST_DIR}/stream.cc
#    PARENT_SCOPE
#  )
#  set(
#    TESTS_LIBRARIES
#    ${TESTS_LIBRARIES}
#    ${DUMP}
#    PARENT_SCOPE
#  )
#endif(WITH_TESTING)
#

# Install rule.
install(TARGETS "${DUMP}"
  LIBRARY DESTINATION "${PREFIX_MODULES}"
)
