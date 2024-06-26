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

if(ENABLE_SCENEGRAPH)
    target_sources_local(apl
            PRIVATE
            accessibility.cpp
            builder.cpp
            edittextconfig.cpp
            filter.cpp
            layer.cpp
            node.cpp
            graphicfragment.cpp
            paint.cpp
            path.cpp
            pathop.cpp
            pathbounds.cpp
            pathparser.cpp
            scenegraph.cpp
            scenegraphupdates.cpp
            textproperties.cpp
            utilities.cpp
            )
else()
    target_sources_local(apl
            PRIVATE
            edittextconfig.cpp
            textproperties.cpp
            utilities.cpp
            )
endif(ENABLE_SCENEGRAPH)

