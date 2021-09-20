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

#include "../testeventloop.h"

namespace apl {

class SetValueTest : public DocumentWrapper {};

static const char *DATA = "{"
                          "\"title\": \"Pecan Pie V\""
                          "}";

static const char *SIMPLE_TEXT = "{"
                                  "  \"type\": \"APL\","
                                  "  \"version\": \"1.0\","
                                  "  \"theme\": \"dark\","
                                  "  \"mainTemplate\": {"
                                  "    \"parameters\": ["
                                  "      \"payload\""
                                  "    ],"
                                  "    \"item\": {"
                                  "      \"id\": \"abc\","
                                  "      \"type\": \"Text\""
                                  "    }"
                                  "  }"
                                  "}";

TEST_F(SetValueTest, SimpleText)
{
    loadDocument(SIMPLE_TEXT, DATA);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(400, component->getCalculated(kPropertyFontWeight)));

    component->setProperty(kPropertyText, "Bear");

    // The text component should be dirty
    ASSERT_TRUE(CheckDirty(component, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(IsEqual("Bear", component->getCalculated(kPropertyText).asString()));

    // Now we set text twice and color once
    component->setProperty(kPropertyText, "Fuzzy");
    component->setProperty(kPropertyText, "Fozzie");
    component->setProperty(kPropertyColor, "green");

    ASSERT_TRUE(CheckDirty(component, kPropertyText, kPropertyColor, kPropertyColorNonKaraoke,
                           kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(IsEqual("Fozzie", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColorNonKaraoke)));
}

TEST_F(SetValueTest, NonDynamicProperty)
{
    loadDocument(SIMPLE_TEXT, DATA);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(400, component->getCalculated(kPropertyFontWeight)));

    ASSERT_FALSE(ConsoleMessage());

    component->setProperty("foo", "Dummy");
    ASSERT_TRUE(ConsoleMessage());

    component->setProperty(kPropertyLetterSpacing, "2dp");
    ASSERT_TRUE(ConsoleMessage());

    // Nothing should be dirty
    ASSERT_TRUE(CheckDirty(root));
}

static const char *SET_VALUE_WITH_STYLE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"base\": {"
    "      \"values\": ["
    "        {"
    "          \"color\": \"red\","
    "          \"fontStyle\": \"normal\""
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"id\": \"abc\","
    "      \"style\": \"base\","
    "      \"text\": \"One\","
    "      \"fontSize\": \"22px\""
    "    }"
    "  }"
    "}";

TEST_F(SetValueTest, StyledProperty)
{
    loadDocument(SET_VALUE_WITH_STYLE, DATA);
    ASSERT_TRUE(component);

    // auto map = component->getCalculated();
    ASSERT_TRUE(IsEqual("One", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Dimension(22), component->getCalculated(kPropertyFontSize)));
    ASSERT_TRUE(IsEqual(Color(0xff0000ff), component->getCalculated(kPropertyColor)));
    ASSERT_EQ(Object(kFontStyleNormal), component->getCalculated(kPropertyFontStyle));

    // Set a dynamic property that was already set
    component->setProperty(kPropertyText, "Two");
    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(component, kPropertyText, kPropertyVisualHash));
    ASSERT_EQ("Two", component->getCalculated(kPropertyText).asString());

    // Now set a dynamic property that was set by a style
    component->setProperty(kPropertyColor, "#1234");
    ASSERT_TRUE(CheckDirty(component, kPropertyColor, kPropertyColorNonKaraoke, kPropertyColorKaraokeTarget,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(IsEqual(Color(0x11223344), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(0x11223344), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Color(0x11223344), component->getCalculated(kPropertyColorNonKaraoke)));

    root->clearDirty();
}

const char *ON_PRESS_HANDLER =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"base\": {"
    "      \"values\": ["
    "        {"
    "          \"color\": \"red\""
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"color\": \"blue\""
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
    "      \"onPress\": {"
    "        \"type\": \"SetValue\","
    "        \"componentId\": \"abc\","
    "        \"property\": \"text\","
    "        \"value\": \"Two\""
    "      },"
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"abc\","
    "        \"style\": \"base\","
    "        \"text\": \"One\","
    "        \"inheritParentState\": true"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(SetValueTest, SimulatePressEvent)
{
    loadDocument(ON_PRESS_HANDLER, DATA);
    ASSERT_TRUE(component);

    auto text = context->findComponentById("abc");
    ASSERT_TRUE(text);

    auto map = text->getCalculated();
    ASSERT_EQ("One", map[kPropertyText].asString());
    ASSERT_EQ(Object(Color(0xff0000ff)), map[kPropertyColor]);

    ASSERT_TRUE(component->hasProperty(kPropertyOnPress));

    // First, tap down
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(1,1)));
    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_EQ(Object(Color(0x0000ffff)), text->getCalculated(kPropertyColor));
    root->clearDirty();

    // Next, release the tap
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(1,1)));
    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_EQ(Object(Color(0xff0000ff)), text->getCalculated(kPropertyColor));
    loop->advanceToEnd();
    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_EQ("Two", text->getCalculated(kPropertyText).asString());

}

const char *ON_PRESS_HANDLER_CHECKED =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"base\": {"
    "      \"values\": ["
    "        {"
    "          \"color\": \"red\""
    "        },"
    "        {"
    "          \"when\": \"${state.checked}\","
    "          \"color\": \"blue\""
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
    "      \"onPress\": {"
    "        \"type\": \"SetValue\","
    "        \"property\": \"checked\","
    "        \"value\": \"${!event.source.value}\""
    "      },"
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"abc\","
    "        \"style\": \"base\","
    "        \"text\": \"One\","
    "        \"inheritParentState\": true"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(SetValueTest, SimulateCheckedEvent)
{
    loadDocument(ON_PRESS_HANDLER_CHECKED, DATA);
    ASSERT_TRUE(component);

    auto text = context->findComponentById("abc");
    ASSERT_TRUE(text);

    ASSERT_EQ(Object(Color(Color::BLUE)), text->getCalculated(kPropertyColor));
    ASSERT_TRUE(CheckState(text, kStateChecked));

    performTap(1,1);
    ASSERT_TRUE(CheckDirty(text, kPropertyColor, kPropertyChecked, kPropertyColorKaraokeTarget,
                           kPropertyColorNonKaraoke, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component, kPropertyChecked));
    ASSERT_TRUE(CheckDirty(root, text, component));
    ASSERT_EQ(Object(Color(Color::RED)), text->getCalculated(kPropertyColor));
    ASSERT_TRUE(CheckState(text));
    ASSERT_TRUE(CheckState(component));

    // This should toggle it again
    performTap(1,1);
    ASSERT_TRUE(CheckDirty(text, kPropertyColor, kPropertyChecked, kPropertyColorKaraokeTarget,
                           kPropertyColorNonKaraoke, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component, kPropertyChecked));
    ASSERT_TRUE(CheckDirty(root, text, component));
    ASSERT_EQ(Object(Color(Color::BLUE)), text->getCalculated(kPropertyColor));
    ASSERT_TRUE(CheckState(text,kStateChecked));
    ASSERT_TRUE(CheckState(component, kStateChecked));
}


// TODO: Check the Yoga layout properties

}  // namespace apl
