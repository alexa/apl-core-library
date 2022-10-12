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

#ifndef _APL_AUDIO_PLAYER_FACTORY_H
#define _APL_AUDIO_PLAYER_FACTORY_H

#include "apl/audio/audioplayer.h"

namespace apl {

/**
 * Factory for creating AudioPlayers.
 *
 * Please note that an AudioPlayerFactory may be shared across multiple view hosts.  An implementation
 * of the AudioPlayerFactory should implement thread safety.
 */
class AudioPlayerFactory {
public:
    virtual ~AudioPlayerFactory() = default;

    /**
     * Construct an audio-only player.
     * @param playerCallback Invoked as the audio player changes state.
     * @param speechMarkCallback Optional callback invoked when speech marks are read from the audio file.
     * @return A new audio player.
     */
    virtual AudioPlayerPtr createPlayer(AudioPlayerCallback playerCallback,
                                        SpeechMarkCallback speechMarkCallback) = 0;
};


} // namespace apl

#endif // _APL_AUDIO_PLAYER_FACTORY_H
