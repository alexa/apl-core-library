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
set(CMAKE_CXX_STANDARD 11)

# Disable what needs to be disabled
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

project(AlexaExt
        VERSION
        1.0.0
        LANGUAGES
        CXX C)

add_library(alexaext STATIC
            src/APLAudioPlayerExtension/AplAudioPlayerExtension.cpp
            src/APLE2EEncryptionExtension/AplE2eEncryptionExtension.cpp
        src/APLWebflowExtension/AplWebflowBase.cpp
        src/APLWebflowExtension/AplWebflowExtension.cpp
        src/APLMusicAlarmExtension/AplMusicAlarmExtension.cpp
            src/executor.cpp
            src/extensionmessage.cpp
            src/extensionregistrar.cpp
            src/localextensionproxy.cpp
            src/random.cpp
            src/sessiondescriptor.cpp
    )

if (BUILD_SHARED OR ENABLE_PIC)
    set_target_properties(alexaext
                PROPERTIES
                POSITION_INDEPENDENT_CODE ON
                )
endif()

set_target_properties(alexaext
    PROPERTIES
        EXPORT_NAME extensions
)

target_include_directories(alexaext
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${RAPIDJSON_INCLUDE}>
)

target_compile_options(alexaext
    PRIVATE
        -Werror
)

install(
    TARGETS
        alexaext
    EXPORT
        alexaext-targets
    ARCHIVE DESTINATION
        lib
    LIBRARY DESTINATION
        lib
    PUBLIC_HEADER DESTINATION
        include
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/include/alexaext
    DESTINATION
        include
    FILES_MATCHING PATTERN "*.h"
)

export(
    EXPORT
        alexaext-targets
    NAMESPACE
        alexa::
)

install(
    EXPORT
        alexaext-targets
    NAMESPACE
        alexa::
    DESTINATION
        lib/cmake/alexaext
    FILE
        alexaextConfig.cmake
)

add_library(alexa::extensions ALIAS alexaext)

