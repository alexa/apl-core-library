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
        unittest_accessibility_actions.cpp
        unittest_accessibility_api.cpp
        unittest_bounds.cpp
        unittest_component_events.cpp
        unittest_default_component_size.cpp
        unittest_draw.cpp
        unittest_dynamic_component.cpp
        unittest_dynamic_container_properties.cpp
        unittest_dynamic_properties.cpp
        unittest_edit_text_component.cpp
        unittest_find_component_at_position.cpp
        unittest_flexbox.cpp
        unittest_frame_component.cpp
        unittest_grid_sequence_component.cpp
        unittest_layout_direction.cpp
        unittest_pager.cpp
        unittest_properties.cpp
        unittest_scroll.cpp
        unittest_serialize.cpp
        unittest_signature.cpp
        unittest_state.cpp
        unittest_text_component.cpp
        unittest_tick.cpp
        unittest_transform.cpp
        unittest_visual_context.cpp
        )