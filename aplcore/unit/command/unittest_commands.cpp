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
#include "apl/engine/event.h"

#include "../testeventloop.h"

using namespace apl;


static const char *DATA = R"({
  "title": "Pecan Pie V"
})";


const char *TOUCH_WRAPPER_EMPTY = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "TouchWrapper",
      "items": {
        "type": "Text",
        "text": "${payload.title}"
      }
    }
  }
})";

TEST_F(CommandTest, OnEmptyPress)
{
    loadDocument(TOUCH_WRAPPER_EMPTY, DATA);

    auto map = component->getCalculated();
    auto onPress = map["onPress"];
    ASSERT_TRUE(onPress.isArray());
    ASSERT_EQ(0, onPress.size());

    // Children
    ASSERT_EQ(1, component->getChildCount());
    auto text = component->getChildAt(0)->getCalculated();
    ASSERT_STREQ("Pecan Pie V", text["text"].asString().c_str());
}


const char *TOUCH_WRAPPER_OTHER = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SetValue",
        "property": "opacity",
        "value": 0.5,
        "componentId": "foo"
      },
      "items": {
        "type": "Text",
        "id": "foo",
        "text": "${payload.title}"
      }
    }
  }
})";

TEST_F(CommandTest, OnSetValueOther)
{
    loadDocument(TOUCH_WRAPPER_OTHER, DATA);
    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    ASSERT_EQ(1, component->getChildCount());
    auto text = component->getChildAt(0);

    ASSERT_TRUE(onPress.isArray());
    ASSERT_EQ(1, onPress.size());
    ASSERT_STREQ("SetValue", onPress.at(0).get("type").asString().c_str());

    performTap(0, 0);

    loop->advanceToEnd();
    ASSERT_EQ(1, mCommandCount[kCommandTypeSetValue]);
    ASSERT_EQ(1, mActionCount[kCommandTypeSetValue]);

    ASSERT_EQ(1, mIssuedCommands.size());
    auto command = std::static_pointer_cast<CoreCommand>(mIssuedCommands.at(0));
    ASSERT_EQ(Object("opacity"), command->getValue(kCommandPropertyProperty));
    ASSERT_EQ(Object(0.5), command->getValue(kCommandPropertyValue));

    ASSERT_EQ(Object(0.5), text->getCalculated(kPropertyOpacity));
}


const char *TOUCH_WRAPPER_SELF = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SetValue",
        "property": "opacity",
        "value": 0.5
      },
      "items": {
        "type": "Text",
        "text": "${payload.title}"
      }
    }
  }
})";

TEST_F(CommandTest, OnSetValueSelf)
{
    loadDocument(TOUCH_WRAPPER_SELF, DATA);
    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    ASSERT_TRUE(onPress.isArray());
    ASSERT_EQ(1, onPress.size());
    ASSERT_STREQ("SetValue", onPress.at(0).get("type").asString().c_str());

    performTap(0, 0);

    loop->advanceToEnd();
    ASSERT_EQ(1, mCommandCount[kCommandTypeSetValue]);
    ASSERT_EQ(1, mActionCount[kCommandTypeSetValue]);

    ASSERT_EQ(1, mIssuedCommands.size());
    auto command = std::static_pointer_cast<CoreCommand>(mIssuedCommands.at(0));
    ASSERT_EQ(Object("opacity"), command->getValue(kCommandPropertyProperty));
    ASSERT_EQ(Object(0.5), command->getValue(kCommandPropertyValue));

    ASSERT_EQ(Object(0.5), component->getCalculated(kPropertyOpacity));
}


const char *TOUCH_WRAPPER_DISABLED = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SendEvent",
        "when": false
      },
      "items": {
        "type": "Text",
        "text": "${payload.title}"
      }
    }
  }
})";

TEST_F(CommandTest, OnPressDisabled)
{
    loadDocument(TOUCH_WRAPPER_DISABLED, DATA);
    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    ASSERT_TRUE(onPress.isArray());
    ASSERT_EQ(1, onPress.size());
    ASSERT_STREQ("SendEvent", onPress.at(0).get("type").asString().c_str());

    performTap(1, 1);

    loop->advanceToEnd();
    ASSERT_EQ(0, mCommandCount[kCommandTypeSendEvent]);  // when is false; no command generated
    ASSERT_EQ(0, mActionCount[kCommandTypeSendEvent]);
}


const char *TOUCH_WRAPPER_DELAYED = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SendEvent",
        "when": true,
        "delay": 100
      },
      "items": {
        "type": "Text",
        "text": "${payload.title}"
      }
    }
  }
})";

TEST_F(CommandTest, OnPressDelayed)
{
    loadDocument(TOUCH_WRAPPER_DELAYED, DATA);

    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    ASSERT_TRUE(onPress.isArray());
    ASSERT_EQ(1, onPress.size());
    ASSERT_STREQ("SendEvent", onPress.at(0).get("type").asString().c_str());

    performTap(0, 0);

    ASSERT_EQ(1, mCommandCount[kCommandTypeSendEvent]);

    loop->advanceToTime(50);
    ASSERT_EQ(0, mActionCount[kCommandTypeSendEvent]);

    loop->advanceToTime(100);
    ASSERT_EQ(1, mActionCount[kCommandTypeSendEvent]);
    ASSERT_TRUE(CheckSendEvent(root));
}


const char *TOUCH_WRAPPER_ARRAY = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "TouchWrapper",
      "onPress": [
        {
          "type": "SendEvent",
          "when": true,
          "delay": 100,
          "arguments": [1,2,"3"]
        },
        {
          "type": "Idle",
          "when": false,
          "delay": 50
        },
        {
          "type": "Idle",
          "when": true,
          "delay": 100
        }
      ],
      "items": {
        "type": "Text",
        "text": "${payload.title}"
      }
    }
  }
})";

TEST_F(CommandTest, OnPressCommandArray)
{
    loadDocument(TOUCH_WRAPPER_ARRAY, DATA);

    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    ASSERT_TRUE(onPress.isArray());
    ASSERT_EQ(3, onPress.size());

    performTap(1, 1);

    ASSERT_EQ(1, mCommandCount[kCommandTypeSendEvent]);

    loop->advanceToTime(50);  // Should still be sitting in the delay
    ASSERT_EQ(0, mActionCount[kCommandTypeSendEvent]);
    ASSERT_EQ(0, mActionCount[kCommandTypeIdle]);

    loop->advanceToTime(100);  // The SendEvent should fire; the idle is queued but not fired
    ASSERT_EQ(1, mCommandCount[kCommandTypeSendEvent]);
    ASSERT_EQ(1, mCommandCount[kCommandTypeIdle]);
    ASSERT_EQ(1, mActionCount[kCommandTypeSendEvent]);
    ASSERT_EQ(0, mActionCount[kCommandTypeIdle]);

    loop->advanceToEnd();  // Each command should have fired once
    ASSERT_EQ(1, mCommandCount[kCommandTypeSendEvent]);
    ASSERT_EQ(1, mCommandCount[kCommandTypeIdle]);
    ASSERT_EQ(1, mActionCount[kCommandTypeSendEvent]);
    ASSERT_EQ(1, mActionCount[kCommandTypeIdle]);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    auto args = event.getValue(kEventPropertyArguments);
    ASSERT_TRUE(args.isArray());
    ASSERT_EQ(1, args.at(0).asNumber());
    ASSERT_EQ(2, args.at(1).asNumber());
    ASSERT_STREQ("3", args.at(2).asString().c_str());
}

TEST_F(CommandTest, OnPressCommandArrayTerminateFast)
{
    loadDocument(TOUCH_WRAPPER_ARRAY, DATA);

    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    performClick(1, 1);

    ASSERT_EQ(1, mCommandCount[kCommandTypeSendEvent]);

    context->sequencer().reset();  // Kill everything - no timers should be left alive or running
    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandTest, OnPressCommandArrayTerminate)
{
    loadDocument(TOUCH_WRAPPER_ARRAY, DATA);

    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    performClick(1, 1);

    ASSERT_EQ(1, mCommandCount[kCommandTypeSendEvent]);

    loop->advanceToTime(50);  // Should still be sitting in the delay
    ASSERT_EQ(0, mActionCount[kCommandTypeSendEvent]);
    ASSERT_EQ(0, mActionCount[kCommandTypeIdle]);

    loop->advanceToTime(100);  // The SendEvent should fire; the idle is queued but not fired
    ASSERT_EQ(1, mCommandCount[kCommandTypeSendEvent]);
    ASSERT_EQ(1, mCommandCount[kCommandTypeIdle]);
    ASSERT_EQ(1, mActionCount[kCommandTypeSendEvent]);
    ASSERT_EQ(0, mActionCount[kCommandTypeIdle]);

    while (root->hasEvent())
        root->popEvent();

    context->sequencer().reset();  // Kill everything - no timers should be left alive or running
    ASSERT_EQ(0, loop->size());
}

const char *LARGE_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "Container",
      "direction": "row",
      "items": [
        {
          "type": "TouchWrapper",
          "id": "myTouchWrapper",
          "onPress": [
            {
              "type": "Sequential",
              "commands": [
                {
                  "type": "Parallel",
                  "commands": [
                    {
                      "type": "SetValue",
                      "property": "text",
                      "value": "Hello 1",
                      "componentId": "text1"
                    },
                    {
                      "type": "SetValue",
                      "property": "text",
                      "value": "Hello 2",
                      "componentId": "text2"
                    }
                  ]
                },
                {
                  "type": "Idle",
                  "delay": 1000
                },
                {
                  "type": "SetValue",
                  "property": "backgroundColor",
                  "value": "red",
                  "componentId": "frame1"
                },
                {
                  "type": "Idle",
                  "delay": 1000
                },
                {
                  "type": "SetValue",
                  "property": "backgroundColor",
                  "value": "yellow",
                  "componentId": "frame2"
                }
              ]
            }
          ],
          "width": 100,
          "height": 100,
          "item": {
            "type": "Frame",
            "width": "100%",
            "height": "100%",
            "backgroundColor": "green"
          }
        },
        {
          "type": "Container",
          "direction": "column",
          "items": [
            {
              "type": "Frame",
              "id": "frame1",
              "backgroundColor": "yellow",
              "item": {
                "type": "Text",
                "text": "Item 1",
                "id": "text1"
              }
            },
            {
              "type": "Frame",
              "id": "frame2",
              "backgroundColor": "red",
              "item": {
                "type": "Text",
                "text": "Item 2",
                "id": "text2"
              }
            }
          ]
        }
      ]
    }
  }
})";


TEST_F(CommandTest, ParallelSequentialMix)
{
    loadDocument(LARGE_TEST, DATA);

    auto touchwrapper = context->findComponentById("myTouchWrapper");
    auto onPress = touchwrapper->getCalculated(kPropertyOnPress);

    auto text1 = context->findComponentById("text1");
    auto text2 = context->findComponentById("text2");
    auto frame1 = context->findComponentById("frame1");
    auto frame2 = context->findComponentById("frame2");

    // Nothing has run
    ASSERT_TRUE(IsEqual("Item 1", text1->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual("Item 2", text2->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(session, "yellow"), frame1->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(session, "red"), frame2->getCalculated(kPropertyBackgroundColor)));

    performTap(1, 1);

    loop->advanceToTime(1);   // The text changes should have run
    ASSERT_TRUE(IsEqual("Hello 1", text1->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual("Hello 2", text2->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(session, "yellow"), frame1->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(session, "red"), frame2->getCalculated(kPropertyBackgroundColor)));

    loop->advanceToTime(1000);  // The first background color should have run
    ASSERT_TRUE(IsEqual("Hello 1", text1->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual("Hello 2", text2->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(session, "red"), frame1->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(session, "red"), frame2->getCalculated(kPropertyBackgroundColor)));

    loop->advanceToEnd();  // Everything has run
    ASSERT_TRUE(IsEqual("Hello 1", text1->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual("Hello 2", text2->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(session, "red"), frame1->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(session, "yellow"), frame2->getCalculated(kPropertyBackgroundColor)));
}

static const char *SET_STATE_DISABLED = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "width": "100%",
      "height": "100%",
      "onPress": [
        {
          "type": "SendEvent",
          "arguments": [
            "Sending"
          ]
        },
        {
          "type": "SetState",
          "state": "disabled",
          "value": true
        }
      ]
    }
  }
})";

TEST_F(CommandTest, SetStateDisabled)
{
    loadDocument(SET_STATE_DISABLED);
    ASSERT_FALSE(component->getState().get(kStateDisabled));

    performClick(1, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    ASSERT_TRUE(component->getState().get(kStateDisabled));

    // The component is disabled - it should not press
    performClick(1, 1);
    ASSERT_FALSE(root->hasEvent());
}

static const char *SET_STATE_CHECKED = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "width": "100%",
      "height": "100%",
      "onPress": {
        "type": "SetState",
        "state": "checked",
        "value": "${!event.source.value}"
      }
    }
  }
})";

TEST_F(CommandTest, SetStateChecked)
{
    loadDocument(SET_STATE_CHECKED);
    ASSERT_FALSE(component->getState().get(kStateChecked));
    ASSERT_TRUE(CheckDirty(root));
    ASSERT_TRUE(CheckState(component));

    performTap(1, 1);
    ASSERT_TRUE(component->getState().get(kStateChecked));
    ASSERT_TRUE(CheckDirty(component, kPropertyChecked));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(CheckState(component, kStateChecked));

    performTap(1, 1);
    ASSERT_FALSE(component->getState().get(kStateChecked));

    performTap(1, 1);
    ASSERT_TRUE(component->getState().get(kStateChecked));
}

static const char *SET_STATE_FOCUSED = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "items": [
        {
          "type": "TouchWrapper",
          "id": "thing1",
          "width": 20,
          "height": 20,
          "onPress": {
            "type": "SetState",
            "state": "focused",
            "value": true,
            "componentId": "thing2"
          }
        },
        {
          "type": "TouchWrapper",
          "id": "thing2",
          "width": 20,
          "height": 20,
          "onPress": {
            "type": "SetState",
            "state": "focused",
            "value": true,
            "componentId": "thing1"
          }
        }
      ]
    }
  }
})";

TEST_F(CommandTest, SetStateFocused)
{
    loadDocument(SET_STATE_FOCUSED);
    auto thing1 = CoreComponent::cast(root->context().findComponentById("thing1"));
    auto thing2 = CoreComponent::cast(root->context().findComponentById("thing2"));
    ASSERT_TRUE(thing1);
    ASSERT_TRUE(thing2);

    ASSERT_FALSE(thing1->getState().get(kStateFocused));
    ASSERT_FALSE(thing2->getState().get(kStateFocused));

    performTap(0, 0);
    ASSERT_FALSE(thing1->getState().get(kStateFocused));
    ASSERT_TRUE(thing2->getState().get(kStateFocused));
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(thing2, event.getComponent());

    performTap(0, 20);
    ASSERT_TRUE(thing1->getState().get(kStateFocused));
    ASSERT_FALSE(thing2->getState().get(kStateFocused));
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(thing1, event.getComponent());

    performTap(0, 0);
    ASSERT_FALSE(thing1->getState().get(kStateFocused));
    ASSERT_TRUE(thing2->getState().get(kStateFocused));
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(thing2, event.getComponent());
}

static const char *SET_FOCUS_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": [
        {
          "type": "TouchWrapper",
          "id": "touch1",
          "height": 10,
          "onPress": {
            "type": "SetFocus",
            "componentId": "touch2"
          }
        },
        {
          "type": "TouchWrapper",
          "id": "touch2",
          "height": 10,
          "onPress": {
            "type": "SetFocus",
            "componentId": "touch1"
          }
        }
      ]
    }
  }
})";

TEST_F(CommandTest, SetFocus)
{
    loadDocument(SET_FOCUS_TEST);

    auto touch1 = context->findComponentById("touch1");
    ASSERT_TRUE(touch1);

    auto touch2 = context->findComponentById("touch2");
    ASSERT_TRUE(touch2);

    performTap(1, 0);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(touch2, event.getComponent());
    ASSERT_FALSE(root->hasEvent());

    performTap(1, 10);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(touch1, event.getComponent());
    ASSERT_FALSE(root->hasEvent());

    // Hit the same component again - nothing should happen
    performTap(1, 10);
    ASSERT_FALSE(root->hasEvent());
}

static const char *CLEAR_FOCUS = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": [
        {
          "type": "TouchWrapper",
          "id": "touch1",
          "height": 10,
          "onPress": {
            "type": "SetFocus",
            "componentId": "touch2"
          }
        },
        {
          "type": "TouchWrapper",
          "id": "touch2",
          "height": 10,
          "onPress": {
            "type": "ClearFocus"
          }
        }
      ]
    }
  }
})";

TEST_F(CommandTest, ClearFocus)
{
    loadDocument(CLEAR_FOCUS);

    auto touch1 = context->findComponentById("touch1");
    ASSERT_TRUE(touch1);

    auto touch2 = context->findComponentById("touch2");
    ASSERT_TRUE(touch2);

    performTap(0, 0);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(touch2, event.getComponent());
    ASSERT_FALSE(root->hasEvent());

    performTap(0, 10);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_FALSE(event.getComponent().get());
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(event.getActionRef().empty());
    root->clearPending();

    // Hit it again
    performTap(0, 10);
    ASSERT_FALSE(root->hasEvent());
}

static const char *EXECUTE_FOCUS = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": [
        {
          "type": "TouchWrapper",
          "id": "touch1"
        },
        {
          "type": "TouchWrapper",
          "id": "touch2"
        }
      ]
    }
  }
})";

TEST_F(CommandTest, ExecuteFocus)
{
    loadDocument(EXECUTE_FOCUS);

    auto touch1 = context->findComponentById("touch1");
    ASSERT_TRUE(touch1);

    auto touch2 = context->findComponentById("touch2");
    ASSERT_TRUE(touch2);

    // Set focus explicitly
    executeCommand("SetFocus", {{"componentId", "touch1"}}, false);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(touch1, event.getComponent());
    ASSERT_TRUE(CoreComponent::cast(touch1)->getState().get(kStateFocused));

    // Try to set the focus on a non-existing component
    ASSERT_FALSE(ConsoleMessage());
    executeCommand("SetFocus", {{"componentId", "touch7"}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CoreComponent::cast(touch1)->getState().get(kStateFocused));
    ASSERT_TRUE(ConsoleMessage());  // There should be a warning about a missing component

    // Leave out the component ID
    executeCommand("SetFocus", {}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CoreComponent::cast(touch1)->getState().get(kStateFocused));
    ASSERT_TRUE(ConsoleMessage());  // Warn about missing componentId

    // Refocus the component that already has the focus
    executeCommand("SetFocus", {{"componentId", "touch1"}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CoreComponent::cast(touch1)->getState().get(kStateFocused));

    // Clear focus
    executeCommand("ClearFocus", {}, false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(nullptr, event.getComponent().get());
    ASSERT_TRUE(event.getActionRef().empty());
    root->clearPending();
    ASSERT_FALSE(CoreComponent::cast(touch1)->getState().get(kStateFocused));
    ASSERT_FALSE(CoreComponent::cast(touch2)->getState().get(kStateFocused));

    // Clear focus when no focus set
    executeCommand("ClearFocus", {}, false);
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(CommandTest, ExecuteFocusDisabled)
{
    loadDocument(EXECUTE_FOCUS);

    auto touch1 = context->findComponentById("touch1");
    ASSERT_TRUE(touch1);

    auto touch2 = context->findComponentById("touch2");
    ASSERT_TRUE(touch2);

    // Set focus explicitly
    executeCommand("SetFocus", {{"componentId", "touch1"}}, false);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(touch1, event.getComponent());
    ASSERT_TRUE(CoreComponent::cast(touch1)->getState().get(kStateFocused));

    // Disable the component
    executeCommand("SetValue", {{"componentId", "touch1"}, {"property", "disabled"}, {"value", true}}, false);
    ASSERT_TRUE(CoreComponent::cast(touch1)->getState().get(kStateDisabled));
    ASSERT_FALSE(CoreComponent::cast(touch1)->getState().get(kStateFocused));

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(nullptr, event.getComponent().get());
    ASSERT_FALSE(root->hasEvent());

    // Try to execute set focus on the disabled component
    executeCommand("SetFocus", {{"componentId", "touch1"}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(CoreComponent::cast(touch1)->getState().get(kStateFocused));
}

static const char *FINISH_BACK = R"({
  "type": "APL",
  "version": "1.3",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "width": "100%",
      "height": "100%",
      "onPress": [
        {
          "type": "Finish",
          "reason": "back"
        }
      ]
    }
  }
})";

TEST_F(CommandTest, FinishBack)
{
    loadDocument(FINISH_BACK);

    performClick(1, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFinish, event.getType());
    ASSERT_EQ(kEventReasonBack, event.getValue(kEventPropertyReason).asInt());
}

static const char *FINISH_EXIT = R"({
  "type": "APL",
  "version": "1.3",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "width": "100%",
      "height": "100%",
      "onPress": [
        {
          "type": "Finish",
          "reason": "exit"
        }
      ]
    }
  }
})";

TEST_F(CommandTest, FinishExit)
{
    loadDocument(FINISH_EXIT);

    performClick(1, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFinish, event.getType());
    ASSERT_EQ(kEventReasonExit, event.getValue(kEventPropertyReason).asInt());
}

static const char *FINISH_DEFAULT = R"({
  "type": "APL",
  "version": "1.3",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "width": "100%",
      "height": "100%",
      "onPress": [
        {
          "type": "Finish"
        }
      ]
    }
  }
})";

TEST_F(CommandTest, FinishDefault)
{
    loadDocument(FINISH_DEFAULT);

    performClick(1, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFinish, event.getType());
    ASSERT_EQ(kEventReasonExit, event.getValue(kEventPropertyReason).asInt());
}

static const char *FINISH_COMMAND_LAST = R"({
  "type": "APL",
  "version": "1.3",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "width": "100%",
      "height": "100%",
      "onPress": [
        {
          "type": "SendEvent",
          "arguments": [
            "Sending"
          ]
        },
        {
          "type": "Finish",
          "reason": "back"
        }
      ]
    }
  }
})";

TEST_F(CommandTest, FinishCommandLast)
{
    loadDocument(FINISH_COMMAND_LAST);

    performClick(1, 1);

    ASSERT_TRUE(CheckSendEvent(root, "Sending"));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFinish, event.getType());
    ASSERT_EQ(kEventReasonBack, event.getValue(kEventPropertyReason).asInt());
}

static const char *FINISH_COMMAND_FIRST = R"({
  "type": "APL",
  "version": "1.3",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "width": "100%",
      "height": "100%",
      "onPress": [
        {
          "type": "Finish",
          "reason": "back"
        },
        {
          "type": "SendEvent",
          "arguments": [
            "Sending"
          ]
        }
      ]
    }
  }
})";

TEST_F(CommandTest, FinishCommandFirst)
{
    loadDocument(FINISH_COMMAND_FIRST);

    performClick(1, 1);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFinish, event.getType());
    ASSERT_EQ(kEventReasonBack, event.getValue(kEventPropertyReason).asInt());

    ASSERT_FALSE(root->hasEvent());
}

static const char *EXECUTE_FINISH = R"({
  "type": "APL",
  "version": "1.3",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "green"
    }
  }
})";

TEST_F(CommandTest, ExecuteFinishBack)
{
    loadDocument(EXECUTE_FINISH);

    executeCommand("Finish", {{"reason", "back"}}, false);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFinish, event.getType());
    ASSERT_EQ(kEventReasonBack, event.getValue(kEventPropertyReason).asInt());

    ASSERT_FALSE(root->hasEvent());
}

TEST_F(CommandTest, ExecuteFinishExit)
{
    loadDocument(EXECUTE_FINISH);

    executeCommand("Finish", {{"reason", "exit"}}, false);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFinish, event.getType());
    ASSERT_EQ(kEventReasonExit, event.getValue(kEventPropertyReason).asInt());

    ASSERT_FALSE(root->hasEvent());
}

TEST_F(CommandTest, ExecuteFinishDefault)
{
    loadDocument(EXECUTE_FINISH);

    executeCommand("Finish", {}, false);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFinish, event.getType());
    ASSERT_EQ(kEventReasonExit, event.getValue(kEventPropertyReason).asInt());

    ASSERT_FALSE(root->hasEvent());
}

TEST_F(CommandTest, ExecuteFinishFastMode)
{
    loadDocument(EXECUTE_FINISH);

    executeCommand("Finish", {{"reason", "back"}}, true);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFinish, event.getType());
    ASSERT_EQ(kEventReasonBack, event.getValue(kEventPropertyReason).asInt());

    ASSERT_FALSE(root->hasEvent());
}

static const char *EXTERNAL_BINDING_UPDATE_TRANSFORM_DOCUMENT = R"({
  "type": "APL",
  "version": "1.3",
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "id": "myContainer",
        "width": "100%",
        "height": "100%",
        "bind": [
          {
            "name": "len",
            "value": 64,
            "type": "dimension"
          }
        ],
        "items": [
          {
            "type": "Text",
            "text": "Some text.",
            "transform": [
              {
                "translateX": "${len}"
              }
            ]
          }
        ]
      }
    ]
  }
})";

TEST_F(CommandTest, BindingUpdateTransform){
    loadDocument(EXTERNAL_BINDING_UPDATE_TRANSFORM_DOCUMENT);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    auto text = component->getCoreChildAt(0);
    ASSERT_TRUE(text);
    ASSERT_EQ(kComponentTypeText, text->getType());

    ASSERT_TRUE(IsEqual(Transform2D().translateX(64), text->getCalculated(kPropertyTransform).get<Transform2D>()));

    executeCommand("SetValue", {{"componentId", "myContainer"}, {"property", "len"}, {"value", "${500}"}}, false);

    ASSERT_TRUE(IsEqual(Transform2D().translateX(500), text->getCalculated(kPropertyTransform).get<Transform2D>()));
}

static const char *SIMPLE_VIDEO_DOCUMENT = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": {
        "type": "Video",
        "id": "VIDEO"
      }
    }
  }
})";

TEST_F(CommandTest, DisallowedDoesntExecuteControlMedia) {
    config->set(RootProperty::kDisallowVideo, true);

    loadDocument(SIMPLE_VIDEO_DOCUMENT);
    ASSERT_TRUE(component);
    auto v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    ASSERT_TRUE(v->isDisallowed());

    executeCommand("ControlMedia", {{"componentId", "VIDEO"}, {"command", "play"}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executeCommand("ControlMedia", {{"componentId", "VIDEO"}, {"command", "pause"}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executeCommand("ControlMedia", {{"componentId", "VIDEO"}, {"command", "next"}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executeCommand("ControlMedia", {{"componentId", "VIDEO"}, {"command", "previous"}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executeCommand("ControlMedia", {{"componentId", "VIDEO"}, {"command", "rewind"}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executeCommand("ControlMedia", {{"componentId", "VIDEO"}, {"command", "seek"}, {"value", 900}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executeCommand("ControlMedia", {{"componentId", "VIDEO"}, {"command", "seekTo"}, {"value", 900}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executeCommand("ControlMedia", {{"componentId", "VIDEO"}, {"command", "setTrack"}, {"value", 2}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(CommandTest, DisallowedDoesntExecutePlayMedia) {
    config->set(RootProperty::kDisallowVideo, true);

    loadDocument(SIMPLE_VIDEO_DOCUMENT);
    ASSERT_TRUE(component);

    executeCommand("PlayMedia", {{"componentId", "VIDEO"}, {"source", "http://music.amazon.com/s3/MAGIC_TRACK_HERE"}, {"audioTrack", "foreground"}}, false);

    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(CommandTest, DisallowedDoesntExecuteSetValue) {
    config->set(RootProperty::kDisallowVideo, true);

    loadDocument(SIMPLE_VIDEO_DOCUMENT);
    ASSERT_TRUE(component);

    component->setProperty(apl::kPropertyOpacity, 0.5);
    executeCommand("SetValue", {{"componentId", "VIDEO"}, {"property", "opacity"}, {"value", 1}}, false);

    ASSERT_EQ(0.5, component->getProperty(kPropertyOpacity).asFloat());
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(CommandTest, DisallowedDoesntExecuteSetState) {
    config->set(RootProperty::kDisallowVideo, true);

    loadDocument(SIMPLE_VIDEO_DOCUMENT);
    ASSERT_TRUE(component);

    component->setState(apl::kStateDisabled, true);
    executeCommand("SetState", {{"componentId", "VIDEO"}, {"state", "disabled"}, {"value", false}}, false);
    ASSERT_EQ(true, component->getState().get(kStateDisabled));
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    component->setState(apl::kStateChecked, true);
    executeCommand("SetState", {{"componentId", "VIDEO"}, {"state", "checked"}, {"value", false}}, false);
    ASSERT_EQ(true, component->getState().get(kStateChecked));
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(CommandTest, DisallowedDoesntExecuteSetFocus) {
    config->set(RootProperty::kDisallowVideo, true);

    loadDocument(SIMPLE_VIDEO_DOCUMENT);
    ASSERT_TRUE(component);

    component->setState(apl::kStateFocused, false);
    executeCommand("SetFocus", {{"componentId", "VIDEO"}}, false);

    ASSERT_EQ(false, component->getState().get(kStateFocused));
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(CommandTest, DisallowedDoesntExecuteClearFocus) {
    config->set(RootProperty::kDisallowVideo, true);

    loadDocument(SIMPLE_VIDEO_DOCUMENT);
    ASSERT_TRUE(component);

    component->setState(apl::kStateFocused, true);
    executeCommand("ClearFocus", {}, false);

    ASSERT_EQ(true, component->getState().get(kStateFocused));
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(ConsoleMessage());
}