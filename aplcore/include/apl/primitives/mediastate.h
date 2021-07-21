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

#ifndef _APL_MEDIA_STATE_H
#define _APL_MEDIA_STATE_H

namespace apl {
class MediaState {
public:
    MediaState() :
            mTrackIndex(0), mTrackCount(0), mCurrentTime(0),
            mDuration(0), mPaused(true), mEnded(false) {}
    MediaState(int trackIndex, int trackCount, int currentTime, int duration, bool paused,  bool ended) :
            mTrackIndex(trackIndex), mTrackCount(trackCount), mCurrentTime(currentTime),
            mDuration(duration), mPaused(paused), mEnded(ended) {}

    int getTrackIndex() const { return mTrackIndex; }
    int getTrackCount() const { return mTrackCount; }
    int getCurrentTime() const { return mCurrentTime; }
    int getDuration() const { return mDuration; }
    bool isPaused() const { return mPaused; }
    bool isEnded() const { return mEnded; }

private:
    int mTrackIndex;
    int mTrackCount;
    int mCurrentTime;
    int mDuration;
    bool mPaused;
    bool mEnded;
};
};

#endif // _APL_MEDIA_STATE_H