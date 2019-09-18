/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <iostream>
#include <apl/utils/log.h>

#include "rapidjson/document.h"
#include "gtest/gtest.h"

#include "apl/engine/evaluate.h"
#include "apl/engine/builder.h"
#include "apl/component/component.h"
#include "apl/engine/rootcontext.h"
#include "apl/content/metrics.h"
#include "apl/engine/resources.h"
#include "apl/engine/styles.h"
#include "apl/component/textcomponent.h"
#include "apl/component/textmeasurement.h"

#include "testeventloop.h"

using namespace apl;

// Static method for splitting strings on <br> or the equivalent
static std::vector<std::string>
splitString(const std::string& text, const std::string& delimiter)
{
    std::vector<std::string> lines;
    std::string::size_type offset = 0;
    auto dlen = delimiter.size();
    std::string::size_type nextOffset;
    while ((nextOffset = text.find(delimiter, offset)) != std::string::npos) {
        lines.push_back(text.substr(offset, nextOffset - offset));
        offset = nextOffset + dlen;
    }
    lines.push_back(text.substr(offset));
    return lines;
}

// Custom text measurement class.  All characters are a 10x10 block.
class TestTextMeasurement : public TextMeasurement {
public:
    // We'll assign a 10x10 block for each text square.
    virtual YGSize measure( TextComponent *component,
                            float width,
                            YGMeasureMode widthMode,
                            float height,
                            YGMeasureMode heightMode ) override {

        auto bold = component->getCalculated(kPropertyFontWeight).asInt() >= 700;
        auto text = component->getCalculated(kPropertyText).asString();
        auto len = text.size();
        auto lines = splitString(text, "<br>");

        int lineCount = lines.size();
        auto it = std::max_element(lines.cbegin(), lines.cend(),
                                   [](const std::string& first, const std::string& second){ return first.size() < second.size(); });
        int widestLine = it->size();

        float w = 10 * widestLine * (bold ? 2 : 1);  // Bold fonts are twice as wide
        float h = 10 * lineCount;
        return {.width=w, .height=h};
    }

    virtual float baseline( TextComponent *component, float width, float height ) override
    {
        return height;  // Align to the bottom of the text
    }
};


class FlexboxTest : public DocumentWrapper {};

static const char * SIMPLE_AUTO =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\""
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, SimpleAuto)
{
    loadDocument(SIMPLE_AUTO);

    auto bounds = component->getCalculated(kPropertyBounds);
    ASSERT_TRUE(bounds.isRect());

    ASSERT_EQ(Rect(0,0,1024,800), bounds.getRect());
}

static const char * SIMPLE_FIXED =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"width\": 200,"
    "      \"height\": 300"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, SimpleFixed)
{
    loadDocument(SIMPLE_FIXED);

    auto bounds = component->getCalculated(kPropertyBounds);
    ASSERT_TRUE(bounds.isRect());
    ASSERT_EQ(Rect(0, 0, 200, 300), bounds.getRect());

    auto inner = component->getCalculated(kPropertyInnerBounds);
    ASSERT_TRUE(inner.isRect());
    ASSERT_EQ(Rect(0, 0, 200, 300), inner.getRect());
}

static const char *TOO_LARGE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"width\": 2000,"
    "      \"height\": 2000"
    "    }"
    "  }"
    "}";

// The top-level component can be set to an arbitrary size
TEST_F(FlexboxTest, TooLarge)
{
    loadDocument(TOO_LARGE);
    ASSERT_EQ(Rect(0,0,2000,2000), component->getCalculated(kPropertyBounds).getRect());
}

static const char *SIMPLE_BOUNDS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"maxWidth\": 600,"
    "      \"maxHeight\": 2000"
    "    }"
    "  }"
    "}";

// TODO: Setting the maxWidth/Height on a top-level component causes problems???
//TEST_F(FlexboxTest, SimpleBounds)
//{
//    loadDocument(SIMPLE_BOUNDS);
//    ASSERT_EQ(Rect(0,0,0,0), component->getCalculated(kPropertyBounds).getRect());
//}

static const char *THREE_CHILDREN_TALL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"paddingLeft\": 10,"
    "      \"paddingRight\": 20,"
    "      \"paddingTop\": 30,"
    "      \"paddingBottom\": 40,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 200,"
    "        \"paddingLeft\": 1,"
    "        \"paddingRight\": 2,"
    "        \"paddingTop\": 3,"
    "        \"paddingBottom\": 4"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, ThreeChildrenTall)
{
    loadDocument(THREE_CHILDREN_TALL);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(10, 30, 994, 730), component->getCalculated(kPropertyInnerBounds).getRect());
    ASSERT_EQ(3, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(10, 30, 100, 200), child->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(1, 3, 97, 193), child->getCalculated(kPropertyInnerBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(10, 230, 100, 200), child->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(1, 3, 97, 193), child->getCalculated(kPropertyInnerBounds).getRect());

    child = component->getChildAt(2);
    ASSERT_EQ(Rect(10, 430, 100, 200), child->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(1, 3, 97, 193), child->getCalculated(kPropertyInnerBounds).getRect());
}

static const char *THREE_CHILDREN_WIDE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"paddingLeft\": 10,"
    "      \"paddingRight\": 20,"
    "      \"paddingTop\": 30,"
    "      \"paddingBottom\": 40,"
    "      \"direction\": \"row\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 200,"
    "        \"paddingLeft\": 1,"
    "        \"paddingRight\": 2,"
    "        \"paddingTop\": 3,"
    "        \"paddingBottom\": 4"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, ThreeChildrenWide)
{
    loadDocument(THREE_CHILDREN_WIDE);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(10, 30, 994, 730), component->getCalculated(kPropertyInnerBounds).getRect());
    ASSERT_EQ(3, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(10, 30, 100, 200), child->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(1, 3, 97, 193), child->getCalculated(kPropertyInnerBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(110, 30, 100, 200), child->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(1, 3, 97, 193), child->getCalculated(kPropertyInnerBounds).getRect());

    child = component->getChildAt(2);
    ASSERT_EQ(Rect(210, 30, 100, 200), child->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(1, 3, 97, 193), child->getCalculated(kPropertyInnerBounds).getRect());
}

static const char *OVERLY_TALL_CHILDREN =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 400"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, OverlyTallChildren)
{
    loadDocument(OVERLY_TALL_CHILDREN);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(3, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 0, 100, 400), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(0, 400, 100, 400), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);
    ASSERT_EQ(Rect(0, 800, 100, 400), child->getCalculated(kPropertyBounds).getRect());
}

static const char *SHRINKING_CHILDREN =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 400,"
    "        \"shrink\": \"${data}\""
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, ShrinkingChildren)
{
    loadDocument(SHRINKING_CHILDREN);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(4, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 0, 100, 320), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(0, 320, 100, 240), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);
    ASSERT_EQ(Rect(0, 560, 100, 160), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(3);
    ASSERT_EQ(Rect(0, 720, 100, 80), child->getCalculated(kPropertyBounds).getRect());
}

static const char *GROWING_CHILDREN =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 100,"
    "        \"grow\": \"${data}\""
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, GrowingChildren)
{
    loadDocument(GROWING_CHILDREN);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(4, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 0, 100, 140), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(0, 140, 100, 180), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);
    ASSERT_EQ(Rect(0, 320, 100, 220), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(3);
    ASSERT_EQ(Rect(0, 540, 100, 260), child->getCalculated(kPropertyBounds).getRect());
}

static const char *ABSOLUTE_POSITION =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"position\": \"absolute\","
    "        \"left\": 5,"
    "        \"top\": 10,"
    "        \"bottom\": 15,"
    "        \"right\": 20"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, AbsolutePosition)
{
    loadDocument(ABSOLUTE_POSITION);
    ASSERT_EQ(Rect(0, 0, 1024, 800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(1, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(5, 10, 999, 775), child->getCalculated(kPropertyBounds).getRect());
}

const static char * BORDER_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": \"100%\","
    "        \"height\": \"100%\","
    "        \"borderWidth\": 10,"
    "        \"items\": {"
    "          \"type\": \"Container\","
    "          \"width\": \"100%\","
    "          \"height\": \"100%\""
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, BorderTest)
{
    loadDocument(BORDER_TEST);
    ASSERT_EQ(Rect(0, 0, 1024, 800), component->getCalculated(kPropertyBounds).getRect());

    auto frame = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 0, 1024, 800), frame->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Object(Dimension(10)), frame->getCalculated(kPropertyBorderWidth));
    ASSERT_EQ(Rect(10, 10, 1004, 780), frame->getCalculated(kPropertyInnerBounds).getRect());

    // The child of the frame respects the border
    auto child = frame->getChildAt(0);
    ASSERT_EQ(Rect(10, 10, 1004, 780), child->getCalculated(kPropertyBounds).getRect());
}

static const char *BORDER_TEST_WITH_PADDING =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": \"100%\","
    "        \"height\": \"100%\","
    "        \"borderWidth\": 10,"
    "        \"paddingLeft\": 20,"
    "        \"paddingTop\": 30,"
    "        \"paddingRight\": 40,"
    "        \"paddingBottom\": 50,"
    "        \"items\": {"
    "          \"type\": \"Container\","
    "          \"width\": \"100%\","
    "          \"height\": \"100%\""
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, BorderTestWithPadding)
{
    loadDocument(BORDER_TEST_WITH_PADDING);
    ASSERT_EQ(Rect(0, 0, 1024, 800), component->getCalculated(kPropertyBounds).getRect());

    auto frame = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 0, 1024, 800), frame->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Object(Dimension(10)), frame->getCalculated(kPropertyBorderWidth));
    ASSERT_EQ(Rect(30, 40, 944, 700), frame->getCalculated(kPropertyInnerBounds).getRect());

    // The child of the frame respects the border
    auto child = frame->getChildAt(0);
    ASSERT_EQ(Rect(30, 40, 944, 700), child->getCalculated(kPropertyBounds).getRect());
}

static const char *JUSTIFY_END =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"justifyContent\": \"end\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, JustifyEnd)
{
    loadDocument(JUSTIFY_END);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(2, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 600, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(0, 700, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

static const char *JUSTIFY_CENTER =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"justifyContent\": \"center\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, JustifyCenter)
{
    loadDocument(JUSTIFY_CENTER);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(2, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 300, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(0, 400, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

static const char *JUSTIFY_SPACE_BETWEEN =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"justifyContent\": \"spaceBetween\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, JustifySpaceBetween)
{
    loadDocument(JUSTIFY_SPACE_BETWEEN);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(2, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(0, 700, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

static const char *JUSTIFY_SPACE_AROUND =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"justifyContent\": \"spaceAround\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, JustifySpaceAround)
{
    loadDocument(JUSTIFY_SPACE_AROUND);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(2, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 150, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(0, 550, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

static const char *ALIGN_ITEMS_START =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"alignItems\": \"start\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"height\": 100,"
    "        \"width\": 100,"
    "        \"alignSelf\": \"${data}\""
    "      },"
    "      \"data\": ["
    "        \"auto\","
    "        \"start\","
    "        \"end\","
    "        \"center\""
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, AlignItemsStart)
{
    loadDocument(ALIGN_ITEMS_START);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(4, component->getChildCount());

    auto child = component->getChildAt(0);  // First child is "auto", which will be left-aligned
    ASSERT_EQ(Rect(0, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);  // Second child is "start
    ASSERT_EQ(Rect(0, 100, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);  // Third child is "end"
    ASSERT_EQ(Rect(924, 200, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(3);  // Fourth child is "center"
    ASSERT_EQ(Rect(462, 300, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

static const char *ALIGN_ITEMS_CENTER =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"alignItems\": \"center\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"height\": 100,"
    "        \"width\": 100,"
    "        \"alignSelf\": \"${data}\""
    "      },"
    "      \"data\": ["
    "        \"auto\","
    "        \"start\","
    "        \"end\","
    "        \"center\""
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, AlignItemsCenter)
{
    loadDocument(ALIGN_ITEMS_CENTER);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(4, component->getChildCount());

    auto child = component->getChildAt(0);  // First child is "auto", which will be centered
    ASSERT_EQ(Rect(462, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);  // Second child is "start
    ASSERT_EQ(Rect(0, 100, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);  // Third child is "end"
    ASSERT_EQ(Rect(924, 200, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(3);  // Fourth child is "center"
    ASSERT_EQ(Rect(462, 300, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

static const char *ALIGN_ITEMS_END =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"alignItems\": \"end\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"height\": 100,"
    "        \"width\": 100,"
    "        \"alignSelf\": \"${data}\""
    "      },"
    "      \"data\": ["
    "        \"auto\","
    "        \"start\","
    "        \"end\","
    "        \"center\""
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, AlignItemsEnd)
{
    loadDocument(ALIGN_ITEMS_END);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(4, component->getChildCount());

    auto child = component->getChildAt(0);  // First child is "auto", which will be right-aligned
    ASSERT_EQ(Rect(924, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);  // Second child is "start
    ASSERT_EQ(Rect(0, 100, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);  // Third child is "end"
    ASSERT_EQ(Rect(924, 200, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(3);  // Fourth child is "center"
    ASSERT_EQ(Rect(462, 300, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

static const char *SPACING_VERTICAL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"spacing\": \"${data}\","
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        50,"
    "        50,"
    "        100"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, SpacingVertical)
{
    loadDocument(SPACING_VERTICAL);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(3, component->getChildCount());

    auto child = component->getChildAt(0);  // No spacing for first child
    ASSERT_EQ(Rect(0, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);  // Add spacing for second child of 50
    ASSERT_EQ(Rect(0, 150, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);  // The last child gets another 100
    ASSERT_EQ(Rect(0, 350, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}


static const char *SPACING_HORIZONTAL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"direction\": \"row\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"spacing\": \"${data}\","
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        50,"
    "        50,"
    "        100"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, SpacingHorizontal)
{
    loadDocument(SPACING_HORIZONTAL);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(3, component->getChildCount());

    auto child = component->getChildAt(0);  // No spacing for first child
    ASSERT_EQ(Rect(0, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);  // Add spacing for second child of 50
    ASSERT_EQ(Rect(150, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);  // The last child gets another 100
    ASSERT_EQ(Rect(350, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}



static const char *TEXT_MEASUREMENT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"alignItems\": \"start\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"text\": \"This is line 1.<br>This is line 2.\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, TextCheck)
{
    config.measure(std::make_shared<TestTextMeasurement>());
    loadDocument(TEXT_MEASUREMENT);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(1, component->getChildCount());

    auto child = component->getChildAt(0);  // No spacing for first child
    ASSERT_EQ(Rect(0, 0, 150, 20), child->getCalculated(kPropertyBounds).getRect());
    clearDirty();

    // Now let's change the text - this should trigger a re-layout
    std::dynamic_pointer_cast<CoreComponent>(child)->setProperty(kPropertyText, "Short");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_EQ(Rect(0, 0, 50, 10), child->getCalculated(kPropertyBounds).getRect());
}

static const char *FONT_STYLE_CHECK =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"myFontStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"fontWeight\": \"normal\""
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"fontWeight\": \"bold\""
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"alignItems\": \"start\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"style\": \"myFontStyle\","
    "        \"text\": \"This is line 1.<br>This is line 2.\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, FontStyleCheck)
{
    config.measure(std::make_shared<TestTextMeasurement>());

    loadDocument(FONT_STYLE_CHECK);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(1, component->getChildCount());

    auto child = component->getChildAt(0);  // No spacing for first child
    ASSERT_EQ(Rect(0, 0, 150, 20), child->getCalculated(kPropertyBounds).getRect());
    clearDirty();

    // Now toggle the style - this will force a re-layout
    child->update(kUpdatePressState, 1);
    clearDirty();

    // The bold font is twice as wide as the normal font.
    ASSERT_EQ(Rect(0, 0, 300, 20), child->getCalculated(kPropertyBounds).getRect());

    child->update(kUpdatePressState, 0);
    clearDirty();
    ASSERT_EQ(Rect(0, 0, 150, 20), child->getCalculated(kPropertyBounds).getRect());
}

const static char *BASELINE_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"direction\": \"row\","
    "      \"alignItems\": \"baseline\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"text\": \"${data}\""
    "      },"
    "      \"data\": ["
    "        \"Single line\","
    "        \"Double line<br>Double line\","
    "        \"Triple line<br>Triple line<br>Triple line\""
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, BaselineTest)
{
    config.measure(std::make_shared<TestTextMeasurement>());

    loadDocument(BASELINE_TEST);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(3, component->getChildCount());

    auto child = component->getChildAt(0);  // First child is one line
    ASSERT_EQ(Rect(0, 20, 110, 10), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);  // First child is one line
    ASSERT_EQ(Rect(110, 10, 110, 20), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);  // First child is one line
    ASSERT_EQ(Rect(220, 0, 110, 30), child->getCalculated(kPropertyBounds).getRect());
}

const static char *SCROLL_VIEW_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"ScrollView\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": \"100%\","
    "        \"height\": 4000"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, ScrollViewTest)
{
    loadDocument(SCROLL_VIEW_TEST);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_EQ(kComponentTypeScrollView, component->getType());

    auto frame = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 0, 1024, 4000), frame->getCalculated(kPropertyBounds).getRect());
}

const static char *SEQUENCE_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": \"100%\","
    "        \"height\": 400"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4"
    "      ]"
    "    }"
    "  }"
    "}";


TEST_F(FlexboxTest, SequenceTest)
{
    loadDocument(SEQUENCE_TEST);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(4, component->getChildCount());
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    for (int i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        child->ensureLayout();
        ASSERT_EQ(Rect(0, 400*i, 1024, 400), child->getCalculated(kPropertyBounds).getRect());
    }
}

const static char *HORIZONTAL_SEQUENCE_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"scrollDirection\": \"horizontal\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": \"400\","
    "        \"height\": \"100%\""
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, HorizontalSequenceTest)
{
    loadDocument(HORIZONTAL_SEQUENCE_TEST);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(4, component->getChildCount());
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(kScrollDirectionHorizontal, component->getCalculated(kPropertyScrollDirection).asInt());

    for (int i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        child->ensureLayout();
        ASSERT_EQ(Rect(400*i, 0, 400, 800), child->getCalculated(kPropertyBounds).getRect());
    }
}

const static char *SPACED_SEQUENCE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"item\": {"
    "        \"type\": \"Container\","
    "        \"direction\": \"row\","
    "        \"width\": \"100%\","
    "        \"height\": \"auto\","
    "        \"spacing\": \"${data[0]}\","
    "        \"item\": {"
    "            \"type\": \"Text\","
    "            \"height\": 200,"
    "            \"width\": \"100%\","
    "            \"text\": \"${data[1]}\""
    "        }"
    "      },"
    "      \"data\": ["
    "        [10, \"1\"],"
    "        [20, \"2\"],"
    "        [30, \"3\"],"
    "        [40, \"4\"],"
    "        [50, \"5\"],"
    "        [60, \"6\"],"
    "        [70, \"7\"],"
    "        [80, \"8\"]"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, SequenceWithSpacingTest)
{
    loadDocument(SPACED_SEQUENCE);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(8, component->getChildCount());
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    int y = 0;
    for (int i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        child->ensureLayout();
        ASSERT_EQ(std::to_string(i+1), child->getChildAt(0)->getCalculated(kPropertyText).asString());
        ASSERT_EQ(Rect(0, y, 1024, 200), child->getCalculated(kPropertyBounds).getRect());
        y += 200 + (i + 2) * 10;
    }
}

TEST_F(FlexboxTest, SequenceWithSpacingTestEnsureJump)
{
    loadDocument(SPACED_SEQUENCE);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(8, component->getChildCount());
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Calling ensure of the last child should make all of the others appear
    component->getChildAt(7)->ensureLayout();

    int y = 0;
    for (int i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(std::to_string(i+1), child->getChildAt(0)->getCalculated(kPropertyText).asString());
        ASSERT_EQ(Rect(0, y, 1024, 200), child->getCalculated(kPropertyBounds).getRect());
        y += 200 + (i + 2) * 10;
    }
}

const static char *PAGER_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Pager\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": \"100%\","
    "        \"height\": \"100%\""
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, PagerTest)
{
    loadDocument(PAGER_TEST);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(3, component->getChildCount());
    ASSERT_EQ(kComponentTypePager, component->getType());

    for (int i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(0, 0, 1024, 800), child->getCalculated(kPropertyBounds).getRect());
    }
}

const static char *ALIGNMENT_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"width\": 100.3,"
    "          \"height\": 100.3"
    "        },"
    "        {"
    "          \"type\": \"Frame\","
    "          \"width\": \"100.6dp\","
    "          \"height\": \"100.6dp\""
    "        },"
    "        {"
    "          \"type\": \"Frame\","
    "          \"width\": \"100px\","
    "          \"height\": \"100px\""
    "        },"
    "        {"
    "          \"type\": \"Frame\","
    "          \"width\": \"25vw\","
    "          \"height\": \"25vh\""
    "        },"
    "        {"
    "          \"type\": \"Frame\","
    "          \"width\": \"25%\","
    "          \"height\": \"25%\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FlexboxTest, AlignmentTest)
{
    metrics.dpi(320.0);
    loadDocument(ALIGNMENT_TEST);
    ASSERT_EQ(Rect(0,0,512,400), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(5, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 0, 100.5, 100.5), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(0, 100.5, 100.5, 100.5), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);
    ASSERT_EQ(Rect(0, 201, 50, 50), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(3);
    ASSERT_EQ(Rect(0, 251, 128, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(4);
    ASSERT_EQ(Rect(0, 351, 128, 100), child->getCalculated(kPropertyBounds).getRect());
}

TEST_F(FlexboxTest, AlignmentTestReverse)
{
    metrics.dpi(80.0);
    loadDocument(ALIGNMENT_TEST);
    ASSERT_EQ(Rect(0,0,2048,1600), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(5, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(0, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(0, 100, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);
    ASSERT_EQ(Rect(0, 200, 200, 200), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(3);
    ASSERT_EQ(Rect(0, 400, 512, 400), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(4);
    ASSERT_EQ(Rect(0, 800, 512, 400), child->getCalculated(kPropertyBounds).getRect());
}


// TODO: Test dynamically changing all properties that trigger a re-layout.
// TODO: Test minWidth, minHeight
// TODO: Test maxWidth, maxHeight
// TODO: Test that in relative mode, the left/top/right/bottom are ignored (this is hard!)
// TODO: Remove the dirty event - I don't think it's useful
// TODO: Check the input of scroll position - remember that this is in DP
// TODO: Check the default sizes of all components on non-160 dpi screens
// TODO: Check to ensure that everything we send to the view host layer is in DP
