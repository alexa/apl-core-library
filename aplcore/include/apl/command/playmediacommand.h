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

#ifndef _APL_PLAY_MEDIA_COMMAND_H
#define _APL_PLAY_MEDIA_COMMAND_H

#include "apl/command/corecommand.h"

namespace apl {

class PlayMediaCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base) {
        auto ptr = std::make_shared<PlayMediaCommand>(context, std::move(properties), base);
        return ptr->validate() ? ptr : nullptr;
    }

    PlayMediaCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base)
            : CoreCommand(context, std::move(properties), base)
    {}

    const CommandPropDefSet& propDefSet() const override {
        static CommandPropDefSet sPlayMediaCommandProperties(CoreCommand::propDefSet(), {
                {kCommandPropertyAudioTrack,  kCommandAudioTrackForeground, sCommandAudioTrackMap },
                {kCommandPropertyComponentId, "",                           asString,               kPropRequiredId},
                {kCommandPropertySource,      Object::EMPTY_ARRAY(),        asMediaSourceArray,     kPropRequired}
        });

        return sPlayMediaCommandProperties;
    };

    CommandType type() const override { return kCommandTypePlayMedia; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override {
        if (fastMode) {
            CONSOLE_CTP(mContext) << "Ignoring PlayMedia command in fast mode";
            return nullptr;
        }

        if (!calculateProperties())
            return nullptr;

        return Action::make(timers, [this](ActionRef ref) {
            auto audioTrack = getValue(kCommandPropertyAudioTrack);

            EventBag bag;
            bag.emplace(kEventPropertyAudioTrack, audioTrack);
            bag.emplace(kEventPropertySource, getValue(kCommandPropertySource));

            // An ActionRef is always required.  The viewhost should resolve it immediately if it is background
            // audio and should resolve it after playing if it is foreground audio.
            Event event(kEventTypePlayMedia,
                        std::move(bag),
                        mTarget,
                        ref);

            mContext->pushEvent(std::move(event));
        });
    }
};

} // namespace apl

#endif // _APL_PLAY_MEDIA_COMMAND_H
