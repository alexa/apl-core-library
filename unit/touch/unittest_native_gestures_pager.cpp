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
#include "apl/touch/pointerevent.h"
#include "apl/animation/coreeasing.h"

using namespace apl;

class NativeGesturesPagerTest : public DocumentWrapper {
public:
    NativeGesturesPagerTest() : DocumentWrapper() {
        config->set({
            {RootProperty::kTapOrScrollTimeout, 5},
            {RootProperty::kPointerInactivityTimeout, 250},
            {RootProperty::kPointerSlopThreshold, 10},
            {RootProperty::kDefaultPagerAnimationEasing, CoreEasing::linear()},
        });
    }

    void pageFlingDownDefaultTest(NativeGesturesPagerTest& self);
};

TEST_F(NativeGesturesPagerTest, Configuration) {
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
    ASSERT_EQ(Object(600), config->getProperty(RootProperty::kDefaultPagerAnimationDuration));
    ASSERT_EQ(Object(CoreEasing::linear()), config->getProperty(RootProperty::kDefaultPagerAnimationEasing));
    ASSERT_NEAR(1.48, config->getProperty(RootProperty::kScrollAngleSlopeVertical).getDouble(), 0.01);
    ASSERT_NEAR(0.64, config->getProperty(RootProperty::kScrollAngleSlopeHorizontal).getDouble(), 0.01);
    ASSERT_NEAR(0.84, config->getProperty(RootProperty::kSwipeAngleTolerance).getDouble(), 0.01);
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
          "height": "100%",
          "item": {
            "id": "touchWrapper${index}",
            "type": "TouchWrapper",
            "item": {
              "type": "Text",
              "text": "Focusable Component ${index}"
            }
          }
        }
      ]
    }
  }
})";

TEST_F(NativeGesturesPagerTest, AutoPage)
{
    loadDocument(PAGER_TEST);

    ASSERT_EQ(Point(), component->scrollPosition());

    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());

    auto ptr = executeCommand("AutoPage", {{"componentId", "pagers"}, {"count", 4}, {"duration", 100}}, false);
    advanceTime(200);
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    advanceTime(500);
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    advanceTime(700);
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("blue3", component->getDisplayedChildAt(1)->getId());
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    advanceTime(700);
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("blue3", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("purple4", component->getDisplayedChildAt(1)->getId());
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    loop->advanceToEnd();
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
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

TEST_F(NativeGesturesPagerTest, SetPage)
{
    loadDocument(PAGER_TEST);

    auto ptr = executeCommand("SetPage", {{"componentId", "pagers"}, {"position", "absolute"}, {"value", 8}}, false);

    // Takes no time per requirements.
    advanceTime(10);

    ASSERT_EQ(8, component->pagePosition());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    root->clearDirty();
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow8", component->getDisplayedChildAt(0)->getId());

    ASSERT_TRUE(ptr->isResolved());
    auto visibleChild = component->getCoreChildAt(8);
    ASSERT_EQ(1.0, visibleChild->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    // If we don't have focus on a component in the current page then the focus should not change.
    auto& fm = component->getContext()->focusManager();
    auto focus = fm.getFocus();
    ASSERT_EQ(focus, nullptr);

    // Set focus to a component inside the current page. When the page changes the focus should
    // switch to the pager component
    executeCommand("SetFocus", {{"componentId", "touchWrapper8"}}, false);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());

    ////////////////////////

    ptr = executeCommand("SetPage", {{"componentId", "pagers"}, {"position", "relative"}, {"value", -2}}, false);
    advanceTime(90);
    ASSERT_TRUE(CheckDirty(component->getChildAt(6), kPropertyLaidOut, kPropertyInnerBounds, kPropertyTransform,
                           kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));

    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red6", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow8", component->getDisplayedChildAt(1)->getId());

    advanceTime(600);
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red6", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ(6, component->pagePosition());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    root->clearDirty();

    // Verify the focus was changed to the pager component
    ASSERT_EQ(fm.getFocus(), component);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());

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
TEST_F(NativeGesturesPagerTest, PagerFlingDoesntWrapAtEndOfListForNavigationNormal)
{
    loadDocument(PAGER_END_FLING_BUG);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_EQ(1, component->pagePosition());


    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_EQ(1, component->pagePosition());
}

/**
 * Make sure we can't fling before the start of a list when navigation: normal. This tests a fix for
 * a bug were the pager would wrap at the start of a list if the user started another fling during a
 * fling gesture at the start of a list.
 */
TEST_F(NativeGesturesPagerTest, PagerFlingDoesntWrapAtStartOfListForNavigationNormal) {
    loadDocument(PAGER_END_FLING_BUG);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400, 10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100, 10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100, 10)));
    advanceTime(100);

    ASSERT_EQ(0, component->pagePosition());

    advanceTime(1100);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100, 10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400, 10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400, 10)));
    advanceTime(100);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100, 10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400, 10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400, 10)));
    advanceTime(200);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100, 10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400, 10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400, 10)));
    advanceTime(100);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100, 10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400, 10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400, 10)));
    advanceTime(100);

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

TEST_F(NativeGesturesPagerTest, SetPageRelativeOldVersion)
{
    loadDocument(PAGER_TEST_OLD);

    auto ptr = executeCommand("SetPage", {{"componentId", "pagers"}, {"position", "absolute"}, {"value", 8}}, false);
    // Takes no time per requirements.
    advanceTime(10);

    ASSERT_EQ(8, component->pagePosition());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
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
    advanceTime(10);

    ASSERT_EQ(6, component->pagePosition());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    root->clearDirty();

    visibleChild = component->getCoreChildAt(6);
    ASSERT_EQ(1.0, visibleChild->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesPagerTest, AutoPageOldVersion)
{
    loadDocument(PAGER_TEST_OLD);

    ASSERT_EQ(Point(), component->scrollPosition());

    auto ptr = executeCommand("AutoPage", {{"componentId", "pagers"}, {"count", 3}, {"duration", 100}}, false);
    advanceTime(10);
    ASSERT_EQ(1, component->pagePosition());
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    advanceTime(100);
    ASSERT_EQ(2, component->pagePosition());
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    advanceTime(110);
    ASSERT_EQ(3, component->pagePosition());
    root->clearDirty();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesPagerTest, PageFlingRight)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    advanceTime(1500);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

/*
 * Test with RTL layout
 */
TEST_F(NativeGesturesPagerTest, PageFlingRightRTL)
{
    loadDocument(PAGER_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    advanceTime(1500);

    ASSERT_FALSE(CheckDirty(component, kPropertyCurrentPage));

    ASSERT_EQ(11, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesPagerTest, PageFlingRightTapOrScrollTimeout)
{
    config->set(RootProperty::kTapOrScrollTimeout, 60);
    loadDocument(PAGER_TEST);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(400,10), false));
    advanceTime(50);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(250,10), false));
    advanceTime(50);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100,10), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100,10), true));
    root->clearPending();
    advanceTime(1500);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesPagerTest, PageFlingRightWithCancel)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerCancel, Point(100,10)));
    root->clearPending();
    advanceTime(1500);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesPagerTest, PageFlingLeft)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();
    advanceTime(1500);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(11, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

/**
 * Test fling left with RTL layout
 */
TEST_F(NativeGesturesPagerTest, PageFlingLeftRTL)
{
    loadDocument(PAGER_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();
    advanceTime(1500);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesPagerTest, PageFlingTooWide)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,400)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,400)));
    advanceTime(1500);

    ASSERT_EQ(0, component->pagePosition());
}

TEST_F(NativeGesturesPagerTest, PageFlingScaled)
{
    loadDocument(PAGER_TEST);
    TransformComponent(root, "pagers", "scale", 2);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(1500);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesPagerTest, PageFlingRotated)
{
    loadDocument(PAGER_TEST);
    TransformComponent(root, "pagers", "rotate", 45);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    // Move the pointer ~11 pixels at 45 degrees to match the rotation
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,250)));
    advanceTime(220); // Make sure the velocity just meets the threshold
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(242,242)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(242,242)));
    advanceTime(1380);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesPagerTest, PageFlingSingularity)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    TransformComponent(root, "pagers", "scale", 0);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform, kPropertyNotifyChildrenChanged));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(1500);

    ASSERT_FALSE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, component->pagePosition());
    ASSERT_TRUE(session->checkAndClear());
}

TEST_F(NativeGesturesPagerTest, PageFlingThresholdsRemainInGlobalCoordinateDimensions)
{
    loadDocument(PAGER_TEST);
    TransformComponent(root, "pagers", "scale", 2);
    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    // Pointer slop threshold too small
    advanceTime(0);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(395,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(395,10)));
    advanceTime(1500);

    ASSERT_FALSE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(0, component->pagePosition());
    ASSERT_FALSE(root->hasEvent());

    // Velocity too low
    advanceTime(400);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(600);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(375,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(375,10)));
    advanceTime(1000);

    ASSERT_FALSE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(0, component->pagePosition());
    ASSERT_FALSE(root->hasEvent());

    // Both minimum thresholds met, just barely
    advanceTime(400);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(389,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(389,10)));
    advanceTime(1500);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesPagerTest, PageSequentialFlingRight)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    advanceTime(600);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(2, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

/**
 * Test with RTL layout
 */
TEST_F(NativeGesturesPagerTest, PageSequentialFlingRightRTL)
{
    loadDocument(PAGER_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));

    ASSERT_EQ(11, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    advanceTime(600);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(10, component->pagePosition());
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

TEST_F(NativeGesturesPagerTest, PageSequentialFlingRightWithOnPageChange)
{
    loadDocument(PAGER_ONPAGECHANGE_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->pagePosition());

    ASSERT_TRUE(root->hasEvent());
    root->popEvent();

    advanceTime(600);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(2, component->pagePosition());
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
}

TEST_F(NativeGesturesPagerTest, PageSequentialFlingRightCancelOut)
{
    loadDocument(PAGER_TEST);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    advanceTime(700);

    ASSERT_EQ(0, component->pagePosition());
}

/**
 * Test with RTL layout
 */
TEST_F(NativeGesturesPagerTest, PageSequentialFlingRightCancelOutRTL)
{
    loadDocument(PAGER_TEST);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    advanceTime(100);

    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    advanceTime(700);

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
    "parameters": [ "direction", "nav" ],
    "item": {
      "type": "Pager",
      "pageDirection": "${direction}",
      "navigation": "${nav}",
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

TEST_F(NativeGesturesPagerTest, PageFlingLeftDefault)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_DEFAULT_DATA);
    ASSERT_TRUE(ConsoleMessage());  // Extra "do" data

    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
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

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(-500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, PageFlingChangeOfNav)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_DEFAULT_DATA);
    ASSERT_TRUE(ConsoleMessage());  // Incorrect arguments for data

    // Set page to last
    component->update(kUpdatePagerPosition, 11);
    root->clearPending();
    root->clearDirty();
    ASSERT_EQ(11, component->pagePosition());

    auto currentChild = component->getChildAt(11);
    auto nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));

    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("gray11", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("gray11", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(599);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(-500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());


    // Again but with set direction
    component->setProperty(kPropertyPageDirection, "vertical");
    root->clearPending();
    root->clearDirty();
    ASSERT_EQ(0, component->pagePosition());

    currentChild = component->getChildAt(0);
    nextChild = component->getChildAt(11);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));

    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10,100)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(10,400)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(10,400)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateY(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("gray11", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(599);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateY(500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateY(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(11, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("gray11", component->getDisplayedChildAt(0)->getId());


    // Again but with set navigation
    component->setProperty(kPropertyNavigation, "normal");
    component->update(kUpdatePagerPosition, 11);
    root->clearPending();
    root->clearDirty();
    ASSERT_EQ(11, component->pagePosition());

    currentChild = component->getChildAt(11);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));

    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("gray11", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10,400)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(10,100)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(10,100)));
    root->clearPending();

    // Nothing should really happen
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), currentChild));

    // Finish the fling
    advanceTime(600);

    ASSERT_EQ(11, component->pagePosition());
}

/**
 * Test with RTL layout
 */
TEST_F(NativeGesturesPagerTest, PageFlingLeftDefaultRTL)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_DEFAULT_DATA);
    ASSERT_TRUE(ConsoleMessage());  // Incorrect arguments for data

    advanceTime(10);
    root->clearDirty();
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force layout

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(-500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, PageFlingRightDefault)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_DEFAULT_DATA);
    ASSERT_TRUE(ConsoleMessage());  // Incorrect arguments for data

    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
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

    advanceTime(300);

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


    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(500), currentChild, 1));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

/**
 * Test with RTL layout
 */
TEST_F(NativeGesturesPagerTest, PageFlingRightDefaultRTL)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_DEFAULT_DATA);
    ASSERT_TRUE(ConsoleMessage());  // Incorrect arguments for data

    advanceTime(10);
    root->clearDirty();
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(500), currentChild, 1));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

static const char *PAGER_VERTICAL_DATA = R"apl({
    "do": "higherAbove",
    "nav": "wrap",
    "direction": "vertical"
})apl";

TEST_F(NativeGesturesPagerTest, PageFlingUpDefault)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_VERTICAL_DATA);
    ASSERT_TRUE(ConsoleMessage());  // Incorrect arguments for data

    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10, 400)));
    advanceTime(100);
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

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateY(-500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateY(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

void
NativeGesturesPagerTest::pageFlingDownDefaultTest(NativeGesturesPagerTest& self) {

    auto component = self.component;
    auto root = self.root;
    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10,100)));
    advanceTime(100);
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

    advanceTime(300);

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

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateY(500), currentChild, 1));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateY(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, PageFlingDownDefault)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_VERTICAL_DATA);
    ASSERT_TRUE(ConsoleMessage());  // Incorrect arguments for data

    advanceTime(10);
    root->clearDirty();
    pageFlingDownDefaultTest(*this);
}

/**
 * Make sure RTL layout doesn't break vertical pagers
 */
TEST_F(NativeGesturesPagerTest, PageFlingDownDefaultRTL)
{
    loadDocument(PAGER_TEST_DEFAULT_ANIMATION, PAGER_VERTICAL_DATA);
    ASSERT_TRUE(ConsoleMessage());  // Incorrect arguments for data

    advanceTime(10);
    root->clearDirty();
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    root->clearDirty();
    pageFlingDownDefaultTest(*this);
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

TEST_F(NativeGesturesPagerTest, PageFlingLeftCustom)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_DEFAULT_DATA);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
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

    advanceTime(300);

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

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(-500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, PageFlingRightCustom)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_DEFAULT_DATA);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
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

    advanceTime(300);

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

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(500), currentChild, 1));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, PageFlingLeftCustomRTL)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_DEFAULT_DATA);
    advanceTime(10);
    root->clearDirty();
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force the layout

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, PageFlingRightCustomRTL)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_DEFAULT_DATA);
    advanceTime(10);
    root->clearDirty();
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force the layout

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(-500), currentChild, 1));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

static const char *PAGER_CUSTOM_HIGHER_BELOW_DATA = R"apl({
    "do": "higherBelow",
    "nav": "wrap",
    "direction": "horizontal"
})apl";

TEST_F(NativeGesturesPagerTest, PageFlingLeftCustomHigherBelowRTL)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_HIGHER_BELOW_DATA);
    advanceTime(10);
    root->clearDirty();
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force the layout

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, PageFlingRightCustomHigherBelowRTL)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_HIGHER_BELOW_DATA);
    advanceTime(10);
    root->clearDirty();
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force the layout

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(-500), currentChild, 1));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

static const char *PAGER_CUSTOM_NEXT_ABOVE_DATA = R"apl({
    "do": "nextAbove",
    "nav": "wrap",
    "direction": "horizontal"
})apl";

TEST_F(NativeGesturesPagerTest, PageFlingLeftCustomNextAboveRTL)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NEXT_ABOVE_DATA);
    advanceTime(10);
    root->clearDirty();
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force the layout

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, PageFlingRightCustomNextAboveRTL)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NEXT_ABOVE_DATA);
    advanceTime(10);
    root->clearDirty();
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force the layout

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(-500), currentChild, 1));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

static const char *PAGER_CUSTOM_NEXT_BELOW_DATA = R"apl({
    "do": "nextBelow",
    "nav": "wrap",
    "direction": "horizontal"
})apl";

TEST_F(NativeGesturesPagerTest, PageFlingLeftCustomNextBelowRTL)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NEXT_BELOW_DATA);
    advanceTime(10);
    root->clearDirty();
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force the layout

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, PageFlingRightCustomNextBelowRTL)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NEXT_BELOW_DATA);
    advanceTime(10);
    root->clearDirty();
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force the layout

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(200), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    advanceTime(300);

    ASSERT_TRUE(CheckDirty(currentChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-400), currentChild));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), nextChild));

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    //  just keep flinging
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("green1", component->getDisplayedChildAt(1)->getId());

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(-500), currentChild, 1));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, PageFlingUpCustom)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_VERTICAL_DATA);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10, 400)));
    advanceTime(100);
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

    advanceTime(300);

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

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateY(-500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateY(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, PageFlingDownCustom)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_VERTICAL_DATA);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(10,100)));
    advanceTime(100);
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

    advanceTime(300);

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

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateY(500), currentChild, 1));
    ASSERT_TRUE(CheckDirty(nextChild, kPropertyTransform));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateY(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

TEST_F(NativeGesturesPagerTest, CustomPageHigherAbove)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_DEFAULT_DATA);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,10)));
    advanceTime(100);
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

    advanceTime(100);
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

TEST_F(NativeGesturesPagerTest, CustomPageHigherBelow)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_HIGHER_BELOW_DATA);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,10)));
    advanceTime(100);
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

    advanceTime(100);
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

TEST_F(NativeGesturesPagerTest, CustomPageNextAbove)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NEXT_ABOVE_DATA);
    advanceTime(10);
    root->clearDirty();

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-150), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(350), nextChild));

    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(150), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-350), nextChild));

    root->clearDirty();
}

TEST_F(NativeGesturesPagerTest, CustomPageNextBelow)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NEXT_BELOW_DATA);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(250,10)));
    advanceTime(100);
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

    advanceTime(100);
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

TEST_F(NativeGesturesPagerTest, CustomPageWrap)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NAVIGATE_WRAP);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), nextChild));

    // during fling current and next are on screen
    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    advanceTime(100);
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

    advanceTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(500,10)));

    advanceTime(600);
    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    advanceTime(100);
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

TEST_F(NativeGesturesPagerTest, CustomPageNormal)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NAVIGATE_NORMAL);
    advanceTime(10);
    root->clearDirty();

    auto nextChild = component->getChildAt(2);
    auto currentChild = component->getChildAt(1);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-100), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(400), nextChild));

    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(1)->getId());

    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(500,10)));
    root->clearPending();

    currentChild = component->getChildAt(1);
    nextChild = component->getChildAt(0);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(300), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-200), nextChild));

    ASSERT_EQ(2, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());
    ASSERT_EQ("red0", component->getDisplayedChildAt(1)->getId());

    advanceTime(200);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(500,10)));

    advanceTime(600);
    ASSERT_EQ(0, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    advanceTime(100);
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

TEST_F(NativeGesturesPagerTest, CustomPageForwardOnly)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NAVIGATE_FORWARD_ONLY);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    advanceTime(100);
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

    advanceTime(100);
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

TEST_F(NativeGesturesPagerTest, CustomPageNone)
{
    loadDocument(PAGER_TEST_CUSTOM_ANIMATION, PAGER_CUSTOM_NAVIGATE_NONE);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(200,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->clearPending();

    ASSERT_TRUE(CheckTransform(Transform2D(), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D(), nextChild));

    // only current on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    advanceTime(100);
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

TEST_F(NativeGesturesPagerTest, SequencedAutoPageOnMainInterrupt)
{
    loadDocument(REPEATED_AUTO);

    advanceTime(4000);
    ASSERT_EQ(1, component->pagePosition());

    advanceTime(2000);
    ASSERT_EQ(2, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(50,50)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(650,50)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(650,50)));

    // AutoPage cancelled. Repeat kicks in and continues from manually set page.
    advanceTime(900);
    ASSERT_EQ(1, component->pagePosition());

    advanceTime(2000);
    ASSERT_EQ(2, component->pagePosition());
}

TEST_F(NativeGesturesPagerTest, SequencedAutoPageOnMainInterruptUser)
{
    loadDocument(REPEATED_AUTO);

    advanceTime(4000);
    ASSERT_EQ(1, component->pagePosition());

    advanceTime(2000);
    ASSERT_EQ(2, component->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(50,50)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(650,50)));

    // Gesture reset
    advanceTime(2900);
    ASSERT_EQ(3, component->pagePosition());
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

TEST_F(NativeGesturesPagerTest, PagerInnerWrapperReceivesClick)
{
    loadDocument(TOUCH_WRAPPED_PAGER, START_PAGE_0);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    auto testComp = root->findComponentById("testcomp");
    ASSERT_EQ(Object(Color(Color::RED)), testComp->getCalculated(kPropertyBackgroundColor));
}

TEST_F(NativeGesturesPagerTest, PagerOuterWrapperReceivesClickAfterNavigate)
{
    loadDocument(TOUCH_WRAPPED_PAGER, START_PAGE_0);

    auto pager = root->findComponentById("pager");

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    advanceTime(1500);

    ASSERT_TRUE(CheckDirty(pager, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, pager->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();

    auto testComp = root->findComponentById("testcomp");
    ASSERT_EQ(Object(Color(Color::WHITE)), testComp->getCalculated(kPropertyBackgroundColor));
}

static const char *START_PAGE_1 = R"apl({"ip": 1})apl";

TEST_F(NativeGesturesPagerTest, PagerInnerWrapperReceivesClickAfterNavigate)
{
    loadDocument(TOUCH_WRAPPED_PAGER, START_PAGE_1);

    auto pager = root->findComponentById("pager");
    ASSERT_EQ(1, pager->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
    root->clearPending();
    advanceTime(1500);

    ASSERT_TRUE(CheckDirty(pager, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(0, pager->pagePosition());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
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

TEST_F(NativeGesturesPagerTest, PagerWidthDoubleWrappedPageStillNavigate)
{
    loadDocument(DOUBLE_WRAPPED_IN_PAGER);

    auto pager = root->findComponentById("pager");

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
    root->clearPending();
    advanceTime(1500);

    ASSERT_TRUE(CheckDirty(pager, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, pager->pagePosition());
}

static const char *PAGER_FORWARD_DIRECTION_TEST = R"apl(
{
  "type": "APL",
  "version": "1.7",
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
      "layoutDirection": "RTL",
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
          "drawOrder": "${do}",
          "commands": [
            {
              "type": "SetValue",
              "componentId": "${event.currentChild.uid}",
              "property": "transform",
              "value": [
                {
                  "translateX": "${100 * event.amount * (event.forward ? -1 : 1)}%"
                }
              ]
            },
            {
              "type": "SetValue",
              "componentId": "${event.nextChild.uid}",
              "property": "transform",
              "value": [
                {
                  "translateX": "${100 * (1.0 - event.amount) * (event.forward ? 1 : -1)}%"
                }
              ]
            }
          ]
        }
      ]
    }
  }
}
)apl";

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

TEST_F(NativeGesturesPagerTest, SourcePage)
{
    metrics.dpi(160).size(400,400);
    loadDocument(SOURCE_PAGE);
    advanceTime(10);
    root->clearDirty();

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(300,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(200,10)));
    ASSERT_TRUE(CheckSendEvent(root, 2, 0.25));

    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    ASSERT_TRUE(CheckSendEvent(root, 2, 0.5));

    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(-100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(-100,10)));
    ASSERT_TRUE(CheckSendEvent(root, 2, 1.0));

    ASSERT_FALSE(root->hasEvent());
}

/**
 * Check the forward property works as expected for right swipe
 */
TEST_F(NativeGesturesPagerTest, PageFlingRightForwardDirectionCustom)
{
    loadDocument(PAGER_FORWARD_DIRECTION_TEST, PAGER_DEFAULT_DATA);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(2);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,10)));
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

    advanceTime(300);

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

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(-500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("yellow2", component->getDisplayedChildAt(0)->getId());
}

/**
 * Check the forward property works as expected for left swipe
 */
TEST_F(NativeGesturesPagerTest, PageFlingLeftForwardDirectionCustom)
{
    loadDocument(PAGER_FORWARD_DIRECTION_TEST, PAGER_DEFAULT_DATA);
    advanceTime(10);
    root->clearDirty();

    auto currentChild = component->getChildAt(1);
    auto nextChild = component->getChildAt(0);

    // initial page is index 1
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("green1", component->getDisplayedChildAt(0)->getId());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,10)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,10)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,10)));
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

    advanceTime(300);

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

    // Almost finished
    advanceTime(299);
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(500), currentChild, 1));
    ASSERT_TRUE(CheckTransformApprox(Transform2D::translateX(0), nextChild, 1));

    // Finished
    advanceTime(1);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), currentChild));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), nextChild));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(0, component->pagePosition());

    // fling complete, next page is fully on screen
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ("red0", component->getDisplayedChildAt(0)->getId());
}

