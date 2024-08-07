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

# Disable what needs to be disabled
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

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

if (NOT USE_PROVIDED_YOGA_INLINE)
    target_link_libraries(apl PUBLIC $<BUILD_INTERFACE:libyoga>)
endif()

if (NOT HAS_FETCH_CONTENT)
    add_dependencies(apl pegtl-build)
endif()

add_subdirectory(src/action)
add_subdirectory(src/animation)
add_subdirectory(src/audio)
add_subdirectory(src/command)
add_subdirectory(src/component)
add_subdirectory(src/content)
add_subdirectory(src/datagrammar)
add_subdirectory(src/datasource)
add_subdirectory(src/document)
add_subdirectory(src/embed)
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
add_subdirectory(src/versioning)
add_subdirectory(src/scenegraph)

set(
    PUBLIC_HEADER_LIST
        include/apl/apl.h
        ${PROJECT_BINARY_DIR}/aplcore/include/apl/apl_config.h
        include/apl/dynamicdata.h)

set_target_properties(apl
    PROPERTIES
        VERSION "${PROJECT_VERSION}"
        SOVERSION 1
        PUBLIC_HEADER "${PUBLIC_HEADER_LIST}"
        EXPORT_NAME "core"
)

if (ENABLE_PIC)
  set_target_properties(apl
      PROPERTIES
          POSITION_INDEPENDENT_CODE ON
  )
endif()

# Generate a pkg-config file
configure_file(apl.pc.in apl.pc @ONLY)

target_include_directories(apl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/aplcore/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${PEGTL_INCLUDE}>
        $<BUILD_INTERFACE:${YOGA_INCLUDE}>
)

target_link_libraries(apl PUBLIC rapidjson-apl)

if (USE_PROVIDED_YOGA_INLINE)
    target_sources(apl PRIVATE ${YOGA_SRC})
endif()

# include the alexa extensions library
if (ENABLE_ALEXAEXTENSIONS)
    if (NOT BUILD_ALEXAEXTENSIONS)
        # Check to see if it's available from the system.
        find_package(alexaext REQUIRED)
    endif (NOT BUILD_ALEXAEXTENSIONS)
    target_link_libraries(apl
        PUBLIC
            alexa::extensions)
endif(ENABLE_ALEXAEXTENSIONS)

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

# Test cases are built conditionally. Only affect core do not build them for everything else.
if (BUILD_UNIT_TESTS)
    include_directories(${GTEST_INCLUDE})
    add_subdirectory(unit)
endif (BUILD_UNIT_TESTS)

install(TARGETS apl
        EXPORT apl-targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/apl)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/aplcore/include/apl
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

# Put the generated pkg-config file in the right directory?
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/apl.pc
        DESTINATION lib/pkgconfig)

if (USE_PROVIDED_YOGA_AS_LIB)
    # We built the bundled yoga lib, install it
    install(FILES ${YOGA_LIB}
        DESTINATION lib)
    install(DIRECTORY ${YOGA_INCLUDE}/yoga
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")
    set(YOGA_EXTERNAL_LIB ${YOGA_LIB}) # used by aplcoreConfig.cmake.in
endif()

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