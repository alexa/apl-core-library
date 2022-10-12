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

#ifndef _APL_AUDIO_STATE_H
#define _APL_AUDIO_STATE_H

#include "apl/component/componentproperties.h"

namespace apl {

class AudioState {
public:
    AudioState(int currentTime, int duration, bool paused, bool ended, TrackState trackState)
        : mCurrentTime(currentTime),
          mDuration(duration),
          mPaused(paused),
          mEnded(ended),
          mTrackState(trackState)
    {}

    int getCurrentTime() const { return mCurrentTime; }
    int getDuration() const { return mDuration; }
    bool isPaused() const { return mPaused; }
    bool isEnded() const { return mEnded; }
    TrackState getTrackState() const { return mTrackState; }

    std::string toDebugString() const {
        std::string result;
        result += "AudioState{";
        result += ("time: " + std::to_string(mCurrentTime) + ",");
        result += ("duration: " + std::to_string(mDuration) + ",");
        result += ("paused: " + std::to_string(mPaused) + ",");
        result += ("ended: " + std::to_string(mEnded) + ",");
        result += ("state: " + std::to_string(mTrackState));
        result += "}";
        return result;
    }

private:
    int mCurrentTime;
    int mDuration;
    bool mPaused;
    bool mEnded;
    TrackState mTrackState;
};

} // namespace apl

#endif // _APL_AUDIO_STATE_H
