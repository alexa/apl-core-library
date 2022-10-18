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
#include "apl/content/rootconfig.h"
#include "apl/utils/log.h"
#include "apl/utils/dump_object.h"
#include "apl/utils/session.h"

namespace apl {

const static bool DEBUG_SEND_EVENT = false;

CommandPtr
SendEventCommand::create(const ContextPtr& context,
                         Properties&& properties,
                         const CoreComponentPtr& base,
                         const std::string& parentSequencer) {
    auto ptr = std::make_shared<SendEventCommand>(context, std::move(properties), base, parentSequencer);
    return ptr->validate() ? ptr : nullptr;
}

SendEventCommand::SendEventCommand(const ContextPtr& context,
                                   Properties&& properties,
                                   const CoreComponentPtr& base,
                                   const std::string& parentSequencer)
    : CoreCommand(context, std::move(properties), base, parentSequencer)
{}

const CommandPropDefSet&
SendEventCommand::propDefSet() const {
    static CommandPropDefSet sSendEventCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyArguments,  Object::EMPTY_ARRAY(), asOldArray},
            {kCommandPropertyComponents, Object::EMPTY_ARRAY(), asArray},
            {kCommandPropertyFlags,      Object::EMPTY_MAP(),   asAny}
    });

    return sSendEventCommandProperties;
}

ActionPtr
SendEventCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (fastMode) {
        CONSOLE(mContext) << "Ignoring SendEvent command in fast mode";
        return nullptr;
    }

    if (!mContext || !calculateProperties())
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
    auto event = mContext->opt("event");
    if (event.empty()) {
        LOG(LogLevel::kError)
            << "Event field not available in context. Should not happen during normal operation.";
        return nullptr;
    }
    mSource = event.get("source").serialize(mDocument.GetAllocator());
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

    auto flags = std::make_shared<ObjectMap>();
    auto definedFlags = mValues.at(kCommandPropertyFlags);
    if (!definedFlags.empty() && definedFlags.isMap()) {
        flags->insert(definedFlags.getMap().begin(), definedFlags.getMap().end());
    }

    auto defaultFlags = context()->getRootConfig().getProperty(RootProperty::kSendEventAdditionalFlags);
    if (!defaultFlags.empty() && defaultFlags.isMap()) {
        flags->insert(defaultFlags.getMap().begin(), defaultFlags.getMap().end());
    }

    if (!flags->empty()) {
        bag.emplace(kEventPropertyFlags, flags);
    }

    if (DEBUG_SEND_EVENT) {
        LOG(LogLevel::kDebug).session(mContext) << "SendEvent Bag";
        for (auto& m : bag) {
            LOG(LogLevel::kDebug).session(mContext) << "Property: " << sEventPropertyBimap.at(m.first) << "("
                                 << m.first << ")";
            DumpVisitor::dump(m.second);
        }
    }

    mContext->pushEvent(Event(kEventTypeSendEvent, std::move(bag)));

    return nullptr;
}

} // namespace apl
