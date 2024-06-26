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

#include "apl/engine/evaluate.h"
#include "apl/engine/arrayify.h"
#include "apl/content/rootconfig.h"
#include "apl/primitives/mediasource.h"
#include "apl/primitives/objectdata.h"
#include "apl/media/mediatrack.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"

namespace apl {

MediaSource::MediaSource(URLRequest urlRequest,
        std::string description,
        int duration,
        int repeatCount,
        Object entities,
        int offset,
        TextTrackArray textTracks) : mUrlRequest(std::move(urlRequest)),
    mDescription(std::move(description)),
    mDuration(duration),
    mRepeatCount(repeatCount),
    mEntities(std::move(entities)),
    mOffset(offset),
    mTextTracks(textTracks)
{}

Object
MediaSource::create(const Context& context, const Object& object)
{
    if (object.is<MediaSource>())
        return object;

    if (object.isString()) {
        std::string url = object.asString();
        if (url.empty()) {
            CONSOLE(context) << "Empty string for media source";
            return Object::NULL_OBJECT();
        }
        return Object(MediaSource(URLRequest::create(context, object).get<URLRequest>(),
                                  "",
                                  0,
                                  0,
                                  std::vector<Object>(),
                                  0,
                                  TextTrackArray()));
    }

    if (!object.isMap())
        return Object::NULL_OBJECT();

    auto url = propertyAsString(context, object, "url");
    if(url.empty()) {
        CONSOLE(context) << "Media Source has no URL defined.";
        return Object::NULL_OBJECT();
    }

    std::string description = propertyAsString(context, object, "description");
    int duration = propertyAsInt(context, object, "duration", 0);
    int repeatCount = propertyAsInt(context, object, "repeatCount", 0);
    int offset = propertyAsInt(context, object, "offset", 0);
    auto entities = Object(arrayifyProperty(context, object, "entities", "entity"));

    TextTrackArray tracks;
    for (auto& m : arrayifyProperty(context, object, "textTracks", "textTrack")) {
        if (!m.isMap()) {
            CONSOLE(context) << "Text Track is not an object.";
            continue;
        }

        auto type = requiredMappedProperty<TextTrackType>(context, m, "type", sTextTrackTypeMap);
        if (!type.second) {
            CONSOLE(context) << "Unrecognized type field in Text Track";
            continue;
        }

        // TODO: Why is this here?
        auto dummyURL = propertyAsString(context, m, "url");
        if (dummyURL.empty()) {
            CONSOLE(context) << "Text Track has no URL defined.";
            continue;
        }

        tracks.push_back(TextTrack{
            type.first,
            URLRequest::create(context, m).get<URLRequest>().getUrl(),
            propertyAsString(context, m, "description")
        });
    }

    return MediaSource(URLRequest::create(context, object).get<URLRequest>(),
                              description,
                              duration,
                              repeatCount,
                              entities,
                              offset,
                              tracks);
}

std::string
MediaSource::toDebugString() const {
    return "MediaSource<url=" + getUrl() +
           " duration=" + std::to_string(getDuration()) +
           " repeatCount=" + std::to_string(getRepeatCount()) +
           " offset=" + std::to_string(getOffset()) +
           ">";
}

rapidjson::Value
MediaSource::serialize(rapidjson::Document::AllocatorType& allocator) const {
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);
    v.AddMember("url", Value(getUrl().c_str(), allocator).Move(), allocator);
    v.AddMember("description", Value(getDescription().c_str(), allocator).Move(), allocator);
    v.AddMember("duration", getDuration(), allocator);
    v.AddMember("repeatCount", getRepeatCount(), allocator);
    v.AddMember("offset", getOffset(), allocator);
    rapidjson::Value vHeaders(rapidjson::kArrayType);
    for(const auto& header : getHeaders())
        vHeaders.PushBack(Value(header.c_str(), allocator).Move(), allocator);
    v.AddMember("headers", vHeaders, allocator);
    rapidjson::Value vTextTracks(rapidjson::kArrayType);
    for(const auto& track : getTextTracks()) {
        rapidjson::Value t(rapidjson::kObjectType);
        t.AddMember("url", Value(track.url.c_str(), allocator).Move(), allocator);
        t.AddMember("description", Value(track.description.c_str(), allocator).Move(), allocator);
        t.AddMember("type", Value(sTextTrackTypeMap.at(track.type).c_str(), allocator), allocator);
        vTextTracks.PushBack(t, allocator);
    }
    v.AddMember("textTrack", vTextTracks, allocator);
    return v;
}

bool
MediaSource::operator==(const apl::MediaSource &other) const {
    return mUrlRequest == other.mUrlRequest &&
           mDuration == other.mDuration &&
           mRepeatCount == other.mRepeatCount &&
           mEntities == other.mEntities &&
           mOffset == other.mOffset;
}

} // namespace apl