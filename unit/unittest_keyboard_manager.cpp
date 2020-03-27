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

#include <algorithm>

#include "apl/time/sequencer.h"

#include "gtest/gtest.h"
#include "apl/engine/focusmanager.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/component/component.h"
#include "apl/primitives/object.h"
#include "apl/component/pagercomponent.h"
#include "testeventloop.h"


using namespace apl;

class KeyboardManagerTest : public CommandTest {
public:
    Keyboard BLUE_KEY = Keyboard("KeyB", "b");
    Keyboard GREEN_KEY = Keyboard("KeyG", "g");
    Keyboard YELLOW_KEY = Keyboard("KeyY", "y");
    Keyboard NO_KEY = Keyboard("NO", "NO");

    void setFocus(const CoreComponentPtr& focusComponent) {
        auto& fm = root->context().focusManager();
        fm.setFocus(focusComponent, false);
        ASSERT_EQ(focusComponent, fm.getFocus());
    }
};

static const char* COMPONENT_KEY_HANDLER_DOC =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": ["
        "      {"
        "        \"type\": \"TouchWrapper\","
        "        \"handleKeyUp\": ["
        "          {"
        "            \"when\": \"${event.keyboard.code == 'KeyG'}\","
        "            \"propagate\": true,"
        "            \"commands\": ["
        "              {"
        "                \"type\": \"SetValue\","
        "                \"property\": \"backgroundColor\","
        "                \"value\": \"green\","
        "                \"componentId\": \"testFrame\""
        "              }"
        "            ]"
        "          }"
        "        ],"
        "        \"handleKeyDown\": ["
        "          {"
        "            \"when\": \"${event.keyboard.code == 'KeyB'}\","
        "            \"propagate\": true,"
        "            \"commands\": ["
        "              {"
        "                \"type\": \"SetValue\","
        "                \"property\": \"backgroundColor\","
        "                \"value\": \"blue\","
        "                \"componentId\": \"testFrame\""
        "              }"
        "            ]"
        "          },"
        "          {"
        "            \"when\": \"${event.keyboard.code == 'Enter'}\","
        "            \"description\": \"Block the normal 'enter' behavior\""
        "          }"
        "        ],"
        "        \"item\": {"
        "          \"type\": \"Frame\","
        "          \"id\": \"testFrame\","
        "          \"backgroundColor\": \"red\""
        "        }"
        "      }"
        "    ]"
        "  }"
        "}";

/**
 * Test that RootContext targets the focused component.
 */
TEST_F(KeyboardManagerTest, ComponentWithFocus) {

    loadDocument(COMPONENT_KEY_HANDLER_DOC);
    ASSERT_TRUE(component);

    // set the focused component
    setFocus(component);

    // update component with key press
    root->handleKeyboard(kKeyDown, BLUE_KEY);
    ASSERT_TRUE(root->isDirty());

    // verify target component changed
    auto target = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("testFrame"));
    ASSERT_TRUE(target);
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), target->getCalculated(kPropertyBackgroundColor)));
}


/**
 * Test that KeyboardManager does nothing when there is no focus.
 */
TEST_F(KeyboardManagerTest, ComponentNoFocus) {

    loadDocument(COMPONENT_KEY_HANDLER_DOC);
    ASSERT_TRUE(component);

    // send keypress without focus component
    ASSERT_NO_FATAL_FAILURE(root->handleKeyboard(kKeyDown, Keyboard::ENTER_KEY()));

    // verify no changes
    ASSERT_FALSE(root->isDirty());
}

/**
 * Test that a when clause validates to true.
 */
TEST_F(KeyboardManagerTest, WhenIsTrue) {

    loadDocument(COMPONENT_KEY_HANDLER_DOC);
    ASSERT_TRUE(component);

    // set the focused component
    setFocus(component);

    // verify initial state of the command target component
    auto target = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("testFrame"));
    ASSERT_TRUE(target);
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

    // update component with key press
    root->handleKeyboard(kKeyDown, BLUE_KEY);
    // verify down command was executed
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), target->getCalculated(kPropertyBackgroundColor)));

    // update component with key press
    root->handleKeyboard(kKeyUp, GREEN_KEY);
    // verify up command was executed
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), target->getCalculated(kPropertyBackgroundColor)));
}

TEST_F(KeyboardManagerTest, WhenIsFalse) {
    loadDocument(COMPONENT_KEY_HANDLER_DOC);
    ASSERT_TRUE(component);

    // set the focused component
    setFocus(component);

    // verify initial state of the command target component
    auto target = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("testFrame"));
    ASSERT_TRUE(target);
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

    auto badKey = Keyboard("BadKey", "BadKey");

    // send invalid key for down
    root->handleKeyboard(kKeyDown, badKey);
    // verify state unchanged
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

    // send invalid key for up
    root->handleKeyboard(kKeyUp, badKey);
    // verify state unchanged
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

    // send valid key, incorrect down
    root->handleKeyboard(kKeyDown, GREEN_KEY);
    // verify state unchanged
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

    // send valid key, incorrect up
    root->handleKeyboard(kKeyUp, BLUE_KEY);
    // verify state unchanged
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));
}

static const char* DOCUMENT_KEY_HANDLER_DOC =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"handleKeyUp\": ["
        "    {"
        "      \"when\": \"${event.keyboard.code == 'KeyG'}\","
        "      \"commands\": ["
        "        {"
        "          \"type\": \"SetValue\","
        "          \"property\": \"backgroundColor\","
        "          \"value\": \"green\","
        "          \"componentId\": \"testFrame\""
        "        }"
        "      ]"
        "    }"
        "  ],"
        "  \"handleKeyDown\": ["
        "    {"
        "      \"when\": \"${event.keyboard.code == 'KeyB'}\","
        "      \"commands\": ["
        "        {"
        "          \"type\": \"SetValue\","
        "          \"property\": \"backgroundColor\","
        "          \"value\": \"blue\","
        "          \"componentId\": \"testFrame\""
        "        }"
        "      ]"
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'Enter'}\","
        "      \"description\": \"Block the normal 'enter' behavior\""
        "    }"
        "  ],"
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Frame\","
        "      \"id\": \"testFrame\","
        "      \"backgroundColor\": \"red\""
        "    }"
        "  }"
        "}";

/**
 * Test that a when clause validates to true for Document.
 */
TEST_F(KeyboardManagerTest, DocumentWhenIsTrue) {

    loadDocument(DOCUMENT_KEY_HANDLER_DOC);
    ASSERT_TRUE(root);

    // verify initial state of the command target component
    auto target = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("testFrame"));
    ASSERT_TRUE(target);
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

    // send valid key down
    root->handleKeyboard(kKeyDown, BLUE_KEY);
    // verify down command was executed
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), target->getCalculated(kPropertyBackgroundColor)));

    // send valid key up
    root->handleKeyboard(kKeyUp, GREEN_KEY);
    // verify up command was executed
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), target->getCalculated(kPropertyBackgroundColor)));
}


TEST_F(KeyboardManagerTest, DocumentWhenIsFalse) {

    loadDocument(DOCUMENT_KEY_HANDLER_DOC);
    ASSERT_TRUE(root);

    // verify initial state of the command target component
    auto target = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("testFrame"));
    ASSERT_TRUE(target);
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

    auto badKey = Keyboard("BadKey", "BadKey");

    // send invalid key for down
    root->handleKeyboard(kKeyDown, badKey);
    // verify state unchanged
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

    // send invalid key for up
    root->handleKeyboard(kKeyUp, badKey);
    // verify state unchanged
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

    // send valid key, incorrect down
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    // verify state unchanged
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

    // send valid key, incorrect up
    root->handleKeyboard(kKeyUp, Keyboard::ARROW_DOWN_KEY());
    // verify state unchanged
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

}

static const char* PROPAGATE_KEY_HANDLER_DOC =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"handleKeyUp\": ["
        "    {"
        "      \"when\": \"${event.keyboard.code == 'KeyG'}\","
        "      \"commands\": ["
        "        {"
        "          \"type\": \"SetValue\","
        "          \"property\": \"backgroundColor\","
        "          \"value\": \"green\","
        "          \"componentId\": \"testFrame\""
        "        }"
        "      ]"
        "    }"
        "  ],"
        "  \"handleKeyDown\": ["
        "    {"
        "      \"when\": \"${event.keyboard.code == 'KeyY'}\","
        "      \"commands\": ["
        "        {"
        "          \"type\": \"SetValue\","
        "          \"property\": \"backgroundColor\","
        "          \"value\": \"yellow\","
        "          \"componentId\": \"testFrame\""
        "        }"
        "      ]"
        "    }"
        "  ],"
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"TouchWrapper\","
        "      \"id\": \"thing0\","
        "      \"width\": 20,"
        "      \"height\": 20,"
        "      \"handleKeyDown\": ["
        "        {"
        "          \"when\": \"${event.keyboard.code == 'KeyB'}\","
        "          \"commands\": ["
        "            {"
        "              \"type\": \"SetValue\","
        "              \"property\": \"backgroundColor\","
        "              \"value\": \"blue\","
        "              \"componentId\": \"testFrame\""
        "            }"
        "          ]"
        "        },"
        "        {"
        "          \"when\": \"${event.keyboard.code == 'KeyW'}\","
        "          \"commands\": ["
        "            {"
        "              \"type\": \"SetValue\","
        "              \"property\": \"backgroundColor\","
        "              \"value\": \"white\","
        "              \"componentId\": \"testFrame\""
        "            }"
        "          ]"
        "        }"
        "      ],"
        "      \"item\": {"
        "        \"type\": \"Container\","
        "        \"width\": \"100%\","
        "        \"height\": \"100%\","
        "        \"items\": ["
        "          {"
        "            \"type\": \"TouchWrapper\","
        "            \"id\": \"thing1\","
        "            \"width\": 20,"
        "            \"height\": 20"
        "          },"
        "          {"
        "            \"type\": \"TouchWrapper\","
        "            \"id\": \"thing2\","
        "            \"width\": 20,"
        "            \"height\": 20,"
        "            \"handleKeyDown\": ["
        "              {"
        "                \"when\": \"${event.keyboard.code == 'Enter'}\","
        "                \"description\": \"Block the normal 'enter' behavior\""
        "              }"
        "            ]"
        "          },"
        "          {"
        "            \"type\": \"Frame\","
        "            \"id\": \"testFrame\","
        "            \"backgroundColor\": \"red\""
        "          }"
        "        ]"
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(KeyboardManagerTest, PropagateToParent) {

    loadDocument(PROPAGATE_KEY_HANDLER_DOC);
    ASSERT_TRUE(component);

    auto thing1 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing1"));
    auto thing2 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing2"));
    ASSERT_TRUE(thing1);
    ASSERT_TRUE(thing2);

    // send a "Blue Key" to the touch wrapper without a key handler
    setFocus(thing1);
    root->handleKeyboard(kKeyDown, BLUE_KEY);

    // verify key update propagated to top Component
    ASSERT_TRUE(root->isDirty());
    auto target = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("testFrame"));
    ASSERT_TRUE(target);
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), target->getCalculated(kPropertyBackgroundColor)));
}

TEST_F(KeyboardManagerTest, PropagateBlock) {

    loadDocument(PROPAGATE_KEY_HANDLER_DOC);
    ASSERT_TRUE(component);

    auto thing1 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing1"));
    auto thing2 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing2"));
    ASSERT_TRUE(thing1);
    ASSERT_TRUE(thing2);

    // send an "Enter" to touch wrapper with handler that has no commands
    setFocus(thing2);
    root->handleKeyboard(kKeyDown, Keyboard::ENTER_KEY());
    ASSERT_FALSE(root->isDirty());

    // verify the key was consumed, and no change in the target component
    auto target = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("testFrame"));
    ASSERT_FALSE(root->isDirty());
    ASSERT_TRUE(target);
    ASSERT_TRUE(IsEqual(Color(Color::RED), target->getCalculated(kPropertyBackgroundColor)));

}

TEST_F(KeyboardManagerTest, PropagateToDocument) {

    loadDocument(PROPAGATE_KEY_HANDLER_DOC);
    ASSERT_TRUE(component);

    auto thing1 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing1"));
    auto thing2 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing2"));
    ASSERT_TRUE(thing1);
    ASSERT_TRUE(thing2);

    // send a "ArrowUp" keyUp to the touch wrapper without matching handler
    setFocus(thing1);
    root->handleKeyboard(kKeyUp, GREEN_KEY);

    // verify key update propagated to Document
    ASSERT_TRUE(root->isDirty());
    auto target = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("testFrame"));
    ASSERT_TRUE(target);
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), target->getCalculated(kPropertyBackgroundColor)));

}


/**
 * Test the RootContext return "consumed" state.
 */
TEST_F(KeyboardManagerTest, Consumed) {

    loadDocument(PROPAGATE_KEY_HANDLER_DOC);
    ASSERT_TRUE(component);

    auto thing1 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing1"));
    auto thing2 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing2"));
    ASSERT_TRUE(thing1);
    ASSERT_TRUE(thing2);
    auto target = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("testFrame"));
    ASSERT_TRUE(target);

    // send an "No Key" keydown to touch wrapper with handler, expect not consumed
    setFocus(thing1);
    auto consumed = root->handleKeyboard(kKeyDown, NO_KEY);
    ASSERT_FALSE(consumed);
    ASSERT_FALSE(root->isDirty());

    // send a "Blue Key" to the touch wrapper without a key handler
    // verify key update propagated and was consumed by top Component
    setFocus(thing1);
    consumed = root->handleKeyboard(kKeyDown, BLUE_KEY);
    ASSERT_TRUE(root->isDirty());
    ASSERT_TRUE(consumed);
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), target->getCalculated(kPropertyBackgroundColor)));

    // send a "Green Key" keyUp to the touch wrapper without matching handler
    // verify key update consumed by Document
    setFocus(thing1);
    consumed = root->handleKeyboard(kKeyUp, GREEN_KEY);
    ASSERT_TRUE(root->isDirty());
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), target->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(consumed);

    // send a "Yellow Key" keyDown to the touch wrapper without matching handler
    // verify key update consumed by Document
    setFocus(thing1);
    consumed = root->handleKeyboard(kKeyDown, YELLOW_KEY);
    ASSERT_TRUE(root->isDirty());
    ASSERT_TRUE(IsEqual(Color(Color::YELLOW), target->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(consumed);
}

static const char*  INTRINSIC_UNHANDLED =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"handleKeyUp\": ["
        "    {"
        "      \"when\": \"${event.keyboard.code == 'BrowserBack'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'Enter'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'Tab'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'Tab' && event.keyboard.shift == true}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'ArrowUp'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'ArrowDown'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'ArrowRight'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'ArrowLeft'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'PageUp'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'PageDown'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'Home'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'End'}\""
        "    }"
        "  ],"
        "  \"handleKeyDown\": ["
        "    {"
        "      \"when\": \"${event.keyboard.code == 'BrowserBack'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'Enter'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'Tab'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'Tab' && event.keyboard.shift == true}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'ArrowUp'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'ArrowDown'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'ArrowRight'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'ArrowLeft'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'PageUp'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'PageDown'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'Home'}\""
        "    },"
        "    {"
        "      \"when\": \"${event.keyboard.code == 'End'}\""
        "    }"
        "  ],"
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Frame\","
        "      \"id\": \"testFrame\","
        "      \"backgroundColor\": \"red\""
        "    }"
        "  }"
        "}";

/**
  * Test that all intrinsic keys are blocked from evaluation
  */
TEST_F(KeyboardManagerTest, IntrinsicNotConsumed) {
    loadDocument(INTRINSIC_UNHANDLED);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::BACK_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::ENTER_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::PAGE_UP_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::PAGE_DOWN_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::HOME_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::END_KEY()));

    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::BACK_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::ENTER_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::TAB_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::SHIFT_TAB_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::ARROW_UP_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::ARROW_DOWN_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::ARROW_LEFT_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::ARROW_RIGHT_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::PAGE_UP_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::PAGE_DOWN_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::HOME_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::END_KEY()));
}

static const char *DEFAULT_COMPONENT_WHEN_TRUE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"text\": \"Not set\","
    "        \"id\": \"TestId\""
    "      },"
    "      \"handleKeyDown\": ["
    "        {"
    "          \"commands\": {"
    "            \"type\": \"SetValue\","
    "            \"componentId\": \"TestId\","
    "            \"property\": \"text\","
    "            \"value\": \"Is Set\""
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

/**
 * Test that the keyboard "when" clause defaults to true for keyboard handler in a component
 */
TEST_F(KeyboardManagerTest, DefaultComponentWhenTrue)
{
    loadDocument(DEFAULT_COMPONENT_WHEN_TRUE);
    ASSERT_TRUE(component);
    auto text = root->context().findComponentById("TestId");
    ASSERT_TRUE(text);

    component->update(kUpdateTakeFocus, 1);
    ASSERT_EQ(component, context->focusManager().getFocus());

    root->handleKeyboard(kKeyDown, BLUE_KEY);

    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("Is Set", text->getCalculated(kPropertyText).asString()));
}

static const char *DEFAULT_WHEN_TRUE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"handleKeyDown\": ["
    "    {"
    "      \"commands\": {"
    "        \"type\": \"SetValue\","
    "        \"componentId\": \"TestId\","
    "        \"property\": \"text\","
    "        \"value\": \"Is Set\""
    "      }"
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"Not set\","
    "      \"id\": \"TestId\""
    "    }"
    "  }"
    "}";

/**
 * Test that the keyboard "when" clause defaults to true
 */
TEST_F(KeyboardManagerTest, DefaultWhenTrue)
{
    loadDocument(DEFAULT_WHEN_TRUE);
    ASSERT_TRUE(component);

    root->handleKeyboard(kKeyDown, BLUE_KEY);

    ASSERT_TRUE(CheckDirty(component, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(IsEqual("Is Set", component->getCalculated(kPropertyText).asString()));
}

static const char *ACCESS_ENVIRONMENT_IN_COMPONENT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"text\": \"Not set\","
    "        \"id\": \"TestId\""
    "      },"
    "      \"handleKeyDown\": ["
    "        {"
    "          \"commands\": {"
    "            \"type\": \"SetValue\","
    "            \"componentId\": \"TestId\","
    "            \"property\": \"text\","
    "            \"value\": \"${event.keyboard.code} is set\""
    "          }"
    "        }"
    "      ],"
    "      \"handleKeyUp\": ["
    "        {"
    "          \"commands\": {"
    "            \"type\": \"SetValue\","
    "            \"componentId\": \"TestId\","
    "            \"property\": \"text\","
    "            \"value\": \"${event.keyboard.code} is not set\""
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

/**
 * Test that keyboard events can access environment variables passed in the key event.
 * This tests if a component-level keyboard handler can access the ${event.keyboard.code} property
 */
TEST_F(KeyboardManagerTest, AccessEnvironmentValuesInComponent)
{
    loadDocument(ACCESS_ENVIRONMENT_IN_COMPONENT);
    ASSERT_TRUE(component);
    auto text = root->context().findComponentById("TestId");
    ASSERT_TRUE(text);

    component->update(kUpdateTakeFocus, 1);
    ASSERT_EQ(component, context->focusManager().getFocus());

    root->handleKeyboard(kKeyDown, BLUE_KEY);

    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("KeyB is set", text->getCalculated(kPropertyText).asString()));

    root->handleKeyboard(kKeyUp, BLUE_KEY);

    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("KeyB is not set", text->getCalculated(kPropertyText).asString()));
}

static const char *ACCESS_ENVIRONMENT_VALUES =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"handleKeyDown\": ["
    "    {"
    "      \"commands\": {"
    "        \"type\": \"SetValue\","
    "        \"componentId\": \"TestId\","
    "        \"property\": \"text\","
    "        \"value\": \"${event.keyboard.code} is set\""
    "      }"
    "    }"
    "  ],"
    "  \"handleKeyUp\": ["
    "    {"
    "      \"commands\": {"
    "        \"type\": \"SetValue\","
    "        \"componentId\": \"TestId\","
    "        \"property\": \"text\","
    "        \"value\": \"${event.keyboard.code} is not set\""
    "      }"
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"Not set\","
    "      \"id\": \"TestId\""
    "    }"
    "  }"
    "}";

/**
 * Test that keyboard events can access environment variables passed in the key event
 * This tests if a document-level keyboard handler can access the ${event.keyboard.code} property
 */
TEST_F(KeyboardManagerTest, AccessEnvironmentValues)
{
    loadDocument(ACCESS_ENVIRONMENT_VALUES);
    ASSERT_TRUE(component);

    root->handleKeyboard(kKeyDown, BLUE_KEY);

    ASSERT_TRUE(CheckDirty(component, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(IsEqual("KeyB is set", component->getCalculated(kPropertyText).asString()));

    root->handleKeyboard(kKeyUp, BLUE_KEY);

    ASSERT_TRUE(CheckDirty(component, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(IsEqual("KeyB is not set", component->getCalculated(kPropertyText).asString()));
}

static const char *ACCESS_ENVIRONMENT_AND_PAYLOAD =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"item\": {"
    "      \"type\": \"Text\","
    "      \"id\": \"MyText\","
    "      \"text\": \"${payload.start}\""
    "    }"
    "  },"
    "  \"handleKeyDown\": {"
    "    \"commands\": {"
    "      \"type\": \"SetValue\","
    "      \"componentId\": \"MyText\","
    "      \"property\": \"text\","
    "      \"value\": \"${event.keyboard.code} ${payload.end}\""
    "    }"
    "  }"
    "}";

/**
 * Test that a document-level keyboard event can access the payload.
 */
TEST_F(KeyboardManagerTest, AccessEnvironmentAndPayload)
{
    loadDocument(ACCESS_ENVIRONMENT_AND_PAYLOAD, R"({"start": "START", "end": "END"})");
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("START", component->getCalculated(kPropertyText).asString()));

    root->handleKeyboard(kKeyDown, BLUE_KEY);
    ASSERT_TRUE(IsEqual("KeyB END", component->getCalculated(kPropertyText).asString()));
}