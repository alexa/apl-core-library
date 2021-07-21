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

#include "gtest/gtest.h"

#include "apl/content/metrics.h"
#include "apl/engine/builder.h"
#include "apl/engine/rootcontext.h"
#include "apl/time/sequencer.h"

#include "../testeventloop.h"

namespace apl {

class SetStateTest : public DocumentWrapper {};

static const char *DATA = "{"
                          "\"title\": \"Pecan Pie V\""
                          "}";

static const char *BASIC_STATE_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"base\": {"
    "      \"values\": ["
    "        {"
    "          \"color\": \"red\","
    "          \"fontStyle\": \"normal\""
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"color\": \"blue\","
    "          \"fontStyle\": \"italic\""
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"abc\","
    "        \"style\": \"base\","
    "        \"text\": \"One\","
    "        \"fontSize\": \"22px\","
    "        \"inheritParentState\": true"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(SetStateTest, BasicStateChange)
{
    loadDocument(BASIC_STATE_TEST, DATA);
    ASSERT_TRUE(component);

    auto text = context->findComponentById("abc");
    ASSERT_TRUE(text);

    auto map = text->getCalculated();
    ASSERT_EQ("One", map["text"].asString());
    ASSERT_EQ(Object(Dimension(22)), map["fontSize"]);
    ASSERT_EQ(Object(Color(Color::RED)), map[kPropertyColor]);
    ASSERT_EQ(Object(kFontStyleNormal), map[kPropertyFontStyle]);

    // Pressing should change the color, karaoke non-color and the font style of the child (inheritParentState=true)
    component->setState(kStatePressed, true);
    ASSERT_TRUE(CheckDirty(text, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke, kPropertyColor, kPropertyFontStyle));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(CheckState(text, kStatePressed));

    ASSERT_EQ(Object(kFontStyleItalic), text->getCalculated(kPropertyFontStyle));
    ASSERT_EQ(Object(Color(Color::BLUE)), text->getCalculated(kPropertyColor));
    ASSERT_EQ(Object(Color(Color::BLUE)), text->getCalculated(kPropertyColorKaraokeTarget));
    ASSERT_EQ(Object(Color(Color::BLUE)), text->getCalculated(kPropertyColorNonKaraoke));

    // Now toggle a completely unrepresented state
    component->setState(kStateKaraoke, true);
    ASSERT_TRUE(CheckDirty(root));
    ASSERT_TRUE(CheckState(text, kStatePressed, kStateKaraoke));

    // And return back to the normal state
    component->setState(kStatePressed, false);
    component->setState(kStateKaraoke, false);
    ASSERT_TRUE(CheckDirty(text, kPropertyColor, kPropertyFontStyle, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(CheckState(text));
    ASSERT_EQ(Object(kFontStyleNormal), text->getCalculated(kPropertyFontStyle));
    ASSERT_EQ(Object(Color(Color::RED)), text->getCalculated(kPropertyColor));
    ASSERT_EQ(Object(Color(Color::RED)), text->getCalculated(kPropertyColorNonKaraoke));
    ASSERT_EQ(Object(Color(Color::RED)), text->getCalculated(kPropertyColorKaraokeTarget));

    // Try to explicitly set state on the child fails because of the inheritParentState value
    ASSERT_FALSE(ConsoleMessage());
    std::static_pointer_cast<CoreComponent>(text)->setState(kStatePressed, true);
    ASSERT_TRUE(CheckDirty(root));
    ASSERT_TRUE(CheckState(text));
    ASSERT_TRUE(ConsoleMessage());

    // Explicitly set the color and then try changing state
    std::dynamic_pointer_cast<CoreComponent>(text)->setProperty(kPropertyColor, Color(0x112233ff));
    component->setState(kStatePressed, true);
    ASSERT_TRUE(CheckDirty(text, kPropertyFontStyle, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(CheckState(text, kStatePressed));
    ASSERT_EQ(Object(kFontStyleItalic), text->getCalculated(kPropertyFontStyle));
    ASSERT_EQ(Object(Color(0x112233ff)), text->getCalculated(kPropertyColor));
    ASSERT_EQ(Object(Color(0x112233ff)), text->getCalculated(kPropertyColorKaraokeTarget));
    ASSERT_EQ(Object(Color(0x112233ff)), text->getCalculated(kPropertyColorNonKaraoke));
}

TEST_F(SetStateTest, StateAndPropertyChange)
{
    loadDocument(BASIC_STATE_TEST, DATA);
    ASSERT_TRUE(component);

    auto text = context->findComponentById("abc");
    ASSERT_TRUE(text);

    // Explicitly set the color and then try changing state
    std::dynamic_pointer_cast<CoreComponent>(text)->setProperty(kPropertyColor, Color(0x112233ff));
    ASSERT_TRUE(CheckDirty(text, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke));
    ASSERT_TRUE(CheckDirty(root, text));

    // The color was overridden, so only the font style will change
    component->setState(kStatePressed, true);
    ASSERT_TRUE(CheckDirty(text, kPropertyFontStyle));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(CheckState(text, kStatePressed));
}

// Set the property to the SAME value as it currently is, then change the state.
TEST_F(SetStateTest, PropertyMatchesState)
{
    loadDocument(BASIC_STATE_TEST, DATA);
    ASSERT_TRUE(component);

    auto text = context->findComponentById("abc");
    ASSERT_TRUE(text);

    // Explicitly set the color to the existing color
    Object origColor = text->getCalculated(kPropertyColor);
    std::dynamic_pointer_cast<CoreComponent>(text)->setProperty(kPropertyColor, origColor);
    // Because the value didn't change, we should not get a dirty flag
    ASSERT_TRUE(CheckDirty(root));

    // Now change the state.  The color should remain the same
    component->setState(kStatePressed, true);
    ASSERT_TRUE(CheckDirty(text, kPropertyFontStyle));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_EQ(Object(kFontStyleItalic), text->getCalculated(kPropertyFontStyle));
    ASSERT_EQ(origColor, text->getCalculated(kPropertyColor));
}

static const char *STARTING_STATE_WITH_INHERIT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"base\": {"
    "      \"values\": ["
    "        {"
    "          \"color\": \"red\","
    "          \"fontStyle\": \"normal\""
    "        },"
    "        {"
    "          \"when\": \"${state.checked}\","
    "          \"color\": \"blue\","
    "          \"fontStyle\": \"italic\""
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"checked\": true,"
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"abc\","
    "        \"style\": \"base\","
    "        \"text\": \"One\","
    "        \"fontSize\": \"22px\","
    "        \"inheritParentState\": true"
    "      }"
    "    }"
    "  }"
    "}";

// The starting state is checked.  Verify that the child gets the state and the property set
TEST_F(SetStateTest, StartingStateWithInherit)
{
    loadDocument(STARTING_STATE_WITH_INHERIT, DATA);
    ASSERT_TRUE(component);

    auto text = context->findComponentById("abc");
    ASSERT_TRUE(text);

    ASSERT_TRUE(IsEqual(true, component->getCalculated(kPropertyChecked)));
    ASSERT_TRUE(IsEqual(true, text->getCalculated(kPropertyChecked)));
    ASSERT_TRUE(CheckState(component, kStateChecked));
    ASSERT_TRUE(CheckState(text, kStateChecked));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), text->getCalculated(kPropertyColor)));

    // Change the checked state
    component->setProperty(kPropertyChecked, false);
    ASSERT_TRUE(CheckDirty(component, kPropertyChecked));
    ASSERT_TRUE(CheckDirty(text, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
                           kPropertyFontStyle, kPropertyChecked));
    ASSERT_TRUE(CheckDirty(root, component, text));
    ASSERT_TRUE(CheckState(component));
    ASSERT_TRUE(CheckState(text));

    // Change the checked state with SetProperty (they are coupled)
    component->setProperty(kPropertyChecked, true);
    ASSERT_TRUE(CheckDirty(component, kPropertyChecked));
    ASSERT_TRUE(CheckDirty(text, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
                           kPropertyFontStyle, kPropertyChecked));
    ASSERT_TRUE(CheckDirty(root, component, text));
    ASSERT_TRUE(CheckState(component, kStateChecked));
    ASSERT_TRUE(CheckState(text, kStateChecked));
}

static const char * INHERITED_STYLES =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"touchWrapperStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"opacity\": 1.0"
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"opacity\": 0.5"
    "        }"
    "      ]"
    "    },"
    "    \"textStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"opacity\": 0"
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"opacity\": 1"
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"id\": \"myTouchWrapper\","
    "      \"style\": \"touchWrapperStyle\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"myText\","
    "        \"style\": \"textStyle\","
    "        \"inheritParentState\": true"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(SetStateTest, InheritedStyles)
{
    loadDocument(INHERITED_STYLES, DATA);
    ASSERT_TRUE(component);

    auto touch = context->findComponentById("myTouchWrapper");
    auto text = context->findComponentById("myText");

    ASSERT_EQ(1.0, touch->getCalculated(kPropertyOpacity).asNumber());
    ASSERT_EQ(0.0, text->getCalculated(kPropertyOpacity).asNumber());

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(1,1)));
    ASSERT_TRUE(CheckDirty(touch, kPropertyOpacity, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(text, kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(root, text, touch));
    ASSERT_TRUE(CheckState(touch, kStatePressed));
    ASSERT_TRUE(CheckState(text, kStatePressed));

    ASSERT_EQ(0.5, touch->getCalculated(kPropertyOpacity).asNumber());
    ASSERT_EQ(1, text->getCalculated(kPropertyOpacity).asNumber());
}

static const char *INHERITED_DEEP =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"touchWrapperStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"opacity\": 1.0"
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"opacity\": 0.5"
    "        }"
    "      ]"
    "    },"
    "    \"textStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"opacity\": 0"
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"opacity\": 1"
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"id\": \"myTouchWrapper\","
    "      \"style\": \"touchWrapperStyle\","
    "      \"items\": {"
    "        \"type\": \"Container\","
    "        \"id\": \"myContainer\","
    "        \"inheritParentState\": true,"
    "        \"items\": {"
    "          \"type\": \"Text\","
    "          \"id\": \"myText\","
    "          \"style\": \"textStyle\","
    "          \"inheritParentState\": true"
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(SetStateTest, InheritedDeepStyles)
{
    loadDocument(INHERITED_DEEP, DATA);
    ASSERT_TRUE(component);

    auto touch = context->findComponentById("myTouchWrapper");
    auto text = context->findComponentById("myText");
    auto container = context->findComponentById("myContainer");

    ASSERT_EQ(1.0, touch->getCalculated(kPropertyOpacity).asNumber());
    ASSERT_EQ(0.0, text->getCalculated(kPropertyOpacity).asNumber());

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(1,1)));
    ASSERT_TRUE(CheckDirty(touch, kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(text, kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(root, touch, text, container));
    ASSERT_TRUE(CheckState(touch, kStatePressed));
    ASSERT_TRUE(CheckState(text, kStatePressed));

    ASSERT_EQ(0.5, touch->getCalculated(kPropertyOpacity).asNumber());
    ASSERT_EQ(1, text->getCalculated(kPropertyOpacity).asNumber());
}

}  // namespace apl
