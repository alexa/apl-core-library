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

#ifndef _APL_SEND_EVENT_COMMAND_H
#define _APL_SEND_EVENT_COMMAND_H

#include "apl/command/corecommand.h"
#include "apl/utils/log.h"
#include "apl/utils/dump_object.h"

namespace apl {

const static bool DEBUG_SEND_EVENT = false;

class SendEventCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base) {
        auto ptr = std::make_shared<SendEventCommand>(context, std::move(properties), base);
        return ptr->validate() ? ptr : nullptr;
    }

    SendEventCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base)
            : CoreCommand(context, std::move(properties), base)
    {}

    const CommandPropDefSet& propDefSet() const override {
        static CommandPropDefSet sSendEventCommandProperties(CoreCommand::propDefSet(), {
                {kCommandPropertyArguments,  Object::EMPTY_ARRAY(), asOldArray},
                {kCommandPropertyComponents, Object::EMPTY_ARRAY(), asArray}
        });

        return sSendEventCommandProperties;
    };

    CommandType type() const override { return kCommandTypeSendEvent; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override {
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

        EventBag bag;
        bag.emplace(kEventPropertySource, mContext->opt("event").get("source"));
        bag.emplace(kEventPropertyArguments, mValues.at(kCommandPropertyArguments));
        bag.emplace(kEventPropertyComponents, componentsMap);

        if (DEBUG_SEND_EVENT) {
            LOG(LogLevel::DEBUG) << "SendEvent Bag";
            for (auto m : bag) {
                LOG(LogLevel::DEBUG) << "Property: " << sEventPropertyBimap.at(m.first) << "("
                                     << m.first << ")";
                DumpVisitor::dump(m.second);
            }
        }

        mContext->pushEvent(Event(kEventTypeSendEvent, std::move(bag)));

        return nullptr;
    }
};

} // namespace apl

#endif // _APL_SEND_EVENT_COMMAND_H
