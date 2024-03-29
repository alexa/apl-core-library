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

# Use the FetchContent cmake module to unpack Yoga.  This may not be available
# on older systems
include(FetchContent OPTIONAL RESULT_VARIABLE HAS_FETCH_CONTENT)

cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0135)
    # If available, use the new timestamp policy for FetchContent
    cmake_policy(SET CMP0135 NEW)
endif()

project(APLCoreEngine
        VERSION 1.0.0
        LANGUAGES CXX)

if (NOT APL_CORE_DIR)
    set(APL_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

include(options.cmake)

set(APL_PATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/patches")

include(components.cmake)

if (BUILD_UNIT_TESTS)
    include(CTest)

    set(MEMCHECK_OPTIONS "--tool=memcheck --leak-check=full --show-reachable=no --error-exitcode=1 --errors-for-leak-kinds=definite,possible")
    add_custom_target(unittest_memcheck
        COMMAND ${CMAKE_CTEST_COMMAND} -VV
        --overwrite MemoryCheckCommandOptions=${MEMCHECK_OPTIONS}
        -T memcheck)
endif()
