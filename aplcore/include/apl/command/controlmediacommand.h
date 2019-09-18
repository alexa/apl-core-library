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

#ifndef _APL_CONTROL_MEDIA_COMMAND_H
#define _APL_CONTROL_MEDIA_COMMAND_H

#include "apl/command/corecommand.h"
#include "apl/component/videocomponent.h"
#include "apl/utils/session.h"

namespace apl {

class ControlMediaCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base) {
        auto ptr = std::make_shared<ControlMediaCommand>(context, std::move(properties), base);
        return ptr->validate() ? ptr : nullptr;
    }

    ControlMediaCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base)
            : CoreCommand(context, std::move(properties), base)
    {}

    const CommandPropDefSet& propDefSet() const override {
        static CommandPropDefSet sControlMediaCommandProperties(CoreCommand::propDefSet(), {
                {kCommandPropertyCommand,     kCommandControlMediaPlay, sControlMediaMap,   kPropRequired},
                {kCommandPropertyComponentId, "",                       asString,           kPropRequiredId},
                {kCommandPropertyValue,       0,                        asInteger }
        });

        return sControlMediaCommandProperties;
    };

    CommandType type() const override { return kCommandTypeControlMedia; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override {
        if (!calculateProperties())
            return nullptr;

        // All commands except "Play" are allowed in fast mode
        auto command = getValue(kCommandPropertyCommand);
        if (fastMode && command.getInteger() == kCommandControlMediaPlay) {
            CONSOLE_CTP(mContext) << "Ignoring ControlMedia.play in fast mode";
            return nullptr;
        }

        if (kComponentTypeVideo != target()->getType()) {
            CONSOLE_CTP(mContext) << "ControlMedia command targeting non-Video component";
            // TODO: Check if we actually sanitize commands for target component type.
            return nullptr;
        }

        const auto value = getValue(kCommandPropertyValue);
        if (kCommandControlMediaSetTrack == command.getInteger()) {
            auto targetVideo = std::static_pointer_cast<VideoComponent>(target());
            const auto& mediaState = targetVideo->getMediaState();
            const auto& mediaSource = targetVideo->getCalculated(kPropertySource);
            int maxIndex = mediaSource.isArray() ? mediaSource.size() - 1 : 0;

            if (value.asInt() > maxIndex) {
                CONSOLE_CTP(mContext) << "ControlMedia track index out of bounds";
                return nullptr;
            }
        }

        return Action::make(timers, [this, &command, value](ActionRef ref) {
            EventBag bag;
            bag.emplace(kEventPropertyCommand, command);
            bag.emplace(kEventPropertyValue, value);
            mContext->pushEvent(Event(kEventTypeControlMedia, std::move(bag), mTarget, ref));
        });
    }
};

} // namespace apl

#endif // _APL_CONTROL_MEDIA_COMMAND_H
