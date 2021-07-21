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


# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
    ${CMAKE_BINARY_DIR}/googletest-build
    EXCLUDE_FROM_ALL)

find_path(RAPIDJSON_INCLUDE
        NAMES rapidjson/document.h
        REQUIRED)
include_directories(${RAPIDJSON_INCLUDE})


add_executable(alexaext-unittest)

target_link_libraries(alexaext-unittest
        alexaext
        gtest
        gtest_main
        )

target_sources(alexaext-unittest
        PRIVATE
        unittest_apl_audio_player.cpp
        unittest_extension_message.cpp
        unittest_extension_provider.cpp
        unittest_extension_schema.cpp
        unittest_local_extensions.cpp
        )

add_test(all-tests alexaext-unittest)

