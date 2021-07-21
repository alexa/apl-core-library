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

# APL options
option(DEBUG_MEMORY_USE "Track memory use." OFF)
option(TRACING "Enable tracing." OFF)
option(COVERAGE "Coverage instrumentation" OFF)
option(WERROR "Build with -Werror enabled." OFF)
option(VALIDATE_HEADERS "Validate that only external headers are (transitively) included from apl.h" ON)
option(USER_DATA_RELEASE_CALLBACKS "Enable release callbacks in UserData" ON)
option(BUILD_SHARED "Build as shared library." OFF)
option(ENABLE_PIC "Build position independent code (i.e. -fPIC)" OFF)

option(USE_SYSTEM_RAPIDJSON "Use the system-provided RapidJSON instead of the bundled one." OFF)
option(USE_SYSTEM_YOGA "Use the system-provided Yoga library instead of the bundled one." OFF)
option(BUILD_ALEXAEXTENSIONS "Use the Alexa Extensions library." OFF)

# Test options
option(BUILD_TESTS "Build unit tests and test programs." OFF)
option(BUILD_TEST_PROGRAMS "Build test programs. Included if BUILD_TESTS=ON" OFF)
option(BUILD_UNIT_TESTS "Build unit tests. Included if BUILD_TESTS=ON" OFF)
option(BUILD_GMOCK "Build googlemock instead of googletest." OFF)
option(INSTALL_GTEST "Install googletest as library." OFF)

# Doxygen build
option(BUILD_DOC "Build documentation." ON)

# Capture the compile-time options to apl_config.h so that headers can be distributed
configure_file(${APL_CORE_DIR}/aplcore/include/apl/apl_config.h.in aplcore/include/apl/apl_config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/aplcore/include)
