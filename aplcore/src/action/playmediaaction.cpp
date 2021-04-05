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

#include "apl/action/playmediaaction.h"
#include "apl/command/corecommand.h"
#include "apl/time/sequencer.h"
#include "apl/utils/session.h"

namespace apl {

PlayMediaAction::PlayMediaAction(const TimersPtr& timers,
                                 const std::shared_ptr<CoreCommand>& command,
                                 const ComponentPtr& target)
        : ResourceHoldingAction(timers, command->context()),
          mCommand(command),
          mTarget(target)
{}

std::shared_ptr<PlayMediaAction>
PlayMediaAction::make(const TimersPtr& timers,
                         const std::shared_ptr<CoreCommand>& command)
{
    auto ptr = std::make_shared<PlayMediaAction>(timers, command, command->target());

    auto audioTrack = command->getValue(kCommandPropertyAudioTrack);
    if (audioTrack == kCommandAudioTrackForeground) {
        command->context()->sequencer().claimResource(kExecutionResourceForegroundAudio, ptr);
    } else if (audioTrack == kCommandAudioTrackBackground) {
        command->context()->sequencer().claimResource(kExecutionResourceBackgroundAudio, ptr);
    }

    ptr->start();
    return ptr;
}

void
PlayMediaAction::start()
{
    auto audioTrack = mCommand->getValue(kCommandPropertyAudioTrack);
    auto source = mCommand->getValue(kCommandPropertySource);

    EventBag bag;
    bag.emplace(kEventPropertyAudioTrack, audioTrack);
    bag.emplace(kEventPropertySource, source);

    // An ActionRef is always required.  The viewhost should resolve it immediately if it is background
    // audio and should resolve it after playing if it is foreground audio.
    mCommand->context()->pushEvent(Event(kEventTypePlayMedia, std::move(bag), mTarget, shared_from_this()));
}

} // namespace apl