/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "testeventloop.h"

#include "apl/engine/focusmanager.h"

using namespace apl;

class FocusManagerTest : public DocumentWrapper {};

static const char *FOCUS_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": [],"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"thing1\","
    "          \"width\": 20,"
    "          \"height\": 20"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"thing2\","
    "          \"width\": 20,"
    "          \"height\": 20"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";


TEST_F(FocusManagerTest, ManualControl)
{
    loadDocument(FOCUS_TEST);
    auto thing1 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing1"));
    auto thing2 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing2"));
    ASSERT_TRUE(thing1);
    ASSERT_TRUE(thing2);

    ASSERT_FALSE(thing1->getState().get(kStateFocused));
    ASSERT_FALSE(thing2->getState().get(kStateFocused));

    auto& fm = root->context().focusManager();
    ASSERT_FALSE(fm.getFocus());

    fm.setFocus(thing1, true);
    ASSERT_TRUE(thing1->getState().get(kStateFocused));
    ASSERT_FALSE(thing2->getState().get(kStateFocused));
    ASSERT_EQ(thing1, fm.getFocus());
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(thing1, event.getComponent());

    fm.setFocus(thing2, true);
    ASSERT_FALSE(thing1->getState().get(kStateFocused));
    ASSERT_TRUE(thing2->getState().get(kStateFocused));
    ASSERT_EQ(thing2, fm.getFocus());
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(thing2, event.getComponent());

    fm.clearFocus(true);
    ASSERT_FALSE(thing1->getState().get(kStateFocused));
    ASSERT_FALSE(thing2->getState().get(kStateFocused));
    ASSERT_FALSE(fm.getFocus());
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_FALSE(event.getComponent());

    thing1->update(kUpdateTakeFocus, 1);
    ASSERT_TRUE(thing1->getState().get(kStateFocused));
    ASSERT_FALSE(thing2->getState().get(kStateFocused));
    ASSERT_EQ(thing1, fm.getFocus());
    ASSERT_FALSE(root->hasEvent());

    thing1->update(kUpdateTakeFocus, 1);
    ASSERT_TRUE(thing1->getState().get(kStateFocused));
    ASSERT_FALSE(thing2->getState().get(kStateFocused));
    ASSERT_EQ(thing1, fm.getFocus());
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(FocusManagerTest, ManualControlDontNotifyViewhost)
{
    loadDocument(FOCUS_TEST);
    auto thing1 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing1"));
    auto thing2 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing2"));
    ASSERT_TRUE(thing1);
    ASSERT_TRUE(thing2);

    ASSERT_FALSE(thing1->getState().get(kStateFocused));
    ASSERT_FALSE(thing2->getState().get(kStateFocused));

    auto& fm = root->context().focusManager();
    ASSERT_FALSE(fm.getFocus());

    fm.setFocus(thing1, false);
    ASSERT_TRUE(thing1->getState().get(kStateFocused));
    ASSERT_FALSE(thing2->getState().get(kStateFocused));
    ASSERT_EQ(thing1, fm.getFocus());
    ASSERT_FALSE(root->hasEvent());

    fm.setFocus(thing2, false);
    ASSERT_FALSE(thing1->getState().get(kStateFocused));
    ASSERT_TRUE(thing2->getState().get(kStateFocused));
    ASSERT_EQ(thing2, fm.getFocus());
    ASSERT_FALSE(root->hasEvent());

    fm.clearFocus(false);
    ASSERT_FALSE(thing1->getState().get(kStateFocused));
    ASSERT_FALSE(thing2->getState().get(kStateFocused));
    ASSERT_FALSE(fm.getFocus());
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(FocusManagerTest, ClearCheck)
{
    loadDocument(FOCUS_TEST);
    auto thing1 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing1"));
    auto thing2 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing2"));
    ASSERT_TRUE(thing1);
    ASSERT_TRUE(thing2);

    ASSERT_TRUE(CheckState(thing1));
    ASSERT_TRUE(CheckState(thing2));

    auto& fm = root->context().focusManager();
    ASSERT_FALSE(fm.getFocus());

    fm.clearFocus(true);
    ASSERT_FALSE(fm.getFocus());
    ASSERT_FALSE(root->hasEvent());

    // Switch focus to thing1
    thing1->update(kUpdateTakeFocus, 100);
    ASSERT_EQ(thing1, fm.getFocus());
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CheckState(thing1, kStateFocused));
    ASSERT_TRUE(CheckState(thing2));

    // Tell thing2 to release focus
    thing2->update(kUpdateTakeFocus, 0);
    ASSERT_EQ(thing1, fm.getFocus());
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CheckState(thing1, kStateFocused));
    ASSERT_TRUE(CheckState(thing2));

    // Tell thing1 to release focus
    thing1->update(kUpdateTakeFocus, 0);
    ASSERT_FALSE(fm.getFocus());
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CheckState(thing1));
    ASSERT_TRUE(CheckState(thing2));
}

static const char *BLUR_FOCUS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"data\": ["
    "        1,"
    "        2"
    "      ],"
    "      \"items\": ["
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"thing${data}\","
    "          \"onFocus\": {"
    "            \"type\": \"SetValue\","
    "            \"componentId\": \"frame${data}\","
    "            \"property\": \"borderColor\","
    "            \"value\": \"red\""
    "          },"
    "          \"onBlur\": {"
    "            \"type\": \"SetValue\","
    "            \"componentId\": \"frame${data}\","
    "            \"property\": \"borderColor\","
    "            \"value\": \"black\""
    "          },"
    "          \"item\": {"
    "            \"type\": \"Frame\","
    "            \"id\": \"frame${data}\","
    "            \"borderColor\": \"black\","
    "            \"borderWidth\": 1"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}"
;


TEST_F(FocusManagerTest, BlurFocus)
{
    loadDocument(BLUR_FOCUS);

    auto thing1 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing1"));
    auto thing2 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing2"));
    ASSERT_TRUE(thing1);
    ASSERT_TRUE(thing2);

    auto frame1 = root->context().findComponentById("frame1");
    auto frame2 = root->context().findComponentById("frame2");
    ASSERT_TRUE(frame1);
    ASSERT_TRUE(frame2);

    ASSERT_TRUE(CheckState(thing1));
    ASSERT_TRUE(CheckState(thing2));

    auto& fm = root->context().focusManager();
    ASSERT_FALSE(fm.getFocus());

    // Switch focus to thing1
    thing1->update(kUpdateTakeFocus, 100);
    ASSERT_EQ(thing1, fm.getFocus());

    ASSERT_FALSE(root->hasEvent());

    // Verify that thing1 now has focus and the border width was set
    ASSERT_TRUE(CheckState(thing1, kStateFocused));
    ASSERT_TRUE(CheckState(thing2));
    ASSERT_TRUE(IsEqual(Color(Color::RED), frame1->getCalculated(kPropertyBorderColor)));
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBorderColor));
    ASSERT_TRUE(CheckDirty(root, frame1));

    // Switch focus to thing2
    thing2->update(kUpdateTakeFocus, 100);
    ASSERT_EQ(thing2, fm.getFocus());

    ASSERT_FALSE(root->hasEvent());

    // Verify that thing1 has dropped focus and has a black border; thing2 has focus and a red border
    ASSERT_TRUE(CheckState(thing1));
    ASSERT_TRUE(CheckState(thing2, kStateFocused));
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), frame1->getCalculated(kPropertyBorderColor)));
    ASSERT_TRUE(IsEqual(Color(Color::RED), frame2->getCalculated(kPropertyBorderColor)));
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBorderColor));
    ASSERT_TRUE(CheckDirty(frame2, kPropertyBorderColor));
    ASSERT_TRUE(CheckDirty(root, frame1, frame2));

    // Now remove the focus
    thing2->update(kUpdateTakeFocus, 0);
    ASSERT_EQ(nullptr, fm.getFocus());

    ASSERT_FALSE(root->hasEvent());

    // Verify that thing2 has dropped focus and has a black border
    ASSERT_TRUE(CheckState(thing1));
    ASSERT_TRUE(CheckState(thing2));
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), frame1->getCalculated(kPropertyBorderColor)));
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), frame2->getCalculated(kPropertyBorderColor)));
    ASSERT_TRUE(CheckDirty(frame2, kPropertyBorderColor));
    ASSERT_TRUE(CheckDirty(root, frame2));
}


static const char *FOCUS_EVENT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onFocus\": {"
    "        \"type\": \"SetValue\","
    "        \"componentId\": \"frame\","
    "        \"property\": \"text\","
    "        \"value\": \"${event.source.handler}:${event.source.focused}\""
    "      },"
    "      \"onBlur\": {"
    "        \"type\": \"SetValue\","
    "        \"componentId\": \"frame\","
    "        \"property\": \"text\","
    "        \"value\": \"${event.source.handler}:${event.source.focused}\""
    "      },"
    "      \"item\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"frame\""
    "      }"
    "    }"
    "  }"
    "}";

/**
 * Check that the event.source.handler and event.source.focused properties are set
 */
TEST_F(FocusManagerTest, FocusEvent)
{
    loadDocument(FOCUS_EVENT);

    auto& fm = root->context().focusManager();
    auto text = root->context().findComponentById("frame");
    ASSERT_TRUE(text);
    ASSERT_TRUE(IsEqual("", text->getCalculated(kPropertyText).asString()));

    // Take focus
    component->update(kUpdateTakeFocus, 1);
    ASSERT_TRUE(component->getState().get(kStateFocused));
    ASSERT_EQ(component, fm.getFocus());
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(CheckState(component, kStateFocused));
    ASSERT_TRUE(IsEqual("Focus:true", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root,text));

    // Drop focus
    component->update(kUpdateTakeFocus, 0);
    ASSERT_FALSE(component->getState().get(kStateFocused));
    ASSERT_EQ(nullptr, fm.getFocus());
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(CheckState(component));
    ASSERT_TRUE(IsEqual("Blur:false", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root,text));
}