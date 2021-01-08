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

using namespace apl;

class AccessibilityActionTest : public DocumentWrapper {};

static const char *BASIC_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "Frame",
          "backgroundColor": "green",
          "actions": [
            {
              "name": "MakeRed",
              "label": "Make the background red",
              "commands": {
                "type": "SetValue",
                "property": "backgroundColor",
                "value": "red"
              }
            }
          ]
        }
      }
    }
)apl";

TEST_F(AccessibilityActionTest, Basic)
{
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyBackgroundColor)));

    // Check that the action stored in the component is what we expect
    auto actions = component->getCalculated(kPropertyAccessibilityActions);
    ASSERT_TRUE(actions.isArray());
    ASSERT_EQ(1, actions.size());
    auto action = actions.at(0).getAccessibilityAction();
    ASSERT_STREQ("MakeRed", action->getName().c_str());
    ASSERT_STREQ("Make the background red", action->getLabel().c_str());
    ASSERT_TRUE(action->enabled());

    // Invoke the action and verify that it changes the background color
    component->update(kUpdateAccessibilityAction, "MakeRed");
    ASSERT_TRUE(CheckDirty(component, kPropertyBackgroundColor));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyBackgroundColor)));

    // Invoke a non-existent action
    component->update(kUpdateAccessibilityAction, "DoesNotExist");
    ASSERT_TRUE(CheckDirty(root));
}


static const char *EQUALITY_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "Frame",
          "actions": [
            {
              "name": "MakeRed",
              "label": "Make the background red",
              "commands": {
                "type": "SetValue",
                "property": "backgroundColor",
                "color": "red"
              }
            },
            {
              "name": "MakeGreen",
              "label": "Make the background red",
              "commands": {
                "type": "SetValue",
                "property": "backgroundColor",
                "color": "red"
              }
            },
            {
              "name": "MakeRed",
              "label": "Make the background green",
              "commands": {
                "type": "SetValue",
                "property": "backgroundColor",
                "color": "red"
              }
            },
            {
              "name": "MakeRed",
              "label": "Make the background red",
              "commands": {
                "type": "SetValue",
                "property": "backgroundColor",
                "color": "green"
              }
            },
            {
              "name": "MakeRed",
              "label": "Make the background red",
              "commands": {
                "type": "SetValue",
                "property": "backgroundColor",
                "color": "red"
              }
            }
          ],
          "items": {
            "type": "Frame",
            "actions": {
              "name": "MakeRed",
              "label": "Make the background red",
              "commands": {
                "type": "SetValue",
                "property": "backgroundColor",
                "color": "red"
              }
            }
          }
        }
      }
    }
)apl";

// Verify that accessibility actions can be compared for equality
TEST_F(AccessibilityActionTest, Equality)
{
    loadDocument(EQUALITY_TEST);
    ASSERT_TRUE(component);

    const auto& actions = component->getCalculated(kPropertyAccessibilityActions);
    ASSERT_TRUE(actions.isArray());
    ASSERT_EQ(5, actions.size());

    ASSERT_TRUE(IsEqual(actions.at(0), actions.at(0)));  // An action is equal to itself
    ASSERT_FALSE(IsEqual(actions.at(0), actions.at(1)));
    ASSERT_FALSE(IsEqual(actions.at(0), actions.at(2)));
    ASSERT_FALSE(IsEqual(actions.at(0), actions.at(3)));
    ASSERT_TRUE(IsEqual(actions.at(0), actions.at(4)));  // The last action is a copy of the first

    // The child has an action that looks identical to the top component's action, but it is attached to a different
    // component.
    auto child = component->getChildAt(0);
    ASSERT_TRUE(child);

    const auto& actions2 = child->getCalculated(kPropertyAccessibilityActions);
    ASSERT_TRUE(actions2.isArray());
    ASSERT_EQ(1, actions2.size());

    ASSERT_FALSE(IsEqual(actions.at(0), actions2.at(0)));
}


static const char *MALFORMED_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper"
        }
      }
    }
)apl";

static std::vector<std::string> MALFORMED = {
    R"([])",                   // Not an object
    R"("item")",               // Not an object
    R"({  })",                 // Nothing defined
    R"({ "name": "Fred" })",   // Missing label
    R"({ "name": null, "label": "Null" })",  // Bad name
    R"({ "name": "", "label": "Null" })",  // Bad name
    R"({ "label": "Fred" })",  // Missing name
    R"({ "label": null, "name": "Null" })",  // Bad name
    R"({ "label": "", "name": "Null" })",  // Bad name
};

TEST_F(AccessibilityActionTest, Malformed)
{
    loadDocument(MALFORMED_TEST);
    ASSERT_TRUE(component);

    for (const auto& m : MALFORMED) {
        auto data = JsonData(m);
        ASSERT_TRUE(data);
        auto aa = AccessibilityAction::create(component, Object(data.get()));
        ASSERT_FALSE(aa);
        ASSERT_TRUE(ConsoleMessage());
    }
}


static const char *ACTIVATE_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "bind": {
            "name": "X",
            "value": 0
          },
          "items": {
            "type": "Text",
            "text": "X=${X}"
          },
          "onPress": {
            "type": "SetValue",
            "property": "X",
            "value": "${X+1}"
          },
          "actions": {
            "name": "activate",
            "label": "Activate Test"
          }
        }
      }
    }
)apl";

/*
 * The "activate" accessibility action will use the "onPress" command of a touch wrapper if it does not have
 * any attached commands
 */
TEST_F(AccessibilityActionTest, Activate)
{
    loadDocument(ACTIVATE_TEST);
    ASSERT_TRUE(component);

    auto text = component->getChildAt(0);
    ASSERT_TRUE(text);
    ASSERT_TRUE(IsEqual("X=0", text->getCalculated(kPropertyText).asString()));

    // Verify that the "onPress" command runs normally when the component is pressed
    component->update(kUpdatePressed, 0);  // Toggle the pressed button
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("X=1", text->getCalculated(kPropertyText).asString()));

    // Verify that the action fires
    component->update(kUpdateAccessibilityAction, "activate");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("X=2", text->getCalculated(kPropertyText).asString()));
}


static const char *GESTURE_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "bind": {
            "name": "X",
            "value": "Idle"
          },
          "items": {
            "type": "Text",
            "text": "${X}"
          },
          "gestures": [
            {
              "type": "DoublePress",
              "onDoublePress": {
                "type": "SetValue",
                "property": "X",
                "value": "DPress"
              }
            },
            {
              "type": "LongPress",
              "onLongPressEnd": {
                "type": "SetValue",
                "property": "X",
                "value": "LPress"
              }
            },
            {
              "type": "SwipeAway",
              "direction": "left",
              "onSwipeDone": {
                "type": "SetValue",
                "property": "X",
                "value": "SDone"
              }
            }
          ],
          "actions": [
            {
              "name": "doubletap",
              "label": "DoublePress Test"
            },
            {
              "name": "longpress",
              "label": "LongPress Test"
            },
            {
              "name": "swipeaway",
              "label": "SwipeAway Test"
            }
          ]
        }
      }
    }
)apl";

TEST_F(AccessibilityActionTest, Gestures)
{
    loadDocument(GESTURE_TEST);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);
    ASSERT_TRUE(text);
    ASSERT_TRUE(IsEqual("Idle", text->getCalculated(kPropertyText).asString()));

    component->update(kUpdateAccessibilityAction, "doubletap");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("DPress", text->getCalculated(kPropertyText).asString()));

    component->update(kUpdateAccessibilityAction, "longpress");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("LPress", text->getCalculated(kPropertyText).asString()));

    component->update(kUpdateAccessibilityAction, "swipeaway");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("SDone", text->getCalculated(kPropertyText).asString()));
}


static const char *ACTIONS_WITH_COMMANDS = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "bind": {
            "name": "X",
            "value": "Idle"
          },
          "items": {
            "type": "Text",
            "text": "${X}"
          },
          "onPress": {
            "type": "SetValue",
            "property": "X",
            "value": "OnPress"
          },
          "gestures": [
            {
              "type": "DoublePress",
              "onDoublePress": {
                "type": "SetValue",
                "property": "X",
                "value": "DPress"
              }
            }
          ],
          "actions": [
            {
              "name": "doubletap",
              "label": "DoublePress Test",
              "commands": {
                "type": "SetValue",
                "property": "X",
                "value": "Defined DPress"
              }
            },
            {
              "name": "activate",
              "label": "Activate Test",
              "commands": {
                "type": "SetValue",
                "property": "X",
                "value": "Defined Activate"
              }
            }
          ]
        }
      }
    }
)apl";

/**
 * Test that actions with defined commands do NOT invoke their default event handlers
 */
TEST_F(AccessibilityActionTest, ActionsWithCommands)
{
    loadDocument(ACTIONS_WITH_COMMANDS);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);
    ASSERT_TRUE(text);
    ASSERT_TRUE(IsEqual("Idle", text->getCalculated(kPropertyText).asString()));

    // The double tap gesture should run internal commands
    component->update(kUpdateAccessibilityAction, "doubletap");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("Defined DPress", text->getCalculated(kPropertyText).asString()));

    // The activate action should run its own commands
    component->update(kUpdateAccessibilityAction, "activate");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("Defined Activate", text->getCalculated(kPropertyText).asString()));

    // Pressing on the component will run the built-in command
    component->update(kUpdatePressed, 0);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("OnPress", text->getCalculated(kPropertyText).asString()));
}


static const char *ENABLED = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "bind": {
            "name": "X",
            "value": 0
          },
          "items": {
            "type": "Text",
            "text": "${X}"
          },
          "onPress": {
            "type": "SetValue",
            "property": "X",
            "value": "${X+1}"
          },
          "actions": {
            "name": "test",
            "label": "Test label",
            "enabled": "${X % 2 == 1}",
            "commands": {
              "type": "SetValue",
              "property": "X",
              "value": 10
            }
          }
        }
      }
    }
)apl";

/**
 * Test the enabled property.  We start with a counter at 0.  The gesture
 * is enabled when the value is odd and disabled when the value is even.
 * Pressing the touchwrapper increments the count by 1; firing the gesture resets
 * the count to -1.
 */
TEST_F(AccessibilityActionTest, Enabled)
{
    loadDocument(ENABLED);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);
    ASSERT_TRUE(text);
    ASSERT_TRUE(IsEqual("0", text->getCalculated(kPropertyText).asString()));

    // Check that the gesture is currently not enabled
    const auto& actions = component->getCalculated(kPropertyAccessibilityActions);
    ASSERT_TRUE(actions.isArray());
    ASSERT_EQ(1, actions.size());
    ASSERT_FALSE(actions.at(0).getAccessibilityAction()->enabled());

    // Attempt to invoke the disabled gesture
    component->update(kUpdateAccessibilityAction, "test");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(root));  // Nothing has changed - it is disabled

    // The press event will advance the value of X
    component->update(kUpdatePressed, 1);
    root->clearPending();
    ASSERT_TRUE(IsEqual("1", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(component->getCalculated(kPropertyAccessibilityActions).at(0).getAccessibilityAction()->enabled());
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(component, kPropertyAccessibilityActions));
    ASSERT_TRUE(CheckDirty(root, text, component));

    // Attempt to invoke the ENABLED gesture
    component->update(kUpdateAccessibilityAction, "test");
    root->clearPending();
    ASSERT_TRUE(IsEqual("10", text->getCalculated(kPropertyText).asString()));
    ASSERT_FALSE(component->getCalculated(kPropertyAccessibilityActions).at(0).getAccessibilityAction()->enabled());
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(component, kPropertyAccessibilityActions));
    ASSERT_TRUE(CheckDirty(root, text, component));

    // Now the gesture should be disabled again...
    component->update(kUpdateAccessibilityAction, "test");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(root));  // Nothing has changed - it is disabled
}


static const char *BLOCKING = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "bind": {
            "name": "X",
            "value": 0
          },
          "items": {
            "type": "Text",
            "text": "${X}"
          },
          "actions": [
            {
              "name": "test",
              "label": "Test label",
              "enabled": false,
              "commands": {
                "type": "SetValue",
                "property": "X",
                "value": 10
              }
            },
            {
              "name": "test",
              "label": "Test label",
              "enabled": true,
              "commands": {
                "type": "SetValue",
                "property": "X",
                "value": 20
              }
            }
          ]
        }
      }
    }
)apl";

/**
 * The blocking test verifies that the first action with a matching name is the one that will be checked,
 * even if it is not enabled.
 */
TEST_F(AccessibilityActionTest, Blocking)
{
    loadDocument(BLOCKING);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);
    ASSERT_TRUE(text);
    ASSERT_TRUE(IsEqual("0", text->getCalculated(kPropertyText).asString()));

    // Attempt to invoke the disabled gesture
    component->update(kUpdateAccessibilityAction, "test");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(root));  // Nothing has changed - it is disabled
}


static const char *EVENT_CONTEXT = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "id": "MyTouchId",
          "onPress": {
            "type": "SendEvent",
            "arguments": [
              "ONPRESS",
              "${event.source.source}",
              "${event.source.handler}",
              "${event.source.id}",
              "${event.source.value}",
              "${event.target.source}"
            ]
          },
          "actions": [
            {
              "name": "test",
              "label": "Test label",
              "commands": {
                "type": "SendEvent",
                "arguments": [
                  "TEST",
                  "${event.source.source}",
                  "${event.source.handler}",
                  "${event.source.id}",
                  "${event.source.value}",
                  "${event.target.source}"
                ]
              }
            },
            {
              "name": "activate",
              "label": "fake press"
            }
          ]
        }
      }
    }
)apl";

/**
 * Verify that the event context is correctly set up within the action
 */
TEST_F(AccessibilityActionTest, EventContext)
{
    loadDocument(EVENT_CONTEXT);
    ASSERT_TRUE(component);

    // This action invokes its own command.  The name of the handler is set to the name of the action
    component->update(kUpdateAccessibilityAction, "test");
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "TouchWrapper", "test", "MyTouchId", 0, Object::NULL_OBJECT()));

    // This action invokes the onPress command.  The name of the handler is set to the normal "Press" handler.
    component->update(kUpdateAccessibilityAction, "activate");
    ASSERT_TRUE(CheckSendEvent(root, "ONPRESS", "TouchWrapper", "Press", "MyTouchId", 0, Object::NULL_OBJECT()));
}


static const char *ARGUMENT_PASSING = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "layouts": {
        "TestLayout": {
          "parameters": [
            "NAME",
            "LABEL",
            "COMMANDS",
            "ACTIONS"
          ],
          "items": {
            "type": "TouchWrapper",
            "actions": [
              {
                "name": "${NAME}",
                "label": "${LABEL}",
                "commands": "${COMMANDS}"
              },
              "${ACTIONS}"
            ]
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "TestLayout",
          "NAME": "testAction",
          "LABEL": "This is a test action",
          "COMMANDS": {
            "type": "SendEvent",
            "arguments": [
              "Command Argument",
              "${event.source.handler}"
            ]
          },
          "ACTIONS": {
            "name": "testAction2",
            "label": "This is another test action",
            "commands": {
              "type": "SendEvent",
              "arguments": [
                "Another Command Argument",
                "${event.source.handler}"
              ]
            }
          }
        }
      }
    }
)apl";

/**
 * This tests if we can pass arguments into the actions list
 */
TEST_F(AccessibilityActionTest, ArgumentPassing)
{
    loadDocument(ARGUMENT_PASSING);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeTouchWrapper, component->getType());

    const auto& actions = component->getCalculated(kPropertyAccessibilityActions);
    ASSERT_TRUE(actions.isArray());
    ASSERT_EQ(2, actions.size());

    auto a0 = actions.at(0).getAccessibilityAction();
    ASSERT_STREQ("testAction", a0->getName().c_str());
    ASSERT_STREQ("This is a test action", a0->getLabel().c_str());

    auto a1 = actions.at(1).getAccessibilityAction();
    ASSERT_STREQ("testAction2", a1->getName().c_str());
    ASSERT_STREQ("This is another test action", a1->getLabel().c_str());

    component->update(kUpdateAccessibilityAction, "testAction");
    ASSERT_TRUE(CheckSendEvent(root, "Command Argument", "testAction"));

    // This action invokes the onPress command.  The name of the handler is set to the normal "Press" handler.
    component->update(kUpdateAccessibilityAction, "testAction2");
    ASSERT_TRUE(CheckSendEvent(root, "Another Command Argument", "testAction2"));
}

