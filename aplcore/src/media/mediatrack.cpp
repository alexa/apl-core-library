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

#include "apl/media/mediatrack.h"

namespace apl {

Bimap<TextTrackType, std::string> sTextTrackTypeMap = {
    {kTextTrackTypeCaption, "caption"},
};

// Utility function that creates a MediaTrack from a Speech Object
MediaTrack createMediaTrack(const Object &speech, const std::shared_ptr<Context> &context) {
    const std::string SPEECH_URL = "url";
    const std::string SPEECH_TEXT_TRACK = "textTrack";
    const std::string TEXT_TRACK_CONTENT = "content";
    const std::string TEXT_TRACK_TYPE = "type";
    const std::string TEXT_TRACK_TYPE_CAPTION = "caption";

    std::string url;
    TextTrackArray trackArray;
    // if speech is a string then it is a single url
    if (speech.isString()) {
        url = speech.getString();
        if (url.empty()) {
            CONSOLE(context).log("Audio source missing in playback");
        }
    }

    // if speech is a map then it may contain a textTrack
    else if (speech.isMap()) {
        url = speech.get(SPEECH_URL).asString();
        if (!url.empty()) {
            auto textTrack = speech.get(SPEECH_TEXT_TRACK);
            auto content = textTrack.get(TEXT_TRACK_CONTENT).asString();
            auto type = textTrack.get(TEXT_TRACK_TYPE).asString();
            if (!content.empty()) {
                if (!type.empty()) {
                    if (type == TEXT_TRACK_TYPE_CAPTION) {
                        // A textTrack is only valid if it has a source and a type of caption
                        trackArray.push_back(TextTrack{kTextTrackTypeCaption, content, ""});
                    } else {
                        CONSOLE(context).log("TextTrack has an invalid type");
                    }
                } else {
                    CONSOLE(context).log("TextTrack is missing a type");
                }

            } else {
                CONSOLE(context).log("TextTrack is missing an url");
            }
        }
    }

    return MediaTrack{
            url,           // URL
            0,      // Start
            0,    // Duration (play the entire track)
            0, // Repeat Count
            {},            // Headers
            trackArray     // textTrack Array
    };
}

} // namespace apl