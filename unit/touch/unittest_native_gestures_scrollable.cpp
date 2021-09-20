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

#include "apl/touch/pointerevent.h"
#include "apl/animation/coreeasing.h"

using namespace apl;

class NativeGesturesScrollableTest : public DocumentWrapper {
public:
    NativeGesturesScrollableTest() : DocumentWrapper() {
        config->set({
            {RootProperty::kTapOrScrollTimeout, 5},
            {RootProperty::kPointerInactivityTimeout, 250},
            {RootProperty::kPointerSlopThreshold, 10},
            {RootProperty::kUEScrollerDeceleration, 0.2},
            {RootProperty::kUEScrollerVelocityEasing, "linear"},
            {RootProperty::kScrollFlingVelocityLimitEasingVertical, CoreEasing::bezier(0,1,0,1)},
            {RootProperty::kScrollFlingVelocityLimitEasingHorizontal, CoreEasing::bezier(0,1,0,1)},
        });
    }
};

TEST_F(NativeGesturesScrollableTest, Configuration) {
    ASSERT_EQ(Object(5), config->getProperty(RootProperty::kTapOrScrollTimeout));
    ASSERT_EQ(Object(0.5), config->getProperty(RootProperty::kSwipeAwayFulfillDistancePercentageThreshold));
    ASSERT_EQ(Object(CoreEasing::bezier(0,0,0.58,1)), config->getProperty(RootProperty::kSwipeAwayAnimationEasing));
    ASSERT_EQ(Object(500), config->getProperty(RootProperty::kSwipeVelocityThreshold));
    ASSERT_EQ(Object(2000), config->getProperty(RootProperty::kSwipeMaxVelocity));
    ASSERT_EQ(Object(200), config->getProperty(RootProperty::kDefaultSwipeAnimationDuration));
    ASSERT_EQ(Object(400), config->getProperty(RootProperty::kMaxSwipeAnimationDuration));
    ASSERT_EQ(Object(50), config->getProperty(RootProperty::kMinimumFlingVelocity));
    ASSERT_EQ(Object(1200), config->getProperty(RootProperty::kMaximumFlingVelocity));
    ASSERT_EQ(Object(250), config->getProperty(RootProperty::kPointerInactivityTimeout));
    ASSERT_EQ(Object(10), config->getProperty(RootProperty::kPointerSlopThreshold));
    ASSERT_EQ(Object(1000), config->getProperty(RootProperty::kScrollCommandDuration));
    ASSERT_EQ(Object(500), config->getProperty(RootProperty::kScrollSnapDuration));
    ASSERT_NEAR(1.48, config->getProperty(RootProperty::kScrollAngleSlopeVertical).getDouble(), 0.01);
    ASSERT_NEAR(0.64, config->getProperty(RootProperty::kScrollAngleSlopeHorizontal).getDouble(), 0.01);
    ASSERT_NEAR(0.84, config->getProperty(RootProperty::kSwipeAngleTolerance).getDouble(), 0.01);
    ASSERT_EQ(Object(CoreEasing::linear()), config->getProperty(RootProperty::kUEScrollerVelocityEasing));
    ASSERT_EQ(Object(CoreEasing::bezier(.65,0,.35,1)), config->getProperty(RootProperty::kUEScrollerDurationEasing));
    ASSERT_EQ(Object(3000), config->getProperty(RootProperty::kUEScrollerMaxDuration));
    ASSERT_EQ(Object(0.2), config->getProperty(RootProperty::kUEScrollerDeceleration));
}

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

TEST_F(NativeGesturesScrollableTest, Scroll)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(0, 725), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), false));

    // Scroll back up
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:yellow8"));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true, "onMove:yellow8"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:yellow8"));
    ASSERT_EQ(Point(0, 675), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,200), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 625), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollRotated)
{
    loadDocument(SCROLL_TEST);
    TransformComponent(root, "scrollings", "rotate", 90);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false,
        "onDown:yellow2"));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,100), true, "onMove:yellow2"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:yellow2"));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(0, 725), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollScaled)
{
    loadDocument(SCROLL_TEST);
    TransformComponent(root, "scrollings", "scale", 2);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false,
        "onDown:green1"));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
    ASSERT_EQ(Point(0, 25), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 50), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(0, 362.5), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollThresholdsRemainInGlobalCoordinateDimensions)
{
    loadDocument(SCROLL_TEST);
    TransformComponent(root, "scrollings", "scale", 2);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    ASSERT_EQ(Point(), component->scrollPosition());

    // Pointer slop threshold not met
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,95), false, "onMove:green1"));
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    advanceTime(300);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,90), true, "onUp:green1"));

    ASSERT_FALSE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckSendEvent(root, "onPress:green1"));

    // Min velocity not met
    advanceTime(600);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    advanceTime(800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,90), false, "onMove:green1"));
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    advanceTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,90), true, "onUp:green1"));
    ASSERT_EQ(Point(0, 0), component->scrollPosition());

    ASSERT_FALSE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckSendEvent(root, "onPress:green1"));

    // Min velocity and pointer slop thresholds met
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,88), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
    ASSERT_EQ(Point(0, 6), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,88), true));
    advanceTime(2900);

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(0, component->scrollPosition().getX());
    ASSERT_FLOAT_EQ(156, component->scrollPosition().getY());
}

TEST_F(NativeGesturesScrollableTest, ScrollSingularity)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false,
        "onDown:green1"));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    TransformComponent(root, "scrollings", "scale", 0);
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_FALSE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(session->checkAndClear());
}

TEST_F(NativeGesturesScrollableTest, ScrollHover)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,200), false, "onDown:yellow2"));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true, "onMove:yellow2"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:yellow2"));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), false));

    advanceTime(2600);
    ASSERT_EQ(Point(0, 725), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollTerminate)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    advanceTime(1600);
    // Interrupted here.
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), true, "onDown:red6"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:red6"));
    advanceTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    auto currentPosition = component->scrollPosition();
    advanceTime(500);
    ASSERT_EQ(currentPosition, component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollTapOrScrollTimeout)
{
    config->set(RootProperty::kTapOrScrollTimeout, 60);
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(1,100), false, "onDown:green1"));
    // Under the timeout is not recognized as move that can trigger the gesture
    advanceTime(50);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(1,75), false, "onMove:green1"));
    // After actually triggers
    advanceTime(50);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(1,50), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(1,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());

    advanceTime(2900);
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollCommand)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    auto ptr = executeCommand("Scroll", {{"componentId", "scrollings"}, {"distance", 1}}, false);

    loop->advanceToEnd();
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(300, component->scrollPosition().getY());
}

TEST_F(NativeGesturesScrollableTest, ScrollToCommand)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    auto ptr = executeCommand("ScrollToIndex", {{"componentId", "scrollings"}, {"index", 4}}, false);

    loop->advanceToEnd();
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(200, component->scrollPosition().getY());
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

TEST_F(NativeGesturesScrollableTest, LiveScroll)
{
    config->pointerInactivityTimeout(100);
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config->liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,250), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    // No update happened as not enough children to scroll
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    advanceTime(100);

    // LiveArray got more items here.
    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(myArray->size(), extender.begin(), extender.end());
    root->clearPending();

    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, LiveScrollBackwards)
{
    config->pointerInactivityTimeout(100);
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config->liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,150), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    // No update happened as not enough children to scroll
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    advanceTime(100);

    // LiveArray got more items here.
    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();

    ASSERT_EQ(Point(0, 500), component->scrollPosition());

    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,300), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,300), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 400), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, LiveFling)
{
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config->liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_TEST);
    advanceTime(10);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,200), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_EQ(Point(), component->scrollPosition());

    // LiveArray got more items here.
    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(0, extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());
    root->clearPending();
    ASSERT_EQ(Point(0, 500), component->scrollPosition());

    advanceTime(100);
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();
    advanceTime(100);
    advanceTime(2400);
    ASSERT_EQ(Point(0, 1225), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, LiveFlingBackwards)
{
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config->liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_TEST);
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    // Give ability to scroll backwards
    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 9}, true));

    ASSERT_EQ(Point(0, 500), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,200), true));

    ASSERT_EQ(Point(0, 400), component->scrollPosition());

    // LiveArray got more items here.
    myArray->insert(0, extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());

    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 2}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {3, 19}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {20, 24}, false));
    ASSERT_EQ(Point(0, 600), component->scrollPosition());

    advanceTime(100);
    ASSERT_EQ(Point(0, 675), component->scrollPosition());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 19}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {20, 24}, false));
    advanceTime(100);
    ASSERT_EQ(Point(0, 650), component->scrollPosition());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 19}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {20, 24}, false));
    advanceTime(2400);
    ASSERT_EQ(Point(0, 275), component->scrollPosition());
}

static const char *LIVE_SCROLL_SPACED_TEST = R"({
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
          "spacing": 20,
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

TEST_F(NativeGesturesScrollableTest, LiveScrollBackwardsSpaced)
{
    config->pointerInactivityTimeout(100);
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config->liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_SPACED_TEST);
    advanceTime(10);

    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();

    ASSERT_EQ(Point(0, 600), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,150), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    // No update happened as not enough children to scroll
    ASSERT_EQ(Point(0, 550), component->scrollPosition());
    advanceTime(100);

    // LiveArray got even more items here.
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();

    ASSERT_EQ(Point(0, 690), component->scrollPosition());

    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,300), true));
    ASSERT_EQ(Point(0, 710), component->scrollPosition());

    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,300), true));

    ASSERT_EQ(Point(0, 710), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, LiveFlingBackwardsSpaced)
{
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config->liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_SPACED_TEST);
    advanceTime(10);

    // Give ability to scroll backwards
    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();

    ASSERT_EQ(Point(0, 600), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,200), true));

    ASSERT_EQ(Point(0, 500), component->scrollPosition());

    // LiveArray got more items here.
    myArray->insert(0, extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());

    root->clearPending();
    ASSERT_EQ(Point(0, 640), component->scrollPosition());

    advanceTime(100);
    advanceTime(100);
    advanceTime(2400);
    ASSERT_EQ(Point(0, 475), component->scrollPosition());
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

TEST_F(NativeGesturesScrollableTest, ScrollSnapStart)
{
    loadDocument(SCROLL_SNAP_START_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(0, 725), component->scrollPosition());
    advanceTime(500);
    ASSERT_EQ(Point(0, 700), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollSnapStartLimit)
{
    loadDocument(SCROLL_SNAP_START_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    advanceTime(980);
    advanceTime(1000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(0, 950), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 850), component->scrollPosition());

    advanceTime(980);
    advanceTime(1000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(0, 850), component->scrollPosition());
}


static const char *HORIZONTAL_SCROLL_SNAP_START_TEST = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "snap": "start",
      "width": 250,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 100,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapStart)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_START_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(50, 0), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(100, 0), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(725, 0), component->scrollPosition());
    advanceTime(500);
    ASSERT_EQ(Point(700, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapStartRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_START_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-50, 0), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-100, 0), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(-725, 0), component->scrollPosition());
    advanceTime(500);
    ASSERT_EQ(Point(-700, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapStartLimitRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_START_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-50, 0), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-100, 0), component->scrollPosition());

    advanceTime(1000);
    advanceTime(1000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(-950, 0), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-900, 0), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-850, 0), component->scrollPosition());

    advanceTime(980);
    advanceTime(1000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(-850, 0), component->scrollPosition());
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

TEST_F(NativeGesturesScrollableTest, ScrollSnapForceStartLowVelocity)
{
    loadDocument(SCROLL_SNAP_FORCE_START_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,150), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 150), component->scrollPosition());

    advanceTime(1000);
    ASSERT_EQ(Point(0, 100), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollSnapForceStartLimit)
{
    loadDocument(SCROLL_SNAP_FORCE_START_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    advanceTime(1000);
    advanceTime(1000);
    advanceTime(1000);
    // Should not forcefully snap if scrolled to end of list
    ASSERT_EQ(Point(0, 950), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 850), component->scrollPosition());

    advanceTime(980);
    advanceTime(1000);
    // Should be at the end limit (which is accidentally snap).
    ASSERT_EQ(Point(), component->scrollPosition());
}

static const char *HORIZONTAL_SCROLL_SNAP_FORCE_START_TEST = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "snap": "forceStart",
      "width": 250,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 100,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapForceStartLowVelocity)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_FORCE_START_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(150,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,0), true));
    ASSERT_EQ(Point(50, 0), component->scrollPosition());
    advanceTime(800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(150, 0), component->scrollPosition());

    advanceTime(1000);
    ASSERT_EQ(Point(100, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapForceStartLowVelocityRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_FORCE_START_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-50, 0), component->scrollPosition());
    advanceTime(800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(150,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(150,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-150, 0), component->scrollPosition());

    advanceTime(1000);
    ASSERT_EQ(Point(-100, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapForceStartLimitRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_FORCE_START_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-50, 0), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-100, 0), component->scrollPosition());

    advanceTime(980);
    advanceTime(1000);
    advanceTime(1000);
    // Should not forcefully snap if scrolled to end of list
    ASSERT_EQ(Point(-950, 0), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-900, 0), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-850, 0), component->scrollPosition());

    advanceTime(980);
    advanceTime(1000);
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

TEST_F(NativeGesturesScrollableTest, ScrollSnapCenter)
{
    loadDocument(SCROLL_SNAP_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,110), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 60), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 110), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(0, 785), component->scrollPosition());
    advanceTime(500);
    ASSERT_EQ(Point(0, 825), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollSnapCenterLimit)
{
    loadDocument(SCROLL_SNAP_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    // root->updateTime(10);
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    // root->updateTime(20);
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    // root->updateTime(1000);
    advanceTime(980);
    // root->updateTime(2000);
    advanceTime(1000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(0, 950), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    // root->updateTime(2010);
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    // root->updateTime(20);
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 850), component->scrollPosition());

    // root->updateTime(3000);
    advanceTime(980);
    // root->updateTime(4000);
    advanceTime(1000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(), component->scrollPosition());
}

static const char *HORIZONTAL_SCROLL_SNAP_CENTER_TEST = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "snap": "center",
      "width": 250,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 100,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapCenter)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(110,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(60, 0), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(110, 0), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(785, 0), component->scrollPosition());
    advanceTime(500);
    ASSERT_EQ(Point(825, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapCenterRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_CENTER_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(60,0), true));
    ASSERT_EQ(Point(-60, 0), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(110,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(110,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-110, 0), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(-785, 0), component->scrollPosition());
    advanceTime(500);
    ASSERT_EQ(Point(-825, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapCenterLimitRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_CENTER_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-50, 0), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(10,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-100, 0), component->scrollPosition());

    advanceTime(980);
    advanceTime(1000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(-950, 0), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-900, 0), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-850, 0), component->scrollPosition());

    advanceTime(980);
    advanceTime(1000);
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

TEST_F(NativeGesturesScrollableTest, ScrollSnapForceCenterLowVelocity)
{
    loadDocument(SCROLL_SNAP_FORCE_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,150), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 150), component->scrollPosition());

    advanceTime(1000);
    ASSERT_EQ(Point(0, 125), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollSnapForceCenterLimit)
{
    loadDocument(SCROLL_SNAP_FORCE_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    advanceTime(5);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(5);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    advanceTime(1490);
    ASSERT_EQ(Point(0, 950), component->scrollPosition());
    advanceTime(1000);
    advanceTime(500);
    // Should not forcefully snap if scrolled to end of list
    ASSERT_EQ(Point(0, 950), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 850), component->scrollPosition());

    advanceTime(980);
    ASSERT_EQ(Point(), component->scrollPosition());
    advanceTime(1000);
    // Should not forcefully snap if scrolled to start of list
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
}

static const char *HORIZONTAL_SCROLL_SNAP_FORCE_CENTER_TEST = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "snap": "forceCenter",
      "width": 250,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 100,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapForceCenterLowVelocity)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_FORCE_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(150,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,0), true));
    ASSERT_EQ(Point(50, 0), component->scrollPosition());
    advanceTime(800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(150, 0), component->scrollPosition());

    advanceTime(1000);
    ASSERT_EQ(Point(125, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapForceCenterLowVelocityRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_FORCE_CENTER_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-50, 0), component->scrollPosition());
    advanceTime(800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(150,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(150,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-150, 0), component->scrollPosition());

    advanceTime(1000);
    ASSERT_EQ(Point(-125, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapForceCenterLimitRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_FORCE_CENTER_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(5);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-50, 0), component->scrollPosition());
    advanceTime(5);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-100, 0), component->scrollPosition());

    advanceTime(1490);
    ASSERT_EQ(Point(-950, 0), component->scrollPosition());
    advanceTime(1000);
    advanceTime(500);
    // Should not forcefully snap if scrolled to end of list
    ASSERT_EQ(Point(-950, 0), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-900, 0), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-850, 0), component->scrollPosition());

    advanceTime(980);
    ASSERT_EQ(Point(), component->scrollPosition());
    advanceTime(1000);
    // Should not forcefully snap if scrolled to start of list
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
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

TEST_F(NativeGesturesScrollableTest, ScrollSnapEnd)
{
    loadDocument(SCROLL_SNAP_END_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,110), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 60), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 110), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(0, 785), component->scrollPosition());
    advanceTime(500);
    ASSERT_EQ(Point(0, 750), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollSnapEndLimit)
{
    loadDocument(SCROLL_SNAP_END_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    advanceTime(980);
    advanceTime(1000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(0, 950), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 850), component->scrollPosition());

    advanceTime(980);
    advanceTime(1000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(), component->scrollPosition());
}

static const char *HORIZONTAL_SCROLL_SNAP_END_TEST = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "snap": "end",
      "width": 250,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 100,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapEnd)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_END_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(110,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(60, 0), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(110, 0), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(785, 0), component->scrollPosition());
    advanceTime(500);
    ASSERT_EQ(Point(750, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapEndRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_END_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(60,0), true));
    ASSERT_EQ(Point(-60, 0), component->scrollPosition());
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(110,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(110,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-110, 0), component->scrollPosition());

    advanceTime(2600);
    ASSERT_EQ(Point(-785, 0), component->scrollPosition());
    advanceTime(500);
    ASSERT_EQ(Point(-750, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapEndLimitRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_END_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-50, 0), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-100, 0), component->scrollPosition());

    advanceTime(980);
    advanceTime(1000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(-950, 0), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-900, 0), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-850, 0), component->scrollPosition());

    advanceTime(1980);
    advanceTime(1000);
    // Should be at the end limit, and not snap to item.
    ASSERT_EQ(Point(), component->scrollPosition());
}

static const char *HORIZONTAL_SCROLL_SNAP_FORCE_END_TEST = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "snap": "forceEnd",
      "width": 250,
      "height": 250,
      "data": ["red", "green", "yellow", "blue", "purple", "gray", "red", "green", "yellow", "blue", "purple", "gray"],
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}${index}",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Frame",
            "backgroundColor": "${data}",
            "width": 100,
            "height": 100
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapForceEndLowVelocity)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_FORCE_END_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(50, 0), component->scrollPosition());
    advanceTime(800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(100, 0), component->scrollPosition());

    advanceTime(1000);
    ASSERT_EQ(Point(150, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapForceEndLowVelocityRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_FORCE_END_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-50, 0), component->scrollPosition());
    advanceTime(800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-100, 0), component->scrollPosition());

    advanceTime(1000);
    ASSERT_EQ(Point(-150, 0), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, HorizontalScrollSnapForceEndLimitRTL)
{
    loadDocument(HORIZONTAL_SCROLL_SNAP_FORCE_END_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition,
                           kPropertyVisualHash));

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-50, 0), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-100, 0), component->scrollPosition());

    advanceTime(1480);
    ASSERT_EQ(Point(-950, 0), component->scrollPosition());
    advanceTime(500);
    // Should forcefully snap
    ASSERT_EQ(Point(-950, 0), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,0), true));
    ASSERT_EQ(Point(-900, 0), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(-850, 0), component->scrollPosition());

    advanceTime(1980);
    ASSERT_EQ(Point(), component->scrollPosition());
    advanceTime(1000);
    // Should not forcefully snap if scrolled to end of list
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
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

TEST_F(NativeGesturesScrollableTest, ScrollSnapForceEndLowVelocity)
{
    loadDocument(SCROLL_SNAP_FORCE_END_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    advanceTime(1000);
    ASSERT_EQ(Point(0, 150), component->scrollPosition());
}

TEST_F(NativeGesturesScrollableTest, ScrollSnapForceEndLimit)
{
    loadDocument(SCROLL_SNAP_FORCE_END_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    advanceTime(1480);
    ASSERT_EQ(Point(0, 950), component->scrollPosition());
    advanceTime(500);
    // Should forcefully snap
    ASSERT_EQ(Point(0, 950), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    advanceTime(10);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 850), component->scrollPosition());

    advanceTime(1980);
    ASSERT_EQ(Point(), component->scrollPosition());
    advanceTime(1000);
    // Should not forcefully snap if scrolled to limit
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
}

static const char *SCROLL_SNAP_SPACED_CENTER_TEST = R"({
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
          "spacing": 50,
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

TEST_F(NativeGesturesScrollableTest, ScrollSnapSpacedCenter)
{
    config->pointerInactivityTimeout(600);
    loadDocument(SCROLL_SNAP_SPACED_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    advanceTime(500);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,50), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(Point(0, 50), component->scrollPosition());

    advanceTime(2500);
    ASSERT_EQ(Point(0, 300), component->scrollPosition());
    advanceTime(1);

    advanceTime(1000);
    ASSERT_EQ(Point(0, 225), component->scrollPosition());
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
TEST_F(NativeGesturesScrollableTest, ScrollTriggersScroll)
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
    auto delta = config->getScrollCommandDuration();   // How long the scroll command should take
    advanceTime(delta * 2);
    ASSERT_EQ(Point(0,300), component->scrollPosition());  // distance = 100% + 50% = 300 dp
    ASSERT_FALSE(action->isPending());

    // The THIRD scroll command will complete within this time.  It will try to trigger a FOURTH scroll command,
    // but that will be dropped because the scroll view is already at the maximum scroll position
    advanceTime(delta * 2);
    ASSERT_EQ(Point(0,400), component->scrollPosition());
}


// When native scrolling (using touch), once we trigger the "Scroll" command the touch interaction terminates.
TEST_F(NativeGesturesScrollableTest, ScrollViewCancelNativeScrolling)
{
    metrics.size(200,200);
    loadDocument(SCROLL_TRIGGERS_SCROLL);

    ASSERT_FALSE(root->handlePointerEvent({PointerEventType::kPointerDown, Point(10,190)}));

    // Scroll up 90 units
    advanceTime(100);
    ASSERT_TRUE(root->handlePointerEvent({PointerEventType::kPointerMove, Point(10,100)}));
    ASSERT_EQ(Point(0,90), component->scrollPosition());

    // Scroll up another 50 units.  The Scroll method should execute and cancel the manual scrolling
    advanceTime(100);
    ASSERT_TRUE(root->handlePointerEvent({PointerEventType::kPointerMove, Point(10,50)}));
    ASSERT_EQ(Point(0,140), component->scrollPosition());

    // Keep scrolling - but the gesture should be cancelled now, so nothing happens
    ASSERT_TRUE(root->handlePointerEvent({PointerEventType::kPointerMove, Point(10,10)}));
    ASSERT_EQ(Point(0,140), component->scrollPosition());

    // Now delay until the Scroll command has finished
    auto delta = config->getScrollCommandDuration();   // How long the scroll command should take
    advanceTime(delta);
    ASSERT_EQ(Point(0,240), component->scrollPosition());

    // Releasing the pointer should not do anything
    ASSERT_TRUE(root->handlePointerEvent({PointerEventType::kPointerUp, Point(10,0)}));
    ASSERT_EQ(Point(0,240), component->scrollPosition());
}

static const char *EDIT_TEXT_IN_TAP_TOUCHABLE = R"apl({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Sequence",
        "width": "100%",
        "height": "100%",
        "alignItems": "center",
        "justifyContent": "spaceAround",
        "data": [{"color": "blue", "text": "Magic"}],
        "items": [
          {
            "type": "Frame",
            "backgroundColor": "white",
            "items": [
              {
                "type": "TouchWrapper",
                "width": 500,
                "item": {
                  "type": "Frame",
                  "backgroundColor": "${data.color}",
                  "height": 200,
                  "items": {
                    "type": "EditText",
                    "id": "targetEdit",
                    "text": "${data.text}",
                    "width": 500,
                    "height": 100,
                    "fontSize": 60
                  }
                },
                "onDown": {
                  "type": "SendEvent",
                  "arguments": "onDown",
                  "sequencer": "MAIN"
                },
                "onUp": {
                  "type": "SendEvent",
                  "arguments": "onUp",
                  "sequencer": "MAIN"
                }
              }
            ]
          }
        ]
      }
    ]
  }
})apl";

TEST_F(NativeGesturesScrollableTest, WrappedEditTextTap)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureRequestKeyboard);
    loadDocument(EDIT_TEXT_IN_TAP_TOUCHABLE);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(400,50), false,
                                   "onDown"));
    advanceTime(20);
    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,50))));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(apl::kEventTypeOpenKeyboard, event.getType());

    ASSERT_TRUE(CheckSendEvent(root, "onUp"));
}

static const char *EDIT_TEXT_IN_UP_TOUCHABLE = R"apl({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Sequence",
        "width": "100%",
        "height": "100%",
        "alignItems": "center",
        "justifyContent": "spaceAround",
        "data": [{"color": "blue", "text": "Magic"}],
        "items": [
          {
            "type": "Frame",
            "backgroundColor": "white",
            "items": [
              {
                "type": "TouchWrapper",
                "width": 500,
                "item": {
                  "type": "Frame",
                  "backgroundColor": "${data.color}",
                  "height": 200,
                  "items": {
                    "type": "EditText",
                    "id": "targetEdit",
                    "text": "${data.text}",
                    "width": 500,
                    "height": 100,
                    "fontSize": 60
                  }
                },
                "onUp": {
                  "type": "SendEvent",
                  "arguments": "onUp",
                  "sequencer": "MAIN"
                }
              }
            ]
          }
        ]
      }
    ]
  }
})apl";

TEST_F(NativeGesturesScrollableTest, WrappedEditTextUp)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureRequestKeyboard);
    loadDocument(EDIT_TEXT_IN_UP_TOUCHABLE);

    ASSERT_FALSE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,50))));
    advanceTime(20);
    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,50))));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(apl::kEventTypeOpenKeyboard, event.getType());

    ASSERT_TRUE(CheckSendEvent(root, "onUp"));
}

static const char *EDIT_TEXT_IN_NESTED_TOUCHABLES = R"apl({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Sequence",
        "width": "100%",
        "height": "100%",
        "alignItems": "center",
        "justifyContent": "spaceAround",
        "data": [{"color": "blue", "text": "Magic"}],
        "items": [
          {
            "type": "Frame",
            "backgroundColor": "white",
            "items": [
              {
                "type": "TouchWrapper",
                "width": 500,
                "item": {
                  "type": "Frame",
                  "backgroundColor": "${data.color}",
                  "height": 200,
                  "items": {
                    "type": "TouchWrapper",
                    "item": {
                      "type": "EditText",
                      "id": "targetEdit",
                      "text": "${data.text}",
                      "width": 500,
                      "height": 100,
                      "fontSize": 60
                    },
                    "onUp": {
                      "type": "SendEvent",
                      "arguments": "onUpInner",
                      "sequencer": "MAIN"
                    }
                  }
                },
                "onUp": {
                  "type": "SendEvent",
                  "arguments": "onUpOuter",
                  "sequencer": "MAIN"
                }
              }
            ]
          }
        ]
      }
    ]
  }
})apl";

TEST_F(NativeGesturesScrollableTest, WrappedEditTextNestedTouchWrappers)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureRequestKeyboard);
    loadDocument(EDIT_TEXT_IN_NESTED_TOUCHABLES);

    ASSERT_FALSE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,50))));
    advanceTime(20);
    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,50))));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(apl::kEventTypeOpenKeyboard, event.getType());

    ASSERT_TRUE(CheckSendEvent(root, "onUpInner"));
    ASSERT_FALSE(root->hasEvent());
}

static const char *EDIT_TEXT_IN_SWIPE_TOUCHABLE = R"apl({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Sequence",
        "width": "100%",
        "height": "100%",
        "alignItems": "center",
        "justifyContent": "spaceAround",
        "data": [{"color": "blue", "text": "Magic"}],
        "items": [
          {
            "type": "Frame",
            "backgroundColor": "white",
            "items": [
              {
                "type": "TouchWrapper",
                "width": 500,
                "item": {
                  "type": "Frame",
                  "backgroundColor": "${data.color}",
                  "height": 200,
                  "items": {
                    "type": "EditText",
                    "id": "targetEdit",
                    "text": "${data.text}",
                    "width": 500,
                    "height": 100,
                    "fontSize": 60
                  }
                },
                "gestures": [
                  {
                    "type": "SwipeAway",
                    "direction": "left",
                    "action": "reveal",
                    "items": {
                      "type": "Frame",
                      "backgroundColor": "purple",
                      "width": "100%",
                      "items": {
                        "type": "Frame",
                        "width": "50%",
                        "backgroundColor": "red",
                        "items": {
                          "type": "Text",
                          "text": "You've swiped",
                          "fontSize": 60,
                          "fontColor": "white"
                        }
                      }
                    },
                    "onSwipeDone": {
                      "type": "SendEvent",
                      "arguments": ["delete", "${index}"]
                    }
                  }
                ]
              }
            ]
          }
        ]
      }
    ]
  }
})apl";

TEST_F(NativeGesturesScrollableTest, WrappedEditTextSwipe)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureRequestKeyboard);
    loadDocument(EDIT_TEXT_IN_SWIPE_TOUCHABLE);

    ASSERT_FALSE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,50))));

    advanceTime(2000);

    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(50,50))));
    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(50,50))));

    advanceTime(2000);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

static const char *EDITTEXT = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "height": 100,
      "hint": "Example EditText",
      "hintWeight": "100",
      "hintColor": "grey"
    }
  }
})";

TEST_F(NativeGesturesScrollableTest, KeyboardRequestedOnTap)
{
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureRequestKeyboard);
    loadDocument(EDITTEXT);

    ASSERT_FALSE(root->handlePointerEvent(PointerEvent(apl::kPointerDown, Point(10, 10))));
    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(apl::kPointerUp, Point(10, 10))));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(apl::kEventTypeOpenKeyboard, event.getType());
}

static const char *DISPLAY_CONDITIONAL = R"({
  "type": "APL",
  "version": "1.7",
  "layouts": {
    "AlexaTextListItem": {
      "parameters": [
        { "name": "primaryText", "type": "string" },
        { "name": "secondaryText", "type": "string" },
        { "name": "primaryAction", "type": "any" }
      ],
      "items": [
        {
          "type": "TouchWrapper",
          "width": "100%",
          "height": 150,
          "onPress": "${primaryAction}",
          "item": {
            "type": "Container",
            "width": "100%",
            "inheritParentState": true,
            "items": [
              {
                "type": "Container",
                "grow": 1,
                "shrink": 1,
                "width": "100%",
                "items": [
                  { "type": "Text", "text": "${primaryText}", "fontSize": 80 },
                  { "type": "Text", "text": "${secondaryText}", "fontSize": 50 }
                ]
              }
            ]
          }
        }
      ]
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "items": [
          {
            "type": "Text",
            "text": "Recently Played",
            "fontSize": "25",
            "paddingLeft": 20,
            "paddingBottom": 50,
            "paddingTop": 20
          },
          {
            "type": "Sequence",
            "id": "scrollable",
            "height": "100%",
            "shrink": 1,
            "data": [
              "I am string One",
              "I am string Two",
              "I am string Three",
              "I am string Four",
              "I am string Five",
              "I am string Six",
              "I am string Seven",
              "I am string Eight",
              "I am string Nine"
            ],
            "scrollDirection": "vertical",
            "items": [
              {
                "type": "AlexaTextListItem",
                "display": "${index <= 5 ? 'normal' : 'none'}",
                "primaryText": "${data}",
                "secondaryText": "${index}",
                "primaryAction": {
                  "type": "SendEvent",
                  "arguments": ["${index}"]
                }
              }
            ]
          }
        ]
      }
    ]
  }
})";

TEST_F(NativeGesturesScrollableTest, DisplayConditional)
{
    metrics.size(1280, 800);
    loadDocument(DISPLAY_CONDITIONAL);

    auto scrollable = component->getCoreChildAt(1);

    ASSERT_EQ(9, scrollable->getChildCount());
    ASSERT_EQ(Point(0,0), scrollable->scrollPosition());

    ASSERT_EQ(kDisplayNormal, scrollable->getCoreChildAt(0)->getCalculated(kPropertyDisplay).getInteger());
    ASSERT_EQ(kDisplayNormal, scrollable->getCoreChildAt(1)->getCalculated(kPropertyDisplay).getInteger());
    ASSERT_EQ(kDisplayNormal, scrollable->getCoreChildAt(2)->getCalculated(kPropertyDisplay).getInteger());
    ASSERT_EQ(kDisplayNormal, scrollable->getCoreChildAt(3)->getCalculated(kPropertyDisplay).getInteger());
    ASSERT_EQ(kDisplayNormal, scrollable->getCoreChildAt(4)->getCalculated(kPropertyDisplay).getInteger());
    ASSERT_EQ(kDisplayNormal, scrollable->getCoreChildAt(5)->getCalculated(kPropertyDisplay).getInteger());
    ASSERT_EQ(kDisplayNone, scrollable->getCoreChildAt(6)->getCalculated(kPropertyDisplay).getInteger());
    ASSERT_EQ(kDisplayNone, scrollable->getCoreChildAt(7)->getCalculated(kPropertyDisplay).getInteger());
    ASSERT_EQ(kDisplayNone, scrollable->getCoreChildAt(8)->getCalculated(kPropertyDisplay).getInteger());

    ASSERT_FALSE(root->handlePointerEvent(PointerEvent(apl::kPointerDown, Point(10, 400))));
    advanceTime(50);
    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(apl::kPointerMove, Point(10, 100))));
    advanceTime(500);
    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(apl::kPointerUp, Point(10, 100))));
    advanceTime(50);

    ASSERT_EQ(Point(0,180), scrollable->scrollPosition());
}