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

#ifndef _APL_AUDIO_PLAYER_H
#define _APL_AUDIO_PLAYER_H

#include <functional>
#include <utility>
#include <vector>

#include "apl/common.h"
#include "apl/audio/audiostate.h"
#include "apl/audio/speechmark.h"
#include "apl/media/mediaplayer.h"

namespace apl {

enum AudioPlayerEventType {
    kAudioPlayerEventEnd,
    kAudioPlayerEventPause,
    kAudioPlayerEventPlay,
    kAudioPlayerEventTimeUpdate,
    kAudioPlayerEventReady,
    kAudioPlayerEventFail
};

/**
 * The speech mark callback should be executed by the view host in a thread-safe manner.
 */
using SpeechMarkCallback = std::function<void(const std::vector<SpeechMark>&)>;

/**
 * The media player callback should be executed by the view host in a thread-safe manner.
 * Pass in the event type and the current state of the media object.
 */
using AudioPlayerCallback = std::function<void(AudioPlayerEventType, const AudioState&)>;

/**
 * The public interface to an audio-only player.
 *
 * This is an abstract class.  The view host should implement this class in a thread-safe manner.
 * These methods are intended to be used by the core engine and should not be called by the view
 * host.
 */
class AudioPlayer {
public:
    /**
     * Construct an audio player.
     * @param playerCallback Required callback for audio events.
     * @param speechMarkCallback Optional callback for speech marks
     */
    explicit AudioPlayer(AudioPlayerCallback playerCallback,
                         SpeechMarkCallback speechMarkCallback)
        : mPlayerCallback(std::move(playerCallback)),
          mSpeechMarkCallback(std::move(speechMarkCallback))
    {}

    virtual ~AudioPlayer() = default;

    /**
     * Release this audio player and associated resources.  After this method is called the audio
     * player should not respond to commands from the core or from the view host.
     */
    virtual void release() = 0;

    /**
     * Assign a media track.  This will pause all audio playback and set up a new audio track.
     * The player should queue the track up for playing, but not start playing.
     * @param track A single MediaTrack for playback.  The repeatCount field should be ignored.
     */
    virtual void setTrack(MediaTrack track) = 0;

    /**
     * Start or resume playing the current media track.  This is ignored if no media track has
     * been set, if the media track has finished playing, or if the media track has an error.
     * @param actionRef An optional action reference to resolve when playback finishes.
     */
    virtual void play(ActionRef actionRef) = 0;

    /**
     * Pause audio playback.
     */
    virtual void pause() = 0;

    virtual rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const {
        return rapidjson::Value();
    }

protected:
    AudioPlayerCallback mPlayerCallback;
    SpeechMarkCallback mSpeechMarkCallback;
};

} // namespace apl

#endif // _APL_AUDIO_PLAYER_H
