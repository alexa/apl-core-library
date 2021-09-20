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

#include "fakeplayer.h"


namespace apl {

/**
 * Convenience class for reasoning about integers where a negative number means infinity.
 * We use this class to reason about the "repeatCount" setting for how many times a track
 * should repeat.  It's useful for calculating the overall duration of how long the
 * video will play for because an infinite repeat means that the video plays for infinitely
 * long.
 */
class InfiniteInt {
public:
    static InfiniteInt zero() { return InfiniteInt(0); }
    static InfiniteInt infinity() { return InfiniteInt(-1); }

    explicit InfiniteInt( int duration ) : mValue(duration) { assert(duration >= -1); }

    bool infinite() const { return mValue < 0; }
    bool empty() const { return mValue == 0; }
    int value() const { assert(mValue >= 0); return mValue; }

    InfiniteInt operator+(int right) const {
        if (infinite())
            return InfiniteInt::infinity();
        return InfiniteInt(mValue + right);
    }

    InfiniteInt operator-(int right) const {
        assert(right >= 0);
        if (infinite())
            return InfiniteInt::infinity();
        return InfiniteInt(std::max(0, mValue - right));
    }

    InfiniteInt operator*(InfiniteInt right) const {
        if (empty() || right.empty())
            return InfiniteInt::zero();
        if (infinite() || right.infinite())
            return InfiniteInt::infinity();
        return InfiniteInt(mValue * right.value());
    }

    bool operator==(int other) { return mValue == other; }

private:
    int mValue;
};

// Minimum of two infinite numbers
InfiniteInt min(InfiniteInt left, InfiniteInt right) {
    if (left.infinite())
        return right;
    if (right.infinite())
        return left;
    return InfiniteInt(std::min(left.value(), right.value()));
}

// Minimum of a regular number and an infinite number
int min(int left, InfiniteInt right) {
    return right.infinite() ? left : std::min(left, right.value());
}


/*****************************************************************************/

int calculateDuration( int start, int duration, int actualDuration )
{
    // If the requested duration is finite (non-zero), clip to the actual duration
    if (duration > 0)
        return min(start + duration, InfiniteInt(actualDuration)) - start;

    // The requested duration is infinite.
    // If the actual duration is infinite, we return infinity
    if (actualDuration < 0)
        return -1;

    // The actual duration is finite
    return std::max(0, actualDuration - start);
}

std::unique_ptr<FakePlayer>
FakePlayer::create(const MediaTrack& mediaTrack,
                  int actualDuration,
                  int initialDelay,
                  int failAfter)
{
    int offset = std::max(0, mediaTrack.offset);     // Ensure the offset is non-negative

    // Clip the start position to the actual duration
    int start = min(offset, InfiniteInt(actualDuration));
    int duration = calculateDuration(start, std::max(0, mediaTrack.duration), actualDuration);

    return std::unique_ptr<FakePlayer>(new FakePlayer(mediaTrack.duration, mediaTrack.repeatCount,
                                                      failAfter, start, duration, initialDelay));
}

FakePlayer::FakePlayer(int requestedDuration,
                       int repeatCount,
                       int failAfter,
                       int start,
                       int duration,
                       int initialDelay)
    : mRequestedDuration(requestedDuration),
      mRepeatCount(repeatCount),
      mFailAfter(failAfter),
      mStart(start),
      mDuration(duration),
      mBufferingTime(initialDelay),
      mTrackPosition(start),
      mCompletedPlays(0) {}

bool
FakePlayer::play()
{
    if (mState == IDLE) {
        mState = PLAYING;
        return true;
    }

    return false;
}

bool
FakePlayer::pause()
{
    if (mState == PLAYING) {
        mState = IDLE;
        return true;
    }

    return false;
}

bool
FakePlayer::rewind()
{
    if (mState == FAILED)
        return false;

    // A video that is DONE and has no duration cannot go back to IDLE.
    if (mState == DONE && mDuration == 0)
        return false;

    const auto changed = mTrackPosition != mStart || mCompletedPlays != 0;

    mState = IDLE;
    mTrackPosition = mStart;
    mCompletedPlays = 0;
    return changed;
}

bool
FakePlayer::finish()
{
    if (mState == FAILED || mState == DONE)
        return false;

    mState = DONE;
    mTrackPosition = mDuration >= 0 ? mStart + mDuration : mStart;
    mCompletedPlays = 0;
    return true;
}

bool
FakePlayer::seek(int offset)
{
    if (mState == FAILED)
        return false;

    auto oldPosition = mTrackPosition;
    mTrackPosition = clipPosition(offset);

    // If the position didn't change, don't do anything (but we set ourselves to idle)
    if (oldPosition == mTrackPosition) {
        if (mState == PLAYING)   // Note that a DONE state does not change
            mState = IDLE;
        return false;
    }

    // We may have used seek to go to the end of the track
    // Seek does not change the repeat counter, but you can end up at the end of a track
    if (positionAtEnd(mTrackPosition) && mRepeatCount >= 0 && mCompletedPlays == mRepeatCount)
        mState = DONE;
    else
        mState = IDLE;

    return true;
}

bool
FakePlayer::clearRepeat()
{
    // Clear how many times we've looped through this track
    mCompletedPlays = 0;

    if (mState == DONE && mRepeatCount > 0) {
        // Reset the head to the START position (the DONE state leaves it at the END position)
        mTrackPosition = mStart;
        mState = IDLE;
        return true;
    }

    return false;
}

bool
FakePlayer::positionAtEnd(int position) const
{
    if (mDuration < 0)
        return false;
    return position >= mStart + mDuration;
}

int
FakePlayer::clipPosition(int position) const
{
    if (position < mStart)
        return mStart;
    if (mDuration >= 0 && position > mStart + mDuration)
        return mStart + mDuration;
    return position;
}

std::pair<FakePlayer::FakeEvent, apl_duration_t>
FakePlayer::advanceTime(apl_duration_t maxTimeToAdvance)
{
    assert(maxTimeToAdvance > 0);

    if (mState == DONE || mState == FAILED)
        return {NO_REPORT, maxTimeToAdvance};

    // In IDLE and PLAYING states we always start by burning down the content buffer
    int elapsedTime = std::min(mBufferingTime, static_cast<int>(maxTimeToAdvance));
    mBufferingTime -= elapsedTime;
    if (mBufferingTime == 0 && !mReadyDispatched) {
        // You can fail immediately after buffering
        if (mFailAfter == 0) {
            mState = FAILED;
            return { TRACK_FAIL, elapsedTime };
        }

        mReadyDispatched = true;
        return {TRACK_READY, elapsedTime};
    }

    // Calculate the total amount of time that this track can play for, including repeats
    // This is the minimum of the actual track length after the offset and the requested duration
    if (mDuration == 0) {
        mState = DONE;
        return {TRACK_DONE, elapsedTime};
    }

    // Total amount of time we've played content on this fake player
    int msPlayed = mTrackPosition - mStart;
    if (mCompletedPlays > 0)
        msPlayed += mCompletedPlays *
                    mDuration; // Note mDuration could be -1, but that means mCompletedPlays==0

    InfiniteInt timeUntilDone = (InfiniteInt(mRepeatCount) + 1) * InfiniteInt(mDuration) - msPlayed;
    InfiniteInt timeUntilFailure = InfiniteInt(mFailAfter) - msPlayed;

    // Nothing to report if we are either IDLE or out of time
    if (mState == IDLE || elapsedTime == maxTimeToAdvance)
        return {NO_REPORT, maxTimeToAdvance};

    // Calculate how far we can move ahead.  Note that remainingTime is never infinite.
    int remainingTime =
        min(InfiniteInt(maxTimeToAdvance - elapsedTime), min(timeUntilDone, timeUntilFailure))
            .value();
    elapsedTime += remainingTime;

    // Move the play-head forward.  This may cause some repeats.
    // Note that the track position can be left at the END (which is appropriate for FAIL and DONE)
    mTrackPosition += remainingTime;
    if (mDuration > 0) {
        // TODO: Rewrite this to use modulo arithmetic.
        while (mTrackPosition > mStart + mDuration) {
            mTrackPosition -= mDuration;
            mCompletedPlays++;
        }
    }

    if (timeUntilFailure == remainingTime) {
        mState = FAILED;
        return {TRACK_FAIL, elapsedTime};
    }

    if (timeUntilDone == remainingTime) {
        mState = DONE;
        return {TRACK_DONE, elapsedTime};
    }

    // If the track position was left at the END, wrap it around to the front
    if (positionAtEnd(mTrackPosition)) {
        mTrackPosition = mStart;
        mCompletedPlays++;
    }

    return {TIME_UPDATE, elapsedTime};
}

std::string
FakePlayer::toDebugString() const
{
    static auto sStateToString = std::map<FakeState, std::string>{
        {IDLE, "idle"},
        {PLAYING, "playing"},
        {DONE, "done"},
        {FAILED, "failed"},
    };

    return "FakePlayer<state=" + sStateToString.at(mState) +
           " buffer=" + std::to_string(mBufferingTime) +
           " position=" + std::to_string(mTrackPosition) +
           " completed=" + std::to_string(mCompletedPlays) + ">";
}

} // namespace apl