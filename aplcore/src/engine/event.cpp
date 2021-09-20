/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <cassert>

#include "apl/engine/event.h"
#include "apl/command/command.h"
#include "apl/command/corecommand.h"

namespace apl {

Bimap<int, std::string> sEventTypeBimap = {
    {kEventTypeControlMedia,           "controlMedia"},
    {kEventTypeDataSourceFetchRequest, "dataSourceFetchRequest"},
    {kEventTypeExtension,              "extension"},
    {kEventTypeFocus,                  "focus"},
    {kEventTypeFinish,                 "finish"},
    {kEventTypeMediaRequest,           "mediaRequest"},
    {kEventTypeOpenURL,                "openURL"},
    {kEventTypePlayMedia,              "playMedia"},
    {kEventTypePreroll,                "preroll"},
    {kEventTypeReinflate,              "reinflate"},
    {kEventTypeRequestFirstLineBounds, "requestFirstLineBounds"},
    {kEventTypeSendEvent,              "sendEvent"},
    {kEventTypeSpeak,                  "speak"}
};

Bimap<int, std::string> sEventPropertyBimap = {
    {kEventPropertyAlign,                    "align"},
    {kEventPropertyArguments,                "arguments"},
    {kEventPropertyAudioTrack,               "audioTrack"},
    {kEventPropertyCommand,                  "command"},
    {kEventPropertyComponent,                "component"},
    {kEventPropertyComponents,               "components"},
    {kEventPropertyDirection,                "direction"},
    {kEventPropertyExtension,                "extension"},
    {kEventPropertyExtensionURI,             "extensionURI"},
    {kEventPropertyExtensionResourceId,      "resourceId"},
    {kEventPropertyFlags,                    "flags"},
    {kEventPropertyHighlightMode,            "highlightMode"},
    {kEventPropertyMediaType,                "mediaType"},
    {kEventPropertyName,                     "name"},
    {kEventPropertyPosition,                 "position"},
    {kEventPropertyReason,                   "reason"},
    {kEventPropertySource,                   "source"},
    {kEventPropertyValue,                    "value"}
};

class EventData {
public:
    EventData(EventType eventType, EventBag&& bag, const ComponentPtr& component, ActionRef actionRef)
        : eventType(eventType),
          bag(std::move(bag)),
          component(component),
          actionRef(actionRef)
    {}

public:
    const EventType    eventType;
    const EventBag     bag;
    const ComponentPtr component;
    const ActionRef    actionRef;
};

Event::Event(EventType eventType, EventBag&& bag)
    : Event(eventType, std::move(bag), nullptr, ActionRef(nullptr))
{
}

Event::Event(EventType eventType, const ComponentPtr& component)
    : Event(eventType, EventBag(), component, ActionRef(nullptr))
{
}

Event::Event(EventType eventType, EventBag&& bag, const ComponentPtr& component)
    : Event(eventType, std::move(bag), component, ActionRef(nullptr))
{
}

Event::Event(EventType eventType, EventBag&& bag, const ComponentPtr& component, ActionRef actionRef)
    : mData(std::make_shared<EventData>(eventType, std::move(bag), component, actionRef))
{
}

Event::Event(EventType eventType, const ComponentPtr& component, ActionRef actionRef)
    : mData(std::make_shared<EventData>(eventType, EventBag(), component, actionRef))
{
}

EventType
Event::getType() const
{
    return mData->eventType;
}

Object
Event::getValue(EventProperty key) const
{
    auto it = mData->bag.find(key);
    if (it != mData->bag.end())
        return mData->bag.at(key);
    return Object::NULL_OBJECT();
}

ComponentPtr
Event::getComponent() const
{
    return mData->component;
}

ActionRef
Event::getActionRef() const
{
    return mData->actionRef;
}

rapidjson::Value
Event::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value event(rapidjson::kObjectType);

    event.AddMember("type", mData->eventType, allocator);

    if (mData->component != nullptr)
        event.AddMember("id", rapidjson::Value(mData->component->getUniqueId().c_str(), allocator), allocator);

    for (auto& m : mData->bag) {
        if (!sEventPropertyBimap.has(m.first)) {
            LOG(LogLevel::kError) << "Unknown property enum: " << m.first;
            continue;
        }
        const std::string& prop = sEventPropertyBimap.at(m.first);
        auto value = m.second.serialize(allocator);
        event.AddMember(rapidjson::StringRef(prop.c_str()), value, allocator);
    }

    return event;
}

bool
Event::matches(const Event& rhs) const
{
    return mData->eventType == rhs.mData->eventType &&
            mData->component == rhs.mData->component &&
            mData->bag == rhs.mData->bag;
}


} // namespace apl
