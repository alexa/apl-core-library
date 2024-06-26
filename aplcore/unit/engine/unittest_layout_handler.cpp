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

class LayoutHandlerTest : public DocumentWrapper {};

static const char *BASIC_TEST = R"({
  "type": "APL",
  "version": "2023.3",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "id": "parent",
        "height": "100%",
        "width": "100%",
        "direction": "row",
        "onLayout": {
          "type": "SendEvent",
          "sequencer": "LAYOUT_EVENT",
          "arguments": [
            "${event.source.id}",
            "${event.width}",
            "${event.height}",
            "${event.x}",
            "${event.y}"
          ]
        },
        "items": [
          {
            "type": "Frame",
            "id": "f1",
            "height": "50%",
            "width": "200",
            "background": "red",
            "onLayout": {
              "type": "SendEvent",
              "sequencer": "LAYOUT_EVENT",
              "arguments": [
                "${event.source.id}",
                "${event.width}",
                "${event.height}",
                "${event.x}",
                "${event.y}"
              ]
            }
          },
          {
            "type": "Frame",
            "id": "f2",
            "height": "50%",
            "width": "30%",
            "background": "green",
            "onLayout": {
              "type": "SendEvent",
              "sequencer": "LAYOUT_EVENT",
              "arguments": [
                "${event.source.id}",
                "${event.width}",
                "${event.height}",
                "${event.x}",
                "${event.y}"
              ]
            }
          },
          {
            "type": "Text",
            "id": "f3",
            "height": "50%",
            "width": "auto",
            "maxLines": 1,
            "text": "Verry terrible text which does not fit.",
            "onLayout": {
              "type": "SendEvent",
              "sequencer": "LAYOUT_EVENT",
              "arguments": [
                "${event.source.id}",
                "${event.width}",
                "${event.height}",
                "${event.x}",
                "${event.y}"
              ]
            }
          }
        ]
      }
    ]
  }
})";

TEST_F(LayoutHandlerTest, Basic)
{
    metrics.size(600, 600);

    loadDocument(BASIC_TEST);

    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "parent", 600, 600, 0, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f1", 200, 300, 0, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f2", 180, 300, 200, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f3", 390, 300, 380, 0));
}

TEST_F(LayoutHandlerTest, FireOnRelayout)
{
    metrics.size(600, 600);

    loadDocument(BASIC_TEST);

    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "parent", 600, 600, 0, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f1", 200, 300, 0, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f2", 180, 300, 200, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f3", 390, 300, 380, 0));

    executeCommand("SetValue", {{"componentId", "f2"}, {"property", "width"}, {"value", 200}}, true);
    advanceTime(1);

    ASSERT_TRUE(CheckSendEvent(root, "f2", 200, 300, 200, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f3", 390, 300, 400, 0));
}

TEST_F(LayoutHandlerTest, NoHandlerOnNoChange)
{
    metrics.size(600, 600);

    loadDocument(BASIC_TEST);

    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "parent", 600, 600, 0, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f1", 200, 300, 0, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f2", 180, 300, 200, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f3", 390, 300, 380, 0));

    executeCommand("SetValue", {{"componentId", "f3"}, {"property", "width"}, {"value", 390}}, true);
    advanceTime(1);

    ASSERT_FALSE(CheckSendEvent(root));
}

static const char *SCROLLABLE = R"({
  "type": "APL",
  "version": "2023.3",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Sequence",
        "id": "parent",
        "height": 200,
        "width": 200,
        "onMount": {
          "type": "SendEvent",
          "sequencer": "MOUNT_EVENT",
          "arguments": [ "${event.source.id}" ]
        },
        "onLayout": {
          "type": "SendEvent",
          "sequencer": "LAYOUT_EVENT",
          "arguments": [
            "${event.source.id}",
            "${event.width}",
            "${event.height}",
            "${event.x}",
            "${event.y}"
          ]
        },
        "data": [1, 2, 3, 4, 5, 6],
        "items": [
          {
            "type": "Frame",
            "id": "f${data}",
            "height": 100,
            "width": "100%",
            "background": "red",
            "onMount": {
              "type": "SendEvent",
              "sequencer": "MOUNT_EVENT",
              "arguments": [ "${event.source.id}" ]
            },
            "onLayout": {
              "type": "SendEvent",
              "sequencer": "LAYOUT_EVENT",
              "arguments": [
                "${event.source.id}",
                "${event.width}",
                "${event.height}",
                "${event.x}",
                "${event.y}"
              ]
            }
          }
        ]
      }
    ]
  }
})";

TEST_F(LayoutHandlerTest, LazyInflationAndLayout)
{
    metrics.size(600, 600);
    config->set(apl::RootProperty::kSequenceChildCache, 1);

    loadDocument(SCROLLABLE);

    ASSERT_TRUE(component);

    // Let's judge it against onMount. Initial ones happen pretty much at the same time.
    // Only first page laid out
    ASSERT_TRUE(CheckSendEvent(root, "parent", 200, 200, 0, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f1", 200, 100, 0, 0));
    ASSERT_TRUE(CheckSendEvent(root, "f2", 200, 100, 0, 100));
    ASSERT_TRUE(CheckSendEvent(root, "f3", 200, 100, 0, 200));

    ASSERT_TRUE(CheckSendEvent(root, "parent"));
    ASSERT_TRUE(CheckSendEvent(root, "f1"));
    ASSERT_TRUE(CheckSendEvent(root, "f2"));
    ASSERT_TRUE(CheckSendEvent(root, "f3"));
    ASSERT_TRUE(CheckSendEvent(root, "f4"));
    ASSERT_TRUE(CheckSendEvent(root, "f5"));
    ASSERT_TRUE(CheckSendEvent(root, "f6"));

    // Next frame
    advanceTime(1);

    // Lazilly laid out
    ASSERT_TRUE(CheckSendEvent(root, "f4", 200, 100, 0, 300));
    ASSERT_TRUE(CheckSendEvent(root, "f5", 200, 100, 0, 400));

    root->clearDirty();

    // Scroll a bit
    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));
    advanceTime(2600);

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    // Laid out after scrolling.
    ASSERT_TRUE(CheckSendEvent(root, "f6", 200, 100, 0, 500));
}
