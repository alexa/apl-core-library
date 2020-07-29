# Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

set(CMAKE_CXX_STANDARD 11)

include_directories(../aplcore/include)
include_directories(${RAPIDJSON_INCLUDE})
include_directories(${PEGTL_INCLUDE})
include_directories(${YOGA_INCLUDE})

add_executable(parseColor parseColor.cpp)
target_link_libraries(parseColor apl)

add_executable(parseExpression parseExpression.cpp)
target_link_libraries(parseExpression apl)

add_executable(parseResources parseResources.cpp)
target_link_libraries(parseResources apl)

add_executable(parseDimension parseDimension.cpp)
target_link_libraries(parseDimension apl)

add_executable(parseLayout parseLayout.cpp)
target_link_libraries(parseLayout apl)

add_executable(parseDirective parseDirective.cpp)
target_link_libraries(parseDirective apl)

add_executable(fuzzColor fuzzColor.cpp)
target_link_libraries(fuzzColor apl)

add_executable(fuzzExpression fuzzExpression.cpp)
target_link_libraries(fuzzExpression apl)

add_executable(fuzzEasing fuzzEasing.cpp)
target_link_libraries(fuzzEasing apl)

add_executable(parseTransform parseTransform.cpp)
target_link_libraries(parseTransform apl)

add_executable(parseEasing parseEasing.cpp)
target_link_libraries(parseEasing apl)
