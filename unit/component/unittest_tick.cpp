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

class TickTest : public DocumentWrapper {};

static const char *SIMPLE = R"(
{
  "type": "APL",
  "version": "1.4",
  "handleTick": [
    {
      "minimumDelay": 300,
      "when": true,
      "commands": [
        {
          "type": "SendEvent",
          "sequencer": "SEQUENCER_DOCUMENT",
          "arguments": [ "DOCUMENT" ]
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": [ 100, 200 ],
      "item": [
        {
          "type": "Text",
          "id": "${data}",
          "text": "${data}",
          "handleTick": [
            {
              "minimumDelay": "${data}",
              "when": true,
              "commands": [
                {
                  "type": "SendEvent",
                  "sequencer": "SEQUENCER_${data}",
                  "arguments": [ "${event.source.value}" ]
                }
              ]
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(TickTest, Simple) {
    loadDocument(SIMPLE);

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "100"));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "200"));
    ASSERT_TRUE(CheckSendEvent(root, "100"));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "DOCUMENT"));
    ASSERT_TRUE(CheckSendEvent(root, "100"));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "200"));
    ASSERT_TRUE(CheckSendEvent(root, "100"));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "100"));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "200"));
    ASSERT_TRUE(CheckSendEvent(root, "100"));
    ASSERT_TRUE(CheckSendEvent(root, "DOCUMENT"));
}

static const char *REPEAT_COUNTER = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "bind": [
        {
          "name": "Pressed",
          "value": false,
          "type": "boolean"
        },
        {
          "name": "RepeatCounter",
          "value": 0,
          "type": "number"
        }
      ],
      "handleTick": {
        "when": "${Pressed}",
        "minimumDelay": 100,
        "commands": [
          {
            "type": "SetValue",
            "property": "RepeatCounter",
            "value": "${RepeatCounter + 1}"
          },
          {
            "type": "SendEvent",
            "sequencer": "SEQUENCER_TICK",
            "arguments": [ "${RepeatCounter}" ]
          }
        ]
      },
      "onDown": [
        {
          "type": "SetValue",
          "property": "Pressed",
          "value": true
        }
      ],
      "onUp": [
        {
          "type": "SetValue",
          "property": "Pressed",
          "value": false
        }
      ],
      "onPress": [
        {
          "type": "SetValue",
          "property": "RepeatCounter",
          "value": 0
        },
        {
          "type": "SendEvent",
          "arguments": [ "${RepeatCounter}" ]
        }
      ],
      "item": {
        "type": "Text",
        "text": "Count"
      }
    }
  }
})";

TEST_F(TickTest, Counter) {
    loadDocument(REPEAT_COUNTER);

    advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(0, 0)));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, 1.0));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, 2.0));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, 3.0));

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(0, 0)));
    ASSERT_TRUE(CheckSendEvent(root, 0.0));

    advanceTime(100);
    ASSERT_FALSE(root->hasEvent());
}

static const char *REPEAT_COUNTER_DOUBLE = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "bind": [
        {
          "name": "Pressed",
          "value": false,
          "type": "boolean"
        },
        {
          "name": "RepeatCounter",
          "value": 0,
          "type": "number"
        }
      ],
      "handleTick": [
        {
          "when": "${Pressed}",
          "minimumDelay": 100,
          "commands": [
            {
              "type": "SetValue",
              "property": "RepeatCounter",
              "value": "${RepeatCounter + 1}"
            }
          ]
        },
        {
          "minimumDelay": 250,
          "commands": [
            {
              "type": "SendEvent",
              "sequencer": "SEQUENCER_TICK",
              "arguments": [ "${RepeatCounter}" ]
            }
          ]
        }
      ],
      "onDown": [
        {
          "type": "SetValue",
          "property": "Pressed",
          "value": true
        }
      ],
      "onUp": [
        {
          "type": "SetValue",
          "property": "Pressed",
          "value": false
        }
      ],
      "onPress": [
        {
          "type": "SetValue",
          "property": "RepeatCounter",
          "value": 0
        },
        {
          "type": "SendEvent",
          "arguments": [ "${RepeatCounter}" ]
        }
      ],
      "item": {
        "type": "Text",
        "text": "Count"
      }
    }
  }
})";

TEST_F(TickTest, CounterDouble) {
    loadDocument(REPEAT_COUNTER_DOUBLE);

    advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, 0.0));

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(0, 0)));

    // 1 on 300, 1 on 400 and one on 5000
    advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, 3.0));

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(0, 0)));
    ASSERT_TRUE(CheckSendEvent(root, 0.0));

    advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, 0.0));
}

static const char *RATE_LIMITING = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "ScrollView",
      "width": 100,
      "height": 100,
      "bind": [
        {
          "name": "ScrollPosition",
          "value": 0,
          "type": "number"
        },
        {
          "name": "LastScrollPosition",
          "value": 0,
          "type": "number"
        }
      ],
      "handleTick": {
        "minimumDelay": 100,
        "when": "${ScrollPosition != LastScrollPosition}",
        "commands": [
          {
            "type": "SetValue",
            "property": "LastScrollPosition",
            "value": "${ScrollPosition}"
          },
          {
            "type": "SendEvent",
            "sequencer": "SendEventSequencer",
            "arguments": [
              "${ScrollPosition}"
            ]
          }
        ]
      },
      "onScroll": {
        "type": "SetValue",
        "property": "ScrollPosition",
        "value": "${event.source.position}"
      },
      "item": {
        "type": "Container",
        "width": "100%",
        "height": 1000
      }
    }
  }
})";

TEST_F(TickTest, RateLimiting) {
    loadDocument(RATE_LIMITING);

    advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    component->update(UpdateType::kUpdateScrollPosition, 100);

    advanceTime(50);
    ASSERT_FALSE(root->hasEvent());

    advanceTime(50);
    ASSERT_TRUE(CheckSendEvent(root, 1));

    advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    component->update(UpdateType::kUpdateScrollPosition, 300);

    advanceTime(50);
    ASSERT_FALSE(root->hasEvent());

    advanceTime(50);
    ASSERT_TRUE(CheckSendEvent(root, 3));
}

static const char *REMOVE_TICKER = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "item": [
        {
          "type": "Text",
          "id": "${data}",
          "text": "${data}",
          "handleTick": [
            {
              "minimumDelay": "${data}",
              "when": true,
              "commands": [
                {
                  "type": "SendEvent",
                  "sequencer": "SEQUENCER_${data}",
                  "arguments": [ "${event.source.value}" ]
                }
              ]
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(TickTest, RemoveTicker) {
    auto myArray = LiveArray::create(ObjectArray{100, 200});
    config->liveData("TestArray", myArray);
    loadDocument(REMOVE_TICKER);

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "100"));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "200"));
    ASSERT_TRUE(CheckSendEvent(root, "100"));

    myArray->remove(0);
    root->clearPending();

    advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "200"));
}

static const char *UNLIMITED_UPDATES = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "bind": [
        {
          "name": "RepeatCounter",
          "value": 0,
          "type": "number"
        }
      ],
      "handleTick": {
        "minimumDelay": 0,
        "commands": [
          {
            "type": "SetValue",
            "property": "RepeatCounter",
            "value": "${RepeatCounter + 1}"
          },
          {
            "type": "SendEvent",
            "sequencer": "SEQUENCER_TICK",
            "arguments": [ "${RepeatCounter}" ]
          }
        ]
      },
      "item": {
        "type": "Text",
        "text": "Test"
      }
    }
  }
})";

TEST_F(TickTest, FpsLimitedByDefault) {
    loadDocument(UNLIMITED_UPDATES);

    advanceTime(10);
    ASSERT_FALSE(root->hasEvent());

    advanceTime(10);
    ASSERT_TRUE(CheckSendEvent(root, 1.0));

    advanceTime(10);
    ASSERT_FALSE(root->hasEvent());

    advanceTime(10);
    ASSERT_TRUE(CheckSendEvent(root, 2.0));
}

TEST_F(TickTest, AdjustedFpsLimit) {
    config->tickHandlerUpdateLimit(10);
    loadDocument(UNLIMITED_UPDATES);

    advanceTime(10);
    ASSERT_TRUE(CheckSendEvent(root, 1.0));

    advanceTime(10);
    ASSERT_TRUE(CheckSendEvent(root, 2.0));

    advanceTime(10);
    ASSERT_TRUE(CheckSendEvent(root, 3.0));

    advanceTime(10);
    ASSERT_TRUE(CheckSendEvent(root, 4.0));
}

TEST_F(TickTest, CantGo0) {
    config->tickHandlerUpdateLimit(0);
    ASSERT_EQ(1.0, config->getTickHandlerUpdateLimit());
    loadDocument(UNLIMITED_UPDATES);

    advanceTime(0);
    ASSERT_FALSE(root->hasEvent());

    advanceTime(1);
    ASSERT_TRUE(CheckSendEvent(root, 1.0));

    advanceTime(1);
    ASSERT_TRUE(CheckSendEvent(root, 2.0));
}
