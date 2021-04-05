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

#include <memory>

#include "gtest/gtest.h"

#include "../testeventloop.h"

#include "apl/content/jsondata.h"
#include "apl/content/metrics.h"
#include "apl/engine/context.h"
#include "apl/graphic/graphicfilter.h"
#include "apl/primitives/dimension.h"

using namespace apl;

TEST(GraphicFilterTest, Basic)
{
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());

    JsonData json(R"({"type":"DropShadow"})");
    auto f = GraphicFilter::create(*context, json.get());

    ASSERT_TRUE(f.isGraphicFilter());
    ASSERT_EQ(kGraphicFilterTypeDropShadow, f.getGraphicFilter().getType());
    ASSERT_TRUE(IsEqual(Color::BLACK, f.getGraphicFilter().getValue(kGraphicPropertyFilterColor)));
    ASSERT_TRUE(IsEqual(Object(0), f.getGraphicFilter().getValue(kGraphicPropertyFilterHorizontalOffset)));
    ASSERT_TRUE(IsEqual(Object(0), f.getGraphicFilter().getValue(kGraphicPropertyFilterRadius)));
    ASSERT_TRUE(IsEqual(Object(0), f.getGraphicFilter().getValue(kGraphicPropertyFilterVerticalOffset)));
}

TEST(GraphicFilterTest, BadGraphicFilter)
{
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());

    JsonData json(R"({"type":"DropShadoww"})");
    auto f = GraphicFilter::create(*context, json.get());

    ASSERT_FALSE(f.isGraphicFilter());
    ASSERT_EQ(Object::NULL_OBJECT(), f);
}

TEST(GraphicFilterTest, Equality)
{
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());

    JsonData filter1(R"( {"type": "DropShadow", "color": "blue"} )");
    JsonData filter2(R"( {"type": "DropShadow"} )");

    ASSERT_EQ(GraphicFilter::create(*context, filter1.get()), GraphicFilter::create(*context, filter1.get()));
    ASSERT_NE(GraphicFilter::create(*context, filter1.get()), GraphicFilter::create(*context, filter2.get()));
}

namespace {
    struct DropShadowGraphicFilterTest {
        std::string json;
        Color color;
        double horizontalOffset;
        double radius;
        double verticalOffset;
    };

    std::vector<DropShadowGraphicFilterTest> DROP_SHADOW_TESTS = {
        {R"({"type":"DropShadow"})",                                                                            Color::BLACK, 0, 0, 0},
        {R"({"type":"DropShadow", "color": "red"})",                                                            Color::RED, 0, 0, 0},
        {R"({"type":"DropShadow", "color":255, "horizontalOffset": 1})",                                        Color::BLACK, 1, 0, 0},
        {R"({"type":"DropShadow", "color":"#FFFFFF", "horizontalOffset": 1, "radius": 0.5})",                   Color::WHITE, 1, 0.5, 0},
        {R"({"type":"DropShadow", "color":"blue", "horizontalOffset": 1, "radius": 5, "verticalOffset": 2})",   Color::BLUE, 1, 5, 2},
        {R"({"type":"DropShadow", "color":"wxyz", "horizontalOffset": 1.5, "radius": 5, "verticalOffset": 2})", Color::TRANSPARENT, 1.5, 5, 2},
        {R"({"type":"DropShadow", "color":"blue", "horizontalOffset": 1, "radius": -5, "verticalOffset": 2})",  Color::BLUE, 1, 0, 2},
    };
}

TEST(GraphicFilterTest, DropShadowGraphicFilter)
{
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());

    for (auto& m : DROP_SHADOW_TESTS) {
        JsonData json(m.json);
        auto filterObject = GraphicFilter::create(*context, json.get());
        ASSERT_TRUE(filterObject.isGraphicFilter()) << m.json;
        const auto& filter = filterObject.getGraphicFilter();
        ASSERT_EQ(kGraphicFilterTypeDropShadow, filter.getType()) << m.json;
        ASSERT_TRUE(IsEqual(m.color, filter.getValue(kGraphicPropertyFilterColor).asColor())) << m.json;
        ASSERT_TRUE(IsEqual(m.horizontalOffset, filter.getValue(kGraphicPropertyFilterHorizontalOffset))) << m.json;
        ASSERT_TRUE(IsEqual(m.radius, filter.getValue(kGraphicPropertyFilterRadius))) << m.json;
        ASSERT_TRUE(IsEqual(m.verticalOffset, filter.getValue(kGraphicPropertyFilterVerticalOffset))) << m.json;
    }
}

TEST(GraphicFilterTest, ResourceSubstitution)
{
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());
    context->putConstant("@filterSize", Object(10));

    JsonData json1(R"({"type": "DropShadow", "radius": "@filterSize"})");
    auto f = GraphicFilter::create(*context, json1.get());
    ASSERT_TRUE(f.isGraphicFilter());
    ASSERT_EQ(Object(10), f.getGraphicFilter().getValue(kGraphicPropertyFilterRadius));

    JsonData json2(R"({"type": "DropShadow", "radius": "${@filterSize * 2}"})");
    f = GraphicFilter::create(*context, json2.get());
    ASSERT_TRUE(f.isGraphicFilter());
    ASSERT_EQ(Object(20), f.getGraphicFilter().getValue(kGraphicPropertyFilterRadius));
}