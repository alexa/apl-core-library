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

project(host-tools)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APL_PATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../patches")
set(APL_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

include(../thirdparty/thirdparty.cmake)

include_directories(include)

add_executable(enumgen
        src/enumparser.cpp
        src/enumgen.cpp)

if (NOT HAS_FETCH_CONTENT)
        add_dependencies(enumgen pegtl-build)
endif()

include_directories(${PEGTL_INCLUDE})

if(MSVC)
        SET_TARGET_PROPERTIES(enumgen PROPERTIES LINK_FLAGS "/link setargv.obj")
endif()

install(TARGETS enumgen RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
