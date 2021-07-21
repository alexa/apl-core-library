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

if(COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    if(APPLE)
        # xcrun from within the generated makefile is failing to find these tools, so find the full path here
        find_program(XCRUN_PATH xcrun)
        execute_process(COMMAND ${XCRUN_PATH}
                -find
                llvm-profdata
                OUTPUT_VARIABLE LLVM_PROFDATA)
        execute_process(COMMAND ${XCRUN_PATH}
                -find
                llvm-cov
                OUTPUT_VARIABLE LLVM_COV)
        string(REGEX REPLACE "\n$" "" LLVM_PROFDATA "${LLVM_PROFDATA}")
        string(REGEX REPLACE "\n$" "" LLVM_COV "${LLVM_COV}")
        message("Found LLVM coverage at ${LLVM_PROFDATA} and ${LLVM_COV}")
    else()
        set(LLVM_PROFDATA "llvm-profdata")
        set(LLVM_COV "llvm-cov")
    endif()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0.0)
        set(EXCLUDE_COMMAND, -ignore-filename-regex='.*unit.*')
    else()
        message(STATUS "Clang version " ${CMAKE_CXX_COMPILER_VERSION} " is too old to exclude unit test and external files from coverage results")
    endif()

    function(target_add_code_coverage EXEC_NAME TARGET_NAME)
        add_custom_target(coverage-${TARGET_NAME}
                COMMAND
                    LLVM_PROFILE_FILE=${TARGET_NAME}.profraw $<TARGET_FILE:${EXEC_NAME}>
                COMMAND
                    ${LLVM_PROFDATA}
                    merge
                    -sparse
                    ${TARGET_NAME}.profraw
                    -o
                    ${TARGET_NAME}.profdata
                COMMAND
                    ${LLVM_COV}
                    show
                    $<TARGET_FILE:${EXEC_NAME}>
                    -format="html"
                    ${EXCLUDE_COMMAND}
                    -instr-profile=${TARGET_NAME}.profdata
                    -output-dir=${CMAKE_BINARY_DIR}/coverage
                    ${CMAKE_SOURCE_DIR}/aplcore
                DEPENDS ${TARGET_NAME} ${EXEC_NAME}
                )
        add_custom_command(TARGET coverage-${TARGET_NAME} POST_BUILD
                COMMAND ;
                COMMENT "Open file:///${CMAKE_BINARY_DIR}/coverage/index.html to see the coverage."
                )
        if(NOT TARGET local-coverage)
            add_custom_target(local-coverage)
        endif()
        add_dependencies(local-coverage coverage-${TARGET_NAME})
    endfunction()
endif(COVERAGE)