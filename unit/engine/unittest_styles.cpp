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

#include <iostream>

#include "rapidjson/document.h"
#include "gtest/gtest.h"

#include "apl/engine/evaluate.h"
#include "apl/engine/rootcontext.h"
#include "apl/engine/styles.h"
#include "apl/content/metrics.h"
#include "apl/engine/resources.h"
#include "apl/content/content.h"
#include "apl/primitives/dimension.h"
#include "apl/engine/context.h"

#include "../testeventloop.h"

using namespace apl;

class StylesTest : public DocumentWrapper {};

const char *TEST_DATA =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\""
    "    }"
    "  },"
    "  \"resources\": ["
    "    {"
    "      \"description\": \"Stock colors for the light theme\","
    "      \"colors\": {"
    "        \"colorBackground\": \"#F0F1EF\","
    "        \"colorAccent\": \"#0070ba\","
    "        \"colorTextPrimary\": \"#151920\""
    "      }"
    "    },"
    "    {"
    "      \"description\": \"Stock colors for the dark theme\","
    "      \"when\": \"${viewport.theme == 'dark'}\","
    "      \"colors\": {"
    "        \"colorBackground\": \"#151920\","
    "        \"colorAccent\": \"#00caff\","
    "        \"colorTextPrimary\": \"#f0f1ef\""
    "      }"
    "    },"
    "    {"
    "      \"description\": \"Standard font sizes\","
    "      \"dimensions\": {"
    "        \"textSizeClock\": 84,"
    "        \"textSizeDisplay\": 120"
    "      }"
    "    }"
    "  ],"
    "  \"styles\": {"
    "    \"textStyleBase\": {"
    "      \"description\": \"Base font description; set color and core font family\","
    "      \"values\": ["
    "        {"
    "          \"color\": \"@colorTextPrimary\","
    "          \"fontFamily\": \"Amazon Ember\""
    "        },"
    "        {"
    "          \"when\": \"${state.karaoke}\","
    "          \"color\": \"@colorAccent\""
    "        }"
    "      ]"
    "    },"
    "    \"textStyleBase0\": {"
    "      \"description\": \"Thin version of basic font\","
    "      \"extend\": \"textStyleBase\","
    "      \"values\": {"
    "        \"fontWeight\": 100"
    "      }"
    "    },"
    "    \"textStyleBase1\": {"
    "      \"description\": \"Light version of basic font\","
    "      \"extend\": \"textStyleBase\","
    "      \"values\": {"
    "        \"fontWeight\": 300"
    "      }"
    "    },"
    "    \"mixinDisplay\": {"
    "      \"values\": {"
    "        \"fontSize\": \"@textSizeDisplay\""
    "      }"
    "    },"
    "    \"mixinClock\": {"
    "      \"values\": {"
    "        \"fontSize\": \"@textSizeClock\""
    "      }"
    "    },"
    "    \"textStyleDisplay0\": {"
    "      \"extend\": ["
    "        \"textStyleBase0\","
    "        \"mixinDisplay\""
    "      ]"
    "    },"
    "    \"textStyleDisplay1\": {"
    "      \"extend\": ["
    "        \"textStyleBase1\","
    "        \"mixinDisplay\""
    "      ]"
    "    },"
    "    \"textStyleClock0\": {"
    "      \"extend\": ["
    "        \"textStyleBase0\","
    "        \"mixinClock\""
    "      ]"
    "    },"
    "    \"textStyleClock1\": {"
    "      \"extend\": ["
    "        \"textStyleBase1\","
    "        \"mixinClock\""
    "      ]"
    "    }"
    "  }"
    "}";

// Useful utility method for style dumping.
//static void dump(std::ostream& os, StyleInstancePtr ptr)
//{
//    streamer s;
//    for (const auto& kv : *ptr) {
//        s << kv.first << ": " << kv.second << "";
//    }
//    os << s.str();
//}

TEST_F(StylesTest, Basic)
{
    loadDocument(TEST_DATA);

    ASSERT_EQ(9, root->info().count(Info::kInfoTypeStyle));

    State state;
    auto base = context->getStyle("textStyleBase", state);

    ASSERT_TRUE(base);
    ASSERT_EQ(2, base->size());
    ASSERT_TRUE(base->find("fontFamily") != base->end());
    ASSERT_EQ(Object("Amazon Ember"), base->at("fontFamily"));
    ASSERT_TRUE(base->at("color").isColor());
    ASSERT_EQ(0xf0f1efff, base->at("color").getColor());

    // Sanity check that path values match rapidjson Pointer architecture
    ASSERT_STREQ(followPath(base->provenance("color"))->GetString(), "@colorTextPrimary");

    ASSERT_STREQ("_main/styles/textStyleBase/values/0/color", base->provenance("color").c_str());
    ASSERT_STREQ("_main/styles/textStyleBase/values/0/fontFamily", base->provenance("fontFamily").c_str());

    state.toggle(kStateKaraoke);
    base = context->getStyle("textStyleBase", state);

    ASSERT_EQ(2, base->size());
    ASSERT_EQ(Object("Amazon Ember"), base->at("fontFamily"));
    ASSERT_EQ(0x00caffff, base->at("color").getColor());

    ASSERT_STREQ("_main/styles/textStyleBase/values/1/color", base->provenance("color").c_str());
    ASSERT_STREQ("_main/styles/textStyleBase/values/0/fontFamily", base->provenance("fontFamily").c_str());
}

static const std::map<std::string, std::string> EXPECTED = {
    { "textStyleBase", "_main/styles/textStyleBase" },
    { "textStyleBase0", "_main/styles/textStyleBase0" },
    { "textStyleBase1", "_main/styles/textStyleBase1" },
    { "mixinDisplay", "_main/styles/mixinDisplay" },
    { "mixinClock", "_main/styles/mixinClock" },
    { "textStyleDisplay0", "_main/styles/textStyleDisplay0" },
    { "textStyleDisplay1", "_main/styles/textStyleDisplay1" },
    { "textStyleClock0", "_main/styles/textStyleClock0" },
    { "textStyleClock1", "_main/styles/textStyleClock1" },
};

TEST_F(StylesTest, BasicInfo)
{
    loadDocument(TEST_DATA);

    auto count = root->info().count(Info::kInfoTypeStyle);
    ASSERT_EQ(EXPECTED.size(), count);

    for (int i = 0 ; i < count ; i++) {
        auto s = root->info().at(Info::kInfoTypeStyle, i);
        auto it = EXPECTED.find(s.first);
        ASSERT_NE(it, EXPECTED.end());
        ASSERT_EQ(it->second, s.second);
    }
}

TEST_F(StylesTest, Override)
{
    loadDocument(TEST_DATA);

    State state;
    auto base = context->getStyle("textStyleClock1", state);

    ASSERT_EQ(4, base->size());

    auto fontSize = base->at("fontSize");
    ASSERT_TRUE(fontSize.isDimension());
    ASSERT_EQ(Object(Dimension(84)), fontSize);

    ASSERT_EQ(300, base->at("fontWeight").asNumber());
    ASSERT_EQ(0xf0f1efff, base->at("color").getColor());
    ASSERT_EQ(Object("Amazon Ember"), base->at("fontFamily"));

    ASSERT_STREQ("_main/styles/textStyleBase/values/0/color", base->provenance("color").c_str());
    ASSERT_STREQ("_main/styles/textStyleBase/values/0/fontFamily", base->provenance("fontFamily").c_str());
    ASSERT_STREQ("_main/styles/textStyleBase1/values/fontWeight", base->provenance("fontWeight").c_str());
    ASSERT_STREQ("_main/styles/mixinClock/values/fontSize", base->provenance("fontSize").c_str());

    // Switch to karaoke mode
    state.toggle(kStateKaraoke);
    base = context->getStyle("textStyleClock1", state);

    ASSERT_EQ(4, base->size());

    fontSize = base->at("fontSize");
    ASSERT_TRUE(fontSize.isDimension());
    ASSERT_EQ(Object(Dimension(84)), fontSize);

    ASSERT_EQ(300, base->at("fontWeight").asNumber());
    ASSERT_EQ(0x00caffff, base->at("color").getColor());
    ASSERT_EQ(Object("Amazon Ember"), base->at("fontFamily"));

    ASSERT_STREQ("_main/styles/textStyleBase/values/1/color", base->provenance("color").c_str());
    ASSERT_STREQ("_main/styles/textStyleBase/values/0/fontFamily", base->provenance("fontFamily").c_str());
    ASSERT_STREQ("_main/styles/textStyleBase1/values/fontWeight", base->provenance("fontWeight").c_str());
    ASSERT_STREQ("_main/styles/mixinClock/values/fontSize", base->provenance("fontSize").c_str());
}

const char *LOOP =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\""
    "    }"
    "  },"
    "  \"styles\": {"
    "    \"a\": {"
    "      \"extends\": \"b\","
    "      \"values\": {"
    "        \"label\": \"a\","
    "        \"extra\": \"a\""
    "      }"
    "    },"
    "    \"b\": {"
    "      \"extends\": \"c\","
    "      \"values\": {"
    "        \"label\": \"b\","
    "        \"extra\": \"b\","
    "        \"bonus\": \"bonus\""
    "      }"
    "    },"
    "    \"c\": {"
    "      \"extends\": \"b\","
    "      \"values\": {"
    "        \"label\": \"c\","
    "        \"extra\": \"c\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(StylesTest, Loop)
{
    loadDocument(LOOP);

    ASSERT_TRUE(ConsoleMessage());  // A warning is issued for the circular definition

    // The loop doesn't prevent styles from working
    ASSERT_EQ(3, root->info().count(Info::kInfoTypeStyle));

    State state;
    auto style_a = context->getStyle("a", state);
    ASSERT_TRUE(style_a != nullptr);
    ASSERT_EQ(Object("a"), style_a->at("extra"));
    ASSERT_EQ(Object("bonus"), style_a->at("bonus"));

    auto style_b = context->getStyle("b", state);
    ASSERT_TRUE(style_b != nullptr);
    ASSERT_EQ(Object("b"), style_b->at("extra"));
    ASSERT_EQ(Object("bonus"), style_b->at("bonus"));

    auto style_c = context->getStyle("c", state);
    ASSERT_TRUE(style_c != nullptr);
    ASSERT_EQ(Object("c"), style_c->at("extra"));
    ASSERT_EQ(style_c->end(), style_c->find("bonus"));   // The loop prevents c from inheriting from b
}

const char *COMPONENTS_STYLING =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"styles\": {"
        "    \"styleOverride\": {"
        "      \"values\": ["
        "        {"
        "          \"opacity\": 1"
        "        },"
        "        {"
        "          \"when\": \"${state.checked}\","
        "          \"opacity\": 0.5,"
        "          \"display\": \"invisible\","
        "          "
        "          \"align\": \"bottom\","
        "          \"borderRadius\": 7,"
        "          \"overlayColor\": \"red\","
        "          \"overlayGradient\": {"
        "            \"type\": \"linear\","
        "            \"colorRange\": ["
        "              \"blue\","
        "              \"transparent\""
        "            ],"
        "            \"inputRange\": ["
        "              0,"
        "              0.4"
        "            ]"
        "          },"
        "          \"scale\": \"best-fill\","
        ""
        "          \"color\": \"red\","
        "          \"fontFamily\": \"comic-sans\","
        "          \"fontSize\": \"20dp\","
        "          \"fontStyle\": \"italic\","
        "          \"fontWeight\": \"bold\","
        "          \"letterSpacing\": \"1dp\","
        "          \"lineHeight\": \"1.5\","
        "          \"maxLines\": \"2\","
        "          \"textAlign\": \"center\","
        "          \"textAlignVertical\": \"center\","
        "          \"text\": \"Styled text.\","
        ""
        "          \"backgroundColor\": \"green\","
        "          \"borderBottomLeftRadius\": \"1dp\","
        "          \"borderBottomRightRadius\": \"2dp\","
        "          \"borderColor\": \"blue\","
        "          \"borderTopLeftRadius\": \"4dp\","
        "          \"borderTopRightRadius\": \"3dp\","
        "          \"borderWidth\": \"2dp\""
        "        }"
        "      ]"
        "    }"
        "  },"
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"TouchWrapper\","
        "      \"id\": \"myTouchWrapper\","
        "      \"style\": \"styleOverride\","
        "      \"items\": {"
        "        \"type\": \"Container\","
        "        \"inheritParentState\": true,"
        "        \"items\": ["
        "          {"
        "            \"type\": \"Image\","
        "            \"id\": \"image\","
        "            \"source\": \"http://images.amazon.com/image/foo.png\","
        "            \"style\": \"styleOverride\","
        "            \"inheritParentState\": true"
        "          },"
        "          {"
        "            \"type\": \"Text\","
        "            \"text\": \"Text.\","
        "            \"id\": \"text\","
        "            \"style\": \"styleOverride\","
        "            \"inheritParentState\": true"
        "          },"
        "          {"
        "            \"type\": \"Frame\","
        "            \"id\": \"frame\","
        "            \"style\": \"styleOverride\","
        "            \"inheritParentState\": true"
        "          },"
        "          {"
        "            \"type\": \"VectorGraphic\","
        "            \"source\": \"iconWifi3\","
        "            \"id\": \"vectorGraphic\","
        "            \"style\": \"styleOverride\","
        "            \"inheritParentState\": true"
        "          }"
        "        ]"
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(StylesTest, ComponentStyling)
{
    loadDocument(COMPONENTS_STYLING);

    ASSERT_EQ(kComponentTypeTouchWrapper, component->getType());
    ASSERT_EQ(1.0, component->getCalculated(kPropertyOpacity).asNumber());
    ASSERT_EQ(kDisplayNormal, component->getCalculated(kPropertyDisplay).asInt());

    auto container = component->getCoreChildAt(0);
    ASSERT_EQ(kComponentTypeContainer, container->getType());

    auto image = container->getCoreChildAt(0);
    ASSERT_EQ(kComponentTypeImage, image->getType());
    ASSERT_EQ(kImageAlignCenter, image->getCalculated(kPropertyAlign).asInt());
    ASSERT_EQ(0, image->getCalculated(kPropertyBorderRadius).asDimension(*context).getValue());
    ASSERT_EQ(Color(), image->getCalculated(kPropertyOverlayColor).getColor());
    ASSERT_TRUE(image->getCalculated(kPropertyOverlayGradient).isNull());
    ASSERT_EQ(kImageScaleBestFit, image->getCalculated(kPropertyScale).asInt());

    auto text = container->getCoreChildAt(1);
    ASSERT_EQ(kComponentTypeText, text->getType());
    // TODO: Text color is static at the moment.
    ASSERT_EQ(Color(0xfafafaff), text->getCalculated(kPropertyColor).getColor());
    ASSERT_EQ("sans-serif", text->getCalculated(kPropertyFontFamily).asString());
    ASSERT_EQ(40, text->getCalculated(kPropertyFontSize).asDimension(*context).getValue());
    ASSERT_EQ(kFontStyleNormal, text->getCalculated(kPropertyFontStyle).asInt());
    ASSERT_EQ(400, text->getCalculated(kPropertyFontWeight).asInt());
    ASSERT_EQ(0, text->getCalculated(kPropertyLetterSpacing).asDimension(*context).getValue());
    ASSERT_EQ(1.25, text->getCalculated(kPropertyLineHeight).asNumber());
    ASSERT_EQ(0, text->getCalculated(kPropertyMaxLines).asInt());
    ASSERT_EQ(kTextAlignAuto, text->getCalculated(kPropertyTextAlign).asInt());
    ASSERT_EQ(kTextAlignVerticalAuto, text->getCalculated(kPropertyTextAlignVertical).asInt());

    auto frame = container->getCoreChildAt(2);
    ASSERT_EQ(kComponentTypeFrame, frame->getType());
    ASSERT_EQ(Color(), frame->getCalculated(kPropertyBackgroundColor).getColor());
    ASSERT_TRUE(frame->getCalculated(kPropertyBorderBottomLeftRadius).isNull());
    ASSERT_TRUE(frame->getCalculated(kPropertyBorderBottomRightRadius).isNull());
    ASSERT_TRUE(frame->getCalculated(kPropertyBorderTopLeftRadius).isNull());
    ASSERT_TRUE(frame->getCalculated(kPropertyBorderTopRightRadius).isNull());
    ASSERT_EQ(Color(), frame->getCalculated(kPropertyBorderColor).getColor());
    ASSERT_EQ(0, frame->getCalculated(kPropertyBorderRadius).asDimension(*context).getValue());

    auto vectorGraphic = container->getCoreChildAt(3);
    ASSERT_EQ(kComponentTypeVectorGraphic, vectorGraphic->getType());
    ASSERT_EQ(kVectorGraphicAlignCenter, vectorGraphic->getCalculated(kPropertyAlign).asInt());
    ASSERT_EQ(kGraphicScaleNone, vectorGraphic->getCalculated(kPropertyScale).asInt());

    component->setState(kStateChecked, true);

    ASSERT_EQ(0.5, component->getCalculated(kPropertyOpacity).asNumber());
    ASSERT_EQ(kDisplayInvisible, component->getCalculated(kPropertyDisplay).asInt());

    ASSERT_EQ(kImageAlignBottom, image->getCalculated(kPropertyAlign).asInt());
    ASSERT_EQ(7, image->getCalculated(kPropertyBorderRadius).asDimension(*context).getValue());
    ASSERT_EQ(Color(session, "red"), image->getCalculated(kPropertyOverlayColor).getColor());
    ASSERT_FALSE(image->getCalculated(kPropertyOverlayGradient).isNull());
    ASSERT_EQ(kImageScaleBestFill, image->getCalculated(kPropertyScale).asInt());

    ASSERT_EQ("Text.", text->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Color(session, "red"), text->getCalculated(kPropertyColor).getColor());
    ASSERT_EQ("comic-sans", text->getCalculated(kPropertyFontFamily).asString());
    ASSERT_EQ(20, text->getCalculated(kPropertyFontSize).asDimension(*context).getValue());
    ASSERT_EQ(kFontStyleItalic, text->getCalculated(kPropertyFontStyle).asInt());
    ASSERT_EQ(700, text->getCalculated(kPropertyFontWeight).asInt());
    ASSERT_EQ(1, text->getCalculated(kPropertyLetterSpacing).asDimension(*context).getValue());
    ASSERT_EQ(1.5, text->getCalculated(kPropertyLineHeight).asNumber());
    ASSERT_EQ(2, text->getCalculated(kPropertyMaxLines).asInt());
    ASSERT_EQ(kTextAlignCenter, text->getCalculated(kPropertyTextAlign).asInt());
    ASSERT_EQ(kTextAlignVerticalCenter, text->getCalculated(kPropertyTextAlignVertical).asInt());

    ASSERT_EQ(Color(session, "green"), frame->getCalculated(kPropertyBackgroundColor).getColor());
    ASSERT_EQ(1, frame->getCalculated(kPropertyBorderBottomLeftRadius).asDimension(*context).getValue());
    ASSERT_EQ(2, frame->getCalculated(kPropertyBorderBottomRightRadius).asDimension(*context).getValue());
    ASSERT_EQ(4, frame->getCalculated(kPropertyBorderTopLeftRadius).asDimension(*context).getValue());
    ASSERT_EQ(3, frame->getCalculated(kPropertyBorderTopRightRadius).asDimension(*context).getValue());
    ASSERT_EQ(Color(session, "blue"), frame->getCalculated(kPropertyBorderColor).getColor());
    ASSERT_EQ(7, frame->getCalculated(kPropertyBorderRadius).asDimension(*context).getValue());

    ASSERT_EQ(kVectorGraphicAlignBottom, vectorGraphic->getCalculated(kPropertyAlign).asInt());
    ASSERT_EQ(kVectorGraphicScaleBestFill, vectorGraphic->getCalculated(kPropertyScale).asInt());
}
