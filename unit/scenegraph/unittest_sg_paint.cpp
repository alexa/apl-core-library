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

#include "apl/scenegraph/paint.h"
#include "apl/scenegraph/builder.h"

using namespace apl;

class SGPaintTest : public ::testing::Test {};

TEST_F(SGPaintTest, ColorPaint)
{
    auto paint = sg::paint(Color(Color::BLUE), 0.5f);
    ASSERT_EQ(paint->toDebugString(), "ColorPaint color=#0000ffff opacity=0.500000");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(paint->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type": "colorPaint",
            "color": "#0000ffff",
            "opacity": 0.5
        }
    )apl")));

    ASSERT_TRUE(paint->setOpacity(1.0f));
    ASSERT_EQ(1.0f, paint->getOpacity());

    ASSERT_FALSE(paint->setTransform(Transform2D()));

    // Check visibility
    ASSERT_TRUE(paint->visible());
    ASSERT_TRUE(paint->setOpacity(0.0f));
    ASSERT_FALSE(paint->visible());

    ASSERT_TRUE(paint->setOpacity(1.0f));
    ASSERT_TRUE(paint->visible());

    ASSERT_TRUE(sg::ColorPaint::is_type(paint));
    ASSERT_TRUE(sg::ColorPaint::cast(paint)->setColor(Color::RED));
    ASSERT_EQ(sg::ColorPaint::cast(paint)->getColor(), Color::RED);
    ASSERT_TRUE(paint->visible());

    ASSERT_TRUE(sg::ColorPaint::cast(paint)->setColor(Color::TRANSPARENT));
    ASSERT_FALSE(paint->visible());
}

static const char *LINEAR_GRADIENT = R"apl(
{
    "type": "linear",
    "colorRange": [
        "black",
        "white"
    ],
    "inputRange": [
        0,
        0.4
    ],
    "angle": 90
}
)apl";

TEST_F(SGPaintTest, StandardLinearGradientPaint)
{
    auto config = RootConfig::create();
    auto metrics = Metrics().size(1024,800).dpi(160).theme("dark");
    auto context = Context::createTestContext(metrics, *config);

    rapidjson::Document doc;
    doc.Parse(LINEAR_GRADIENT);

    auto grad = Gradient::create(*context, Object(doc));
    auto paint = sg::paint(grad.get<Gradient>(), 0.5f, Transform2D::scale(2));

    ASSERT_TRUE(sg::LinearGradientPaint::is_type(paint));
    auto linear = sg::LinearGradientPaint::cast(paint);

    ASSERT_EQ(linear->getStart(), Point(0,0.5));
    ASSERT_EQ(linear->getEnd(), Point(1,0.5));
    ASSERT_EQ(linear->getPoints(), std::vector<double>({0, 0.4}));
    ASSERT_EQ(linear->getColors(), std::vector<Color>({Color::BLACK, Color::WHITE}));
    ASSERT_EQ(linear->getSpreadMethod(), Gradient::GradientSpreadMethod::PAD);
    ASSERT_TRUE(linear->getUseBoundingBox());
    ASSERT_EQ(0.5f, linear->getOpacity());
    ASSERT_EQ(Transform2D::scale(2), linear->getTransform());
    ASSERT_TRUE(linear->visible());

    ASSERT_EQ(linear->toDebugString(), "LinearGradientPaint "
                                       "points=[0.000000,0.400000] "
                                       "colors=[#000000ff,#ffffffff] "
                                       "spread=0 "
                                       "bb=yes "
                                       "opacity=0.500000 "
                                       "transform=Transform2D<2.000000, 0.000000, 0.000000, 2.000000, 0.000000, 0.000000> "
                                       "start=0.000000,0.500000 "
                                       "end=1.000000,0.500000");

    ASSERT_TRUE(IsEqual(linear->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type": "linearGradient",
            "opacity": 0.5,
            "transform": [2.0,0.0,0.0,2.0,0.0,0.0],
            "points": [0.0,0.4],
            "colors": ["#000000ff","#ffffffff"],
            "spreadMethod": "pad",
            "usingBoundingBox": true,
            "start": [0.0,0.5],
            "end": [1.0,0.5]
        }
    )apl")));

    ASSERT_TRUE(linear->setSpreadMethod(Gradient::GradientSpreadMethod::REFLECT));
    ASSERT_EQ(Gradient::GradientSpreadMethod::REFLECT, linear->getSpreadMethod());
    ASSERT_FALSE(linear->setSpreadMethod(Gradient::GradientSpreadMethod::REFLECT));

    ASSERT_TRUE(linear->setUseBoundingBox(false));
    ASSERT_FALSE(linear->getUseBoundingBox());
    ASSERT_FALSE(linear->setUseBoundingBox(false));
}


static const char *RADIAL_GRADIENT = R"apl(
{
    "type": "radial",
    "colorRange": [
        "black",
        "white"
    ],
    "inputRange": [
        0,
        0.4
    ]
}
)apl";

TEST_F(SGPaintTest, StandardRadialGradientPaint)
{
    auto config = RootConfig::create();
    auto metrics = Metrics().size(1024,800).dpi(160).theme("dark");
    auto context = Context::createTestContext(metrics, *config);

    rapidjson::Document doc;
    doc.Parse(RADIAL_GRADIENT);

    auto grad = Gradient::create(*context, Object(doc));
    auto paint = sg::paint(grad.get<Gradient>(), 0.5f, Transform2D::scale(2));

    ASSERT_TRUE(sg::RadialGradientPaint::is_type(paint));
    auto radial = sg::RadialGradientPaint::cast(paint);

    ASSERT_EQ(radial->getCenter(), Point(0.5,0.5));
    ASSERT_FLOAT_EQ(radial->getRadius(), 0.7071);   // Note: Not exactly correct, but what is used elsewhere
    ASSERT_EQ(radial->getPoints(), std::vector<double>({0, 0.4}));
    ASSERT_EQ(radial->getColors(), std::vector<Color>({Color::BLACK, Color::WHITE}));
    ASSERT_EQ(radial->getSpreadMethod(), Gradient::GradientSpreadMethod::PAD);
    ASSERT_TRUE(radial->getUseBoundingBox());
    ASSERT_EQ(0.5f, radial->getOpacity());
    ASSERT_EQ(Transform2D::scale(2), radial->getTransform());
    ASSERT_TRUE(radial->visible());

    ASSERT_EQ(radial->toDebugString(), "RadialGradientPaint "
                                       "points=[0.000000,0.400000] "
                                       "colors=[#000000ff,#ffffffff] "
                                       "spread=0 "
                                       "bb=yes "
                                       "opacity=0.500000 "
                                       "transform=Transform2D<2.000000, 0.000000, 0.000000, 2.000000, 0.000000, 0.000000> "
                                       "center=0.500000,0.500000 "
                                       "radius=0.707100");

    ASSERT_TRUE(IsEqual(radial->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type": "radialGradient",
            "opacity": 0.5,
            "transform": [2.0,0.0,0.0,2.0,0.0,0.0],
            "points": [0.0,0.4],
            "colors": ["#000000ff","#ffffffff"],
            "spreadMethod": "pad",
            "usingBoundingBox": true,
            "center": [0.5,0.5],
            "radius": 0.707099974155426
        }
    )apl")));

    ASSERT_TRUE(radial->setSpreadMethod(Gradient::GradientSpreadMethod::REFLECT));
    ASSERT_EQ(Gradient::GradientSpreadMethod::REFLECT, radial->getSpreadMethod());
    ASSERT_FALSE(radial->setSpreadMethod(Gradient::GradientSpreadMethod::REFLECT));

    ASSERT_TRUE(radial->setUseBoundingBox(false));
    ASSERT_FALSE(radial->getUseBoundingBox());
    ASSERT_FALSE(radial->setUseBoundingBox(false));
}

static const char *PATTERN = R"apl(
{
    "height": 10,
    "width": 10,
    "items": {
        "type": "path",
        "pathData": "M0,5 L5,0 L10,5 L5,10 z",
        "fill": "blue"
    }
}
)apl";

TEST_F(SGPaintTest, PatternPaint) {
    auto config = RootConfig::create();
    auto metrics = Metrics().size(1024, 800).dpi(160).theme("dark");
    auto context = Context::createTestContext(metrics, *config);

    rapidjson::Document doc;
    doc.Parse(PATTERN);

    auto graphic_pattern = GraphicPattern::create(*context, Object(doc));
    auto paint = sg::paint(graphic_pattern);

    ASSERT_TRUE(sg::PatternPaint::is_type(paint));
    auto pattern = sg::PatternPaint::cast(paint);

    ASSERT_EQ(pattern->toDebugString(), "PatternPaint size=10.000000x10.000000 opacity=1.000000");

    ASSERT_TRUE(IsEqual(pattern->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type": "patternPaint",
            "opacity": 1.0,
            "size": [10.0,10.0],
            "content": [
                {
                    "type": "draw",
                    "path": {
                        "type": "generalPath",
                        "values": "MLLLZ",
                        "points": [0.0,5.0,5.0,0.0,10.0,5.0,5.0,10.0]
                    },
                    "op": [
                        {
                            "type": "fill",
                            "fillType": "even-odd",
                            "paint": {
                                "opacity": 1.0,
                                "type": "colorPaint",
                                "color": "#0000ffff"
                            }
                        }
                    ]
                }
            ]
        }
    )apl")));

    ASSERT_FALSE(pattern->setSize(Size(10,10)));
    ASSERT_TRUE(pattern->setSize(Size(20,20)));
}