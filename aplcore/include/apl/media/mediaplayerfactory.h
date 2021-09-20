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

#ifndef _APL_MEDIA_PLAYER_FACTORY_H
#define _APL_MEDIA_PLAYER_FACTORY_H

#include "apl/media/mediaplayer.h"

namespace apl {

/**
 * Factory for creating MediaPlayers.  Each video component requests a player from the installed
 * MediaPlayerFactory and uses that MediaPlayer to play video and audio clips.
 *
 * Please note that a MediaPlayerFactory may be shared across multiple view hosts.  An implementation
 * of the MediaPlayerFactory should implement thread safety.
 */
class MediaPlayerFactory {
public:
    virtual ~MediaPlayerFactory() = default;

    /**
     * @return A new media player
     */
    virtual MediaPlayerPtr createPlayer( MediaPlayerCallback callback ) = 0;
};

} // namespace apl

#endif // _APL_MEDIA_PLAYER_FACTORY_H
