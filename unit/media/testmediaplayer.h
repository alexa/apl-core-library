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

#ifndef APL_TEST_MEDIA_PLAYER_H
#define APL_TEST_MEDIA_PLAYER_H

#include <functional>
#include <vector>

#include "apl/media/mediaplayer.h"
#include "fakeplayer.h"

namespace apl {

class TestMediaPlayerFactory;

/**
 * This is simulated media player.  It implements the MediaPlayer interface and adds one method
 * for moving time forward to generate suitable callbacks.  It relies on the TestMediaPlayerFactory
 * to retrieve information about video files such as how long they are or when they will fail.
 *
 * The implementation internally delegates individual track behavior to the FakePlayer class.
 */
class TestMediaPlayer : public MediaPlayer, public Counter<TestMediaPlayer> {
public:
    TestMediaPlayer(MediaPlayerCallback mediaPlayerCallback, std::shared_ptr<TestMediaPlayerFactory> factory);

    // *********** MediaPlayer Overrides ***********

    void release() override;
    void halt() override;

    void setTrackList(std::vector<MediaTrack> vector) override;
    void setAudioTrack(AudioTrack audioTrack) override;

    void play(ActionRef action) override;
    void pause() override;
    void next() override;
    void previous() override;
    void rewind() override;
    void seek(int offset) override;
    void setTrackIndex(int trackIndex) override;

    // ************** Testing methods ****************

    apl_duration_t advanceByUpTo(apl_duration_t milliseconds); // Return how many milliseconds we advanced
    bool isReleased() const { return mReleased; }

    enum class EventType {
        kPlayerEventSetTrackList,
        kPlayerEventSetAudioTrack,
        kPlayerEventPlay
    };

    using EventCallback = std::function<void(EventType event)>;

    void setEventCallback(EventCallback callback) { mEventCallback = std::move(callback); }

    static const Bimap<EventType, std::string> sEventTypeMap;

private:
    bool createMediaPlayer();
    bool nextTrack(int increment);
    void doCallback(MediaPlayerEventType eventType);
    void resolveExistingAction();
    void publishEvent(EventType event);

private:
    std::shared_ptr<TestMediaPlayerFactory> mMediaPlayerFactory;
    std::vector<MediaTrack> mMediaTracks;

    /// The current player.  There is always a current player unless there are no defined media tracks
    std::unique_ptr<FakePlayer> mPlayer;
    ActionRef mActionRef;

    /// The index of the current track.  This is always valid unless there are no defined media tracks.
    int mTrackIndex = 0;

    AudioTrack mAudioTrack = kAudioTrackForeground;
    bool mReleased = false; // Set when the media player is released and should not be used

    EventCallback mEventCallback = nullptr;
};

} // namespace apl

#endif // APL_TEST_MEDIA_PLAYER_H
