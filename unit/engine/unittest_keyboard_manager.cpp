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

#include <algorithm>

#include "apl/time/sequencer.h"

#include "../testeventloop.h"
#include "apl/component/component.h"
#include "apl/component/pagercomponent.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/focus/focusmanager.h"
#include "apl/primitives/object.h"
#include "gtest/gtest.h"

using namespace apl;

class KeyboardManagerTest : public CommandTest {
public:
    Keyboard BLUE_KEY = Keyboard("KeyB", "b");
    Keyboard GREEN_KEY = Keyboard("KeyG", "g");
    Keyboard YELLOW_KEY = Keyboard("KeyY", "y");
    Keyboard NO_KEY = Keyboard("NO", "NO");

    Keyboard W_KEY = Keyboard("KeyW", "w");
    Keyboard A_KEY = Keyboard("KeyA", "a");
    Keyboard S_KEY = Keyboard("KeyS", "s");
    Keyboard D_KEY = Keyboard("KeyD", "d");

    void setFocus(const CoreComponentPtr& focusComponent) {
        auto& fm = root->context().focusManager();
        fm.setFocus(focusComponent, false);
        ASSERT_EQ(focusComponent, fm.getFocus());
    }
};

static const char* COMPONENT_KEY_HANDLER_DOC = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": [
      {
        "type": "TouchWrapper",
        "handleKeyUp": [
          {
            "when": "${event.keyboard.code == 'KeyG'}",
            "propagate": true,
            "commands": [
              {
                "type": "SetValue",
                "property": "backgroundColor",
                "value": "green",
                "componentId": "testFrame"
              }
            ]
          }
        ],
        "handleKeyDown": [
          {
            "when": "${event.keyboard.code == 'KeyB'}",
            "propagate": true,
            "commands": [
              {
                "type": "SetValue",
                "property": "backgroundColor",
                "value": "blue",
                "componentId": "testFrame"
              }
            ]
          },
          {
            "when": "${event.keyboard.code == 'Enter'}",
            "description": "Block the normal 'enter' behavior"
          }
        ],
        "item": {
          "type": "Frame",
          "id": "testFrame",
          "backgroundColor": "red"
        }
      }
    ]
  }
})";

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

static const char* DOCUMENT_KEY_HANDLER_DOC = R"({
  "type": "APL",
  "version": "1.1",
  "handleKeyUp": [
    {
      "when": "${event.keyboard.code == 'KeyG'}",
      "commands": [
        {
          "type": "SetValue",
          "property": "backgroundColor",
          "value": "green",
          "componentId": "testFrame"
        }
      ]
    }
  ],
  "handleKeyDown": [
    {
      "when": "${event.keyboard.code == 'KeyB'}",
      "commands": [
        {
          "type": "SetValue",
          "property": "backgroundColor",
          "value": "blue",
          "componentId": "testFrame"
        }
      ]
    },
    {
      "when": "${event.keyboard.code == 'Enter'}",
      "description": "Block the normal 'enter' behavior"
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "id": "testFrame",
      "backgroundColor": "red"
    }
  }
})";

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

static const char* PROPAGATE_KEY_HANDLER_DOC = R"({
  "type": "APL",
  "version": "1.1",
  "handleKeyUp": [
    {
      "when": "${event.keyboard.code == 'KeyG'}",
      "commands": [
        {
          "type": "SetValue",
          "property": "backgroundColor",
          "value": "green",
          "componentId": "testFrame"
        }
      ]
    }
  ],
  "handleKeyDown": [
    {
      "when": "${event.keyboard.code == 'KeyY'}",
      "commands": [
        {
          "type": "SetValue",
          "property": "backgroundColor",
          "value": "yellow",
          "componentId": "testFrame"
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "id": "thing0",
      "width": 20,
      "height": 20,
      "handleKeyDown": [
        {
          "when": "${event.keyboard.code == 'KeyB'}",
          "commands": [
            {
              "type": "SetValue",
              "property": "backgroundColor",
              "value": "blue",
              "componentId": "testFrame"
            }
          ]
        },
        {
          "when": "${event.keyboard.code == 'KeyW'}",
          "commands": [
            {
              "type": "SetValue",
              "property": "backgroundColor",
              "value": "white",
              "componentId": "testFrame"
            }
          ]
        }
      ],
      "item": {
        "type": "Container",
        "width": "100%",
        "height": "100%",
        "items": [
          {
            "type": "TouchWrapper",
            "id": "thing1",
            "width": 20,
            "height": 20
          },
          {
            "type": "TouchWrapper",
            "id": "thing2",
            "width": 20,
            "height": 20,
            "handleKeyDown": [
              {
                "when": "${event.keyboard.code == 'Enter'}",
                "description": "Block the normal 'enter' behavior"
              }
            ]
          },
          {
            "type": "Frame",
            "id": "testFrame",
            "backgroundColor": "red"
          }
        ]
      }
    }
  }
})";

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

static const char*  RESERVED_UNHANDLED = R"({
  "type": "APL",
  "version": "1.1",
  "handleKeyUp": [
    {
      "when": "${event.keyboard.code == 'BrowserBack'}"
    },
    {
      "when": "${event.keyboard.code == 'Enter'}"
    },
    {
      "when": "${event.keyboard.code == 'Tab'}"
    },
    {
      "when": "${event.keyboard.code == 'Tab' && event.keyboard.shift == true}"
    },
    {
      "when": "${event.keyboard.code == 'ArrowUp'}"
    },
    {
      "when": "${event.keyboard.code == 'ArrowDown'}"
    },
    {
      "when": "${event.keyboard.code == 'ArrowRight'}"
    },
    {
      "when": "${event.keyboard.code == 'ArrowLeft'}"
    },
    {
      "when": "${event.keyboard.code == 'PageUp'}"
    },
    {
      "when": "${event.keyboard.code == 'PageDown'}"
    },
    {
      "when": "${event.keyboard.code == 'Home'}"
    },
    {
      "when": "${event.keyboard.code == 'End'}"
    }
  ],
  "handleKeyDown": [
    {
      "when": "${event.keyboard.code == 'BrowserBack'}"
    },
    {
      "when": "${event.keyboard.code == 'Enter'}"
    },
    {
      "when": "${event.keyboard.code == 'Tab'}"
    },
    {
      "when": "${event.keyboard.code == 'Tab' && event.keyboard.shift == true}"
    },
    {
      "when": "${event.keyboard.code == 'ArrowUp'}"
    },
    {
      "when": "${event.keyboard.code == 'ArrowDown'}"
    },
    {
      "when": "${event.keyboard.code == 'ArrowRight'}"
    },
    {
      "when": "${event.keyboard.code == 'ArrowLeft'}"
    },
    {
      "when": "${event.keyboard.code == 'PageUp'}"
    },
    {
      "when": "${event.keyboard.code == 'PageDown'}"
    },
    {
      "when": "${event.keyboard.code == 'Home'}"
    },
    {
      "when": "${event.keyboard.code == 'End'}"
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "id": "testFrame",
      "backgroundColor": "red"
    }
  }
})";

/**
  * Test that all intrinsic keys are blocked from evaluation
  */
TEST_F(KeyboardManagerTest, ReservedNotConsumed) {
    loadDocument(RESERVED_UNHANDLED);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::BACK_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::ENTER_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::NUMPAD_ENTER_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::PAGE_UP_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::PAGE_DOWN_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::HOME_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyDown, Keyboard::END_KEY()));

    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::BACK_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::ENTER_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::NUMPAD_ENTER_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::PAGE_UP_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::PAGE_DOWN_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::HOME_KEY()));
    ASSERT_FALSE(root->handleKeyboard(kKeyUp, Keyboard::END_KEY()));
}

static const char *DEFAULT_COMPONENT_WHEN_TRUE = R"({
  "type": "APL",
  "version": "1.3",
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "items": {
        "type": "Text",
        "text": "Not set",
        "id": "TestId"
      },
      "handleKeyDown": [
        {
          "commands": {
            "type": "SetValue",
            "componentId": "TestId",
            "property": "text",
            "value": "Is Set"
          }
        }
      ]
    }
  }
})";

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

static const char *DEFAULT_WHEN_TRUE = R"({
  "type": "APL",
  "version": "1.3",
  "handleKeyDown": [
    {
      "commands": {
        "type": "SetValue",
        "componentId": "TestId",
        "property": "text",
        "value": "Is Set"
      }
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text",
      "text": "Not set",
      "id": "TestId"
    }
  }
})";

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

static const char *ACCESS_ENVIRONMENT_IN_COMPONENT = R"({
  "type": "APL",
  "version": "1.3",
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "items": {
        "type": "Text",
        "text": "Not set",
        "id": "TestId"
      },
      "handleKeyDown": [
        {
          "commands": {
            "type": "SetValue",
            "componentId": "TestId",
            "property": "text",
            "value": "${event.keyboard.code} is set"
          }
        }
      ],
      "handleKeyUp": [
        {
          "commands": {
            "type": "SetValue",
            "componentId": "TestId",
            "property": "text",
            "value": "${event.keyboard.code} is not set"
          }
        }
      ]
    }
  }
})";

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

static const char *ACCESS_ENVIRONMENT_VALUES = R"({
  "type": "APL",
  "version": "1.3",
  "handleKeyDown": [
    {
      "commands": {
        "type": "SetValue",
        "componentId": "TestId",
        "property": "text",
        "value": "${event.keyboard.code} is set"
      }
    }
  ],
  "handleKeyUp": [
    {
      "commands": {
        "type": "SetValue",
        "componentId": "TestId",
        "property": "text",
        "value": "${event.keyboard.code} is not set"
      }
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text",
      "text": "Not set",
      "id": "TestId"
    }
  }
})";

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

static const char *ACCESS_ENVIRONMENT_AND_PAYLOAD = R"({
  "type": "APL",
  "version": "1.3",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "Text",
      "id": "MyText",
      "text": "${payload.start}"
    }
  },
  "handleKeyDown": {
    "commands": {
      "type": "SetValue",
      "componentId": "MyText",
      "property": "text",
      "value": "${event.keyboard.code} ${payload.end}"
    }
  }
})";

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

static const char *ARROW_KEYS_CONTROLLING_AVG = R"({
  "type": "APL",
  "version": "1.4",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "width": 50,
      "height": 50,
      "parameters": [
        "focuscolor"
      ],
      "items": [
        {
          "type": "path",
          "stroke": "${focuscolor}",
          "strokeWidth": "5",
          "pathData": "M 0,0 50,0 50,50 0,50 0,0"
        }
      ]
    },
    "BoxedTurtle": {
      "type": "AVG",
      "version": "1.1",
      "width": 400,
      "height": 400,
      "parameters": [
        "tx",
        "ty",
        "focuscolor"
      ],
      "items": [
        {
          "type": "path",
          "stroke": "${focuscolor}",
          "strokeWidth": "1",
          "pathData": "M 0,0 400,0 400,400 0,400 0,0"
        },
        {
          "type": "group",
          "transform": "translate(${tx} ${ty}) ",
          "items": [
            {
              "type": "path",
              "stroke": "#00ff00ff",
              "strokeWidth": 2,
              "pathData": "M 40,12 a 10,10 0 1,1 20,0 a 10,10 0 1,1 -20,0"
            },
            {
              "type": "path",
              "stroke": "#00ff00ff",
              "strokeWidth": 2,
              "pathData": "M 45,80 a 5,5 0 1,1 10,0 a 5,10 0 1,1 -10,0"
            },
            {
              "type": "path",
              "stroke": "#00ff00ff",
              "strokeWidth": 2,
              "pathData": "M 15,30 a 10,10 0 1,1 20,0 a 10,10 0 1,1 -20,0"
            },
            {
              "type": "path",
              "stroke": "#00ff00ff",
              "strokeWidth": 2,
              "pathData": "M 65,30 a 10,10 0 1,1 20,0 a 10,10 0 1,1 -20,0"
            },
            {
              "type": "path",
              "stroke": "#00ff00ff",
              "strokeWidth": 2,
              "pathData": "M 65,65 a 10,10 0 1,1 20,0 a 10,10 0 1,1 -20,0"
            },
            {
              "type": "path",
              "stroke": "#00ff00ff",
              "strokeWidth": 2,
              "pathData": "M 15,65 a 10,10 0 1,1 20,0 a 10,10 0 1,1 -20,0"
            },
            {
              "type": "path",
              "stroke": "#00ff00ff",
              "fill": "black",
              "strokeWidth": 3,
              "pathData": "M 25, 50 a 25,30 0 1,1 50,0 a 25,30 0 1,1 -50,0"
            }
          ]
        }
      ]
    }
  },
  "styles": {
    "focusStyle": {
      "values": [
        {
          "focuscolor": "white"
        },
        {
          "when": "${state.focused}",
          "focuscolor": "red"
        }
      ]
    }
  },
  "layouts": {
    "Box": {
      "item": {
        "type": "VectorGraphic",
        "width": "50dp",
        "height": "50dp",
        "style": "focusStyle",
        "source": "box"
      }
    }
  },
  "onMount": {
    "type": "SetFocus",
    "componentId": "vg"
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "height": 500,
        "width": 500,
        "direction": "column",
        "items": [
          {
            "type": "Box",
            "position": "absolute",
            "top": 0,
            "left": 225
          },
          {
            "type": "Box",
            "position": "absolute",
            "top": 225,
            "left": 0
          },
          {
            "type": "Box",
            "position": "absolute",
            "top": 450,
            "left": 225
          },
          {
            "type": "Box",
            "position": "absolute",
            "top": 225,
            "left": 450
          },
          {
            "type": "VectorGraphic",
            "id": "vg",
            "style": "focusStyle",
            "bind": [
              {
                "name": "xshift",
                "type": "number",
                "value": 150
              },
              {
                "name": "yshift",
                "type": "number",
                "value": 150
              }
            ],
            "width": 400,
            "height": 400,
            "source": "BoxedTurtle",
            "tx": "${xshift}",
            "ty": "${yshift}",
            "position": "absolute",
            "top": 50,
            "left": 50,
            "handleKeyDown": [
              {
                "when": "${(event.keyboard.code == 'KeyD' || event.keyboard.code == 'ArrowRight') && xshift < 300}",
                "commands": {
                  "type": "SetValue",
                  "property": "xshift",
                  "value": "${xshift + 50}"
                }
              },
              {
                "when": "${(event.keyboard.code == 'KeyA' || event.keyboard.code == 'ArrowLeft') && xshift > 0}",
                "commands": {
                  "type": "SetValue",
                  "property": "xshift",
                  "value": "${xshift - 50}"
                }
              },
              {
                "when": "${(event.keyboard.code == 'KeyW' || event.keyboard.code == 'ArrowUp') && yshift > 0}",
                "commands": {
                  "type": "SetValue",
                  "property": "yshift",
                  "value": "${yshift - 50}"
                }
              },
              {
                "when": "${(event.keyboard.code == 'KeyS' || event.keyboard.code == 'ArrowDown') && yshift < 300}",
                "commands": {
                  "type": "SetValue",
                  "property": "yshift",
                  "value": "${yshift + 50}"
                }
              }
            ]
          }
        ]
      }
    ]
  }
})";

/**
 * Test that a document-level keyboard event can access the payload.
 */
TEST_F(KeyboardManagerTest, ArrowKeysForAvg)
{
    loadDocument(ARROW_KEYS_CONTROLLING_AVG);
    ASSERT_TRUE(component);

    auto vg = std::static_pointer_cast<CoreComponent>(component->findComponentById("vg"));
    ASSERT_EQ(kComponentTypeVectorGraphic, vg->getType());

    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(vg, event.getComponent());

    auto group = vg->getCalculated(kPropertyGraphic).getGraphic()->getRoot()->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());

    auto transform = group->getValue(kGraphicPropertyTransform).getTransform2D();
    ASSERT_EQ(Transform2D::translate(150, 150), transform);


    ASSERT_TRUE(root->handleKeyboard(kKeyDown, D_KEY));
    ASSERT_TRUE(root->handleKeyboard(kKeyDown, D_KEY));
    ASSERT_TRUE(root->handleKeyboard(kKeyDown, W_KEY));
    ASSERT_TRUE(root->handleKeyboard(kKeyDown, W_KEY));
    ASSERT_TRUE(root->handleKeyboard(kKeyDown, A_KEY));
    ASSERT_TRUE(root->handleKeyboard(kKeyDown, S_KEY));

    transform = group->getValue(kGraphicPropertyTransform).getTransform2D();
    ASSERT_EQ(Transform2D::translate(200, 100), transform);

    ASSERT_TRUE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY()));
    ASSERT_TRUE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY()));
    ASSERT_TRUE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY()));
    ASSERT_TRUE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY()));
    ASSERT_TRUE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY()));
    ASSERT_TRUE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY()));

    transform = group->getValue(kGraphicPropertyTransform).getTransform2D();
    ASSERT_EQ(Transform2D::translate(250, 50), transform);

    ASSERT_TRUE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY()));
    transform = group->getValue(kGraphicPropertyTransform).getTransform2D();
    ASSERT_EQ(Transform2D::translate(300, 50), transform);

    // Actually passed to focus nav
    ASSERT_TRUE(root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY()));
    transform = group->getValue(kGraphicPropertyTransform).getTransform2D();
    ASSERT_EQ(Transform2D::translate(300, 50), transform);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_FALSE(event.getComponent());
    event.getActionRef().resolve(true);
    root->clearPending();
}
