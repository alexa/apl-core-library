/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "apl/component/componentpropdef.h"
#include "apl/component/touchwrappercomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/time/sequencer.h"
#include "apl/primitives/keyboard.h"

namespace apl {

CoreComponentPtr
TouchWrapperComponent::create(const ContextPtr& context,
                              Properties&& properties,
                              const std::string& path) {
    auto ptr = std::make_shared<TouchWrapperComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

TouchWrapperComponent::TouchWrapperComponent(const ContextPtr& context,
                                             Properties&& properties,
                                             const std::string& path)
        : ActionableComponent(context, std::move(properties), path) {
}

const ComponentPropDefSet&
TouchWrapperComponent::propDefSet() const {
    static ComponentPropDefSet sTouchWrapperComponentProperties(ActionableComponent::propDefSet(), {
            {kPropertyOnPress, Object::EMPTY_ARRAY(), asCommand, kPropIn}
    });

    return sTouchWrapperComponentProperties;
}

void
TouchWrapperComponent::update(UpdateType type, float value)
{
    if (type == kUpdatePressed) {
        if (!mState.get(kStateDisabled)) {
            // Force the sequencer to stop.  This is needed because the onPress may not fire anything
            mContext->sequencer().reset();

            ContextPtr eventContext = createEventContext("Press", mState.get(kStateChecked));
            auto cal = getCalculated(kPropertyOnPress);
            mContext->sequencer().executeCommands(
                    cal,
                    eventContext,
                    shared_from_this(),
                    false);
        }
    } else
        CoreComponent::update(type, value);
}

bool
TouchWrapperComponent::getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) {
    CoreComponent::getTags(outMap, allocator);
    outMap.AddMember("clickable", true, allocator);
    return true;
}


}  // namespace apl