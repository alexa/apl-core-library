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

target_sources_local(apl
    PRIVATE
    arrayify.cpp
    binding.cpp
    bindingchange.cpp
    builder.cpp
    context.cpp
    contextobject.cpp
    corerootcontext.cpp
    dependant.cpp
    dependantmanager.cpp
    evaluate.cpp
    event.cpp
    hovermanager.cpp
    info.cpp
    keyboardmanager.cpp
    layoutmanager.cpp
    parameterarray.cpp
    propdef.cpp
    properties.cpp
    resources.cpp
    rootcontext.cpp
    sharedcontextdata.cpp
    state.cpp
    styledefinition.cpp
    styleinstance.cpp
    styles.cpp
    tickscheduler.cpp
    uidgenerator.cpp
    uidmanager.cpp
    uidobject.cpp
    visibilitymanager.cpp
)
