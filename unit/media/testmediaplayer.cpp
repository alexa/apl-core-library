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

#include "testmediaplayer.h"
#include "testmediaplayerfactory.h"

namespace apl {

const bool DEBUG_MP = false;

const Bimap<TestMediaPlayer::EventType, std::string> TestMediaPlayer::sEventTypeMap = {
    {TestMediaPlayer::EventType::kPlayerEventSetTrackList, "setTrackList"},
    {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, "setAudioTrack"},
    {TestMediaPlayer::EventType::kPlayerEventPlay, "play"}
};

TestMediaPlayer::TestMediaPlayer(MediaPlayerCallback mediaPlayerCallback,
                                 std::shared_ptr<TestMediaPlayerFactory> factory)
    : MediaPlayer(std::move(mediaPlayerCallback)),
      mMediaPlayerFactory(std::move(factory)),
      mActionRef(nullptr)
{}

void
TestMediaPlayer::release()
{
    mCallback = nullptr;
    mMediaPlayerFactory = nullptr;
    resolveExistingAction();
    mReleased = true;
}

void
TestMediaPlayer::halt()
{
    if (mReleased || !mPlayer)
        return;

    resolveExistingAction();
    mPlayer->pause();
}

/**
 * PlayMedia or SetValue
 * Can be called from normal or fast mode
 * A "play" callback is needed to start playing
 */
void
TestMediaPlayer::setTrackList(std::vector<MediaTrack> vector)
{
    if (mReleased)
        return;

    publishEvent(EventType::kPlayerEventSetTrackList);

    LOG_IF(DEBUG_MP) << "size=" << vector.size();

    resolveExistingAction();

    if (mPlayer)
        mPlayer->pause();

    mMediaTracks = std::move(vector);
    mTrackIndex = 0;
    createMediaPlayer();
}

/**
 * PlayMedia or ControlMedia.play
 * Can only be called from normal mode
 */
void
TestMediaPlayer::play(ActionRef actionRef)
{
    publishEvent(EventType::kPlayerEventPlay);

    if (mReleased || !mPlayer) {
        if (!actionRef.empty())
            actionRef.resolve();
        return;
    }

    LOG_IF(DEBUG_MP) << "actionRef=" << (actionRef.empty() ? "empty" : "active")
                     << " autoTrack=" << sAudioTrackMap.at(mAudioTrack)
                     << " " << mPlayer->toDebugString();

    resolveExistingAction();  // Resolve and clear the old action ref

    if (!actionRef.empty()) {
        // Only hold onto the ActionRef in foreground mode
        if (mAudioTrack == kAudioTrackForeground) {
            mActionRef = actionRef;

            // On a termination we need to discard the action reference or there is a memory cycle
            mActionRef.addTerminateCallback(
                [&](const TimersPtr&) { mActionRef = ActionRef(nullptr); });
        }
        else {
            actionRef.resolve();
        }
    }

    if (mPlayer->play())
        doCallback(kMediaPlayerEventPlay);   // TODO: Should this be fast mode????
}

/**
 * ControlMedia.pause
 * Can be called from normal or fast mode
 */
void
TestMediaPlayer::pause()
{
    if (mReleased || !mPlayer)
        return;

    LOG_IF(DEBUG_MP) << mPlayer->toDebugString();

    resolveExistingAction();

    if (mPlayer->pause())
        doCallback(kMediaPlayerEventPause);
}

/**
 * ControlMedia.next
 * Can be called from normal or fast mode
 */
void
TestMediaPlayer::next()
{
    if (mReleased || !mPlayer)
        return;

    LOG_IF(DEBUG_MP) << mPlayer->toDebugString();

    resolveExistingAction();

    // Pause any current playback.  This does not generate a PAUSE event
    mPlayer->pause();

    // Advance to the next track.  This may fail if there are no more tracks
    if (nextTrack(1))
        doCallback(kMediaPlayerEventTrackUpdate);
    else if (mPlayer->finish())
        doCallback(kMediaPlayerEventTimeUpdate);
}

/**
 * ControlMedia.previous
 * Can be called from normal or fast mode
 */
void
TestMediaPlayer::previous()
{
    if (mReleased || !mPlayer)
        return;

    LOG_IF(DEBUG_MP) << mPlayer->toDebugString();

    resolveExistingAction();

    // Pause any current playback.  This does not generate a PAUSE event
    mPlayer->pause();

    // Go back to the previous track (if one exists)
    if (nextTrack(-1))
        doCallback(kMediaPlayerEventTrackUpdate);
    else if (mPlayer->rewind())
        doCallback(kMediaPlayerEventTimeUpdate);
}

/**
 * ControlMedia.rewind
 * Can be called from normal or fast mode
 */
void
TestMediaPlayer::rewind()
{
    if (mReleased || !mPlayer)
        return;

    LOG_IF(DEBUG_MP) << mPlayer->toDebugString();

    resolveExistingAction();

    mPlayer->pause();
    if (mPlayer->rewind())
        doCallback(kMediaPlayerEventTimeUpdate);
}

/**
 * ControlMedia.seek
 * Can be called from normal or fast mode
 */
void
TestMediaPlayer::seek(int offset)
{
    if (mReleased || !mPlayer)
        return;

    LOG_IF(DEBUG_MP) << "offset=" << offset << " " << mPlayer->toDebugString();

    resolveExistingAction();

    mPlayer->pause();
    if (mPlayer->seek(offset))         // TODO: Should we execute the onDone callback here?
        doCallback(kMediaPlayerEventTimeUpdate);    // Always runs in fast mode
}

/**
 * ControlMedia.setTrack
 * Can be called from normal or fast mode
 */
void
TestMediaPlayer::setTrackIndex(int trackIndex)
{
    if (mReleased || !mPlayer || trackIndex < 0 || trackIndex >= mMediaTracks.size())
        return;

    LOG_IF(DEBUG_MP) << "index=" << trackIndex << " " << mPlayer->toDebugString();

    resolveExistingAction();

    // The current player always pauses, even if the track index does not change
    mPlayer->pause();

    if (trackIndex == mTrackIndex) {
        // If the track was done and had some repeats, then clearing the repeats changes the time back to the beginning
        if (mPlayer->clearRepeat())
            doCallback(kMediaPlayerEventTimeUpdate);
    }
    else {
        mTrackIndex = trackIndex;
        if (createMediaPlayer())
            doCallback(kMediaPlayerEventTrackUpdate);
    }
}

void
TestMediaPlayer::setAudioTrack(AudioTrack audioTrack)
{
    if (mReleased)
        return;

    publishEvent(EventType::kPlayerEventSetAudioTrack);

    LOG_IF(DEBUG_MP) << "audioTrack=" << sAudioTrackMap.at(audioTrack) << " "
                     << (mPlayer ? mPlayer->toDebugString() : "no_player");

    mAudioTrack = audioTrack;
}

/**
 * Advance by at most "milliseconds".  At most a single callback will be invoked here.
 */
apl_duration_t
TestMediaPlayer::advanceByUpTo(apl_duration_t milliseconds)
{
    // If we're not doing anything, we can advance arbitrarily
    if (mReleased || !mPlayer)
        return milliseconds;

    auto result = mPlayer->advanceTime(milliseconds);
    LOG_IF(DEBUG_MP) << "Advanced delta=" << milliseconds << " player=" << mPlayer->toDebugString();

    switch (result.first) {
        case FakePlayer::TIME_UPDATE:
            doCallback(kMediaPlayerEventTimeUpdate);
            break;

        case FakePlayer::TRACK_READY:
            doCallback(kMediaPlayerEventTrackReady);
            break;

        case FakePlayer::TRACK_DONE:
            // When a track finishes, we try to execute the next track
            if (nextTrack(1)) {
                mPlayer->play();
                doCallback(kMediaPlayerEventTrackUpdate);
            }
            else {
                // TODO:  Should the End callback be strung along with this?
                resolveExistingAction();
                doCallback(kMediaPlayerEventEnd);
            }
            break;

        case FakePlayer::TRACK_FAIL:
            // As per the APL specification, playback stops on FAIL
            resolveExistingAction();
            doCallback(kMediaPlayerEventTrackFail);
            break;

        case FakePlayer::NO_REPORT:
            break;
    }

    return result.second;
}

/**
 * Advance to the next valid track.  The increment should be +1 or -1.  We assume that
 * the current track is mTrackIndex, and search outwards from the next track to the last one.
 * @return True if a new FakePlayer has been assigned.
 */
bool
TestMediaPlayer::nextTrack(int increment)
{
    assert(increment == 1 || increment == -1);
    auto trackIndex = mTrackIndex + increment;

    if (trackIndex < 0 || trackIndex >= mMediaTracks.size())
        return false;

    mTrackIndex = trackIndex;

    const auto& media = mMediaTracks.at(mTrackIndex);
    auto fc = mMediaPlayerFactory->findContent(media.url);
    mPlayer = FakePlayer::create(media, fc.actualDuration, fc.initialDelay, fc.failAfter);
    return true;
}

bool
TestMediaPlayer::createMediaPlayer()
{
    if (mTrackIndex < 0 || mTrackIndex >= mMediaTracks.size()) {
        mPlayer = nullptr;
        return false;
    }

    const auto& media = mMediaTracks.at(mTrackIndex);
    auto fc = mMediaPlayerFactory->findContent(media.url);
    mPlayer = FakePlayer::create(media, fc.actualDuration, fc.initialDelay, fc.failAfter);
    return true;
}

void
TestMediaPlayer::doCallback(MediaPlayerEventType eventType)
{
    if (!mCallback)
        return;

    bool atEnd = (mTrackIndex + 1 == mMediaTracks.size()) && mPlayer->isEnded();

    LOG_IF(DEBUG_MP) << sMediaPlayerEventTypeMap.at(eventType)
                     << " position=" << mPlayer->getPosition() << " player=" << mPlayer.get();

    mCallback(eventType,
              MediaState(mTrackIndex,                           // Report being on the last track
                         static_cast<int>(mMediaTracks.size()), // Track count
                         mPlayer->getPosition(),                // Current time
                         mPlayer->getDuration(),                // Current track duration
                         !mPlayer->isPlaying(),                 // paused
                         atEnd)                                 // ended
                  .withTrackState(mPlayer->getTrackState()));
}

void
TestMediaPlayer::resolveExistingAction()
{
    if (!mActionRef.empty()) {
        LOG_IF(DEBUG_MP) << "resolved";
        mActionRef.resolve();
    }
    mActionRef = ActionRef(nullptr);
}
void
TestMediaPlayer::publishEvent(TestMediaPlayer::EventType event)
{
    if (mEventCallback) {
        mEventCallback(event);
    }
}

} // namespace apl