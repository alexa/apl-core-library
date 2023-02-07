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
        test_sg.cpp
        testedittext.cpp
        unittest_sg_accessibility.cpp
        unittest_sg_edit_text.cpp
        unittest_sg_edit_text_config.cpp
        unittest_sg_filter.cpp
        unittest_sg_frame.cpp
        unittest_sg_graphic.cpp
        unittest_sg_graphic_component.cpp
        unittest_sg_graphic_layers.cpp
        unittest_sg_graphic_loading.cpp
        unittest_sg_image.cpp
        unittest_sg_layer.cpp
        unittest_sg_line_highlighting.cpp
        unittest_sg_node.cpp
        unittest_sg_nodebounds.cpp
        unittest_sg_pager.cpp
        unittest_sg_paint.cpp
        unittest_sg_path.cpp
        unittest_sg_pathbounds.cpp
        unittest_sg_pathop.cpp
        unittest_sg_pathparser.cpp
        unittest_sg_text.cpp
        unittest_sg_text_properties.cpp
        unittest_sg_touch.cpp
        )