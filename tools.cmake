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

set(ENUMGEN_INSTALL_DIR "${CMAKE_BINARY_DIR}/tools")
set(ENUMGEN_BIN "${ENUMGEN_INSTALL_DIR}/enumgen")

ExternalProject_Add(enumgen
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${APL_PROJECT_DIR}/tools
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${ENUMGEN_INSTALL_DIR}"
)