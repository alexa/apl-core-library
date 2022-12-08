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
#include "apl/component/videocomponent.h"
#include "apl/media/mediaplayer.h"
#include "apl/media/mediautils.h"
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

    auto videoComponent = VideoComponent::cast(mTarget);
    assert(videoComponent);

    mPlayer = videoComponent->getMediaPlayer();
    if (mPlayer) {
        // Update the video component to list the new sources and audio track
        // TODO: In the future I'd like to remove these from the video component and only store them in the player
        videoComponent->setCalculated(kPropertySource, source);
        videoComponent->setCalculated(kPropertyAudioTrack, audioTrack);

        // Update the media player
        mPlayer->setTrackList(mediaSourcesToTracks(source));
        mPlayer->setAudioTrack(static_cast<AudioTrack>(audioTrack.getInteger()));
        mPlayer->play(shared_from_this());
        // An early termination of the command (for example, by the user touching on the screen)
        // will only stop the video playing if the audioTrack is set to foreground
        if (audioTrack == kAudioTrackForeground) {
            addTerminateCallback([this](const TimersPtr &) {
                mPlayer->pause();
            });
        }
    }
    else {
        EventBag bag;
        bag.emplace(kEventPropertyAudioTrack, audioTrack);
        bag.emplace(kEventPropertySource, source);

        // An ActionRef is always required.  The viewhost should resolve it immediately if it is
        // background audio and should resolve it after playing if it is foreground audio.
        mCommand->context()->pushEvent(
            Event(kEventTypePlayMedia, std::move(bag), mTarget, shared_from_this()));
    }
}

void
PlayMediaAction::freeze()
{
    auto videoComponent = VideoComponent::cast(mTarget);
    assert(videoComponent);

    mPlayingState = videoComponent->getProperty(kPropertyPlayingState);
    mSource = videoComponent->getProperty(kPropertySource);

    videoComponent->detachPlayer();

    if (mCommand) {
        mCommand->freeze();
    }

    ResourceHoldingAction::freeze();
}

bool
PlayMediaAction::rehydrate(const RootContext& context)
{
    if (!ResourceHoldingAction::rehydrate(context)) return false;

    if (mCommand) {
        if (!mCommand->rehydrate(context)) {
            mPlayer->release();
            mPlayer = nullptr;
            return false;
        }
    }

    mTarget = mCommand->target();

    // Ensure that source AND playbackState preserved. If not - we recreate the player (as per spec).
    auto videoComponent = VideoComponent::cast(mTarget);
    if (!videoComponent) return false;
    if (mPlayingState != videoComponent->getProperty(kPropertyPlayingState) ||
        mSource != videoComponent->getProperty(kPropertySource)) {
        mPlayer->release();
        mPlayer = nullptr;
        CONSOLE(mContext->session()) << R"(Can't preserve PlayMedia command without "source" and
            "playingState" preservation on component level.)";
        return false;
    }

    mPlayingState = Object::NULL_OBJECT();
    mSource = Object::NULL_OBJECT();

    videoComponent->attachPlayer(mPlayer);

    auto audioTrack = mCommand->getValue(kCommandPropertyAudioTrack);
    if (audioTrack == kCommandAudioTrackForeground) {
        mCommand->context()->sequencer().claimResource(kExecutionResourceForegroundAudio, shared_from_this());
    } else if (audioTrack == kCommandAudioTrackBackground) {
        mCommand->context()->sequencer().claimResource(kExecutionResourceBackgroundAudio, shared_from_this());
    }
    return true;
}

} // namespace apl