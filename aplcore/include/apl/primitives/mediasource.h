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

#ifndef _APL_MEDIA_SOURCE_H
#define _APL_MEDIA_SOURCE_H

#include <string>

#include "apl/primitives/objecttype.h"
#include "apl/media/mediaobject.h"
#include "apl/media/mediatrack.h"
#include "apl/primitives/urlrequest.h"

namespace apl {

class Context;

class MediaSource {
public:
    /**
     * Build a media source from an Object. The source object may be a source (in
     * which case it is copied), array, single string or map.
     * @param context The data-binding context.
     * @param object The source of the media source.
     * @return An object containing a media source or null.
     */
    static Object create(const Context& context, const Object& object);

    /**
     * @return Get source url.
     */
    std::string getUrl() const { return mUrlRequest.getUrl(); }

    /**
     * @return Optional source description.
     */
    std::string getDescription() const { return mDescription; }

    /**
     * @return Text tracks
     */
    TextTrackArray getTextTracks() const { return mTextTracks; }

    /**
     * @return Media duration.
     */
    int getDuration() const { return mDuration; }

    /**
     * @return Play repeat count.
     */
    int getRepeatCount() const { return mRepeatCount; }

    /**
     * @return Source entities.
     */
    const Object& getEntities() { return mEntities; }

    /**
     * @return Source offset to start playback from.
     */
    int getOffset() const { return mOffset; }

    /**
     * @return headers to append to the request to obtain access to the source
     */
     const HeaderArray& getHeaders() const { return mUrlRequest.getHeaders(); }

    /* Standard Object methods */
    bool operator==(const MediaSource& other) const;

    std::string toDebugString() const;
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    bool empty() const { return false; }
    bool truthy() const { return true; }

    class ObjectType final : public ReferenceHolderObjectType<MediaSource> {};

private:
    MediaSource(URLRequest urlRequest,
                std::string description,
                int duration,
                int repeatCount,
                Object entities,
                int offset,
                TextTrackArray textTracks);

private:
    URLRequest mUrlRequest;
    std::string mDescription;
    int mDuration;
    int mRepeatCount;
    Object mEntities;
    int mOffset;
    TextTrackArray mTextTracks;
};

} // namespace apl

#endif //_APL_MEDIA_SOURCE_H
