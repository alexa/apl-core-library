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

# Unit tests
include_directories(../aplcore/include)
include_directories(${RAPIDJSON_INCLUDE})
include_directories(${PEGTL_INCLUDE})
include_directories(${YOGA_INCLUDE})

# Google testing
add_executable( unittest
        testeventloop.cpp
        debugtools.cpp
        action/unittest_action.cpp
        animation/unittest_easing.cpp
        animation/unittest_easing_approximation.cpp
        command/unittest_command_animateitem.cpp
        command/unittest_command_array.cpp
        command/unittest_command_document.cpp
        command/unittest_command_macros.cpp
        command/unittest_command_media.cpp
        command/unittest_command_openurl.cpp
        command/unittest_command_page.cpp
        command/unittest_command_select.cpp
        command/unittest_command_sendevent.cpp
        command/unittest_command_setvalue.cpp
        command/unittest_commands.cpp
        command/unittest_screenlock.cpp
        command/unittest_serialize_event.cpp
        command/unittest_setstate.cpp
        command/unittest_setvalue.cpp
        command/unittest_speak_item.cpp
        command/unittest_speak_list.cpp
        component/unittest_bounds.cpp
        component/unittest_component_events.cpp
        component/unittest_default_component_size.cpp
        component/unittest_dynamic_component.cpp
        component/unittest_edit_text_component.cpp
        component/unittest_find_component_at_position.cpp
        component/unittest_flexbox.cpp
        component/unittest_frame_component.cpp
        component/unittest_grid_sequence_component.cpp
        component/unittest_scroll.cpp
        component/unittest_serialize.cpp
        component/unittest_signature.cpp
        component/unittest_state.cpp
        component/unittest_tick.cpp
        component/unittest_visual_context.cpp
        content/unittest_apl.cpp
        content/unittest_directive.cpp
        content/unittest_document.cpp
        content/unittest_document_background.cpp
        datagrammar/unittest_arithmetic.cpp
        datagrammar/unittest_grammar.cpp
        datagrammar/unittest_parse.cpp
        datasource/testdatasourceprovider.cpp
        datasource/unittest_datasource.cpp
        datasource/unittest_dynamicindexlist.cpp
        engine/unittest_arrayify.cpp
        engine/unittest_builder.cpp
        engine/unittest_builder_pager.cpp
        engine/unittest_builder_sequence.cpp
        engine/unittest_context.cpp
        engine/unittest_current_time.cpp
        engine/unittest_dependant.cpp
        engine/unittest_focus_manager.cpp
        engine/unittest_hover.cpp
        engine/unittest_keyboard_manager.cpp
        engine/unittest_layouts.cpp
        engine/unittest_memory.cpp
        engine/unittest_propdef.cpp
        engine/unittest_resources.cpp
        engine/unittest_styles.cpp
        engine/unittest_viewhost.cpp
        extension/unittest_extension_command.cpp
        extension/unittest_extension_handler.cpp
        extension/unittest_requested_extension.cpp
        graphic/unittest_dependant_graphic.cpp
        graphic/unittest_graphic.cpp
        graphic/unittest_graphic_component.cpp
        livedata/unittest_livearray_change.cpp
        livedata/unittest_livearray_rebuild.cpp
        livedata/unittest_livemap_change.cpp
        primitives/unittest_color.cpp
        primitives/unittest_dimension.cpp
        primitives/unittest_filters.cpp
        primitives/unittest_keyboard.cpp
        primitives/unittest_object.cpp
        primitives/unittest_rect.cpp
        primitives/unittest_styledtext.cpp
        primitives/unittest_symbols.cpp
        primitives/unittest_time_grammar.cpp
        primitives/unittest_transform.cpp
        primitives/unittest_unicode.cpp
        scaling/unittest_scaling.cpp
        time/unittest_sequencer.cpp
        touch/unittest_gestures.cpp
        touch/unittest_pointer.cpp
        unittest_testeventloop.cpp
        utils/unittest_encoding.cpp
        utils/unittest_log.cpp
        utils/unittest_path.cpp
        utils/unittest_range.cpp
        utils/unittest_session.cpp
        utils/unittest_url.cpp
        utils/unittest_userdata.cpp
        utils/unittest_weakcache.cpp)


# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
        ${CMAKE_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)
if(COVERAGE)
    target_add_code_coverage(unittest apl)
endif()
target_link_libraries(unittest apl gtest gtest_main)
if(CTEST_INDIVIDUALLY)
    # NOTE: discovered ctest targets below are much slower than their direct counterparts.  Ctest loads
    #       tests individually instead of just running all in a class.  This makes it take much
    #       longer for execution.  This is somewhat useful if you want to execute tests using ctest scripts, but is
    #       completely unusable on dev machine: each test takes 800ms vs 20ms, and valgrind takes 4-5s per test.
    include(GoogleTest)
    gtest_discover_tests(unittest)
else()
    # Adds the entire unittest executable as a single ctest.  Great for speed.
    add_test(all-tests unittest)
endif()

