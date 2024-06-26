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
        testdocumentmanager.cpp
        unittest_documentcreate.cpp
        unittest_embedded_extensions.cpp
        unittest_embedded_importpackage.cpp
        unittest_embedded_lifecycle.cpp
        unittest_embedded_reinflate.cpp
        unittest_rootcontexttargeting.cpp
        )
