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

# APL options
option(DEBUG_MEMORY_USE "Track memory use." OFF)
option(TELEMETRY "Telemetry support. Required for performance tests." OFF)
option(COVERAGE "Coverage instrumentation" OFF)

# Test options
option(BUILD_TESTS "Build test programs." OFF)
option(BUILD_GMOCK "Build googlemock instead of googletest." OFF)
option(INSTALL_GTEST "Install googletest as library." OFF)

# Doxygen build
option(BUILD_DOC "Build documentation." ON)
