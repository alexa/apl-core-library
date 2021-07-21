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

# Unit tests
include_directories(../aplcore/include)
include_directories(../tools/src)
include_directories(${RAPIDJSON_INCLUDE})
include_directories(${PEGTL_INCLUDE})
include_directories(${YOGA_INCLUDE})

# Google testing
add_executable( unittest
        testeventloop.cpp
        debugtools.cpp
        unittest_simpletextmeasurement.cpp
        unittest_testeventloop.cpp
        ../tools/src/enumparser.cpp
        unittest_enumgen.cpp
)

add_subdirectory(action)
add_subdirectory(animation)
add_subdirectory(command)
add_subdirectory(component)
add_subdirectory(content)
add_subdirectory(datagrammar)
add_subdirectory(datasource)
add_subdirectory(engine)
add_subdirectory(extension)
add_subdirectory(focus)
add_subdirectory(graphic)
add_subdirectory(livedata)
add_subdirectory(media)
add_subdirectory(primitives)
add_subdirectory(scaling)
add_subdirectory(time)
add_subdirectory(touch)
add_subdirectory(utils)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
        ${CMAKE_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)
if(COVERAGE)
    target_add_code_coverage(unittest apl)
endif()
target_link_libraries(unittest apl gtest gtest_main)
if (BUILD_ALEXAEXTENSIONS)
    target_link_libraries(unittest alexaext)
endif(BUILD_ALEXAEXTENSIONS)

if (ANDROID)
    target_link_libraries(unittest log)
endif(ANDROID)

if(CTEST_INDIVIDUALLY)
    # NOTE: discovered ctest targets below are much slower than their direct counterparts.  Ctest loads
    #       tests individually instead of just running all in a class.  This makes it take much
    #       longer for execution.  This is somewhat useful if you want to execute tests using ctest scripts, but is
    #       completely unusable on dev machine: each test takes 800ms vs 20ms, and valgrind takes 4-5s per test.
    include(GoogleTest)
    gtest_discover_tests(unittest)
else()
    # Adds the entire unittest executable as a single ctest.  Great for speed.
    add_test(all-tests unittest)
    if (BUILD_ALEXAEXTENSIONS)
        add_test(all-tests alexaext-unittest)
    endif(BUILD_ALEXAEXTENSIONS)
endif()

