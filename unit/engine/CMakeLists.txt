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
        unittest_arrayify.cpp
        unittest_builder.cpp
        unittest_builder_bind.cpp
        unittest_builder_config_change.cpp
        unittest_builder_pager.cpp
        unittest_builder_padding.cpp
        unittest_builder_preserve.cpp
        unittest_builder_preserve_scroll.cpp
        unittest_builder_sequence.cpp
        unittest_context.cpp
        unittest_current_time.cpp
        unittest_dependant.cpp
        unittest_hover.cpp
        unittest_keyboard_manager.cpp
        unittest_layouts.cpp
        unittest_memory.cpp
        unittest_propdef.cpp
        unittest_resources.cpp
        unittest_styles.cpp
        unittest_viewhost.cpp
        )