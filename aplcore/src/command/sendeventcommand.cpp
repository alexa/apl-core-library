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

#include "apl/command/sendeventcommand.h"
#include "apl/utils/log.h"
#include "apl/utils/dump_object.h"
#include "apl/utils/session.h"

namespace apl {

const static bool DEBUG_SEND_EVENT = false;

const CommandPropDefSet&
SendEventCommand::propDefSet() const {
    static CommandPropDefSet sSendEventCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyArguments,  Object::EMPTY_ARRAY(), asOldArray},
            {kCommandPropertyComponents, Object::EMPTY_ARRAY(), asArray}
    });

    return sSendEventCommandProperties;
}

ActionPtr
SendEventCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (fastMode) {
        CONSOLE_CTP(mContext) << "Ignoring SendEvent command in fast mode";
        return nullptr;
    }

    if (!calculateProperties())
        return nullptr;

    // Calculate the component map
    auto componentsMap = std::make_shared<ObjectMap>();
    for (auto& compId : mValues.at(kCommandPropertyComponents).getArray()) {
        auto comp = std::dynamic_pointer_cast<CoreComponent>(mContext->findComponentById(compId.getString()));
        if (comp) {
            componentsMap->emplace(compId.getString(), comp->getValue());
        }
    }

    // Freeze the "event.source" property as a JSON object
    mSource = mContext->opt("event").get("source").serialize(mDocument.GetAllocator());
    rapidjson::Document sourceDoc;
    sourceDoc.CopyFrom(mSource, sourceDoc.GetAllocator());

    // Freeze the arguments array as a JSON object
    mArguments = mValues.at(kCommandPropertyArguments).serialize(mDocument.GetAllocator());
    rapidjson::Document argumentsDoc;
    argumentsDoc.CopyFrom(mArguments, argumentsDoc.GetAllocator());

    EventBag bag;
    bag.emplace(kEventPropertySource, Object(std::move(sourceDoc)));
    bag.emplace(kEventPropertyArguments, Object(std::move(argumentsDoc)));
    bag.emplace(kEventPropertyComponents, componentsMap);

    if (DEBUG_SEND_EVENT) {
        LOG(LogLevel::kDebug) << "SendEvent Bag";
        for (auto m : bag) {
            LOG(LogLevel::kDebug) << "Property: " << sEventPropertyBimap.at(m.first) << "("
                                 << m.first << ")";
            DumpVisitor::dump(m.second);
        }
    }

    mContext->pushEvent(Event(kEventTypeSendEvent, std::move(bag)));

    return nullptr;
}

} // namespace apl
