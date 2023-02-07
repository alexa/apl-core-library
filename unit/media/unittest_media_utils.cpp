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

#include "../testeventloop.h"

#include "apl/media/mediatrack.h"
#include "apl/primitives/mediasource.h"
#include "apl/media/mediautils.h"

using namespace apl;

TEST(MediaUtilsTest, CopiesTextTracks) 
{
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());

    JsonData json(R"({"url":"URL", "textTrack": [{ "url": "URL", "type": "caption", "description": "foobar" }]})");
    const auto mediaSourceObject = MediaSource::create(*context, json.get());
    ASSERT_TRUE(mediaSourceObject.is<MediaSource>());
    const auto& ms = mediaSourceObject.get<MediaSource>();

    auto mediaSources = Object(std::vector<Object>({ mediaSourceObject }));

    std::vector<MediaTrack> mediaTracks = mediaSourcesToTracks(mediaSources);

    ASSERT_EQ(1, mediaTracks.size());
    MediaTrack mediaTrack = mediaTracks[0];

    ASSERT_EQ(1, mediaTrack.textTracks.size());
    ASSERT_EQ(1, ms.getTextTracks().size());
    ASSERT_EQ(ms.getTextTracks()[0].type, mediaTrack.textTracks[0].type);
    ASSERT_EQ(ms.getTextTracks()[0].url, mediaTrack.textTracks[0].url);
    ASSERT_EQ(ms.getTextTracks()[0].description, mediaTrack.textTracks[0].description);

    ASSERT_EQ(ms.getUrl(), mediaTrack.url);
    ASSERT_EQ(ms.getOffset(), mediaTrack.offset);
    ASSERT_EQ(ms.getDuration(), mediaTrack.duration);
    ASSERT_EQ(ms.getRepeatCount(), mediaTrack.repeatCount);
    ASSERT_EQ(ms.getHeaders(), mediaTrack.headers);
}

TEST(MediaUtilsTest, CopiesDefault) 
{
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());

    JsonData json(R"("URL")");
    const auto mediaSourceObject = MediaSource::create(*context, json.get());
    ASSERT_TRUE(mediaSourceObject.is<MediaSource>());
    const auto& ms = mediaSourceObject.get<MediaSource>();

    auto mediaSources = Object(std::vector<Object>({ mediaSourceObject }));

    std::vector<MediaTrack> mediaTracks = mediaSourcesToTracks(mediaSources);

    ASSERT_EQ(1, mediaTracks.size());
    MediaTrack mediaTrack = mediaTracks[0];

    ASSERT_EQ(0, mediaTrack.textTracks.size());
    ASSERT_EQ(0, ms.getTextTracks().size());

    ASSERT_EQ(ms.getUrl(), mediaTrack.url);
    ASSERT_EQ(ms.getOffset(), mediaTrack.offset);
    ASSERT_EQ(ms.getDuration(), mediaTrack.duration);
    ASSERT_EQ(ms.getRepeatCount(), mediaTrack.repeatCount);
    ASSERT_EQ(ms.getHeaders(), mediaTrack.headers);
}

TEST(MediaUtilsTest, NotArray) 
{
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());

    JsonData json(R"("URL")");
    const auto mediaSourceObject = MediaSource::create(*context, json.get());
    ASSERT_TRUE(mediaSourceObject.is<MediaSource>());

    auto mediaSources = Object(std::vector<Object>({ mediaSourceObject }));

    std::vector<MediaTrack> mediaTracks = mediaSourcesToTracks(mediaSourceObject);

    ASSERT_EQ(0, mediaTracks.size());
}

TEST(MediaUtilsTest, NotMediaSource) 
{
    auto objectArray = Object(std::vector<Object>({ Object::NULL_OBJECT() }));

    std::vector<MediaTrack> mediaTracks = mediaSourcesToTracks(objectArray);

    ASSERT_EQ(0, mediaTracks.size());
}