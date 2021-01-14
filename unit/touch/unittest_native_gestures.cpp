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

#include "apl/component/touchwrappercomponent.h"
#include "apl/touch/pointerevent.h"

using namespace apl;

class NativeGesturesTest : public DocumentWrapper {
public:
    NativeGesturesTest() : DocumentWrapper() {
        config.enableExperimentalFeature(RootConfig::kExperimentalFeatureHandleScrollingAndPagingInCore);
        config.tapOrScrollTimeout(500);
        config.pointerInactivityTimeout(250);
        config.pointerSlopThreshold(10);
    }
};

static const char *SCROLL_TEST = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "id": "scrollings",
      "width": 200,
      "height": 300,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 200,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 200,
            "height": 100
          },
          "onDown": {
            "type": "SendEvent",
            "sequencer": "MAIN",
            "arguments": [ "onDown:${event.source.id}" ]
          },
          "onMove": {
            "type": "SendEvent",
            "sequencer": "MAIN",
            "arguments": [ "onMove:${event.source.id}" ]
          },
          "onUp": {
            "type": "SendEvent",
            "sequencer": "MAIN",
            "arguments": [ "onUp:${event.source.id}" ]
          },
          "onCancel": {
            "type": "SendEvent",
            "sequencer": "MAIN",
            "arguments": [ "onCancel:${event.source.id}" ]
          },
          "onPress": {
            "type": "SendEvent",
            "arguments": [ "onPress:${event.source.id}" ]
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesTest, Scroll)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(3000);
    ASSERT_EQ(Point(0, 725), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), false));

    // Scroll back up
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:yellow8"));
    root->updateTime(3200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true));
    ASSERT_EQ(Point(0, 675), component->scrollPosition());
    root->updateTime(3400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,200), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 625), component->scrollPosition());

    root->updateTime(6000);
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollRotated)
{
    loadDocument(SCROLL_TEST);
    TransformComponent(root, "scrollings", "rotate", 90);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false,
        "onDown:yellow2"));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,100), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(3000);
    ASSERT_EQ(Point(0, 725), component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollScaled)
{
    loadDocument(SCROLL_TEST);
    TransformComponent(root, "scrollings", "scale", 2);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false,
        "onDown:green1"));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 25), component->scrollPosition());
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 50), component->scrollPosition());

    root->updateTime(3000);
    ASSERT_EQ(Point(0, 362.5), component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollThresholdsRemainInGlobalCoordinateDimensions)
{
    loadDocument(SCROLL_TEST);
    TransformComponent(root, "scrollings", "scale", 2);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    ASSERT_EQ(Point(), component->scrollPosition());

    // Pointer slop threshold not met
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false,
        "onDown:green1"));
    root->updateTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,95), false,
        "onMove:green1"));
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,90), false,
        "onUp:green1"));

    ASSERT_FALSE(CheckDirty(component, kPropertyScrollPosition));
    ASSERT_TRUE(CheckSendEvent(root, "onPress:green1"));

    // Min velocity not met
    root->updateTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false,
        "onDown:green1"));
    root->updateTime(1400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,90), false,
        "onMove:green1"));
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    root->updateTime(1800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,90), false,
        "onUp:green1"));
    ASSERT_EQ(Point(0, 0), component->scrollPosition());

    ASSERT_FALSE(CheckDirty(component, kPropertyScrollPosition));
    ASSERT_TRUE(CheckSendEvent(root, "onPress:green1"));

    // Min velocity and pointer slop thresholds met
    root->updateTime(2000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false,
        "onDown:green1"));
    root->updateTime(2100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,88), true));
    ASSERT_EQ(Point(0, 6), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,88), true));
    root->updateTime(5000);

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));
    ASSERT_EQ(0, component->scrollPosition().getX());
    ASSERT_FLOAT_EQ(156, component->scrollPosition().getY());
}

TEST_F(NativeGesturesTest, ScrollSingularity)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false,
        "onDown:green1"));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    TransformComponent(root, "scrollings", "scale", 0);
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_FALSE(CheckDirty(component, kPropertyScrollPosition));
    ASSERT_TRUE(session->checkAndClear());
}

TEST_F(NativeGesturesTest, ScrollHover)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,200), false, "onDown:yellow2"));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), false));

    root->updateTime(3000);
    ASSERT_EQ(Point(0, 725), component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollTerminate)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(2000);
    // Interrupted here.
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), true));
    root->updateTime(3000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    auto currentPosition = component->scrollPosition();
    root->updateTime(3500);
    ASSERT_EQ(currentPosition, component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollTimedOut)
{
    config.tapOrScrollTimeout(100);
    loadDocument(SCROLL_TEST);

    root->updateTime(100);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(1,100), false, "onDown:green1"));
    root->updateTime(450);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(1,50), false, "onMove:green1"));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(1,50), false, "onUp:green1"));
    ASSERT_EQ(Point(0, 0), component->scrollPosition());

    // Should work afterwards

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    root->updateTime(500);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(550);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    // And again

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), true));
    root->updateTime(1050);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 150), component->scrollPosition());
    root->updateTime(1200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 200), component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollCommand)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    auto ptr = executeCommand("Scroll", {{"componentId", "scrollings"}, {"distance", 1}}, false);

    loop->advanceToEnd();
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(300, component->scrollPosition().getY());
}

TEST_F(NativeGesturesTest, ScrollToCommand)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    auto ptr = executeCommand("ScrollToIndex", {{"componentId", "scrollings"}, {"index", 4}}, false);

    loop->advanceToEnd();
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(200, component->scrollPosition().getY());
}

static const char *PAGER_TEST = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "id": "pagers",
      "width": 500,
      "height": 500,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "onPageChanged": [
        {
          "type": "SendEvent",
          "sequencer": "SET_PAGE"
        }
      ],
      "items": [
        {
          "type": "Frame",
          "id": "${data}",
          "backgroundColor": "${data}",
          "width": "100%",
          "height": "100%"
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesTest, AutoPage)
{
    loadDocument(PAGER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    auto ptr = executeCommand("AutoPage", {{"componentId", "pagers"}, {"count", 4}, {"duration", 100}}, false);

    root->updateTime(700);
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    root->updateTime(1400);
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    root->updateTime(2100);
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    loop->advanceToEnd();
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();

    ASSERT_TRUE(ptr->isResolved());
    auto visibleChild = component->getCoreChildAt(4);
    ASSERT_EQ(1.0, visibleChild->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(4, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesTest, SetPage)
{
    loadDocument(PAGER_TEST);

    auto ptr = executeCommand("SetPage", {{"componentId", "pagers"}, {"position", "absolute"}, {"value", 8}}, false);
    root->updateTime(600);

    ASSERT_EQ(8, component->pagePosition());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();

    ASSERT_TRUE(ptr->isResolved());
    auto visibleChild = component->getCoreChildAt(8);
    ASSERT_EQ(1.0, visibleChild->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    ////////////////////////

    ptr = executeCommand("SetPage", {{"componentId", "pagers"}, {"position", "relative"}, {"value", -2}}, false);
    root->updateTime(1200);
    ASSERT_EQ(6, component->pagePosition());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();

    ASSERT_TRUE(ptr->isResolved());
    visibleChild = component->getCoreChildAt(6);
    ASSERT_EQ(1.0, visibleChild->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesTest, PageFlingRight)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(1600);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}


TEST_F(NativeGesturesTest, PageFlingRightWithCancel)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerCancel, Point(100,10)));
    root->clearPending();
    root->updateTime(1600);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesTest, PageFlingLeft)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();
    root->updateTime(1600);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(11, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesTest, PageFlingScaled)
{
    loadDocument(PAGER_TEST);
    TransformComponent(root, "pagers", "scale", 2);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(1600);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesTest, PageFlingRotated)
{
    loadDocument(PAGER_TEST);
    TransformComponent(root, "pagers", "rotate", 45);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    // Move the pointer ~11 pixels at 45 degrees to match the rotation
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,250)));
    root->updateTime(220); // Make sure the velocity just meets the threshold
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(242,242)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(242,242)));
    root->clearPending();
    root->updateTime(1600);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesTest, PageFlingSingularity)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    TransformComponent(root, "pagers", "scale", 0);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(1600);

    ASSERT_FALSE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, component->pagePosition());
    ASSERT_TRUE(session->checkAndClear());
}

TEST_F(NativeGesturesTest, PageFlingThresholdsRemainInGlobalCoordinateDimensions)
{
    loadDocument(PAGER_TEST);
    TransformComponent(root, "pagers", "scale", 2);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    // Pointer slop threshold too small
    root->updateTime(0);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(395,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(395,10)));
    root->clearPending();
    root->updateTime(1600);

    ASSERT_FALSE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(0, component->pagePosition());
    ASSERT_FALSE(root->hasEvent());

    // Velocity too low
    root->updateTime(2000);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(2600);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(375,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(375,10)));
    root->clearPending();
    root->updateTime(3600);

    ASSERT_FALSE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(0, component->pagePosition());
    ASSERT_FALSE(root->hasEvent());

    // Both minimum thresholds met, just barely
    root->updateTime(4000);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(4100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(389,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(389,10)));
    root->clearPending();
    root->updateTime(5600);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

static const char *PAGER_TEST_DEFAULT_ANIMATION = R"apl({
  "type": "APL",
  "version": "1.4",
  "layouts": {
    "Potato": {
      "parameters": ["c", "i"],
      "item": [
        {
          "type": "Frame",
          "width": "100%",
          "height": "100%",
          "id": "${c}${i}",
          "item": {
            "type": "Frame",
            "backgroundColor": "${c}",
            "width": "100%",
            "height": "100%",
            "item": {
              "type": "Text",
              "text": "${i}"
            }
          }
        }
      ]
    }
  },
  "mainTemplate": {
    "parameters": [ "direction" ],
    "item": {
      "type": "Pager",
      "pageDirection": "${direction}",
      "initialPage": 1,
      "height": 500,
      "width": 500,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "Potato",
          "c": "${data}",
          "i": "${index}"
        }
      ]
    }
  }
})apl";

static const char *PAGER_DEFAULT_DATA = R"apl({
    "do": "higherAbove",
    "nav": "wrap",
    "direction": "horizontal"
})apl";

TEST_F(NativeGesturesTest, PageFlingLeftDefault)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_DEFAULT_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-300), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(200), nextChild));
    ASSERT_EQ(2, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-400), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), nextChild));

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-500), currentChild));
    ASSERT_EQ(0, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(2, component->pagePosition());
}

TEST_F(NativeGesturesTest, PageFlingRightDefault)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_DEFAULT_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), nextChild));

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(500), currentChild));
    ASSERT_EQ(0, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(0, component->pagePosition());
}

static const char *PAGER_VERTICAL_DATA = R"apl({
    "do": "higherAbove",
    "nav": "wrap",
    "direction": "vertical"
})apl";

TEST_F(NativeGesturesTest, PageFlingUpDefault)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_VERTICAL_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10, 400)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(10,100)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(10,100)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-300), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(200), nextChild));
    ASSERT_EQ(2, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-400), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(100), nextChild));

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-500), currentChild));
    ASSERT_EQ(0, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(2, component->pagePosition());
}

TEST_F(NativeGesturesTest, PageFlingDownDefault)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_VERTICAL_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10,100)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(10, 400)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(10,400)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(300), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-200), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(400), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-100), nextChild));

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(500), currentChild));
    ASSERT_EQ(0, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(0, component->pagePosition());
}

static const char *PAGER_TEST_CUSTOM_ANIMATION = R"apl({
  "type": "APL",
  "version": "1.4",
  "layouts": {
    "Potato": {
      "parameters": ["c", "i"],
      "item": [
        {
          "type": "Frame",
          "width": "100%",
          "height": "100%",
          "id": "${c}${i}",
          "item": {
            "type": "Frame",
            "backgroundColor": "${c}",
            "width": "100%",
            "height": "100%",
            "item": {
              "type": "Text",
              "text": "${i}"
            }
          }
        }
      ]
    }
  },
  "mainTemplate": {
    "parameters": [ "do", "nav", "direction" ],
    "item": {
      "type": "Pager",
      "navigation": "${nav}",
      "pageDirection": "${direction}",
      "initialPage": 1,
      "height": 500,
      "width": 500,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "Potato",
          "c": "${data}",
          "i": "${index}"
        }
      ],
      "handlePageMove": [
        {
          "when": "${event.direction == 'left' || event.direction == 'right'}",
          "drawOrder": "${do}",
          "commands": [
            {
              "type": "SetValue",
              "componentId": "${event.currentChild.uid}",
              "property": "transform",
              "value": [
                {
                  "translateX": "${100 * event.amount * (event.direction == 'left' ? -1 : 1)}%"
                }
              ]
            },
            {
              "type": "SetValue",
              "componentId": "${event.nextChild.uid}",
              "property": "transform",
              "value": [
                {
                  "translateX": "${100 * (1.0 - event.amount) * (event.direction == 'left' ? 1 : -1)}%"
                }
              ]
            }
          ]
        },
        {
          "when": "${event.direction == 'up' || event.direction == 'down'}",
          "drawOrder": "${do}",
          "commands": [
            {
              "type": "SetValue",
              "componentId": "${event.currentChild.uid}",
              "property": "transform",
              "value": [
                {
                  "translateY": "${100 * event.amount * (event.direction == 'up' ? -1 : 1)}%"
                }
              ]
            },
            {
              "type": "SetValue",
              "componentId": "${event.nextChild.uid}",
              "property": "transform",
              "value": [
                {
                  "translateY": "${100 * (1.0 - event.amount) * (event.direction == 'up' ? 1 : -1)}%"
                }
              ]
            }
          ]
        }
      ]
    }
  }
})apl";

TEST_F(NativeGesturesTest, PageFlingLeftCustom)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_DEFAULT_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-300), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(200), nextChild));
    ASSERT_EQ(2, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-400), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), nextChild));

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-500), currentChild));
    ASSERT_EQ(0, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(2, component->pagePosition());
}

TEST_F(NativeGesturesTest, PageFlingRightCustom)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_DEFAULT_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), nextChild));

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(500), currentChild));
    ASSERT_EQ(0, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(0, component->pagePosition());
}

TEST_F(NativeGesturesTest, PageFlingUpCustom)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_VERTICAL_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10, 400)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(10,100)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(10,100)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-300), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(200), nextChild));
    ASSERT_EQ(2, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-400), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(100), nextChild));

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-500), currentChild));
    ASSERT_EQ(0, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(2, component->pagePosition());
}

TEST_F(NativeGesturesTest, PageFlingDownCustom)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_VERTICAL_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10,100)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(10, 400)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(10,400)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(300), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-200), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(400), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-100), nextChild));

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform, kPropertyZOrder));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(500), currentChild));
    ASSERT_EQ(0, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(0, component->pagePosition());
}

TEST_F(NativeGesturesTest, CustomPageHigherAbove)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_DEFAULT_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-150), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(350), nextChild));
    ASSERT_EQ(2, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(150), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-350), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->clearDirty();
}

static const char *PAGER_CUSTOM_HIGHER_BELOW_DATA = R"apl({
    "do": "higherBelow",
    "nav": "wrap",
    "direction": "horizontal"
})apl";

TEST_F(NativeGesturesTest, CustomPageHigherBelow)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_HIGHER_BELOW_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-150), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(350), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(150), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-350), nextChild));
    ASSERT_EQ(2, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->clearDirty();
}

static const char *PAGER_CUSTOM_NEXT_ABOVE_DATA = R"apl({
    "do": "nextAbove",
    "nav": "wrap",
    "direction": "horizontal"
})apl";

TEST_F(NativeGesturesTest, CustomPageNextAbove)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NEXT_ABOVE_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-150), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(350), nextChild));
    ASSERT_EQ(2, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(150), currentChild));
    ASSERT_EQ(1, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-350), nextChild));
    ASSERT_EQ(2, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->clearDirty();
}

static const char *PAGER_CUSTOM_NEXT_BELOW_DATA = R"apl({
    "do": "nextBelow",
    "nav": "wrap",
    "direction": "horizontal"
})apl";

TEST_F(NativeGesturesTest, CustomPageNextBelow)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NEXT_BELOW_DATA);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-150), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(350), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(150), currentChild));
    ASSERT_EQ(2, currentChild->getCalculated(kPropertyZOrder).getInteger());

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-350), nextChild));
    ASSERT_EQ(1, nextChild->getCalculated(kPropertyZOrder).getInteger());

    root->clearDirty();
}

static const char *PAGER_CUSTOM_NAVIGATE_WRAP = R"apl({
    "do": "nextAbove",
    "nav": "wrap",
    "direction": "horizontal"
})apl";

TEST_F(NativeGesturesTest, CustomPageWrap)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NAVIGATE_WRAP);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), nextChild));

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(500,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    root->updateTime(400);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(500,10)));

    root->updateTime(1000);
    root->clearPending();
    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    root->updateTime(1100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(500,10)));
    root->clearPending();

    currentChild = component->getChildAt(0);
    nextChild = component->getChildAt(11);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    root->clearDirty();
}

static const char *PAGER_CUSTOM_NAVIGATE_NORMAL = R"apl({
    "do": "nextAbove",
    "nav": "normal",
    "direction": "horizontal"
})apl";

TEST_F(NativeGesturesTest, CustomPageNormal)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NAVIGATE_NORMAL);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), nextChild));

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(500,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    root->updateTime(400);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(500,10)));

    root->updateTime(1000);
    root->clearPending();
    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    root->updateTime(1100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(500,10)));
    root->clearPending();

    currentChild = component->getChildAt(0);
    nextChild = component->getChildAt(11);

    ASSERT_TRUE(CheckTransform(Transform2D(), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D(), nextChild));

    root->clearDirty();
}

static const char *PAGER_CUSTOM_NAVIGATE_FORWARD_ONLY = R"apl({
    "do": "nextAbove",
    "nav": "forward-only",
    "direction": "horizontal"
})apl";

TEST_F(NativeGesturesTest, CustomPageForwardOnly)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NAVIGATE_FORWARD_ONLY);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), nextChild));

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(500,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D(), nextChild));

    root->clearDirty();
}

static const char *PAGER_CUSTOM_NAVIGATE_NONE = R"apl({
    "do": "nextAbove",
    "nav": "none",
    "direction": "horizontal"
})apl";

TEST_F(NativeGesturesTest, CustomPageNone)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NAVIGATE_NONE);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckTransform(Transform2D(), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D(), nextChild));

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(500,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D(), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D(), nextChild));

    root->clearDirty();
}

static const char *LIVE_SCROLL_TEST = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "id": "scrollings",
      "width": 200,
      "height": 500,
      "data": "${TestArray}",
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 200,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 200,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesTest, LiveScroll)
{
    config.pointerInactivityTimeout(100);
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config.liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,250), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    // No update happened as not enough children to scroll
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    root->updateTime(300);

    // LiveArray got more items here.
    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(myArray->size(), extender.begin(), extender.end());
    root->clearPending();

    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    root->updateTime(500);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());
}

TEST_F(NativeGesturesTest, LiveScrollBackwards)
{
    config.pointerInactivityTimeout(100);
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config.liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,150), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    // No update happened as not enough children to scroll
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    root->updateTime(300);

    // LiveArray got more items here.
    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();

    ASSERT_EQ(Point(0, 500), component->scrollPosition());

    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,300), true));
    root->updateTime(500);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,300), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 400), component->scrollPosition());
}

TEST_F(NativeGesturesTest, LiveFling)
{
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config.liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,200), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true));
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_EQ(Point(), component->scrollPosition());

    // LiveArray got more items here.
    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(0, extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());
    root->clearPending();

    root->updateTime(500);
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();
    root->updateTime(600);
    root->updateTime(3000);
    ASSERT_EQ(Point(0, 1225), component->scrollPosition());
}

TEST_F(NativeGesturesTest, LiveFlingBackwards)
{
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config.liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true));
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,200), true));

    ASSERT_EQ(Point(), component->scrollPosition());

    // LiveArray got more items here.
    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(0, extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());

    root->clearPending();
    ASSERT_EQ(Point(0, 500), component->scrollPosition());

    root->updateTime(500);
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();
    root->updateTime(600);
    root->updateTime(3000);
    ASSERT_EQ(Point(0, 375), component->scrollPosition());
}

static const char *SCROLL_SNAP_START_TEST = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "snap": "start",
      "width": 200,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 200,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 200,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesTest, ScrollSnapStart)
{
    loadDocument(SCROLL_SNAP_START_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(3000);
    ASSERT_EQ(Point(0, 725), component->scrollPosition());
    root->updateTime(3500);
    ASSERT_EQ(Point(0, 700), component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollSnapStartLimit)
{
    loadDocument(SCROLL_SNAP_START_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(20);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(1000);
    root->updateTime(2000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(0, 950), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    root->updateTime(2010);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    root->updateTime(20);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 850), component->scrollPosition());

    root->updateTime(3000);
    root->updateTime(4000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(0, 850), component->scrollPosition());
}

static const char *SCROLL_SNAP_FORCE_START_TEST = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "snap": "forceStart",
      "width": 200,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 200,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 200,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesTest, ScrollSnapForceStartLowVelocity)
{
    loadDocument(SCROLL_SNAP_FORCE_START_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,150), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 150), component->scrollPosition());

    root->updateTime(2000);
    ASSERT_EQ(Point(0, 100), component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollSnapForceStartLimit)
{
    loadDocument(SCROLL_SNAP_FORCE_START_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(20);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(1000);
    root->updateTime(2000);
    // Should forcefully snap
    ASSERT_EQ(Point(0, 900), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    root->updateTime(2010);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 850), component->scrollPosition());
    root->updateTime(20);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 800), component->scrollPosition());

    root->updateTime(3000);
    // Should be at the end limit (which is accidentally snap).
    ASSERT_EQ(Point(), component->scrollPosition());
}

static const char *SCROLL_SNAP_CENTER_TEST = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "snap": "center",
      "width": 200,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 200,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 200,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesTest, ScrollSnapCenter)
{
    loadDocument(SCROLL_SNAP_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,110), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 60), component->scrollPosition());
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 110), component->scrollPosition());

    root->updateTime(3000);
    ASSERT_EQ(Point(0, 785), component->scrollPosition());
    root->updateTime(3500);
    ASSERT_EQ(Point(0, 825), component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollSnapCenterLimit)
{
    loadDocument(SCROLL_SNAP_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(20);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(1000);
    root->updateTime(2000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(0, 950), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    root->updateTime(2010);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    root->updateTime(20);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 850), component->scrollPosition());

    root->updateTime(3000);
    root->updateTime(4000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(), component->scrollPosition());
}

static const char *SCROLL_SNAP_FORCE_CENTER_TEST = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "snap": "forceCenter",
      "width": 200,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 200,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 200,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesTest, ScrollSnapForceCenterLowVelocity)
{
    loadDocument(SCROLL_SNAP_FORCE_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,150), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 150), component->scrollPosition());

    root->updateTime(2000);
    ASSERT_EQ(Point(0, 125), component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollSnapForceCenterLimit)
{
    loadDocument(SCROLL_SNAP_FORCE_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(5);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(1000);
    ASSERT_EQ(Point(0, 950), component->scrollPosition());
    root->updateTime(2000);
    // Should forcefully snap
    ASSERT_EQ(Point(0, 925), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    root->updateTime(2010);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 875), component->scrollPosition());
    root->updateTime(20);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 825), component->scrollPosition());

    root->updateTime(3000);
    ASSERT_EQ(Point(), component->scrollPosition());
    root->updateTime(4000);
    // Should be at the end limit (which is accidentally snap).
    ASSERT_EQ(Point(0, 25), component->scrollPosition());
}

static const char *SCROLL_SNAP_END_TEST = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "snap": "end",
      "width": 200,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 200,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 200,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesTest, ScrollSnapEnd)
{
    loadDocument(SCROLL_SNAP_END_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,110), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 60), component->scrollPosition());
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 110), component->scrollPosition());

    root->updateTime(3000);
    ASSERT_EQ(Point(0, 785), component->scrollPosition());
    root->updateTime(3500);
    ASSERT_EQ(Point(0, 750), component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollSnapEndLimit)
{
    loadDocument(SCROLL_SNAP_END_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(20);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(1000);
    root->updateTime(2000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(0, 950), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    root->updateTime(2010);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    root->updateTime(20);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 850), component->scrollPosition());

    root->updateTime(3000);
    root->updateTime(4000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(), component->scrollPosition());
}

static const char *SCROLL_SNAP_FORCE_END_TEST = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "snap": "forceEnd",
      "width": 200,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 200,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 200,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesTest, ScrollSnapForceEndLowVelocity)
{
    loadDocument(SCROLL_SNAP_FORCE_END_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(2000);
    ASSERT_EQ(Point(0, 150), component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollSnapForceEndLimit)
{
    loadDocument(SCROLL_SNAP_FORCE_END_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(20);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(1000);
    ASSERT_EQ(Point(0, 950), component->scrollPosition());
    root->updateTime(2000);
    // Should forcefully snap
    ASSERT_EQ(Point(0, 950), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    root->updateTime(2010);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    root->updateTime(20);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 850), component->scrollPosition());

    root->updateTime(3000);
    ASSERT_EQ(Point(), component->scrollPosition());
    root->updateTime(4000);
    // Should be at the end limit (which is accidentally snap).
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
}


static const char *SCROLL_TRIGGERS_SCROLL = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "ScrollView",
          "id": "SCROLLER",
          "width": 200,
          "height": 200,
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 600
          },
          "onScroll": {
            "when": "${event.source.position > 0.5}",
            "type": "Scroll",
            "distance": 0.5,
            "sequencer": "OTHER"
          }
        }
      }
    }
)apl";

// Execute a "Scroll" command, which will trigger a _second_ "Scroll" command.
TEST_F(NativeGesturesTest, ScrollTriggersScroll)
{
    metrics.size(200,200);
    loadDocument(SCROLL_TRIGGERS_SCROLL);
    ASSERT_TRUE(component);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("Scroll", {{"componentId", "SCROLLER"}, {"distance", 1}}, false);
    ASSERT_TRUE(action);

    // Skip ahead TWO scroll delays.  The first scroll command will complete in a single step and trigger
    // the second scroll command, which will ALSO complete in a single step.  The second scroll command
    // will trigger a THIRD scroll command.
    auto delta = config.getScrollCommandDuration();   // How long the scroll command should take
    root->updateTime(delta * 2);
    ASSERT_EQ(Point(0,300), component->scrollPosition());  // distance = 100% + 50% = 300 dp
    ASSERT_FALSE(action->isPending());

    // The THIRD scroll command will complete within this time.  It will try to trigger a FOURTH scroll command,
    // but that will be dropped because the scroll view is already at the maximum scroll position
    root->updateTime(delta * 4);
    ASSERT_EQ(Point(0,400), component->scrollPosition());
}