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
    message(STATUS "Building with lcov Code Coverage Tools")
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(NOT LCOV_PATH)
        message(FATAL_ERROR "lcov not found! Aborting...")
    endif()
    if(NOT GENHTML_PATH)
        message(FATAL_ERROR "genhtml not found! Aborting...")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")

    function(target_add_code_coverage EXEC_NAME TARGET_NAME)
        set(COVERAGE_INFO "${CMAKE_BINARY_DIR}/${TARGET_NAME}.info")
        set(COVERAGE_CLEANED "${COVERAGE_INFO}.cleaned")

        add_custom_target(coverage-${TARGET_NAME}
                ${LCOV_PATH} --directory . --zerocounters
                COMMAND $<TARGET_FILE:${EXEC_NAME}>
                COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${COVERAGE_INFO}
                COMMAND ${LCOV_PATH} --remove ${COVERAGE_INFO} '**/unit/*' '/usr/*' '**/*build/*' '**/thirdparty/*' --output-file ${COVERAGE_CLEANED}
                COMMAND ${GENHTML_PATH} -o ${CMAKE_BINARY_DIR}/coverage ${COVERAGE_CLEANED}
                COMMAND ${CMAKE_COMMAND} -E remove ${COVERAGE_INFO}
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
endif()