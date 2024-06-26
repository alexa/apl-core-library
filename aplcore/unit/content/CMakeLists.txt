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

target_sources_local(unittest
        PRIVATE
        testpackagemanager.cpp
        unittest_apl.cpp
        unittest_directive.cpp
        unittest_document.cpp
        unittest_document_background.cpp
        unittest_jsondata.cpp
        unittest_metrics.cpp
        unittest_packagemanager.cpp
        unittest_packages.cpp
        unittest_rootconfig.cpp
        unittest_sharedjsondata.cpp
        )