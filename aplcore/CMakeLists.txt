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

if (BUILD_SHARED)
    add_library(apl SHARED ${CMAKE_CURRENT_BINARY_DIR}/buildTimeConstants.cpp)
else()
    add_library(apl STATIC ${CMAKE_CURRENT_BINARY_DIR}/buildTimeConstants.cpp)
endif()

# Make sure 3rd party packages are built before the core library
if (TARGET yoga)
    add_dependencies(apl yoga)
endif()
    add_dependencies(apl pegtl-build)

# When not using the system rapidjson build the library, add a dependency on the build step
if (NOT USE_SYSTEM_RAPIDJSON)
    add_dependencies(apl rapidjson-build)
endif()

add_subdirectory(src/action)
add_subdirectory(src/animation)
add_subdirectory(src/command)
add_subdirectory(src/component)
add_subdirectory(src/content)
add_subdirectory(src/datagrammar)
add_subdirectory(src/datasource)
add_subdirectory(src/document)
add_subdirectory(src/engine)
add_subdirectory(src/extension)
add_subdirectory(src/focus)
add_subdirectory(src/graphic)
add_subdirectory(src/livedata)
add_subdirectory(src/media)
add_subdirectory(src/primitives)
add_subdirectory(src/scaling)
add_subdirectory(src/time)
add_subdirectory(src/touch)
add_subdirectory(src/utils)

set(
    PUBLIC_HEADER_LIST
        include/apl/apl.h
        ${PROJECT_BINARY_DIR}/aplcore/include/apl/apl_config.h
        include/apl/dynamicdata.h)

set_target_properties(apl
    PROPERTIES
        VERSION "${PROJECT_VERSION}"
        SOVERSION 1
        PUBLIC_HEADER "${PUBLIC_HEADER_LIST}")

if (ENABLE_PIC)
  set_target_properties(apl
      PROPERTIES
          POSITION_INDEPENDENT_CODE ON
  )
endif()

configure_file(apl.pc.in apl.pc @ONLY)
target_include_directories(apl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/aplcore/include>
        $<BUILD_INTERFACE:${RAPIDJSON_INCLUDE}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${PEGTL_INCLUDE}>
        $<BUILD_INTERFACE:${YOGA_INCLUDE}>
)

target_link_libraries(apl
        PRIVATE
            libyoga)

# include the alexa extensions library
if (BUILD_ALEXAEXTENSIONS)
    target_link_libraries(apl
        PUBLIC
            alexa::extensions)
endif(BUILD_ALEXAEXTENSIONS)

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

target_link_libraries(apl PRIVATE ${log-lib})

endif(ANDROID)

install(TARGETS apl
        EXPORT apl-targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/apl)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/aplcore/include/apl
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

if (NOT USE_SYSTEM_RAPIDJSON)
install(DIRECTORY ${RAPIDJSON_INCLUDE}/rapidjson
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")
endif()

if (NOT YOGA_EXTERNAL_LIB AND NOT USE_SYSTEM_YOGA)
    # We built the bundled yoga lib, install it
    install(FILES ${YOGA_LIB}
        DESTINATION lib)
endif()

set_target_properties(apl PROPERTIES
    EXPORT_NAME
        core
    INTERFACE_LINK_LIBRARIES
        # Only set this for builds, the find module will handle the other cases
        $<BUILD_INTERFACE:${YOGA_LIB}>
)

export(
    EXPORT
        apl-targets
    NAMESPACE
        apl::)

install(
    EXPORT
        apl-targets
    NAMESPACE
        apl::
    DESTINATION
        lib/cmake/aplcore
    FILE
        aplcoreTargets.cmake)


get_filename_component(YOGA_LIB_NAME "${YOGA_LIB}" NAME)
include(CMakePackageConfigHelpers)
configure_package_config_file(aplcoreConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/aplcoreConfig.cmake
    INSTALL_DESTINATION
        lib/cmake/aplcore
    NO_CHECK_REQUIRED_COMPONENTS_MACRO)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/aplcoreConfig.cmake
    DESTINATION
        lib/cmake/aplcore
)
