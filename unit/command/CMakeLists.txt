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
        unittest_command_animateitem.cpp
        unittest_command_array.cpp
        unittest_command_document.cpp
        unittest_command_event_binding.cpp
        unittest_command_macros.cpp
        unittest_command_media.cpp
        unittest_command_openurl.cpp
        unittest_command_page.cpp
        unittest_command_select.cpp
        unittest_command_sendevent.cpp
        unittest_command_setvalue.cpp
        unittest_commands.cpp
        unittest_screenlock.cpp
        unittest_serialize_event.cpp
        unittest_setstate.cpp
        unittest_setvalue.cpp
        unittest_speak_item.cpp
        unittest_speak_list.cpp
        )