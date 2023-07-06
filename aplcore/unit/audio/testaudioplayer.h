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

#ifndef _APL_TEST_AUDIO_PLAYER_H
#define _APL_TEST_AUDIO_PLAYER_H

#include "apl/audio/audioplayer.h"

namespace apl {

class TestAudioPlayerFactory;

/**
 * Audio player for unit testing.  Create the audio player and set a track to get it started.
 * The audio player has the following states:
 * 
 * INIT      No track has been set
 * PREROLL   The track has been set.  A kPreroll event is sent to the factory and a timer is set
 *           for an initial delay.
 * PREPLAY   In preroll, but the audio has been requested to start playing immediately
 * READY     The preroll has finished and the track can be played. 
 * PLAY      The track is currently playing
 * DONE      The track has finished playing
 * FAIL      The track failed playback and is no longer playable
 * 
 * The mRunRequested flag marks if the track has been asked to play.
 */
class TestAudioPlayer : public AudioPlayer,
                        public Counter<TestAudioPlayer>,
                        public std::enable_shared_from_this<TestAudioPlayer> {
public:
    TestAudioPlayer(AudioPlayerCallback playerCallback,
                    SpeechMarkCallback speechMarkCallback,
                    std::shared_ptr<TestAudioPlayerFactory> factory);

    // *********** AudioPlayer Overrides ***********

    void release() override;

    void setTrack(MediaTrack track) override;
    void play(ActionRef action) override;
    void pause() override;

    // ************** Testing methods ****************

    bool isReleased() const { return mReleased; }

    // ******* Event recording *********

    enum EventType {
        kPreroll,  // Preroll started
        kReady,    // Preroll finished
        kPlay,     // Started playback
        kPause,    // Paused playback for any reason
        kDone,     // Finished playback
        kFail,     // Failed
        kRelease,  // Released
    };

    static std::string toString(EventType eventType);

    timeout_id getTimeoutId() const { return mTimeoutId; }

private:
    enum State {
        INIT,
        PREROLL,
        PREPLAY,
        READY,
        PLAY,
        DONE,
        FAIL
    };
    
    void prerollFinished();
    void startPlayback();
    void pausePlayback();
    void animate(apl_duration_t duration);
    void terminate();

    TrackState getTrackState() const;
    void doCallback(AudioPlayerEventType eventType);

private:
    std::shared_ptr<TestAudioPlayerFactory> mFactory;
    ActionRef mActionRef;
    bool mReleased = false;
    apl_duration_t mActualDuration = 0;
    apl_duration_t mInitialDelay = 0;
    apl_duration_t mFailAfter = -1;
    std::string mURL;
    timeout_id mTimeoutId;
    State mState = INIT;
    apl_duration_t mPlayheadPosition = 0;  // The current position of the play head in the track
    apl_duration_t mPlayheadStart = 0;
};


} // namespace apl

#endif // _APL_TEST_AUDIO_PLAYER_H
