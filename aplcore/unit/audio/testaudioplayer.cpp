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

#include "testaudioplayer.h"
#include "testaudioplayerfactory.h"

#include "apl/utils/log.h"

namespace apl {

const bool DEBUG_TEST_AUDIO_PLAYER = false;

const char *sStateString[] = {
    "INIT",
    "PREROLL",
    "PREPLAY",
    "READY",
    "PLAY",
    "DONE",
    "FAIL"
};

std::string
TestAudioPlayer::toString(EventType eventType) 
{
    switch (eventType) {
        case kPreroll: return "preroll";
        case kReady: return "ready";
        case kPlay: return "play";
        case kPause: return "pause";
        case kDone: return "done";
        case kFail: return "fail";
        case kRelease: return "release";
    }

    return "";
}

TestAudioPlayer::TestAudioPlayer(AudioPlayerCallback playerCallback,
                                 SpeechMarkCallback speechMarkCallback,
                                 std::shared_ptr<TestAudioPlayerFactory> factory)
    : AudioPlayer(std::move(playerCallback), std::move(speechMarkCallback)),
      mFactory(std::move(factory)),
      mActionRef(nullptr),
      mTimeoutId(0)
{}

void
TestAudioPlayer::release()
{
    if (mReleased)
        return;

    mFactory->record(*this, mURL, kRelease);
    mReleased = true;
    mFactory = nullptr;
    if (!mActionRef.empty())
        mActionRef.resolve();
}


bool TextTracksEqual(TextTrackArray track1, TextTrackArray track2){

    if (track1.size() != track2.size()) {
        return false;
    }

    for (int i = 0; i < track1.size(); i++) {
        // Two TextTracks are the same if they have the same source url, type and size
        if (track1.at(i).type != track2.at(i).type || track1.at(i).url != track2.at(i).url) {
            return false;
        }
    }

    return true;
}

void
TestAudioPlayer::setTrack(MediaTrack track)
{
    if (mReleased)
        return;

    LOG_IF(DEBUG_TEST_AUDIO_PLAYER) << "track.url=" << track.url;
    pause();

    if (track.url.empty())
        return;

    auto content = mFactory->findContent(track.url);

    assert(TextTracksEqual(track.textTracks, content.trackArray));
    assert(content.actualDuration > 0);

    mActualDuration = content.actualDuration;
    mInitialDelay = content.initialDelay;
    mFailAfter = content.failAfter;
    mURL = content.url;
    mState = PREROLL;

    mFactory->record(*this, mURL, kPreroll);

    // Set up a timer for preroll
    auto weak = std::weak_ptr<TestAudioPlayer>(shared_from_this());
    mTimeoutId = mFactory->timers()->setTimeout([weak]() {
        auto self = weak.lock();
        if (self)
            self->prerollFinished();
    }, mInitialDelay);

    // Publish any speech mark data
    if (mSpeechMarkCallback && !content.speechMarks.empty())
        mSpeechMarkCallback(content.speechMarks);
}

void
TestAudioPlayer::play(ActionRef actionRef)
{
    if (mReleased || !(mState == PREROLL || mState == READY)) {
        if (!actionRef.empty())
            actionRef.resolve();
        return;
    }

    LOG_IF(DEBUG_TEST_AUDIO_PLAYER) << mURL << " state=" << sStateString[mState];

    mActionRef = actionRef;
    auto weak = std::weak_ptr<TestAudioPlayer>(shared_from_this());
    mActionRef.addTerminateCallback([weak](const TimersPtr&) {
        auto self = weak.lock();
        if (self)
            self->terminate();
    });

    if (mState == PREROLL)
        mState = PREPLAY;
    else  // mState == READY
        startPlayback();
}

void
TestAudioPlayer::pause()
{
    if (mReleased)
        return;

    LOG_IF(DEBUG_TEST_AUDIO_PLAYER) << mURL << " pausing state=" << sStateString[mState];

    if (mState == PREPLAY)
        mState = PREROLL;
    else if (mState == PLAY) {
        mActionRef.resolve();
        pausePlayback();
    }
}

void 
TestAudioPlayer::prerollFinished() 
{
    if (mReleased)
        return;

    LOG_IF(DEBUG_TEST_AUDIO_PLAYER) << mURL << " fail_after=" << mFailAfter << " duration=" << mActualDuration;

    mTimeoutId = 0;
    
    // It's possible to fail immediately after the preroll without recording READY
    if (mFailAfter == 0) {
        mState = FAIL;
        mFactory->record(*this, mURL, kFail);
        if (!mActionRef.empty())
            mActionRef.resolve();
        return;
    }

    mFactory->record(*this, mURL, kReady);
    doCallback(kAudioPlayerEventReady);

    if (mState == PREROLL)
        mState = READY;
    else if (mState == PREPLAY)
        startPlayback();
}

void
TestAudioPlayer::startPlayback()
{
    assert(!mReleased);

    mState = PLAY;
    mFactory->record(*this, mURL, kPlay);

    // Calculate how far we can advance
    auto advance = mActualDuration;
    if (mFailAfter >= 0)
        advance = std::min(advance, mFailAfter);

    LOG_IF(DEBUG_TEST_AUDIO_PLAYER) << mURL << " advance=" << advance;

    auto weak = std::weak_ptr<TestAudioPlayer>(shared_from_this());
    mTimeoutId = mFactory->timers()->setAnimator([weak](apl_duration_t duration) {
        auto self = weak.lock();
        if (self)
            self->animate(duration);
    }, advance);

    doCallback(kAudioPlayerEventPlay);
}

void 
TestAudioPlayer::animate(apl_duration_t duration)
{
    if (mReleased)
        return;

    mPlayheadPosition = duration + mPlayheadStart;

    LOG_IF(DEBUG_TEST_AUDIO_PLAYER) << mURL
                                    << " duration=" << duration
                                    << " actual_duration=" << mActualDuration
                                    << " fail_after=" << mFailAfter;

    // Check for a fail or done condition
    if (mFailAfter == duration) {
        mState = FAIL;
        mFactory->record(*this, mURL, kFail);
        mActionRef.resolve();
        mFactory->timers()->clearTimeout(mTimeoutId);
        mTimeoutId = 0;
        doCallback(kAudioPlayerEventFail);
    }
    else if (mActualDuration == duration) {
        mState = DONE;
        mFactory->record(*this, mURL, kDone);
        mActionRef.resolve();
        mFactory->timers()->clearTimeout(mTimeoutId);
        mTimeoutId = 0;
        doCallback(kAudioPlayerEventEnd);
    }
    else {
        doCallback(kAudioPlayerEventTimeUpdate);
    }
}

void
TestAudioPlayer::terminate() 
{
    if (mReleased)
        return;

    LOG_IF(DEBUG_TEST_AUDIO_PLAYER) << mURL << " state=" << sStateString[mState];

    // Terminate was called on this action ref
    mActionRef = ActionRef(nullptr);

    // I believe this is true - the mActionRef was set due to a PLAY command
    assert(mState == PLAY || mState == PREPLAY);

    if (mState == PLAY)
        pausePlayback();
    else if (mState == PREPLAY)
        mState = PREROLL;
}

void
TestAudioPlayer::pausePlayback()
{
    mState = READY;
    mFactory->record(*this, mURL, kPause);
    mFactory->timers()->clearTimeout(mTimeoutId);
    mTimeoutId = 0;

    // If we terminate playback, we stash the current playhead position for the next playback
    mPlayheadStart = mPlayheadPosition;
    doCallback(kAudioPlayerEventPause);
}

TrackState
TestAudioPlayer::getTrackState() const
{
    switch (mState) {
        case INIT:
        case PREROLL:
        case PREPLAY:
            return kTrackNotReady;
        case READY:
        case PLAY:
        case DONE:
            return kTrackReady;
        case FAIL:
        default:
            return kTrackFailed;
    }
}

void
TestAudioPlayer::doCallback(AudioPlayerEventType eventType)
{
    if (!mPlayerCallback)
        return;

    mPlayerCallback(eventType,
                    AudioState(mPlayheadPosition,                   // Current time
                               -1,                                  // Current track duration
                               mState != PLAY && mState != PREPLAY, // Paused
                               mState == DONE || mState == FAIL,    // Ended
                               getTrackState()));
}

} // namespace apl
