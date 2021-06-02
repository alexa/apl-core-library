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

class NativeGesturesTest : public DocumentWrapper {
public:
    NativeGesturesTest() : DocumentWrapper() {
        config->enableExperimentalFeature(RootConfig::kExperimentalFeatureHandleScrollingAndPagingInCore);
        config->set({
            {RootProperty::kTapOrScrollTimeout, 5},
            {RootProperty::kPointerInactivityTimeout, 250},
            {RootProperty::kPointerSlopThreshold, 10},
            {RootProperty::kUEScrollerDeceleration, 0.2},
            {RootProperty::kUEScrollerVelocityEasing, "linear"},
            {RootProperty::kScrollFlingVelocityLimitEasingVertical, CoreEasing::bezier(0,1,0,1)},
            {RootProperty::kScrollFlingVelocityLimitEasingHorizontal, CoreEasing::bezier(0,1,0,1)},
            {RootProperty::kDefaultPagerAnimationEasing, CoreEasing::linear()},
        });
    }
};

TEST_F(NativeGesturesTest, Configuration) {
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
    ASSERT_EQ(Object(600), config->getProperty(RootProperty::kDefaultPagerAnimationDuration));
    ASSERT_EQ(Object(CoreEasing::linear()), config->getProperty(RootProperty::kDefaultPagerAnimationEasing));
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

TEST_F(NativeGesturesTest, Scroll)
{
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
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
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true, "onMove:yellow8"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:yellow8"));
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
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(50,100), true, "onMove:yellow2"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:yellow2"));
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
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
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
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    root->updateTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,95), false, "onMove:green1"));
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,90), true, "onUp:green1"));

    ASSERT_FALSE(CheckDirty(component, kPropertyScrollPosition));
    ASSERT_TRUE(CheckSendEvent(root, "onPress:green1"));

    // Min velocity not met
    root->updateTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    root->updateTime(1400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,90), false, "onMove:green1"));
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    root->updateTime(1800);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,90), true, "onUp:green1"));
    ASSERT_EQ(Point(0, 0), component->scrollPosition());

    ASSERT_FALSE(CheckDirty(component, kPropertyScrollPosition));
    ASSERT_TRUE(CheckSendEvent(root, "onPress:green1"));

    // Min velocity and pointer slop thresholds met
    root->updateTime(2000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false, "onDown:green1"));
    root->updateTime(2100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,88), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
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
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
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
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true, "onMove:yellow2"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:yellow2"));
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
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,0), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,0), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 100), component->scrollPosition());

    root->updateTime(2000);
    // Interrupted here.
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), true, "onDown:red6"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:red6"));
    root->updateTime(3000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    auto currentPosition = component->scrollPosition();
    root->updateTime(3500);
    ASSERT_EQ(currentPosition, component->scrollPosition());
}

TEST_F(NativeGesturesTest, ScrollTapOrScrollTimeout)
{
    config->set(RootProperty::kTapOrScrollTimeout, 60);
    loadDocument(SCROLL_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(1,100), false, "onDown:green1"));
    // Under the timeout is not recognized as move that can trigger the gesture
    root->updateTime(50);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(1,75), false, "onMove:green1"));
    // After actually triggers
    root->updateTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(1,50), true, "onMove:green1"));
    ASSERT_TRUE(CheckSendEvent(root, "onCancel:green1"));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(1,50), true));
    ASSERT_EQ(Point(0, 50), component->scrollPosition());

    root->updateTime(3000);
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
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
  "version": "1.6",
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
          "id": "${data}${index}",
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

    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());

    auto ptr = executeCommand("AutoPage", {{"componentId", "pagers"}, {"count", 4}, {"duration", 100}}, false);
    root->updateTime(200);
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    root->updateTime(700);
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    root->updateTime(1400);
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("blue3", component->getDisplayedChildAt(1)->getId());
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    root->updateTime(2100);
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("blue3", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("purple4", component->getDisplayedChildAt(1)->getId());
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
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("purple4", visibleChild->getId());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesTest, SetPage)
{
    loadDocument(PAGER_TEST);

    auto ptr = executeCommand("SetPage", {{"componentId", "pagers"}, {"position", "absolute"}, {"value", 8}}, false);
    // Takes no time per requirements.
    root->updateTime(10);

    ASSERT_EQ(8, component->pagePosition());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow8", component->getDisplayedChildAt(0)->getId());

    ASSERT_TRUE(ptr->isResolved());
    auto visibleChild = component->getCoreChildAt(8);
    ASSERT_EQ(1.0, visibleChild->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    ////////////////////////

    ptr = executeCommand("SetPage", {{"componentId", "pagers"}, {"position", "relative"}, {"value", -2}}, false);
    root->updateTime(100);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component->getChildAt(6), kPropertyLaidOut, kPropertyInnerBounds, kPropertyTransform, kPropertyBounds));

    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red6", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow8", component->getDisplayedChildAt(1)->getId());

    root->updateTime(700);
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red6", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ(6, component->pagePosition());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();

    ASSERT_TRUE(ptr->isResolved());
    visibleChild = component->getCoreChildAt(6);
    ASSERT_EQ(1.0, visibleChild->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

static const char *PAGER_END_FLING_BUG = R"(
{
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "height": "500px",
      "width": "500px",
      "navigation": "normal",
      "items": [
        {
          "type": "Text",
          "text": "Text content shown on page #0"
        },
        {
          "type": "Text",
          "text": "Text content shown on page #1"
        }
      ]
    }
  }
}
)";

/**
 * Make sure we can't fling past the end of a list when navigation: normal. This tests a fix for a bug
 * were the pager would wrap at the end of a list if the user started another fling during a fling
 * gesture at the end of a list.
 */
TEST_F(NativeGesturesTest, PagerFlingDoesntWrapAtEndOfListForNavigationNormal)
{
    loadDocument(PAGER_END_FLING_BUG);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(200);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(300);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(400);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(1, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(500);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(600);

    ASSERT_EQ(1, component->pagePosition());


    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(700);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(800);

    ASSERT_EQ(1, component->pagePosition());
}

/**
 * Make sure we can't fling before the start of a list when navigation: normal. This tests a fix for
 * a bug were the pager would wrap at the start of a list if the user started another fling during a
 * fling gesture at the start of a list.
 */
TEST_F(NativeGesturesTest, PagerFlingDoesntWrapAtStartOfListForNavigationNormal) {
    loadDocument(PAGER_END_FLING_BUG);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400, 10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100, 10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100, 10)));
    root->clearPending();
    root->updateTime(200);

    ASSERT_EQ(0, component->pagePosition());

    root->updateTime(1300);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100, 10)));
    root->updateTime(1400);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400, 10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400, 10)));
    root->clearPending();
    root->updateTime(1500);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(1, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100, 10)));
    root->updateTime(1600);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400, 10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400, 10)));
    root->clearPending();
    root->updateTime(1700);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100, 10)));
    root->updateTime(1800);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400, 10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400, 10)));
    root->clearPending();
    root->updateTime(1900);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100, 10)));
    root->updateTime(2000);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400, 10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400, 10)));
    root->clearPending();
    root->updateTime(2100);

    ASSERT_EQ(0, component->pagePosition());
}


static const char *PAGER_TEST_OLD = R"({
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
          "id": "${data}${index}",
          "backgroundColor": "${data}",
          "width": "100%",
          "height": "100%"
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesTest, SetPageRelativeOldVersion)
{
    loadDocument(PAGER_TEST_OLD);

    auto ptr = executeCommand("SetPage", {{"componentId", "pagers"}, {"position", "absolute"}, {"value", 8}}, false);
    // Takes no time per requirements.
    root->updateTime(10);

    ASSERT_EQ(8, component->pagePosition());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow8", component->getDisplayedChildAt(0)->getId());

    ASSERT_TRUE(ptr->isResolved());
    auto visibleChild = component->getCoreChildAt(8);
    ASSERT_EQ(1.0, visibleChild->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    ////////////////////////

    ptr = executeCommand("SetPage", {{"componentId", "pagers"}, {"position", "relative"}, {"value", -2}}, false);
    // Takes no time for compatibility purposes on < 1.6
    root->updateTime(20);

    ASSERT_EQ(6, component->pagePosition());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    root->clearDirty();

    visibleChild = component->getCoreChildAt(6);
    ASSERT_EQ(1.0, visibleChild->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesTest, AutoPageOldVersion)
{
    loadDocument(PAGER_TEST_OLD);

    ASSERT_EQ(Point(), component->scrollPosition());

    auto ptr = executeCommand("AutoPage", {{"componentId", "pagers"}, {"count", 3}, {"duration", 100}}, false);
    root->updateTime(10);
    ASSERT_EQ(1, component->pagePosition());
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    root->updateTime(110);
    ASSERT_EQ(2, component->pagePosition());
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    root->updateTime(220);
    ASSERT_EQ(3, component->pagePosition());
    root->clearDirty();
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

TEST_F(NativeGesturesTest, PageFlingRightTapOrScrollTimeout)
{
    config->set(RootProperty::kTapOrScrollTimeout, 60);
    loadDocument(PAGER_TEST);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(400,10), false));
    root->updateTime(50);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(250,10), false));
    root->updateTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,10), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100,10), true));
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

TEST_F(NativeGesturesTest, PageFlingTooWide)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,400)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,400)));
    root->clearPending();
    root->updateTime(1600);

    ASSERT_EQ(0, component->pagePosition());
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

TEST_F(NativeGesturesTest, PageSequentialFlingRight)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(200);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(300);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(400);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    root->updateTime(1000);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(2, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

static const char *PAGER_ONPAGECHANGE_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.1",
      "mainTemplate": {
        "items": {
          "type": "Pager",
          "id": "myPager",
          "width": 500,
          "height": 500,
          "items": {
            "type": "Text",
            "id": "id${data}",
            "text": "TEXT${data}",
            "speech": "URL${data}"
          },
          "data": [
            1,
            2,
            3,
            4
          ],
          "onPageChanged": {
            "type": "SendEvent",
            "arguments": [
              "${event.target.page}"
            ]
          }
        }
      }
    }
)apl";

TEST_F(NativeGesturesTest, PageSequentialFlingRightWithOnPageChange)
{
    loadDocument(PAGER_ONPAGECHANGE_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(200);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(300);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(400);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(1, component->pagePosition());

    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    root->updateTime(1000);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(2, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesTest, PageSequentialFlingRightCancelOut)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(200);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    root->updateTime(300);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();
    root->updateTime(1000);

    ASSERT_EQ(0, component->pagePosition());
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

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-500), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesTest, PageFlingRightDefault)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_DEFAULT_DATA);

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(500), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

static const char *PAGER_VERTICAL_DATA = R"apl({
    "do": "higherAbove",
    "nav": "wrap",
    "direction": "vertical"
})apl";

TEST_F(NativeGesturesTest, PageFlingUpDefault)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_VERTICAL_DATA);

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10, 400)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(10,100)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(10,100)));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-500), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesTest, PageFlingDownDefault)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_VERTICAL_DATA);

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10,100)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(10, 400)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(10,400)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateY(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateY(500), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
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

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-500), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesTest, PageFlingRightCustom)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_DEFAULT_DATA);

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(500), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesTest, PageFlingUpCustom)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_VERTICAL_DATA);

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10, 400)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(10,100)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(10,100)));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-500), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesTest, PageFlingDownCustom)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_VERTICAL_DATA);

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10,100)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(10, 400)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(10,400)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateY(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    root->updateTime(400);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    root->updateTime(700);
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateY(500), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesTest, CustomPageHigherAbove)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_DEFAULT_DATA);

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();


    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-150), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(350), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(150), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-350), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

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

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-150), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(350), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(150), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-350), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

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
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(350), nextChild));

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(150), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-350), nextChild));

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

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-150), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(350), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(150), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-350), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

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

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(500,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

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

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("gray11", component->getDisplayedChildAt(1)->getId());

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

    auto nextChild = component->getChildAt(2);
    auto currentChild = component->getChildAt(1);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), nextChild));

    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(500,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

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

    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());

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

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(500,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), nextChild));

    // during fling current and next are on screen
    root->clearPending();
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

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

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D(), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D(), nextChild));

    // only current on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->updateTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(500,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D(), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D(), nextChild));

    // only current on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->clearDirty();
}

static const char *REPEATED_AUTO = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "height": "100%",
      "width": "100%",
      "id": "mainPager",
      "data": ["red", "blue", "green", "yellow"],
      "items": [
        {
            "type": "Frame",
            "width": 200,
            "height": 200,
            "alignSelf": "center",
            "backgroundColor": "${data}"
        }
      ],
      "onMount": {
        "type": "Sequential",
        "sequencer": "pagingSequencer",
        "repeatCount": 3,
        "commands": [
          {
            "type": "AutoPage",
            "delay": 2000,
            "duration": 2000
          }
        ]
      }
    }
  }
})";

TEST_F(NativeGesturesTest, SequencedAutoPageOnMainInterrupt)
{
    loadDocument(REPEATED_AUTO);

    root->updateTime(4000);
    root->clearPending();
    ASSERT_EQ(1, component->pagePosition());

    root->updateTime(6000);
    root->clearPending();
    ASSERT_EQ(2, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(50,50)));
    root->updateTime(6100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(650,50)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(650,50)));

    // AutoPage cancelled. Repeat kicks in and continues from manually set page.
    root->updateTime(7000);
    root->clearPending();
    ASSERT_EQ(1, component->pagePosition());

    root->updateTime(9000);
    root->clearPending();
    ASSERT_EQ(2, component->pagePosition());
}

TEST_F(NativeGesturesTest, SequencedAutoPageOnMainInterruptUser)
{
    loadDocument(REPEATED_AUTO);

    root->updateTime(4000);
    root->clearPending();
    ASSERT_EQ(1, component->pagePosition());

    root->updateTime(6000);
    root->clearPending();
    ASSERT_EQ(2, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(50,50)));
    root->updateTime(6100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(650,50)));

    // Gesture reset
    root->updateTime(9000);
    root->clearPending();
    ASSERT_EQ(3, component->pagePosition());
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
    config->pointerInactivityTimeout(100);
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config->liveData("TestArray", myArray);
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
    config->pointerInactivityTimeout(100);
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config->liveData("TestArray", myArray);
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
    config->liveData("TestArray", myArray);
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
    config->liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_TEST);

    // Give ability to scroll backwards
    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();

    ASSERT_EQ(Point(0, 500), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true));
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,200), true));

    ASSERT_EQ(Point(0, 400), component->scrollPosition());

    // LiveArray got more items here.
    myArray->insert(0, extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());

    root->clearPending();
    ASSERT_EQ(Point(0, 600), component->scrollPosition());

    root->updateTime(500);
    root->clearPending();
    root->updateTime(600);
    root->clearPending();
    root->updateTime(3000);
    root->clearPending();
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

TEST_F(NativeGesturesTest, LiveScrollBackwardsSpaced)
{
    config->pointerInactivityTimeout(100);
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config->liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_SPACED_TEST);

    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();

    ASSERT_EQ(Point(0, 600), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,150), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    // No update happened as not enough children to scroll
    ASSERT_EQ(Point(0, 550), component->scrollPosition());
    root->updateTime(300);

    // LiveArray got even more items here.
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();

    ASSERT_EQ(Point(0, 690), component->scrollPosition());

    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,300), true));
    ASSERT_EQ(Point(0, 710), component->scrollPosition());

    root->updateTime(500);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,300), true));

    ASSERT_EQ(Point(0, 710), component->scrollPosition());
}

TEST_F(NativeGesturesTest, LiveFlingBackwardsSpaced)
{
    auto myArray = LiveArray::create(ObjectArray{"red", "green", "yellow", "blue", "purple"});
    config->liveData("TestArray", myArray);
    loadDocument(LIVE_SCROLL_SPACED_TEST);

    // Give ability to scroll backwards
    auto extender = ObjectArray{"red", "green", "yellow", "blue", "purple"};
    myArray->insert(0, extender.begin(), extender.end());
    root->clearPending();

    ASSERT_EQ(Point(0, 600), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,150), true));
    root->updateTime(400);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,200), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,200), true));

    ASSERT_EQ(Point(0, 500), component->scrollPosition());

    // LiveArray got more items here.
    myArray->insert(0, extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());
    myArray->insert(myArray->size(), extender.begin(), extender.end());

    root->clearPending();
    ASSERT_EQ(Point(0, 640), component->scrollPosition());

    root->updateTime(500);
    root->clearPending();
    root->updateTime(600);
    root->clearPending();
    root->updateTime(3000);
    root->clearPending();
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
    root->updateTime(3000);
    // Should forcefully snap
    ASSERT_EQ(Point(0, 900), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    root->updateTime(3010);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 850), component->scrollPosition());
    root->updateTime(3020);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 800), component->scrollPosition());

    root->updateTime(4000);
    root->updateTime(5000);
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

    root->updateTime(1500);
    ASSERT_EQ(Point(0, 950), component->scrollPosition());
    root->updateTime(2500);
    root->updateTime(3000);
    // Should forcefully snap
    ASSERT_EQ(Point(0, 925), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    root->updateTime(3010);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 875), component->scrollPosition());
    root->updateTime(3020);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 825), component->scrollPosition());

    root->updateTime(4000);
    ASSERT_EQ(Point(), component->scrollPosition());
    root->updateTime(5000);
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

    root->updateTime(1500);
    ASSERT_EQ(Point(0, 950), component->scrollPosition());
    root->updateTime(2000);
    // Should forcefully snap
    ASSERT_EQ(Point(0, 950), component->scrollPosition());

    // Go to start
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,0), false));
    root->updateTime(2010);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    root->updateTime(2020);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,100), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 850), component->scrollPosition());

    root->updateTime(4000);
    ASSERT_EQ(Point(), component->scrollPosition());
    root->updateTime(5000);
    // Should be at the end limit (which is accidentally snap).
    ASSERT_EQ(Point(0, 50), component->scrollPosition());
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

TEST_F(NativeGesturesTest, ScrollSnapSpacedCenter)
{
    config->pointerInactivityTimeout(600);
    loadDocument(SCROLL_SNAP_SPACED_CENTER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
    root->updateTime(500);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0,50), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(0,50), true));

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition));

    ASSERT_EQ(Point(0, 50), component->scrollPosition());

    root->updateTime(3000);
    ASSERT_EQ(Point(0, 300), component->scrollPosition());
    root->updateTime(3001);

    root->updateTime(4000);
    ASSERT_EQ(Point(0, 375), component->scrollPosition());
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
    auto delta = config->getScrollCommandDuration();   // How long the scroll command should take
    root->updateTime(delta * 2);
    ASSERT_EQ(Point(0,300), component->scrollPosition());  // distance = 100% + 50% = 300 dp
    ASSERT_FALSE(action->isPending());

    // The THIRD scroll command will complete within this time.  It will try to trigger a FOURTH scroll command,
    // but that will be dropped because the scroll view is already at the maximum scroll position
    root->updateTime(delta * 4);
    ASSERT_EQ(Point(0,400), component->scrollPosition());
}


// When native scrolling (using touch), once we trigger the "Scroll" command the touch interaction terminates.
TEST_F(NativeGesturesTest, ScrollViewCancelNativeScrolling)
{
    metrics.size(200,200);
    loadDocument(SCROLL_TRIGGERS_SCROLL);

    ASSERT_FALSE(root->handlePointerEvent({PointerEventType::kPointerDown, Point(10,190)}));

    // Scroll up 90 units
    root->updateTime(100);
    ASSERT_TRUE(root->handlePointerEvent({PointerEventType::kPointerMove, Point(10,100)}));
    ASSERT_EQ(Point(0,90), component->scrollPosition());

    // Scroll up another 50 units.  The Scroll method should execute and cancel the manual scrolling
    root->updateTime(200);
    ASSERT_TRUE(root->handlePointerEvent({PointerEventType::kPointerMove, Point(10,50)}));
    ASSERT_EQ(Point(0,140), component->scrollPosition());

    // Keep scrolling - but the gesture should be cancelled now, so nothing happens
    ASSERT_TRUE(root->handlePointerEvent({PointerEventType::kPointerMove, Point(10,10)}));
    ASSERT_EQ(Point(0,140), component->scrollPosition());

    // Now delay until the Scroll command has finished
    auto delta = config->getScrollCommandDuration();   // How long the scroll command should take
    root->updateTime(delta + 200);
    ASSERT_EQ(Point(0,240), component->scrollPosition());

    // Releasing the pointer should not do anything
    ASSERT_TRUE(root->handlePointerEvent({PointerEventType::kPointerUp, Point(10,0)}));
    ASSERT_EQ(Point(0,240), component->scrollPosition());
}

static const char *TOUCH_WRAPPED_PAGER = R"apl({
  "type": "APL",
  "version": "1.5",
  "theme": "dark",
  "mainTemplate": {
    "parameters": ["ip"],
    "item": {
      "type": "Frame",
      "backgroundColor": "black",
      "id": "testcomp",
      "width": 500,
      "height": 500,
      "item": {
        "type": "TouchWrapper",
        "id": "outerwrapper",
        "width": "100%",
        "height": "100%",
        "onPress": {
          "type": "SetValue",
          "componentId": "testcomp",
          "property": "backgroundColor",
          "value": "white"
        },
        "item": {
          "type": "Pager",
          "id": "pager",
          "initialPage": "${ip}",
          "width": "100%",
          "height": "100%",
          "items": [
            {
              "type": "TouchWrapper",
              "id": "inner",
              "onPress": {
                "type": "SetValue",
                "componentId": "testcomp",
                "property": "backgroundColor",
                "value": "red"
              },
              "item": {
                "type": "Text",
                "text": "Text on page #1"
              }
            },
            {
              "type": "Text",
              "text": "Text on page #2"
            }
          ]
        }
      }
    }
  }
})apl";

static const char *START_PAGE_0 = R"apl({"ip": 0})apl";

TEST_F(NativeGesturesTest, PagerInnerWrapperReceivesClick)
{
    loadDocument(TOUCH_WRAPPED_PAGER, START_PAGE_0);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    auto testComp = root->findComponentById("testcomp");
    ASSERT_EQ(Object(Color(Color::RED)), testComp->getCalculated(kPropertyBackgroundColor));
}

TEST_F(NativeGesturesTest, PagerOuterWrapperReceivesClickAfterNavigate)
{
    loadDocument(TOUCH_WRAPPED_PAGER, START_PAGE_0);

    auto pager = root->findComponentById("pager");

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(1600);

    ASSERT_TRUE(CheckDirty(pager, kPropertyCurrentPage));

    ASSERT_EQ(1, pager->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(1700);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    auto testComp = root->findComponentById("testcomp");
    ASSERT_EQ(Object(Color(Color::WHITE)), testComp->getCalculated(kPropertyBackgroundColor));
}

static const char *START_PAGE_1 = R"apl({"ip": 1})apl";

TEST_F(NativeGesturesTest, PagerInnerWrapperReceivesClickAfterNavigate)
{
    loadDocument(TOUCH_WRAPPED_PAGER, START_PAGE_1);

    auto pager = root->findComponentById("pager");
    ASSERT_EQ(1, pager->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();
    root->updateTime(1600);

    ASSERT_TRUE(CheckDirty(pager, kPropertyCurrentPage));

    ASSERT_EQ(0, pager->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(1700);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    auto testComp = root->findComponentById("testcomp");
    ASSERT_EQ(Object(Color(Color::RED)), testComp->getCalculated(kPropertyBackgroundColor));
}

static const char *DOUBLE_WRAPPED_IN_PAGER = R"apl({
  "type": "APL",
  "version": "1.5",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "backgroundColor": "black",
      "id": "testcomp",
      "width": 500,
      "height": 500,
      "item": {
        "type": "Pager",
        "id": "pager",
        "width": "100%",
        "height": "100%",
        "items": [
          {
            "type": "TouchWrapper",
            "id": "inner1",
            "item": {
              "type": "TouchWrapper",
              "id": "inner2",
              "item": {
                "type": "Text",
                "text": "Text on page #1"
              }
            }
          },
          {
            "type": "Text",
            "text": "Text on page #2"
          }
        ]
      }
    }
  }
})apl";

TEST_F(NativeGesturesTest, PagerWidthDoubleWrappedPageStillNavigate)
{
    loadDocument(DOUBLE_WRAPPED_IN_PAGER);

    auto pager = root->findComponentById("pager");

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    root->updateTime(1600);

    ASSERT_TRUE(CheckDirty(pager, kPropertyCurrentPage));

    ASSERT_EQ(1, pager->pagePosition());
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

TEST_F(NativeGesturesTest, WrappedEditTextTap)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureRequestKeyboard);
    loadDocument(EDIT_TEXT_IN_TAP_TOUCHABLE);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(400,50), false,
                                   "onDown"));
    root->updateTime(20);
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

TEST_F(NativeGesturesTest, WrappedEditTextUp)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureRequestKeyboard);
    loadDocument(EDIT_TEXT_IN_UP_TOUCHABLE);

    ASSERT_FALSE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,50))));
    root->updateTime(20);
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

TEST_F(NativeGesturesTest, WrappedEditTextNestedTouchWrappers)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureRequestKeyboard);
    loadDocument(EDIT_TEXT_IN_NESTED_TOUCHABLES);

    ASSERT_FALSE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,50))));
    root->updateTime(20);
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

TEST_F(NativeGesturesTest, WrappedEditTextSwipe)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureRequestKeyboard);
    loadDocument(EDIT_TEXT_IN_SWIPE_TOUCHABLE);

    ASSERT_FALSE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,50))));

    root->updateTime(2000);

    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(50,50))));
    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(50,50))));

    root->updateTime(4000);

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

TEST_F(NativeGesturesTest, KeyboardRequestedOnTap)
{
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureRequestKeyboard);
    loadDocument(EDITTEXT);

    ASSERT_FALSE(root->handlePointerEvent(PointerEvent(apl::kPointerDown, Point(10, 10))));
    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(apl::kPointerUp, Point(10, 10))));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(apl::kEventTypeOpenKeyboard, event.getType());
}


static const char * SOURCE_PAGE = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Pager",
          "width": 400,
          "height": 400,
          "initialPage": 2,
          "data": "${Array.range(10)}",
          "items": {
            "type": "Text",
            "text": "Item ${data}",
            "width": "100%",
            "height": "100%"
          },
          "handlePageMove": {
            "commands": {
              "type": "SendEvent",
              "sequencer": "foo",
              "arguments": [
                "${event.source.page}",
                "${event.amount}"
              ]
            }
          }
        }
      }
    }
)apl";

TEST_F(NativeGesturesTest, SourcePage)
{
    metrics.dpi(160).size(400,400);
    loadDocument(SOURCE_PAGE);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(300,10)));
    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(200,10)));
    ASSERT_TRUE(CheckSendEvent(root, 2, 0.25));

    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    ASSERT_TRUE(CheckSendEvent(root, 2, 0.5));

    root->updateTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(-100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(-100,10)));
    ASSERT_TRUE(CheckSendEvent(root, 2, 1.0));

    ASSERT_FALSE(root->hasEvent());
}

