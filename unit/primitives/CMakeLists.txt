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
        unittest_color.cpp
        unittest_dimension.cpp
        unittest_filters.cpp
        unittest_keyboard.cpp
        unittest_object.cpp
        unittest_rect.cpp
        unittest_styledtext.cpp
        unittest_symbols.cpp
        unittest_time_grammar.cpp
        unittest_transform.cpp
        unittest_unicode.cpp
        )