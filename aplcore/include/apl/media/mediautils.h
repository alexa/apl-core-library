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

#ifndef _APL_MEDIA_UTILS_H
#define _APL_MEDIA_UTILS_H

#include <vector>
#include "apl/media/mediaplayer.h"

namespace apl {

/**
 * Convenience method for converting media sources (used internally by APL) into media tracks which
 * are used by the media player.
 * @param mediaSources An Object containing an array of media sources
 * @return A vector of media tracks
 */
std::vector<MediaTrack> mediaSourcesToTracks(const Object& mediaSources);

} // namespace apl

#endif // _APL_MEDIA_UTILS_H
