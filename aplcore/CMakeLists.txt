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

# Set template depth to support pegtl on older versions of clang.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=1024")
endif()

include(target_sources_local.cmake)

# Check for the presence of GIT
# Code from: https://gist.github.com/rkitover
find_package(Git)
if (GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" describe --tags --dirty
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE res
            OUTPUT_VARIABLE CORE_REPOSITORY_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    if (res)
        message(VERBOSE "Git repository not found in directory ${CMAKE_CURRENT_SOURCE_DIR}, not setting version info")
        set(CORE_REPOSITORY_VERSION "unknown")
    endif()
else()
    message(VERBOSE "Git program not found (${GIT_FOUND}), not setting version info")
    SET(CORE_REPOSITORY_VERSION "unknown")
endif()

message(STATUS "Core Repository Version ${CORE_REPOSITORY_VERSION}")
configure_file(buildTimeConstants.cpp.in buildTimeConstants.cpp @ONLY)

add_library(apl STATIC ${CMAKE_CURRENT_BINARY_DIR}/buildTimeConstants.cpp)

add_subdirectory(src/action)
add_subdirectory(src/animation)
add_subdirectory(src/command)
add_subdirectory(src/component)
add_subdirectory(src/content)
add_subdirectory(src/datagrammar)
add_subdirectory(src/datasource)
add_subdirectory(src/engine)
add_subdirectory(src/extension)
add_subdirectory(src/graphic)
add_subdirectory(src/livedata)
add_subdirectory(src/primitives)
add_subdirectory(src/scaling)
add_subdirectory(src/time)
add_subdirectory(src/touch)
add_subdirectory(src/utils)

set(PUBLIC_HEADER_LIST
        include/apl/apl.h
        include/apl/dynamicdata.h)

set_target_properties(apl PROPERTIES
                      VERSION "${PROJECT_VERSION}"
                      SOVERSION 1
                      PUBLIC_HEADER "${PUBLIC_HEADER_LIST}")

configure_file(apl.pc.in apl.pc @ONLY)
target_include_directories(apl PRIVATE include)

add_library(libyoga STATIC IMPORTED)
set_target_properties(libyoga PROPERTIES
    IMPORTED_LINK_INTERFACE_LIBRARIES ""
    IMPORTED_LOCATION "${YOGA_LIB}"
)
target_link_libraries(apl libyoga)


# The core library
add_dependencies(apl yoga)
add_dependencies(apl pegtl-build)
add_dependencies(apl rapidjson-build)

include_directories(include)
include_directories(${RAPIDJSON_INCLUDE})
include_directories(${PEGTL_INCLUDE})
include_directories(${YOGA_INCLUDE})

# Log dependencies for Android

if (ANDROID)

# Searches for the prebuilt Android NDK log library and stores the path as a
# variable. CMake verifies that the library exists before
# completing its build.
find_library(
        # Sets the name of the path variable.
        log-lib
        # Specifies the name of the NDK library that you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.
target_link_libraries(
        # Specifies the target library.
        apl
        # Links the target library to the log library included in the NDK.
        ${log-lib})

endif(ANDROID)
