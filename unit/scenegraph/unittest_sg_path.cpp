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
#include "test_sg.h"
#include "apl/scenegraph/builder.h"

using namespace apl;

TEST(SGPathTest, Rectangle)
{
    ASSERT_EQ(0, sg::path(Rect{0, 20, 100, 100})->toDebugString().rfind("RectPath Rect"));

    ASSERT_EQ(sg::path(Rect{0, 20, 100, 100}), sg::path(Rect{0, 20, 100, 100}));
    ASSERT_NE(sg::path(Rect{10, 20, 30, 40}), sg::path(Rect{10, 20, 30, 60}));

    ASSERT_FALSE(sg::path(Rect{10, 20, 10, 0})->empty());
    ASSERT_FALSE(sg::path(Rect{10, 20, 10, 10})->empty());

    // Check other types for comparison
    ASSERT_NE(sg::path(Rect{0,0,10,10}), sg::path(Rect{0,0,10,10}, 2));
}

TEST(SGPathTest, RoundedRect)
{
    ASSERT_EQ(0, sg::path(Rect{0, 20, 100, 100}, 5)->toDebugString().rfind("RoundedRectPath Rect"));

    ASSERT_EQ(sg::path(Rect{10, 20, 30, 40}, 5), sg::path(Rect{10, 20, 30, 40}, 5));
    ASSERT_NE(sg::path(Rect{10, 20, 30, 40}, 5), sg::path(Rect{10, 20, 30, 77}, 5));
    ASSERT_NE(sg::path(Rect{10, 20, 30, 40}, 5), sg::path(Rect{10, 20, 30, 40}, 2));

    ASSERT_FALSE(sg::path(Rect{10, 20, 10, 0}, 5)->empty());
    ASSERT_FALSE(sg::path(Rect{10, 20, 10, 10}, 5)->empty());

    // Rounded rectangles with complicated radii
    ASSERT_EQ(sg::path(Rect{10, 20, 30, 40}, Radii{1, 2, 3, 4}),
              sg::path(Rect{10, 20, 30, 40}, Radii{1, 2, 3, 4}));
    ASSERT_NE(sg::path(Rect{10, 20, 30, 40}, Radii{1, 2, 3, 4}),
              sg::path(Rect{10, 20, 30, 77}, Radii{1, 2, 3, 4}));
    ASSERT_NE(sg::path(Rect{10, 20, 30, 40}, Radii{1, 2, 3, 4}),
              sg::path(Rect{10, 20, 30, 40}, Radii{1, 2, 3, 7}));

    // Other variations of constructors with RoundedRect
    ASSERT_EQ(sg::path(Rect{10, 20, 30, 40}, Radii{1, 2, 3, 4}),
              sg::path(RoundedRect(Rect{10, 20, 30, 40}, Radii{1, 2, 3, 4})));
    ASSERT_EQ(sg::path(Rect{10, 20, 30, 40}, 5),
              sg::path(RoundedRect(Rect{10, 20, 30, 40}, Radii{5, 5, 5, 5})));
}

TEST(SGPathTest, GeneralPath)
{
    ASSERT_EQ(0, sg::path("h20 v20 h-20 z")->toDebugString().rfind("GeneralPath MLLLZ"));

    ASSERT_EQ(sg::path("M5,5 h20"), sg::path("M 5  5 L25,5"));
    ASSERT_NE(sg::path("M5,5 h20"), sg::path("M 5  5 L25,6"));

    ASSERT_TRUE(sg::path("M10,10 m20,20 z")->empty());
    ASSERT_FALSE(sg::path("L10,10")->empty());

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(sg::path("M5,5 L10,10")->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
                            {
                                "type": "generalPath",
                                "values": "ML",
                                "points": [5.0,5.0,10.0,10.0]
                            }
                        )apl")));
}

TEST(SGPathTest, FramePath)
{
    ASSERT_EQ(0, sg::path(RoundedRect(Rect{0,0,10,10}, 4), 2)->toDebugString().rfind("FramePath Rect"));

    ASSERT_FALSE(sg::path(RoundedRect(Rect{0,0,10,10}, 4), 2)->empty());
    ASSERT_FALSE(sg::path(RoundedRect(Rect{0,0,0,10}, 4), 2)->empty());

    // Frame variations
    ASSERT_EQ(sg::path(RoundedRect(Rect{10,20,30,40}, 4), 10),
              sg::path(RoundedRect(Rect{10,20,30,40}, 4), 10));
    ASSERT_NE(sg::path(RoundedRect(Rect{5,20,30,40}, 4), 10),
              sg::path(RoundedRect(Rect{10,20,30,40}, 4), 10));
    ASSERT_NE(sg::path(RoundedRect(Rect{10,20,30,40}, 5), 10),
              sg::path(RoundedRect(Rect{10,20,30,40}, 4), 10));
    ASSERT_NE(sg::path(RoundedRect(Rect{10,20,30,40}, 4), 12),
              sg::path(RoundedRect(Rect{10,20,30,40}, 4), 10));

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(sg::path(RoundedRect(Rect{0,0,10,10}, 4), 2)->serialize(doc.GetAllocator()),
                StringToMapObject(R"apl(
                    {
                        "type": "framePath",
                        "rect": [0.0,0.0,10.0,10.0],
                        "radii": [4.0,4.0,4.0,4.0],
                        "inset": 2.0
                    }
                )apl")));
}