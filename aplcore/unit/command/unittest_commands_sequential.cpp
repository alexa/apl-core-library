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


const char * SEQ_TEST = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "Sequential",
        "delay": 100,
        "repeatCount": 1,
        "commands": {
          "type": "SendEvent"
        }
      }
    }
  }
})";

TEST_F(CommandTest, SequentialTest)
{
    loadDocument(SEQ_TEST, R"({ "title": "Pecan Pie V" })");

    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    performClick(1, 1);

    // The sequential command has been created; now we must wait for 100
    ASSERT_EQ(1, mCommandCount[kCommandTypeSequential]);
    ASSERT_EQ(0, mCommandCount[kCommandTypeSendEvent]);
    ASSERT_EQ(0, mActionCount[kCommandTypeSequential]);
    ASSERT_EQ(0, mActionCount[kCommandTypeSendEvent]);

    loop->advanceToEnd();  // Each command should have fired appropriately
    ASSERT_EQ(1, mCommandCount[kCommandTypeSequential]);
    ASSERT_EQ(2, mCommandCount[kCommandTypeSendEvent]);
    ASSERT_EQ(1, mActionCount[kCommandTypeSequential]);
    ASSERT_EQ(2, mActionCount[kCommandTypeSendEvent]);

    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

static const char *TRY_CATCH_FINALLY = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "Sequential",
        "repeatCount": 2,
        "commands": {
          "type": "Custom",
          "delay": 1000,
          "arguments": [
            "try"
          ]
        },
        "catch": [
          {
            "type": "Custom",
            "arguments": [
              "catch1"
            ],
            "delay": 1000
          },
          {
            "type": "Custom",
            "arguments": [
              "catch2"
            ],
            "delay": 1000
          },
          {
            "type": "Custom",
            "arguments": [
              "catch3"
            ],
            "delay": 1000
          }
        ],
        "finally": [
          {
            "type": "Custom",
            "arguments": [
              "finally1"
            ],
            "delay": 1000
          },
          {
            "type": "Custom",
            "arguments": [
              "finally2"
            ],
            "delay": 1000
          },
          {
            "type": "Custom",
            "arguments": [
              "finally3"
            ],
            "delay": 1000
          }
        ]
      }
    }
  }
})";


// Let the entire command run normally through the "try" and "finally" parts
TEST_F(CommandTest, TryCatchFinally)
{
    loadDocument(TRY_CATCH_FINALLY);
    performClick(1, 1);

    // Time 0
    ASSERT_FALSE(root->hasEvent());

    // Standard commands
    for (int i = 0 ; i < 3 ; i++) {
        loop->advanceToTime(1000 + 1000 * i);
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();
        ASSERT_EQ(Object("try"), event.getValue(kEventPropertyArguments).at(0));
        ASSERT_FALSE(root->hasEvent());
    }

    // Finally commands, running in normal mode
    for (int i = 0 ; i < 3 ; i++) {
        loop->advanceToTime(4000 + 1000 * i);
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();
        std::string str = "finally"+std::to_string(i+1);
        ASSERT_EQ(Object(str), event.getValue(kEventPropertyArguments).at(0));
        ASSERT_FALSE(root->hasEvent());
    }
}

// Abort immediately.  This should run only catch and finally commands
TEST_F(CommandTest, TryCatchFinallyAbortImmediately)
{
    loadDocument(TRY_CATCH_FINALLY);
    performClick(1, 1);

    ASSERT_FALSE(root->hasEvent());
    root->cancelExecution();   // Cancel immediately.  This should switch to fastmode catch commands and finally commands

    // Catch commands
    for (int i = 0 ; i < 3 ; i++) {
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();
        std::string str = "catch"+std::to_string(i+1);
        ASSERT_EQ(Object(str), event.getValue(kEventPropertyArguments).at(0));
    }

    // Finally commands, running in fast mode
    for (int i = 0 ; i < 3 ; i++) {
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();
        std::string str = "finally"+std::to_string(i+1);
        ASSERT_EQ(Object(str), event.getValue(kEventPropertyArguments).at(0));
    }

    ASSERT_FALSE(root->hasEvent());
}

// Abort after a few "try" commands have run. This should execute catch and finally.
TEST_F(CommandTest, TryCatchFinallyAbortAfterOne)
{
    loadDocument(TRY_CATCH_FINALLY);
    performClick(1, 1);

    ASSERT_FALSE(root->hasEvent());

    // Standard commands
    loop->advanceToTime(1000);
    ASSERT_TRUE(root->hasEvent());
    auto evt = root->popEvent();
    ASSERT_EQ(Object("try"), evt.getValue(kEventPropertyArguments).at(0));
    ASSERT_FALSE(root->hasEvent());

    root->cancelExecution();   // Cancel.  This should run catch commands

    // Catch commands
    for (int i = 0 ; i < 3 ; i++) {
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();
        std::string str = "catch"+std::to_string(i+1);
        ASSERT_EQ(Object(str), event.getValue(kEventPropertyArguments).at(0));
    }

    // Finally commands, running in fast mode
    for (int i = 0 ; i < 3 ; i++) {
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();
        std::string str = "finally"+std::to_string(i+1);
        ASSERT_EQ(Object(str), event.getValue(kEventPropertyArguments).at(0));
    }

    ASSERT_FALSE(root->hasEvent());
}

// Abort after all of the regular commands, but before finally commands start.
TEST_F(CommandTest, TryCatchFinallyAbortAfterTry)
{
    loadDocument(TRY_CATCH_FINALLY);
    performClick(1, 1);

    ASSERT_FALSE(root->hasEvent());

    // Standard commands
    for (int i = 0 ; i < 3 ; i++) {
        loop->advanceToTime(1000 + 1000 * i);
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();
        ASSERT_EQ(Object("try"), event.getValue(kEventPropertyArguments).at(0));
        ASSERT_FALSE(root->hasEvent());
    }

    root->cancelExecution();

    // The first "finally" command was queued up and has been terminated, so we see the OTHER finally commands
    for (int i = 1 ; i < 3 ; i++) {
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();
        std::string str = "finally"+std::to_string(i+1);
        ASSERT_EQ(Object(str), event.getValue(kEventPropertyArguments).at(0));
    }

    ASSERT_FALSE(root->hasEvent());
}

static const char *REPEATED_SET_VALUE = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "width": 100,
      "height": 100,
      "items": {
        "type": "Text",
        "text": "Woof",
        "id": "dogText"
      },
      "onPress": {
        "type": "Sequential",
        "repeatCount": 6,
        "commands": {
          "type": "SetValue",
          "componentId": "dogText",
          "property": "opacity",
          "value": "${event.target.opacity - 0.2}",
          "delay": 100
        }
      }
    }
  }
})";

TEST_F(CommandTest, RepeatedSetValue)
{
    loadDocument(REPEATED_SET_VALUE);
    auto text = component->getChildAt(0);

    performClick(1, 1);

    ASSERT_FALSE(root->hasEvent());

    for (int i = 1 ; i <= 7 ; i++) {
        loop->advanceToTime(i * 100 + 1);
        ASSERT_NEAR(std::max(1.0 - i * 0.2, 0.0),
                    text->getCalculated(kPropertyOpacity).asNumber(), .001);
    }
}

const char *SEQUENTIAL_DATA_TEST = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "Sequential",
        "data": [
          { "delay": 250, "argument": "first" },
          { "delay": 300, "argument": "second" },
          { "delay": 350, "argument": "third" }
        ],
        "commands": [
          {
            "delay": "${data.delay}",
            "type": "SendEvent",
            "arguments": [ "first", "${data.argument}" ]
          },
          {
            "delay": "${data.delay}",
            "type": "SendEvent",
            "arguments": [ "second", "${data.argument}" ]
          },
          {
            "delay": "${data.delay}",
            "type": "SendEvent",
            "arguments": [ "third", "${data.argument}" ]
          }
        ]
      }
    }
  }
})";

TEST_F(CommandTest, SequentialDataTest)
{
    loadDocument(SEQUENTIAL_DATA_TEST);

    auto map = component->getCalculated();

    performClick(1, 1);

    // We create sequence of commands for every data element and execute them in parallel.

    // First data sequence
    advanceTime(250);

    ASSERT_TRUE(CheckSendEvent(root, "first", "first"));
    ASSERT_TRUE(!root->hasEvent());

    advanceTime(250);

    ASSERT_TRUE(CheckSendEvent(root, "second", "first"));
    ASSERT_TRUE(!root->hasEvent());

    advanceTime(250);

    ASSERT_TRUE(CheckSendEvent(root, "third", "first"));
    ASSERT_TRUE(!root->hasEvent());

    // Second data sequence
    advanceTime(300);

    ASSERT_TRUE(CheckSendEvent(root, "first", "second"));
    ASSERT_TRUE(!root->hasEvent());

    advanceTime(300);

    ASSERT_TRUE(CheckSendEvent(root, "second", "second"));
    ASSERT_TRUE(!root->hasEvent());

    advanceTime(300);

    ASSERT_TRUE(CheckSendEvent(root, "third", "second"));
    ASSERT_TRUE(!root->hasEvent());

    // Third data sequence
    advanceTime(350);

    ASSERT_TRUE(CheckSendEvent(root, "first", "third"));
    ASSERT_TRUE(!root->hasEvent());

    advanceTime(350);

    ASSERT_TRUE(CheckSendEvent(root, "second", "third"));
    ASSERT_TRUE(!root->hasEvent());

    advanceTime(350);

    ASSERT_TRUE(CheckSendEvent(root, "third", "third"));
    ASSERT_TRUE(!root->hasEvent());
}
