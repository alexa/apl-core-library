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

#ifndef _APL_MEDIA_TRACK_H
#define _APL_MEDIA_TRACK_H

#include <string>
#include <vector>

#include "apl/primitives/header.h"
#include "apl/utils/bimap.h"
#include "apl/utils/session.h"
#include "apl/primitives/object.h"

namespace apl {

/**
 * The type of text track.
 */
enum TextTrackType {
    kTextTrackTypeCaption,
};

/**
 * The description of the subtitle 
 */
struct TextTrack {
    TextTrackType type;
    std::string url;
    std::string description;
};

using TextTrackArray = std::vector<TextTrack>;

/**
 * A description of a media track to be played by the media player.
 */
struct MediaTrack {
    std::string url;           // Source of the video clip
    int offset;                // Starting offset within the media object, in milliseconds
    int duration;              // Duration from the starting offset to play.  If non-positive, play the entire track
    int repeatCount;           // Number of times to repeat this track before moving to the next. Negative numbers repeat forever.
    HeaderArray headers;       // HeaderArray required for the track
    TextTrackArray textTracks; // Distinct subtitle tracks to render
    bool valid() const{
        return !url.empty();
    }
};

extern Bimap<TextTrackType, std::string> sTextTrackTypeMap;

MediaTrack createMediaTrack(const Object &speech, const std::shared_ptr<Context> &context);

} // namespace apl

#endif // _APL_MEDIA_TRACK_H
