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

const char *PARALLEL_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "Parallel",
        "commands": [
          {
            "type": "Idle"
          },
          {
            "type": "Idle",
            "when": false
          },
          {
            "type": "Idle",
            "delay": 100
          },
          {
            "type": "Idle",
            "delay": 150,
            "when": false
          },
          {
            "type": "Idle",
            "delay": 200,
            "when": true
          }
        ]
      }
    }
  }
})";

TEST_F(CommandTest, ParallelTest)
{
    loadDocument(PARALLEL_TEST, R"({ "title": "Pecan Pie V" })");

    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    performClick(1, 1);

    loop->advanceToEnd();
    ASSERT_EQ(3, mCommandCount[kCommandTypeIdle]);
    ASSERT_EQ(3, mActionCount[kCommandTypeIdle]);
    ASSERT_EQ(200, loop->currentTime());
}

TEST_F(CommandTest, ParallelTestTerminated)
{
    loadDocument(PARALLEL_TEST, R"({ "title": "Pecan Pie V" })");

    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    performClick(1, 1);

    loop->advanceToTime(100);
    context->sequencer().reset();

    ASSERT_EQ(3, mCommandCount[kCommandTypeIdle]);
    ASSERT_EQ(2, mActionCount[kCommandTypeIdle]);  // One Idle doesn't fire until 200
    ASSERT_EQ(100, loop->currentTime());
}

const char *PARALLEL_DATA_TEST = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "Parallel",
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

TEST_F(CommandTest, ParallelDataTest)
{
    loadDocument(PARALLEL_DATA_TEST);

    auto map = component->getCalculated();

    performClick(1, 1);

    // We create sequence of commands for every data element and execute them in parallel.

    // First data sequence, 250 ms
    advanceTime(250);

    ASSERT_TRUE(CheckSendEvent(root, "first", "first"));
    ASSERT_TRUE(!root->hasEvent());

    // Second data sequence, 300 ms
    advanceTime(50);

    ASSERT_TRUE(CheckSendEvent(root, "first", "second"));
    ASSERT_TRUE(!root->hasEvent());

    // Third data sequence, 350 ms
    advanceTime(50);

    ASSERT_TRUE(CheckSendEvent(root, "first", "third"));
    ASSERT_TRUE(!root->hasEvent());

    // First data sequence 500 ms
    advanceTime(150);

    ASSERT_TRUE(CheckSendEvent(root, "second", "first"));
    ASSERT_TRUE(!root->hasEvent());

    // Second data sequence 600 ms
    advanceTime(100);

    ASSERT_TRUE(CheckSendEvent(root, "second", "second"));
    ASSERT_TRUE(!root->hasEvent());

    // Third data sequence 700 ms
    advanceTime(100);

    ASSERT_TRUE(CheckSendEvent(root, "second", "third"));
    ASSERT_TRUE(!root->hasEvent());


    // First data sequence 750 ms
    advanceTime(50);

    ASSERT_TRUE(CheckSendEvent(root, "third", "first"));
    ASSERT_TRUE(!root->hasEvent());

    // Second data sequence 900 ms
    advanceTime(150);

    ASSERT_TRUE(CheckSendEvent(root, "third", "second"));
    ASSERT_TRUE(!root->hasEvent());

    // Third data sequence 1050 ms
    advanceTime(150);

    ASSERT_TRUE(CheckSendEvent(root, "third", "third"));
    ASSERT_TRUE(!root->hasEvent());
}
