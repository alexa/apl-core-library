# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
#
#     http://aws.amazon.com/apache2.0/
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.

cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)

## It is expected that the enclosing project provides GTest dependency.

find_path(RAPIDJSON_INCLUDE
        NAMES rapidjson/document.h
        REQUIRED)
include_directories(${RAPIDJSON_INCLUDE})

add_executable(alexaext-unittest
        unittest_apl_audio_player.cpp
        unittest_extension_message.cpp
        unittest_extension_provider.cpp
        unittest_extension_schema.cpp
        unittest_local_extensions.cpp
        )

target_link_libraries(alexaext-unittest
        alexaext
        gtest
        gtest_main
        )

if(CTEST_INDIVIDUALLY)
    # Individual test execution.
    # NOTE: discovered tests are slow to run
    gtest_discover_tests(alexaext-unittest)
else()
    # Batch test execution.
    # Adds the entire unittest executable as a single ctest.  Great for speed.
    add_test(all-extension-tests alexaext-unittest)
endif()

