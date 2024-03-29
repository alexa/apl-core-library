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
option(VALIDATE_FORBIDDEN_FUNCTIONS "Validate that there are no calls to forbidden functions" ON)
option(USER_DATA_RELEASE_CALLBACKS "Enable release callbacks in UserData" ON)
option(BUILD_SHARED "Build as shared library." OFF)
option(ENABLE_PIC "Build position independent code (i.e. -fPIC)" OFF)
option(DISABLE_RTTI "Build code without RTTI (i.e. -fno-rtti)" OFF)

option(USE_SYSTEM_RAPIDJSON "Use the system-provided RapidJSON instead of the bundled one." OFF)

option(USE_SYSTEM_YOGA "Use the system-provided Yoga library instead of the bundled one." OFF)
option(USE_PROVIDED_YOGA_INLINE "Use the provided yoga and build it directly into the library." OFF)
# Not listed: YOGA_EXTERNAL_INSTALL_DIR used for an externally provided Yoga library

option(ENABLE_ALEXAEXTENSIONS "Use the Alexa Extensions library." OFF)
option(BUILD_ALEXAEXTENSIONS "Build Alexa Extensions library as part of the project." OFF)

option(ENABLE_SCENEGRAPH "Build and enable Scene Graph support" OFF)


# Enumgen is only built by default for certain platforms
if (NOT APPLE)
    set(BUILD_ENUMGEN_DEFAULT ON)
else()
    set(BUILD_ENUMGEN_DEFAULT OFF)
endif()
option(BUILD_ENUMGEN "Build the enumgen tool" ${BUILD_ENUMGEN_DEFAULT})

# Test options
option(BUILD_TESTS "Build unit tests and test programs." OFF)
option(BUILD_TEST_PROGRAMS "Build test programs. Included if BUILD_TESTS=ON" OFF)
option(BUILD_UNIT_TESTS "Build unit tests. Included if BUILD_TESTS=ON" OFF)
option(BUILD_GMOCK "Build googlemock instead of googletest." OFF)
option(INSTALL_GTEST "Install googletest as library." OFF)

# Doxygen build
option(BUILD_DOC "Build documentation." ON)

# Extension options set-up.
if(BUILD_ALEXAEXTENSIONS)
    # Backwards compatibility
    set(ENABLE_ALEXAEXTENSIONS ON)
endif(BUILD_ALEXAEXTENSIONS)

if(ENABLE_ALEXAEXTENSIONS)
    set(ALEXAEXTENSIONS ON)
endif(ENABLE_ALEXAEXTENSIONS)

if(ENABLE_SCENEGRAPH)
    set(SCENEGRAPH ON)
endif(ENABLE_SCENEGRAPH)

# Building Yoga inline depends on having the FetchContent module
if(USE_PROVIDED_YOGA_INLINE AND NOT HAS_FETCH_CONTENT)
    message(FATAL_ERROR "The FetchContent module is needed to build yoga inline")
endif()

# Clean up Yoga based on settings.  Throw a fatal error if more than one Yoga option is set
# The default before is to use the provided Yoga.  Start with it off; turn it on if no other option is set
set(USE_PROVIDED_YOGA_AS_LIB OFF)

if(YOGA_EXTERNAL_INSTALL_DIR)
    if (USE_SYSTEM_YOGA OR USE_PROVIDED_YOGA_INLINE)
        message(FATAL_ERROR "An external yoga directory is incompatible with specifying the system or provided yoga")
    endif()
elseif(USE_SYSTEM_YOGA)
    if (USE_PROVIDED_YOGA_INLINE)
        message(FATAL_ERROR "Using the system yoga is incompatible with using the provided yoga")
    endif()
elseif(NOT USE_PROVIDED_YOGA_INLINE)
    set(USE_PROVIDED_YOGA_AS_LIB ON)
endif()

# Check for evaluation depth limit.  If not set, pick a default
# To override this value, set it on the command line.  For example:
#    cmake .. -DEVALUATION_DEPTH_LIMIT=10
if(NOT EVALUATION_DEPTH_LIMIT)
    set(EVALUATION_DEPTH_LIMIT 5)
endif()
message(STATUS "Evaluation depth limit set to ${EVALUATION_DEPTH_LIMIT}")


# Capture the compile-time options to apl_config.h so that headers can be distributed
configure_file(${APL_CORE_DIR}/aplcore/include/apl/apl_config.h.in aplcore/include/apl/apl_config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/aplcore/include)
