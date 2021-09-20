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
project(Extensions
        VERSION
        1.0.0
        LANGUAGES
        CXX C)

option(WERROR "Build with -Werror enabled." OFF)
option(BUILD_UNIT_TESTS "Build unit tests" OFF)

if (WERROR)
    message("alexaext - Paranoid build (-Werror) enabled.")
    add_compile_options(-Wall -Werror -Wendif-labels -Wno-sign-compare -Wno-shadow -Wno-missing-braces )
endif(WERROR)

add_subdirectory(alexaext)

if (BUILD_UNIT_TESTS)
    message("alexaext - Unit test build enabled.")
    enable_testing()
    add_subdirectory(unit)
endif()