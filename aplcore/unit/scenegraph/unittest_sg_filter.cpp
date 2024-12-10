/*
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
 *
 */

#include "../testeventloop.h"
#include "test_sg.h"

#include "apl/scenegraph/builder.h"

using namespace apl;

class FakeMediaObject : public MediaObject {
public:
    FakeMediaObject(std::string url)
        : mURL(std::move(url)) {}

    std::string url() const override { return mURL; }
    State state() const override { return kReady; }
    const HeaderArray& headers() const override { return mHeaders; }
    EventMediaType type() const override { return EventMediaType::kEventMediaTypeImage; }
    Size size() const override { return apl::Size(20,20); }
    CallbackID addCallback(MediaObjectCallback callback) override { return 0; }
    void removeCallback(CallbackID callbackToken) override {}
    int errorCode() const override { return 0; }
    std::string errorDescription() const override { return std::string(); }

    std::string mURL;
    HeaderArray mHeaders;
};


class SGFilterTest : public ::testing::Test {
public:
    sg::FilterPtr makeFilter(std::string url) {
        return sg::filter(std::make_shared<FakeMediaObject>(std::move(url)));
    }
};


TEST_F(SGFilterTest, Filter)
{
    auto filter = makeFilter("URL");
    ASSERT_EQ(filter->toDebugString(), "MediaObject url=URL");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(filter->serialize(doc.GetAllocator()),
                    StringToMapObject(R"apl(
        {
            "type": "mediaObjectFilter",
            "mediaObject": {
                "url": "URL"
            }
        }
    )apl")));
}

TEST_F(SGFilterTest, Blend)
{
    auto filter1 = makeFilter("URL1");
    auto filter2 = makeFilter("URL2");
    auto blend = sg::blend(filter1, filter2, BlendMode::kBlendModeDifference);

    ASSERT_EQ(blend->toDebugString(), "Blend mode=difference");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(blend->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "type": "blendFilter",
            "back": {
                "type": "mediaObjectFilter",
                "mediaObject": {
                    "url": "URL1"
                }
            },
            "front": {
                "type": "mediaObjectFilter",
                "mediaObject": {
                    "url": "URL2"
                }
            },
            "mode": "difference"
        }
    )apl")));
}

TEST_F(SGFilterTest, Blur)
{
    auto filter = makeFilter("URL");
    auto blur = sg::blur(filter, 10.0f);

    ASSERT_EQ(blur->toDebugString(), "Blur radius=10.000000");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(blur->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "type": "blurFilter",
            "filter": {
                "type": "mediaObjectFilter",
                "mediaObject": {
                    "url": "URL"
                }
            },
            "radius": 10.0
        }
    )apl")));
}


TEST_F(SGFilterTest, Grayscale)
{
    auto filter = makeFilter("URL");
    auto grayscale = sg::grayscale(filter, 0.5f);

    ASSERT_EQ(grayscale->toDebugString(), "Grayscale amount=0.500000");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(grayscale->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "type": "grayscaleFilter",
            "filter": {
                "type": "mediaObjectFilter",
                "mediaObject": {
                    "url": "URL"
                }
            },
            "amount": 0.5
        }
    )apl")));
}


TEST_F(SGFilterTest, Noise)
{
    auto filter = makeFilter("URL");
    auto noise = sg::noise(filter, NoiseFilterKind::kFilterNoiseKindUniform, true, 0.5f);

    ASSERT_EQ(noise->toDebugString(), "Noise kind=uniform useColor=yes sigma=0.500000");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(noise->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "type": "noiseFilter",
            "filter": {
                "type": "mediaObjectFilter",
                "mediaObject": {
                    "url": "URL"
                }
            },
            "kind": "uniform",
            "useColor": true,
            "sigma": 0.5
        }
    )apl")));
}


TEST_F(SGFilterTest, Saturate)
{
    auto filter = makeFilter("URL");
    auto saturate = sg::saturate(filter, 0.5f);

    ASSERT_EQ(saturate->toDebugString(), "Saturate amount=0.500000");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(saturate->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "type": "saturateFilter",
            "filter": {
                "type": "mediaObjectFilter",
                "mediaObject": {
                    "url": "URL"
                }
            },
            "amount": 0.5
        }
    )apl")));
}

TEST_F(SGFilterTest, SaturateZero)
{
    auto filter = makeFilter("URL");
    auto saturate = sg::saturate(filter, 0.0f);

    ASSERT_EQ(saturate->toDebugString(), "Saturate amount=0.000000");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(saturate->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "type": "saturateFilter",
            "filter": {
                "type": "mediaObjectFilter",
                "mediaObject": {
                    "url": "URL"
                }
            },
            "amount": 0.0
        }
    )apl")));
}


TEST_F(SGFilterTest, Solid)
{
    auto solid = sg::solid(sg::paint(Color(Color::RED), 0.5f));

    ASSERT_EQ(solid->toDebugString(), "Solid");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(solid->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "type": "solidFilter",
            "paint": {
                "type": "colorPaint",
                "color": "#ff0000ff",
                "opacity": 0.5
            }
        }
    )apl")));
}