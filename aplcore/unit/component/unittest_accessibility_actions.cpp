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

#include "apl/focus/focusmanager.h"

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
    auto action = actions.at(0).get<AccessibilityAction>();
    ASSERT_STREQ("MakeRed", action->getName().c_str());
    ASSERT_STREQ("Make the background red", action->getLabel().c_str());
    ASSERT_TRUE(action->enabled());

    // Invoke the action and verify that it changes the background color
    component->update(kUpdateAccessibilityAction, "MakeRed");
    ASSERT_TRUE(CheckDirty(component, kPropertyBackgroundColor, kPropertyBackground, kPropertyVisualHash));
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
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("X=1", text->getCalculated(kPropertyText).asString()));

    // Verify that the action fires
    component->update(kUpdateAccessibilityAction, "activate");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
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
            },
            {
              "type": "Tap",
              "onTap": {
                "type": "SetValue",
                "property": "X",
                "value": "Tap"
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
            },
            {
              "name": "activate",
              "label": "Tap Test"
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
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("DPress", text->getCalculated(kPropertyText).asString()));

    component->update(kUpdateAccessibilityAction, "longpress");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("LPress", text->getCalculated(kPropertyText).asString()));

    component->update(kUpdateAccessibilityAction, "swipeaway");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("SDone", text->getCalculated(kPropertyText).asString()));

    // The tap gesture is special because it gets triggered by activate
    component->update(kUpdateAccessibilityAction, "activate");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("Tap", text->getCalculated(kPropertyText).asString()));
}

static const char *PAGER_SCROLLING_TEST = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "height": "100%",
      "navigation": "wrap",
      "items": {
        "type": "Text",
        "text": "${data}"
      },
      "data": ["one", "two", "three"]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, PagerScrolling)
{
    loadDocument(PAGER_SCROLLING_TEST);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(component->pagePosition());
    ASSERT_TRUE(text);
    ASSERT_TRUE(IsEqual("one", text->getCalculated(kPropertyText).asString()));

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();

    text = component->getChildAt(component->pagePosition());
    ASSERT_TRUE(IsEqual("two", text->getCalculated(kPropertyText).asString()));

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();
    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();

    text = component->getChildAt(component->pagePosition());
    ASSERT_TRUE(IsEqual("one", text->getCalculated(kPropertyText).asString()));

    component->update(kUpdateAccessibilityAction, "scrollbackward");
    root->clearPending();

    text = component->getChildAt(component->pagePosition());
    ASSERT_TRUE(IsEqual("three", text->getCalculated(kPropertyText).asString()));
}

static const char *PAGER_SCROLLING_EXPLICIT = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "height": "100%",
      "navigation": "wrap",
      "items": {
        "type": "Text",
        "text": "${data}"
      },
      "data": ["one", "two", "three"],
      "actions": [
        {
          "name": "scrollforward",
          "label": "scrollforward Test",
          "enabled": false
        },
        {
          "name": "scrollbackward",
          "label": "scrollbackward Test",
          "commands": {
            "type": "SendEvent",
            "arguments": [ "scrollbackward" ]
          }
        }
      ]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, PagerScrollingExplicit)
{
    loadDocument(PAGER_SCROLLING_EXPLICIT);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(component->pagePosition());
    ASSERT_TRUE(text);
    ASSERT_TRUE(IsEqual("one", text->getCalculated(kPropertyText).asString()));

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();

    text = component->getChildAt(component->pagePosition());
    ASSERT_TRUE(IsEqual("one", text->getCalculated(kPropertyText).asString()));

    component->update(kUpdateAccessibilityAction, "scrollbackward");
    root->clearPending();

    text = component->getChildAt(component->pagePosition());
    ASSERT_TRUE(IsEqual("one", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckSendEvent(root, "scrollbackward"));
}

static const char *SEQUENCE_SCROLLING_TEST = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "height": 100,
      "items": {
        "type": "Text",
        "height": 100,
        "text": "${data}"
      },
      "data": ["one", "two", "three", "four"]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, SequenceScrolling)
{
    loadDocument(SEQUENCE_SCROLLING_TEST);
    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->scrollPosition().getY());

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();

    ASSERT_EQ(100, component->scrollPosition().getY());

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();
    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();

    ASSERT_EQ(300, component->scrollPosition().getY());

    component->update(kUpdateAccessibilityAction, "scrollbackward");
    root->clearPending();

    ASSERT_EQ(200, component->scrollPosition().getY());
}

static const char *SEQUENCE_SCROLLING_EXPLICIT = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "height": "100%",
      "navigation": "wrap",
      "items": {
        "type": "Text",
        "text": "${data}"
      },
      "data": ["one", "two", "three"],
      "actions": [
        {
          "name": "scrollforward",
          "label": "scrollforward Test",
          "enabled": false
        },
        {
          "name": "scrollbackward",
          "label": "scrollbackward Test",
          "commands": {
            "type": "SendEvent",
            "arguments": [ "scrollbackward" ]
          }
        }
      ]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, SequenceScrollingExplicit)
{
    loadDocument(SEQUENCE_SCROLLING_EXPLICIT);
    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->scrollPosition().getY());

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();

    ASSERT_EQ(0, component->scrollPosition().getY());

    component->update(kUpdateAccessibilityAction, "scrollbackward");
    root->clearPending();

    ASSERT_TRUE(CheckSendEvent(root, "scrollbackward"));
}

static const char *ACTIVATE_PREFERS_ON_PRESS_OVER_TAP_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.9",
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "items": {
            "type": "Text",
            "text": "Some text here"
          },
          "onPress": {
            "type": "SendEvent",
            "arguments": [ "onPress" ]
          },
          "gestures": [
            {
              "type": "Tap",
              "onTap": {
                "type": "SendEvent",
                "arguments": [ "onTap" ]
              }
            }
          ],
          "actions": [
            {
              "name": "activate",
              "label": "Activate Test"
            },
            {
              "name": "tap",
              "label": "Tap Test"
            }
          ]
        }
      }
    }
)apl";

/*
 * The "activate" accessibility action will use the "onPress" command of a touch wrapper if it does not have
 * any attached commands, even if "onTap" gesture is also defined.
 */
TEST_F(AccessibilityActionTest, ActivatePrefersOnPressOverOnTap)
{
    loadDocument(ACTIVATE_PREFERS_ON_PRESS_OVER_TAP_TEST);
    ASSERT_TRUE(component);

    component->update(kUpdateAccessibilityAction, "activate");
    root->clearPending();
    ASSERT_TRUE(CheckSendEvent(root, "onPress"));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(AccessibilityActionTest, TapIsSeparateAction)
{
    loadDocument(ACTIVATE_PREFERS_ON_PRESS_OVER_TAP_TEST);
    ASSERT_TRUE(component);

    component->update(kUpdateAccessibilityAction, "tap");
    root->clearPending();
    ASSERT_TRUE(CheckSendEvent(root, "onTap"));
    ASSERT_FALSE(root->hasEvent());
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
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("Defined DPress", text->getCalculated(kPropertyText).asString()));

    // The activate action should run its own commands
    component->update(kUpdateAccessibilityAction, "activate");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_TRUE(IsEqual("Defined Activate", text->getCalculated(kPropertyText).asString()));

    // Pressing on the component will run the built-in command
    component->update(kUpdatePressed, 0);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
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
    ASSERT_FALSE(actions.at(0).get<AccessibilityAction>()->enabled());

    // Attempt to invoke the disabled gesture
    component->update(kUpdateAccessibilityAction, "test");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(root));  // Nothing has changed - it is disabled

    // The press event will advance the value of X
    component->update(kUpdatePressed, 1);
    root->clearPending();
    ASSERT_TRUE(IsEqual("1", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(component->getCalculated(kPropertyAccessibilityActions).at(0).get<AccessibilityAction>()->enabled());
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component, kPropertyAccessibilityActions));
    ASSERT_TRUE(CheckDirty(root, text, component));

    // Attempt to invoke the ENABLED gesture
    component->update(kUpdateAccessibilityAction, "test");
    root->clearPending();
    ASSERT_TRUE(IsEqual("10", text->getCalculated(kPropertyText).asString()));
    ASSERT_FALSE(component->getCalculated(kPropertyAccessibilityActions).at(0).get<AccessibilityAction>()->enabled());
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
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

    auto a0 = actions.at(0).get<AccessibilityAction>();
    ASSERT_STREQ("testAction", a0->getName().c_str());
    ASSERT_STREQ("This is a test action", a0->getLabel().c_str());

    auto a1 = actions.at(1).get<AccessibilityAction>();
    ASSERT_STREQ("testAction2", a1->getName().c_str());
    ASSERT_STREQ("This is another test action", a1->getLabel().c_str());

    component->update(kUpdateAccessibilityAction, "testAction");
    ASSERT_TRUE(CheckSendEvent(root, "Command Argument", "testAction"));

    // This action invokes the onPress command.  The name of the handler is set to the normal "Press" handler.
    component->update(kUpdateAccessibilityAction, "testAction2");
    ASSERT_TRUE(CheckSendEvent(root, "Another Command Argument", "testAction2"));
}

static const char *TOUCHABLE_DYNAMIC_ACTIONS = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "height": "100%",
      "navigation": "normal",
      "bind": [
        { "name": "ActionToggler", "type": "boolean", "value": false }
      ],
      "items": [
        {
          "type": "TouchWrapper",
          "actions": [{ "name": "activate", "label": "Activate with no onPress" }]
        },
        {
          "type": "TouchWrapper",
          "actions": [{ "name": "activate", "label": "Activate with onPress" }],
          "onPress": { "type": "SendEvent" }
        },
        {
          "type": "TouchWrapper",
          "actions": [{ "name": "activate", "label": "Activate with Tap" }],
          "gestures": { "type": "Tap", "onTap": { "type": "SendEvent" }}
        },
        {
          "type": "TouchWrapper",
          "actions": [{ "name": "activate", "label": "Activate with onPress, disabled component" }],
          "onPress": { "type": "SendEvent" },
          "disabled": true
        },
        {
          "type": "TouchWrapper",
          "actions": [{ "name": "activate", "label": "Activate with disabled action", "enabled": "${ActionToggler}" }],
          "onPress": { "type": "SendEvent" }
        },
        {
          "type": "TouchWrapper",
          "actions": [
            {
              "name": "activate",
              "label": "Activate action with commands",
              "commands": { "type": "SendEvent" }
            }
          ]
        }
      ]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, TouchableDynamicActionsOld)
{
    loadDocument(TOUCHABLE_DYNAMIC_ACTIONS);
    ASSERT_TRUE(component);

    // In old "style" actions always reported if explicitly requested
    ASSERT_EQ(1, component->getChildAt(0)->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(1, component->getChildAt(1)->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(1, component->getChildAt(2)->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(1, component->getChildAt(3)->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(1, component->getChildAt(4)->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(1, component->getChildAt(5)->getCalculated(apl::kPropertyAccessibilityActions).size());
}

TEST_F(AccessibilityActionTest, TouchableDynamicActions)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(TOUCHABLE_DYNAMIC_ACTIONS);
    ASSERT_TRUE(component);

    // No onPress/commands or onTap available
    ASSERT_EQ(0, component->getChildAt(0)->getCalculated(apl::kPropertyAccessibilityActions).size());

    // Reported from onPress
    ASSERT_EQ(1, component->getChildAt(1)->getCalculated(apl::kPropertyAccessibilityActions).size());

    // Reported from Tap
    ASSERT_EQ(1, component->getChildAt(2)->getCalculated(apl::kPropertyAccessibilityActions).size());

    // Disabled component
    ASSERT_EQ(0, component->getChildAt(3)->getCalculated(apl::kPropertyAccessibilityActions).size());

    // Disabled action
    ASSERT_EQ(0, component->getChildAt(4)->getCalculated(apl::kPropertyAccessibilityActions).size());

    // Explicit command
    ASSERT_EQ(1, component->getChildAt(5)->getCalculated(apl::kPropertyAccessibilityActions).size());

    // Enabling disabled component should refresh actions
    component->getCoreChildAt(3)->setProperty(apl::kPropertyDisabled, false);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component->getCoreChildAt(3), kPropertyAccessibilityActions, kPropertyDisabled));
    ASSERT_EQ(1, component->getChildAt(3)->getCalculated(apl::kPropertyAccessibilityActions).size());

    // Disabling enabled component should refresh actions too
    component->getCoreChildAt(3)->setProperty(apl::kPropertyDisabled, true);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component->getCoreChildAt(3), kPropertyAccessibilityActions, kPropertyDisabled));
    ASSERT_EQ(0, component->getChildAt(3)->getCalculated(apl::kPropertyAccessibilityActions).size());

    // Changing bound "enabled" in the action enables it
    component->setProperty("ActionToggler", true);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component->getCoreChildAt(4), kPropertyAccessibilityActions));
    ASSERT_EQ(1, component->getChildAt(4)->getCalculated(apl::kPropertyAccessibilityActions).size());

    // Changing bound "enabled" in the action also can disable it
    component->setProperty("ActionToggler", false);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component->getCoreChildAt(4), kPropertyAccessibilityActions));
    ASSERT_EQ(0, component->getChildAt(4)->getCalculated(apl::kPropertyAccessibilityActions).size());
}

static const char *TOUCHABLE_DYNAMIC_GESTURES = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "actions": [
        { "name": "tap", "label": "Enable Tap gesture accessibility" },
        { "name": "doubletap", "label": "Enable DoubleTap gesture accessibility" },
        { "name": "longpress", "label": "Enable LongPress gesture accessibility" },
        { "name": "swipeaway", "label": "Enable SwipeAway gesture accessibility" }
      ],
      "gestures": [
        { "type": "DoublePress", "onDoublePress": { "type": "SendEvent" } },
        { "type": "LongPress", "onLongPressEnd": { "type": "SendEvent" } },
        { "type": "SwipeAway", "direction": "left", "onSwipeDone": { "type": "SendEvent" } },
        { "type": "Tap", "onTap": { "type": "SendEvent" } }
      ]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, TouchableDynamicGesturesOld)
{
    loadDocument(TOUCHABLE_DYNAMIC_GESTURES);
    ASSERT_TRUE(component);

    ASSERT_EQ(4, component->getCalculated(apl::kPropertyAccessibilityActions).size());
}

TEST_F(AccessibilityActionTest, TouchableDynamicGestures)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(TOUCHABLE_DYNAMIC_GESTURES);
    ASSERT_TRUE(component);

    ASSERT_EQ(4, component->getCalculated(apl::kPropertyAccessibilityActions).size());
}

static const char *TOUCHABLE_DYNAMIC_GESTURES_DISABLED = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "actions": [
        { "name": "tap", "label": "Enable Tap gesture accessibility", "enabled": false },
        { "name": "doubletap", "label": "Enable DoubleTap gesture accessibility", "enabled": false },
        { "name": "longpress", "label": "Enable LongPress gesture accessibility", "enabled": false },
        { "name": "swipeaway", "label": "Enable SwipeAway gesture accessibility", "enabled": false }
      ],
      "gestures": [
        { "type": "DoublePress", "onDoublePress": { "type": "SendEvent" } },
        { "type": "LongPress", "onLongPressEnd": { "type": "SendEvent" } },
        { "type": "SwipeAway", "direction": "left", "onSwipeDone": { "type": "SendEvent" } },
        { "type": "Tap", "onTap": { "type": "SendEvent" } }
      ]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, TouchableDynamicGesturesDisabledOld)
{
    loadDocument(TOUCHABLE_DYNAMIC_GESTURES_DISABLED);
    ASSERT_TRUE(component);

    ASSERT_EQ(4, component->getCalculated(apl::kPropertyAccessibilityActions).size());
}

TEST_F(AccessibilityActionTest, TouchableDynamicGesturesDisabled)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(TOUCHABLE_DYNAMIC_GESTURES_DISABLED);
    ASSERT_TRUE(component);

    ASSERT_EQ(0, component->getCalculated(apl::kPropertyAccessibilityActions).size());
}

static const char *PAGER_DYNAMIC_ACTIONS = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "height": "100%",
      "navigation": "normal",
      "items": {
        "type": "TouchWrapper",
        "actions": [ { "name": "activate", "label": "Activate" } ],
        "onPress": { "type": "SendEvent" }
      },
      "data": ["one", "two"],
      "actions": [
        {
          "name": "scrollbackward",
          "label": "scrollbackward disabled Test",
          "enabled": false
        }
      ]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, PagerDynamicActions)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(PAGER_DYNAMIC_ACTIONS);
    ASSERT_TRUE(component);

    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    auto laidOutChild = component->getChildAt(component->pagePosition());
    ASSERT_TRUE(laidOutChild->getCalculated(apl::kPropertyLaidOut).asBoolean());
    ASSERT_TRUE(laidOutChild);
    ASSERT_EQ(1, laidOutChild->getCalculated(apl::kPropertyAccessibilityActions).size());

    auto nonLaidOutChild = component->getChildAt(1);
    ASSERT_FALSE(nonLaidOutChild->getCalculated(apl::kPropertyLaidOut).asBoolean());
    ASSERT_TRUE(nonLaidOutChild);
    ASSERT_EQ(0, nonLaidOutChild->getCalculated(apl::kPropertyAccessibilityActions).size());

    // Switch page, newly laid-out components gets action published
    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();

    ASSERT_TRUE(CheckDirty(nonLaidOutChild,
                           kPropertyLaidOut, kPropertyAccessibilityActions, kPropertyBounds,
                           kPropertyInnerBounds, kPropertyVisualHash,
                           kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(nonLaidOutChild->getCalculated(apl::kPropertyLaidOut).asBoolean());
    ASSERT_TRUE(nonLaidOutChild);
    ASSERT_EQ(1, nonLaidOutChild->getCalculated(apl::kPropertyAccessibilityActions).size());
}

static const char *PAGER_DYNAMIC_SIMPLE_ACTIONS = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "height": "100%",
      "navigation": "normal",
      "items": {
        "type": "TouchWrapper"
      },
      "data": ["one", "two", "three"]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, PagerDynamicSimpleActions)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(PAGER_DYNAMIC_SIMPLE_ACTIONS);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(0, component->pagePosition());

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();
    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyCurrentPage,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();
    ASSERT_EQ(2, component->pagePosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyCurrentPage,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    component->update(kUpdateAccessibilityAction, "scrollbackward");
    root->clearPending();
    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyCurrentPage,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());
}

TEST_F(AccessibilityActionTest, PagerDynamicSimpleActionsFromCommands)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(PAGER_DYNAMIC_SIMPLE_ACTIONS);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(0, component->pagePosition());

    rapidjson::Document scrollForwards;
    scrollForwards.Parse(R"([{"type": "SetPage", "componentId": ":root", "position": "relative", "value": 1}])");
    rootDocument->executeCommands(scrollForwards, false);
    advanceTime(1000);
    root->clearPending();
    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyCurrentPage,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    rootDocument->executeCommands(scrollForwards, false);
    advanceTime(1000);
    root->clearPending();
    ASSERT_EQ(2, component->pagePosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyCurrentPage,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    rapidjson::Document scrollBackwards;
    scrollBackwards.Parse(R"([{"type": "SetPage", "componentId": ":root", "position": "relative", "value": -1}])");
    rootDocument->executeCommands(scrollBackwards, false);
    advanceTime(1000);
    root->clearPending();
    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyCurrentPage,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());
}

static const char *SEQUENCE_DYNAMIC_SIMPLE_ACTIONS = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "height": 100,
      "items": {
        "type": "TouchWrapper",
        "height": "100%"
      },
      "data": ["one", "two", "three"]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, SequenceDynamicSimpleActions)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(SEQUENCE_DYNAMIC_SIMPLE_ACTIONS);

    advanceTime(10);

    root->clearDirty();

    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(Point(0, 0), component->scrollPosition());

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();
    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();
    ASSERT_EQ(Point(0, 200), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    component->update(kUpdateAccessibilityAction, "scrollbackward");
    root->clearPending();
    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());
}

TEST_F(AccessibilityActionTest, SequenceDynamicSimpleActionsFromCommands)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(SEQUENCE_DYNAMIC_SIMPLE_ACTIONS);

    advanceTime(10);

    root->clearDirty();

    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(Point(0, 0), component->scrollPosition());

    rapidjson::Document scrollForwards;
    scrollForwards.Parse(R"([{"type": "Scroll", "componentId": ":root", "distance": 1}])");
    rootDocument->executeCommands(scrollForwards, false);
    advanceTime(1000);
    root->clearPending();
    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    rootDocument->executeCommands(scrollForwards, false);
    advanceTime(1000);
    root->clearPending();
    ASSERT_EQ(Point(0, 200), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    rapidjson::Document scrollBackwards;
    scrollBackwards.Parse(R"([{"type": "Scroll", "componentId": ":root", "distance": -1}])");
    rootDocument->executeCommands(scrollBackwards, false);
    advanceTime(1000);
    root->clearPending();
    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());
}

static const char *SCROLLVIEW_DYNAMIC_SIMPLE_ACTIONS = R"apl({
"type": "APL",
"version": "2023.2",
"mainTemplate": {
  "items": {
    "type": "ScrollView",
    "height": 100,
    "item": {
      "type": "Container",
      "height": 300,
      "items": {
        "type": "Frame",
        "height": 100,
        "backgroundColor": "${data}"
      },
      "data": [
        "blue",
        "green",
        "red"
      ]
    }
  }
}
})apl";

TEST_F(AccessibilityActionTest, ScrollviewDynamicSimpleActions)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(SCROLLVIEW_DYNAMIC_SIMPLE_ACTIONS);

    root->clearDirty();

    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(Point(0, 0), component->scrollPosition());

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();
    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    component->update(kUpdateAccessibilityAction, "scrollforward");
    root->clearPending();
    ASSERT_EQ(Point(0, 200), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    component->update(kUpdateAccessibilityAction, "scrollbackward");
    root->clearPending();
    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());
}

TEST_F(AccessibilityActionTest, ScrollviewDynamicSimpleActionsFromCommands)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(SCROLLVIEW_DYNAMIC_SIMPLE_ACTIONS);

    root->clearDirty();

    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(Point(0, 0), component->scrollPosition());

    rapidjson::Document scrollForwards;
    scrollForwards.Parse(R"([{"type": "Scroll", "componentId": ":root", "distance": 1}])");
    rootDocument->executeCommands(scrollForwards, false);
    advanceTime(1000);
    root->clearPending();

    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    rootDocument->executeCommands(scrollForwards, false);
    advanceTime(1000);
    root->clearPending();

    ASSERT_EQ(Point(0, 200), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, component->getCalculated(apl::kPropertyAccessibilityActions).size());

    rapidjson::Document scrollBackwards;
    scrollBackwards.Parse(R"([{"type": "Scroll", "componentId": ":root", "distance": -1}])");
    rootDocument->executeCommands(scrollBackwards, false);
    advanceTime(1000);
    root->clearPending();

    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component,
                           kPropertyAccessibilityActions, kPropertyScrollPosition,
                           kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->getCalculated(apl::kPropertyAccessibilityActions).size());
}

static const char *PAGER_DYNAMIC_ACTIONS_FOCUS = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "direction": "column",
      "height": 400,
      "width": 100,
      "items": [
        {
          "type": "Pager",
          "id": "focusableChildren",
          "height": "25%",
          "width": "100%",
          "items": {
            "id": "${data}Wrapper",
            "type": "TouchWrapper"
          },
          "data": ["one", "two"]
        },
        {
          "type": "Pager",
          "id": "nonFocusableChildren",
          "height": "25%",
          "width": "100%",
          "items": {
            "type": "Frame",
            "backgroundColor": "${data}"
          },
          "data": ["blue", "red"]
        },
        {
          "type": "Pager",
          "id": "mixedChildren",
          "height": "25%",
          "width": "100%",
          "items": [
            {
              "id": "mixedWrapper",
              "type": "TouchWrapper"
            },
            {
              "type": "Frame",
              "backgroundColor": "red"
            }
          ]
        },
        {
          "type": "Pager",
          "id": "deepChildren",
          "height": "25%",
          "width": "100%",
          "items": [
            {
              "type": "Frame",
              "backgroundColor": "${data}",
              "item": {
                "id": "${data}Wrapper",
                "type": "TouchWrapper",
                "height": "100%",
                "width": "100%"
              },
              "height": "100%",
              "width": "100%"
            }
          ],
          "data": ["blue", "red"]
        }
      ]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, PagerDynamicActionsFocus)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(PAGER_DYNAMIC_ACTIONS_FOCUS);
    ASSERT_TRUE(component);

    advanceTime(10);

    auto fcPager = component->getCoreChildAt(0);
    auto nfcPager = component->getCoreChildAt(1);
    auto mcPager = component->getCoreChildAt(2);
    auto dcPager = component->getCoreChildAt(3);

    ASSERT_EQ(kComponentTypePager, fcPager->getType());
    ASSERT_EQ(kComponentTypePager, nfcPager->getType());
    ASSERT_EQ(kComponentTypePager, mcPager->getType());
    ASSERT_EQ(kComponentTypePager, dcPager->getType());
    auto& fm = root->context().focusManager();

    ASSERT_FALSE(fm.getFocus());

    // Accessibility page switch should switch to the next focusable child on the new page
    root->setFocus(apl::kFocusDirectionNone, Rect(), "oneWrapper");
    ASSERT_TRUE(fm.getFocus());

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(root->findComponentById("oneWrapper"), event.getComponent());

    fcPager->update(kUpdateAccessibilityAction, "scrollforward");

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(root->findComponentById("twoWrapper"), event.getComponent());
    ASSERT_EQ(root->findComponentById("twoWrapper"), fm.getFocus());


    // Focused pager don't move focus
    root->setFocus(apl::kFocusDirectionNone, Rect(), "nonFocusableChildren");
    ASSERT_TRUE(fm.getFocus());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(nfcPager, event.getComponent());

    nfcPager->update(kUpdateAccessibilityAction, "scrollforward");

    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(nfcPager, fm.getFocus());


    // Switch to the page without focusable leads to pager focus
    root->setFocus(apl::kFocusDirectionNone, Rect(), "mixedWrapper");
    ASSERT_TRUE(fm.getFocus());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(root->findComponentById("mixedWrapper"), event.getComponent());

    mcPager->update(kUpdateAccessibilityAction, "scrollforward");

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(mcPager, event.getComponent());
    ASSERT_EQ(mcPager, fm.getFocus());


    // Deeper children switches work similarly to directs
    root->setFocus(apl::kFocusDirectionNone, Rect(), "blueWrapper");
    ASSERT_TRUE(fm.getFocus());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(root->findComponentById("blueWrapper"), event.getComponent());

    dcPager->update(kUpdateAccessibilityAction, "scrollforward");

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(root->findComponentById("redWrapper"), event.getComponent());
    ASSERT_EQ(root->findComponentById("redWrapper"), fm.getFocus());
}

static const char *SEQUENCE_DYNAMIC_ACTIONS_FOCUS = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "height": 100,
      "width": 100,
      "items": [
        {
          "type": "Frame",
          "height": "100%",
          "width": "100%",
          "items": {
            "id": "deepWrapperStart",
            "type": "TouchWrapper",
            "height": "100%",
            "width": "100%"
          }
        },
        {
          "type": "Frame",
          "height": "100%",
          "width": "100%",
          "items": {
            "id": "deepWrapperEnd",
            "type": "TouchWrapper",
            "height": "100%",
            "width": "100%"
          }
        },
        {
          "type": "TouchWrapper",
          "id": "shallowWrapperStart",
          "height": "100%",
          "width": "100%"
        },
        {
          "type": "Frame",
          "id": "emptyFrame",
          "height": "100%",
          "width": "100%"
        },
        {
          "type": "TouchWrapper",
          "id": "shallowWrapperEnd",
          "height": "100%",
          "width": "100%"
        }
      ]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, SequenceDynamicActionsFocus)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(SEQUENCE_DYNAMIC_ACTIONS_FOCUS);
    ASSERT_TRUE(component);

    advanceTime(10);

    auto& fm = root->context().focusManager();

    ASSERT_FALSE(fm.getFocus());

    root->setFocus(apl::kFocusDirectionNone, Rect(), "deepWrapperStart");
    ASSERT_TRUE(fm.getFocus());

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(root->findComponentById("deepWrapperStart"), event.getComponent());
    ASSERT_EQ(root->findComponentById("deepWrapperStart"), fm.getFocus());


    // Accessibility scroll should switch to the next focusable child on the new screen (deep)
    component->update(kUpdateAccessibilityAction, "scrollforward");
    ASSERT_TRUE(fm.getFocus());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(root->findComponentById("deepWrapperEnd"), event.getComponent());
    ASSERT_EQ(root->findComponentById("deepWrapperEnd"), fm.getFocus());


    // Accessibility scroll should switch to the next focusable child on the new screen (deep)
    component->update(kUpdateAccessibilityAction, "scrollbackward");
    ASSERT_TRUE(fm.getFocus());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(root->findComponentById("deepWrapperStart"), event.getComponent());
    ASSERT_EQ(root->findComponentById("deepWrapperStart"), fm.getFocus());


    // Accessibility scroll should switch to the next focusable child on the new screen (deep)
    component->update(kUpdateAccessibilityAction, "scrollforward");
    ASSERT_TRUE(fm.getFocus());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(root->findComponentById("deepWrapperEnd"), event.getComponent());
    ASSERT_EQ(root->findComponentById("deepWrapperEnd"), fm.getFocus());


    // Accessibility scroll should switch to the next focusable child on the new screen (shallow)
    component->update(kUpdateAccessibilityAction, "scrollforward");
    ASSERT_TRUE(fm.getFocus());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(root->findComponentById("shallowWrapperStart"), event.getComponent());
    ASSERT_EQ(root->findComponentById("shallowWrapperStart"), fm.getFocus());


    // Accessibility scroll should switch to the scrollable if focusable child no available
    component->update(kUpdateAccessibilityAction, "scrollforward");
    ASSERT_TRUE(fm.getFocus());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(component, event.getComponent());
    ASSERT_EQ(component, fm.getFocus());


    // Accessibility scroll should not switch focus from itself
    component->update(kUpdateAccessibilityAction, "scrollforward");
    ASSERT_TRUE(fm.getFocus());

    ASSERT_EQ(component, fm.getFocus());
}

static const char *CUSTOM_ACTIONS_ON_MULTICHILD = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "direction": "column",
      "height": 400,
      "width": 100,
      "items": [
        {
          "type": "Pager",
          "id": "pagerio",
          "height": "50%",
          "width": "100%",
          "items": {
            "type": "Frame",
            "backgroundColor": "${data}"
          },
          "data": [
            "blue",
            "red"
          ],
          "actions": [
            {
              "name": "quitecustom",
              "label": "Quite custom",
              "command": {
                "type": "SendEvent"
              }
            }
          ]
        },
        {
          "type": "Sequence",
          "id": "sequencio",
          "height": "50%",
          "width": "100%",
          "items": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "height": 200,
            "width": "100%"
          },
          "data": [
            "blue",
            "red"
          ],
          "actions": [
            {
              "name": "verycustom",
              "label": "Very custom",
              "command": {
                "type": "SendEvent"
              }
            }
          ]
        }
      ]
    }
  }
})apl";

TEST_F(AccessibilityActionTest, CustomActionsOnMultichild)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureDynamicAccessibilityActions);

    loadDocument(CUSTOM_ACTIONS_ON_MULTICHILD);
    ASSERT_TRUE(component);

    advanceTime(10);

    auto pager = component->getCoreChildAt(0);
    auto sequence = component->getCoreChildAt(1);

    ASSERT_EQ(kComponentTypePager, pager->getType());
    ASSERT_EQ(kComponentTypeSequence, sequence->getType());

    ASSERT_EQ(3, pager->getCalculated(apl::kPropertyAccessibilityActions).size());
    ASSERT_EQ(2, sequence->getCalculated(apl::kPropertyAccessibilityActions).size());

    pager->update(apl::kUpdateAccessibilityAction, "quitecustom");
    ASSERT_TRUE(CheckSendEvent(root));

    sequence->update(apl::kUpdateAccessibilityAction, "verycustom");
    ASSERT_TRUE(CheckSendEvent(root));
}
