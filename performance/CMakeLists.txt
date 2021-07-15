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

include_directories(../aplcore/include)
include_directories(${RAPIDJSON_INCLUDE})
include_directories(${PEGTL_INCLUDE})
include_directories(${YOGA_INCLUDE})

if(COVERAGE)
    set(CMAKE_CXX_FLAGS "--coverage")
endif(COVERAGE)

# Google testing
add_executable( perftest
        perftest_base.cpp
        perftest_load.cpp
        perftest_operation.cpp
        perftest_visual_context.cpp
        )

configure_file(basic.json basic.json COPYONLY)
configure_file(basic_data.json basic_data.json COPYONLY)

configure_file(complex-list.json complex-list.json COPYONLY)
configure_file(complex-list_data.json complex-list_data.json COPYONLY)

configure_file(hello-world.json hello-world.json COPYONLY)
configure_file(hello-world_data.json hello-world_data.json COPYONLY)

configure_file(simple-import.json simple-import.json COPYONLY)
configure_file(alexa-styles.json alexa-styles.json COPYONLY)
configure_file(alexa-viewport-profiles.json alexa-viewport-profiles.json COPYONLY)
configure_file(alexa-layouts.json alexa-layouts.json COPYONLY)

target_link_libraries(perftest apl gtest_main)
target_link_libraries(perftest libyoga)
