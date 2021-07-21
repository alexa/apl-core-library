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

project(APLCoreEngine
        VERSION 1.0.0
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT APL_CORE_DIR)
    set(APL_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

include(options.cmake)

set(APL_PATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/patches")

include(components.cmake)
