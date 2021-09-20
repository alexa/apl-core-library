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

#include "apl/media/mediautils.h"
#include "apl/primitives/mediasource.h"

namespace apl {

std::vector<MediaTrack>
mediaSourcesToTracks(const Object& mediaSources)
{
    std::vector<MediaTrack> result;
    for (auto i = 0 ; i < mediaSources.size() ; i++) {
        const auto& ms = mediaSources.at(i).getMediaSource();
        result.emplace_back(MediaTrack{
            ms.getUrl(),        // URL
            ms.getOffset(),     // Offset
            ms.getDuration(),   // Duration
            ms.getRepeatCount() // Repeat count
        });
    }
    return result;
}

} // namespace apl