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

#include "apl/action/controlmediaaction.h"
#include "apl/command/corecommand.h"
#include "apl/time/sequencer.h"
#include "apl/utils/session.h"

namespace apl {

ControlMediaAction::ControlMediaAction(const TimersPtr& timers,
                                       const std::shared_ptr<CoreCommand>& command,
                                       const ComponentPtr& target)
        : ResourceHoldingAction(timers, command->context()),
          mCommand(command),
          mTarget(target)
{}

std::shared_ptr<ControlMediaAction>
ControlMediaAction::make(const TimersPtr& timers,
                         const std::shared_ptr<CoreCommand>& command)
{
    if (kComponentTypeVideo != command->target()->getType()) {
        CONSOLE_CTP(command->context()) << "ControlMedia targeting non-Video component";
        // TODO: Check if we actually sanitize commands for target component type.
        return nullptr;
    }

    auto mediaCommand = command->getValue(kCommandPropertyCommand);
    auto value = command->getValue(kCommandPropertyValue);

    if (kCommandControlMediaSetTrack == mediaCommand.getInteger()) {
        const auto& mediaSource = command->target()->getCalculated(kPropertySource);
        int maxIndex = mediaSource.isArray() ? mediaSource.size() - 1 : 0;

        if (value.asInt() > maxIndex) {
            CONSOLE_CTP(command->context()) << "ControlMedia track index out of bounds";
            return nullptr;
        }
    }

    auto ptr = std::make_shared<ControlMediaAction>(timers, command, command->target());

    auto audioTrack = command->target()->getCalculated(kPropertyAudioTrack);
    if (audioTrack == kCommandAudioTrackForeground) {
        command->context()->sequencer().claimResource(kExecutionResourceForegroundAudio, ptr);
    } else if (audioTrack == kCommandAudioTrackBackground) {
        command->context()->sequencer().claimResource(kExecutionResourceBackgroundAudio, ptr);
    }

    ptr->start();
    return ptr;
}

void
ControlMediaAction::start()
{
    auto mediaCommand = static_cast<CommandControlMedia>(mCommand->getValue(kCommandPropertyCommand).asInt());
    auto value = mCommand->getValue(kCommandPropertyValue);

    EventBag bag;
    bag.emplace(kEventPropertyCommand, mediaCommand);
    bag.emplace(kEventPropertyValue, value);
    mCommand->context()->pushEvent(Event(kEventTypeControlMedia, std::move(bag), mTarget, shared_from_this()));
}

} // namespace apl