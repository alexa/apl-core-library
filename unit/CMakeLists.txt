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

# Unit tests
include_directories(../aplcore/include)
include_directories(${RAPIDJSON_INCLUDE})
include_directories(${PEGTL_INCLUDE})
include_directories(${YOGA_INCLUDE})

# Google testing
add_executable( unittest
        testeventloop.cpp
        unittest_action.cpp
        unittest_apl.cpp
        unittest_arithmetic.cpp
        unittest_arrayify.cpp
        unittest_bounds.cpp
        unittest_builder.cpp
        unittest_builder_pager.cpp
        unittest_color.cpp
        unittest_session.cpp
        unittest_command_animateitem.cpp
        unittest_command_array.cpp
        unittest_command_document.cpp
        unittest_command_macros.cpp
        unittest_command_media.cpp
        unittest_command_openurl.cpp
        unittest_command_page.cpp
        unittest_command_sendevent.cpp
        unittest_command_setvalue.cpp
        unittest_commands.cpp
        unittest_context.cpp
        unittest_component_events.cpp
        unittest_current_time.cpp
        unittest_default_component_size.cpp
        unittest_dependant.cpp
        unittest_dependant_graphic.cpp
        unittest_dimension.cpp
        unittest_directive.cpp
        unittest_document.cpp
        unittest_document_background.cpp
        unittest_dynamic_component.cpp
        unittest_easing.cpp
        unittest_encoding.cpp
        unittest_filters.cpp
        unittest_flexbox.cpp
        unittest_focus_manager.cpp
        unittest_grammar.cpp
        unittest_graphic.cpp
        unittest_graphic_component.cpp
        unittest_hover.cpp
        unittest_find_component_at_position.cpp
        unittest_keyboard_manager.cpp
        unittest_keyboard.cpp
        unittest_layouts.cpp
        unittest_log.cpp
        unittest_object.cpp
        unittest_parse.cpp
        unittest_path.cpp
        unittest_rect.cpp
        unittest_resources.cpp
        unittest_scaling.cpp
        unittest_screenlock.cpp
        unittest_scroll.cpp
        unittest_serialize.cpp
        unittest_setstate.cpp
        unittest_setvalue.cpp
        unittest_signature.cpp
        unittest_speak_item.cpp
        unittest_speak_list.cpp
        unittest_state.cpp
        unittest_styledtext.cpp
        unittest_styles.cpp
        unittest_testeventloop.cpp
        unittest_time_grammar.cpp
        unittest_transform.cpp
        unittest_visual_context.cpp
        unittest_viewhost.cpp)


# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
        ${CMAKE_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)

target_link_libraries(unittest apl gtest gtest_main)

if(COVERAGE)
    target_add_code_coverage(unittest apl)
endif()
