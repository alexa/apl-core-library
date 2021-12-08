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

set(CMAKE_CXX_STANDARD 11)

include_directories(../aplcore/include)
include_directories(${RAPIDJSON_INCLUDE})
include_directories(${PEGTL_INCLUDE})
include_directories(${YOGA_INCLUDE})

if (ANDROID)
    set(OTHER_LIBS log)
endif (ANDROID)

if (ENABLE_ALEXAEXTENSIONS)
    if (NOT BUILD_ALEXAEXTENSIONS)
        # Check to see if it's available from the system.
        find_package(alexaext REQUIRED)
    endif (NOT BUILD_ALEXAEXTENSIONS)

    if(OTHER_LIBS)
        set(OTHER_LIBS "${OTHER_LIBS} alexa::extensions")
    else()
        set(OTHER_LIBS alexa::extensions)
    endif(OTHER_LIBS)

endif(ENABLE_ALEXAEXTENSIONS)

add_executable(parseColor parseColor.cpp)
target_link_libraries(parseColor apl ${OTHER_LIBS})

add_executable(parseExpression parseExpression.cpp)
target_link_libraries(parseExpression apl ${OTHER_LIBS})

add_executable(parseResources parseResources.cpp)
target_link_libraries(parseResources apl ${OTHER_LIBS})

add_executable(parseDimension parseDimension.cpp)
target_link_libraries(parseDimension apl ${OTHER_LIBS})

add_executable(parseLayout parseLayout.cpp)
target_link_libraries(parseLayout apl ${OTHER_LIBS})

add_executable(parseDirective parseDirective.cpp)
target_link_libraries(parseDirective apl ${OTHER_LIBS})

add_executable(fuzzColor fuzzColor.cpp)
target_link_libraries(fuzzColor apl ${OTHER_LIBS})

add_executable(fuzzExpression fuzzExpression.cpp)
target_link_libraries(fuzzExpression apl ${OTHER_LIBS})

add_executable(fuzzEasing fuzzEasing.cpp)
target_link_libraries(fuzzEasing apl ${OTHER_LIBS})

add_executable(parseTransform parseTransform.cpp)
target_link_libraries(parseTransform apl ${OTHER_LIBS})

add_executable(parseEasing parseEasing.cpp)
target_link_libraries(parseEasing apl ${OTHER_LIBS})
