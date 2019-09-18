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
#include "apl/component/scrollablecomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/time/sequencer.h"

namespace apl {

void
ScrollableComponent::update(UpdateType type, float value)
{
    if (type == kUpdateScrollPosition) {
        if (value != mCurrentPosition) {
            mCurrentPosition = value;
            ContextPtr eventContext = createEventContext("Scroll", getValue());
            mContext->sequencer().executeCommands(
                getCalculated(kPropertyOnScroll),
                eventContext,
                shared_from_this(),
                true);
        }
    }
    else
        CoreComponent::update(type, value);
}

std::shared_ptr<ObjectMap>
ScrollableComponent::getEventTargetProperties() const
{
    auto target = CoreComponent::getEventTargetProperties();
    target->emplace("position", getValue());
    return target;
}

bool
ScrollableComponent::getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) {
    bool actionable = CoreComponent::getTags(outMap, allocator);

    std::string direction = scrollType() == kScrollTypeHorizontal ? "horizontal" : "vertical";
    bool allowFwd = allowForward();
    bool allowBackwards = (mCurrentPosition > 0);

    if(mChildren.size() > 0 && (allowBackwards || allowFwd)) {
        rapidjson::Value scrollable(rapidjson::kObjectType);
        scrollable.AddMember("direction", rapidjson::Value(direction.c_str(), allocator).Move(), allocator);
        scrollable.AddMember("allowForward", allowFwd, allocator);
        scrollable.AddMember("allowBackwards", allowBackwards, allocator);
        outMap.AddMember("scrollable", scrollable, allocator);

        actionable = true;
    }

    return actionable;
}


} // namespace apl