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

project (apl VERSION 1.0.0 LANGUAGES C CXX)

if (TRACING)
    message("Tracing enabled.")
    add_definitions(-DENABLE_TRACING)
endif(TRACING)

if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

if (WERROR)
    message("Paranoid build (-Werror) enabled.")
    add_compile_options(-Wall -Werror -Wendif-labels -Wno-sign-compare -Wno-shadow -Wno-missing-braces )
endif(WERROR)

# Set compilation flags for memory debugging
if (DEBUG_MEMORY_USE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG_MEMORY_USE=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG_MEMORY_USE=1")
endif (DEBUG_MEMORY_USE)

if(COVERAGE)
    # We can't really generate core coverage without tests. Option will be applied in clang.cmake as feature is clang
    # specific.
    set(BUILD_TESTS ON)
    message("Coverage instrumentation enabled.")
endif(COVERAGE)

# Enforce clang linking if it's used. Useful when building on Pi/etc.
if(APPLE)
    function(append value)
        foreach(variable ${ARGN})
            set(${variable} "${${variable}} ${value}" PARENT_SCOPE)
        endforeach(variable)
    endfunction()

    append("-stdlib=libc++" CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
    append("-lc++abi" CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
    message("Using Clang")
    include(clang.cmake)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    message("Using gcc")
    include(gcc.cmake)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    message("Using msvc")
    include(msvc.cmake)
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} is not known")
endif()

# Do not move. It requires WASM_FLAGS while defining targets and generates part of environment required by next target.
include(thirdparty/thirdparty.cmake)

# We treat enumgen as an external project because it needs to be built using the host toolchain
include(tools.cmake)

# The core library
add_subdirectory(aplcore)

if(BUILD_DOC)
    include(doxygen.cmake)
endif(BUILD_DOC)

if (BUILD_TESTS)
    set(BUILD_UNIT_TESTS ON)
    set(BUILD_TEST_PROGRAMS ON)
endif (BUILD_TESTS)

# Build with dependency on the Alexa Extensions library.
if (BUILD_ALEXAEXTENSIONS)
    add_subdirectory(extensions)
    target_compile_definitions(alexaext PUBLIC ALEXAEXTENSIONS BUILD_UNIT_TESTS=${BUILD_UNIT_TESTS})
    if (NOT USE_SYSTEM_RAPIDJSON)
        add_dependencies(alexaext rapidjson-build)
    endif()
endif(BUILD_ALEXAEXTENSIONS)

# Test cases are built conditionally. Only affect core do not build them for everything else.
if (BUILD_UNIT_TESTS)
    include(CTest)
    include_directories(${GTEST_INCLUDE})
    add_subdirectory(unit)

    set(MEMCHECK_OPTIONS "--tool=memcheck --leak-check=full --show-reachable=no --error-exitcode=1 --errors-for-leak-kinds=definite,possible")
    add_custom_target(unittest_memcheck
            COMMAND ${CMAKE_CTEST_COMMAND} -VV
            --overwrite MemoryCheckCommandOptions=${MEMCHECK_OPTIONS}
            -T memcheck)
endif (BUILD_UNIT_TESTS)

if (BUILD_TEST_PROGRAMS)
    add_subdirectory(test)
endif (BUILD_TEST_PROGRAMS)

if (VALIDATE_HEADERS)
    add_custom_command(OUTPUT include_validation
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/bin/apl-header-inclusion-validation.sh
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)
    add_custom_target(validation ALL
            DEPENDS include_validation)
endif (VALIDATE_HEADERS)
