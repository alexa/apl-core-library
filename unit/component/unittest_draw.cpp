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

#include "gtest/gtest.h"

#include <cmath>

#include "apl/engine/builder.h"
#include "apl/engine/evaluate.h"

#include "../testeventloop.h"

using namespace apl;

class ComponentDrawTest : public DocumentWrapper {
};

const double EPSILON = 0.05;

inline
::testing::AssertionResult
CheckAABB(const Rect &expected, const ComponentPtr &component) {
    auto t2d = component->getCalculated(kPropertyTransform).getTransform2D();
    auto bounds = component->getCalculated(kPropertyBounds).getRect();

    auto aabb = t2d.calculateAxisAlignedBoundingBox(Rect{0, 0, bounds.getWidth(), bounds.getHeight()});
    aabb.offset(bounds.getTopLeft());

    auto result = std::abs(expected.getX() - aabb.getX()) < EPSILON &&
                  std::abs(expected.getY() - aabb.getY()) < EPSILON &&
                  std::abs(expected.getWidth() - aabb.getWidth()) < EPSILON &&
                  std::abs(expected.getHeight() - aabb.getHeight()) < EPSILON;

    if (!result) {
        return ::testing::AssertionFailure()
                << "aabb is not equal - "
                << " transform: " << t2d.toDebugString()
                << " applied to bounds: " << bounds.toDebugString()
                << ", expected: " << expected.toDebugString()
                << ", actual: " << aabb.toDebugString();
    }

    return ::testing::AssertionSuccess();
}


template<class T>
std::shared_ptr<T>
as(const ComponentPtr &component) {
    return std::static_pointer_cast<T>(component);
}

static const char *CHILD_IN_PARENT =
        R"apl({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 400,
          "height": 400,
          "items": [
            {
              "type": "TouchWrapper",
              "id": "TouchWrapper",
              "position": "absolute",
              "left": 40,
              "top": 50,
              "width": "100",
              "height": "100",
              "item": {
                "type": "Frame",
                "id": "Frame",
                "width": "100%",
                "height": "100%"
              }
            }
          ]
        }
      }
    }
)apl";

/**
 * Simple positive test with multiple levels of parent child.
 */
TEST_F(ComponentDrawTest, ChildInParent) {
    loadDocument(CHILD_IN_PARENT);

    ASSERT_EQ(1, component->getDisplayedChildCount());
    auto child = component->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeTouchWrapper, child->getType());

    ASSERT_EQ(1, child->getDisplayedChildCount());
    child = child->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeFrame, child->getType());

    ASSERT_EQ(0, child->getDisplayedChildCount());
}

/**
 * Test that display invisible and none are not considered in draw region.
 */
TEST_F(ComponentDrawTest, ChildDisplay) {
    loadDocument(CHILD_IN_PARENT);

    auto touchWrapper = as<CoreComponent>(component->findComponentById("TouchWrapper"));
    auto frame = as<CoreComponent>(component->findComponentById("Frame"));
    ASSERT_TRUE(touchWrapper);
    ASSERT_TRUE(frame);

    ASSERT_EQ(1, component->getDisplayedChildCount());
    auto child = component->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeTouchWrapper, child->getType());

    ASSERT_EQ(1, child->getDisplayedChildCount());
    child = child->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeFrame, child->getType());

    ASSERT_EQ(0, child->getDisplayedChildCount());

    // make child invisible
    touchWrapper->setProperty(kPropertyDisplay, "invisible");

    ASSERT_TRUE(CheckDirty(touchWrapper, kPropertyDisplay));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(0, component->getDisplayedChildCount());

    // restore to normal, make it's child gone
    touchWrapper->setProperty(kPropertyDisplay, "normal");
    frame->setProperty(kPropertyDisplay, "none");

    ASSERT_TRUE(CheckDirty(frame, kPropertyDisplay));
    ASSERT_TRUE(CheckDirty(touchWrapper, kPropertyDisplay, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->getDisplayedChildCount());
    child = component->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeTouchWrapper, child->getType());

    ASSERT_EQ(0, child->getDisplayedChildCount());

    // restore all components to normal
    frame->setProperty(kPropertyDisplay, "normal");

    ASSERT_TRUE(CheckDirty(frame, kPropertyDisplay));
    ASSERT_TRUE(CheckDirty(touchWrapper, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->getDisplayedChildCount());
    child = component->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeTouchWrapper, child->getType());

    ASSERT_EQ(1, child->getDisplayedChildCount());
    child = child->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeFrame, child->getType());

    ASSERT_EQ(0, child->getDisplayedChildCount());
}

/**
 * Test that Opaque children are not found in the draw region
 */
TEST_F(ComponentDrawTest, Opacity) {
    loadDocument(CHILD_IN_PARENT);

    auto touchWrapper = as<CoreComponent>(component->findComponentById("TouchWrapper"));
    auto frame = as<CoreComponent>(component->findComponentById("Frame"));
    ASSERT_TRUE(touchWrapper);
    ASSERT_TRUE(frame);

    ASSERT_EQ(1, component->getDisplayedChildCount());
    auto child = component->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeTouchWrapper, child->getType());

    ASSERT_EQ(1, child->getDisplayedChildCount());
    child = child->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeFrame, child->getType());

    ASSERT_EQ(0, child->getDisplayedChildCount());

    // make child invisible
    touchWrapper->setProperty(kPropertyOpacity, 0.00);
    ASSERT_TRUE(CheckDirty(touchWrapper, kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(0, component->getDisplayedChildCount());

    // restore to normal, make it's child gone
    touchWrapper->setProperty(kPropertyOpacity, 1.0);
    ASSERT_TRUE(CheckDirty(touchWrapper, kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    frame->setProperty(kPropertyOpacity, 0.0);
    ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(touchWrapper, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->getDisplayedChildCount());
    child = component->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeTouchWrapper, child->getType());
    ASSERT_EQ(0, child->getDisplayedChildCount());

    // restore as partial opacity
    frame->setProperty(kPropertyOpacity, .5);
    ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(touchWrapper, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, component->getDisplayedChildCount());
    child = component->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeTouchWrapper, child->getType());

    ASSERT_EQ(1, child->getDisplayedChildCount());
    child = child->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeFrame, child->getType());
    ASSERT_EQ(0, child->getDisplayedChildCount());

    // slight change in opacity, but still visible, no display children change
    frame->setProperty(kPropertyOpacity, .2);
    ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity));
    ASSERT_FALSE(CheckDirty(touchWrapper, kPropertyNotifyChildrenChanged));
}

static const char *MULTI_CHILD =
        R"apl({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "parameters": [],
        "item": {
          "type": "Container",
          "id": "CONT",
          "direction": "column",
          "width": "600",
          "height": "600",
            "items": {
              "type": "Frame",
              "id": "${data}",
              "width": 200,
              "height": 200
            },
            "data": [
              0,
              1,
              2,
              3,
              4,
              5,
              6,
              7,
              8,
              9,
              10
            ]
          }
        }
      }
    }
)apl";

/**
 * Test children that overeflow the parent are clipped.
 */
TEST_F(ComponentDrawTest, Bounds) {
    loadDocument(MULTI_CHILD);

    ASSERT_EQ(11, component->getChildCount());
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 3; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i), child->getId());
    }

    component->setProperty(kPropertyHeight, 100);
    root->clearPending(); // force layout changes

    ASSERT_TRUE(CheckDirty(component, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, component->getDisplayedChildCount());
    for (int i = 0; i < 1; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i), child->getId());
    }
}

static const char *PADDING =
        R"apl({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "parameters": [],
        "item": {
          "type": "Container",
          "id": "CONT",
          "direction": "column",
          "width": "1000",
          "height": "1000",
          "padding": 250,
            "items": {
              "type": "Frame",
              "id": "${data}",
              "width": 200,
              "height": 200
            },
            "data": [
              0,
              1,
              2,
              3,
              4,
              5,
              6,
              7,
              8,
              9,
              10
            ]
          }
        }
      }
    }
)apl";

/**
 * Test clipping with padding.
 */
TEST_F(ComponentDrawTest, BoundsCheckWithPadding) {
    loadDocument(PADDING);

    // children overflow the parent and are clipped
    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(IsEqual(Rect(250, 250, 500, 500), component->getCalculated(kPropertyInnerBounds)));

    ASSERT_EQ(4, component->getDisplayedChildCount());
    for (int i = 0; i < 3; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i), child->getId());
    }

    component->setProperty(kPropertyPadding, ObjectArray{10, 10});
    root->clearPending(); // force layout change
    ASSERT_TRUE(CheckDirty(component, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(5, component->getDisplayedChildCount());
    for (int i = 0; i < 5; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i), child->getId());
    }
}

static const char *SCROLL_VIEW =
        R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "parameters": [],
        "item": {
          "type": "ScrollView",
          "id": "Scroll",
          "item": {
            "type": "Container",
            "id": "CONT",
            "width": "200",
            "height": "600",
            "items": {
              "type": "Frame",
              "id": "${data}",
              "width": 200,
              "height": 200
            },
            "data": [
              0,
              1,
              2,
              3,
              4,
              5,
              6,
              7,
              8,
              9,
              10
            ]
          }
        }
      }
    }
)apl";

TEST_F(ComponentDrawTest, ScrollView) {
    loadDocument(SCROLL_VIEW);

    ASSERT_EQ(1, component->getDisplayedChildCount());
    auto container = component->getDisplayedChildAt(0);
    ASSERT_EQ(kComponentTypeContainer, container->getType());
    ASSERT_EQ(11, container->getChildCount());
    ASSERT_EQ(3, container->getDisplayedChildCount());

    // Because draw children is a "local" property, and scroll holds
    // a single component, the children of the container are still reported
    // as displayed
    component->update(kUpdateScrollPosition, 300);
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ(11, container->getChildCount());
}

static const char *VERTICAL_SEQUENCE =
        R"apl({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "parameters": [],
        "item": {
          "type": "Sequence",
          "id": "SEQ",
          "scrollDirection": "vertical",
          "width": 200,
          "height": 500,
          "items": {
            "type": "Frame",
            "id": "${data}",
            "width": 200,
            "height": 200
          },
          "data": [
            0,
            1,
            2,
            3,
            4
          ]
        }
      }
    }
)apl";

/**
 * Vertical sequence w scroll clips children outside
 * of scroll viewport.
 */
TEST_F(ComponentDrawTest, VerticalSequence) {
    loadDocument(VERTICAL_SEQUENCE);

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());

    // expect first 2.5 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 3; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i), child->getId());
    }

    // Scroll full "page"
    component->update(kUpdateScrollPosition, 500);
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    // expect last 2.5 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 3; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i + 2), child->getId());
    }
}

static const char *HORIZONTAL_SEQUENCE =
        R"apl({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "parameters": [],
        "item": {
          "type": "Sequence",
          "id": "SEQ",
          "scrollDirection": "horizontal",
          "width": 500,
          "height": 200,
          "items": {
            "type": "Frame",
            "id": "${data}",
            "width": 200,
            "height": 200
          },
          "data": [
            0,
            1,
            2,
            3,
            4
          ]
        }
      }
    }
)apl";

/**
 * Horizontal Sequence with scroll clips children
 * outside of scroll viewport.
 */
TEST_F(ComponentDrawTest, HorizontalSequence) {
    loadDocument(HORIZONTAL_SEQUENCE);

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());

    // expect first 2.5 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 3; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i), child->getId());
    }

    // scroll full "page"
    component->update(kUpdateScrollPosition, 500);
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    // expect last 2.5 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 3; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i + 2), child->getId());
    }
}

TEST_F(ComponentDrawTest, HorizontalSequenceRTL) {
    loadDocument(HORIZONTAL_SEQUENCE);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());

    // expect first 2.5 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 3; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i), child->getId());
    }

    // scroll full "page"
    component->update(kUpdateScrollPosition, -500);
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    // expect last 2.5 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 3; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i + 2), child->getId());
    }
}

static const char *HORIZONTAL_SEQUENCE_PADDING =
        R"apl({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "parameters": [],
        "item": {
          "type": "Sequence",
          "id": "SEQ",
          "scrollDirection": "horizontal",
          "width": 500,
          "height": 200,
          "padding": 50,
          "items": {
            "type": "Frame",
            "id": "${data}",
            "width": 200,
            "height": 200
          },
          "data": [
            0,
            1,
            2,
            3,
            4
          ]
        }
      }
    }
)apl";

/**
 * Horizontal Sequence with scroll and padding clips children
 * outside of scroll viewport.
 */
TEST_F(ComponentDrawTest, HorizontalSequenceWPadding) {
    loadDocument(HORIZONTAL_SEQUENCE_PADDING);

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());

    // expect padding & first 3 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 2; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i), child->getId());
    }

    // scroll full "page"
    component->update(kUpdateScrollPosition, 500);
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    // expect last 3 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 3; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i + 2), child->getId());
    }
}

TEST_F(ComponentDrawTest, HorizontalSequenceWPaddingRTL) {
    loadDocument(HORIZONTAL_SEQUENCE_PADDING);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());

    // expect padding & first 3 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 2; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i), child->getId());
    }

    // scroll full "page"
    component->update(kUpdateScrollPosition, -500);
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    // expect last 3 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 3; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i + 2), child->getId());
    }
}

static const char *VERTICAL_SEQUENCE_PADDING =
        R"apl({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "parameters": [],
        "item": {
          "type": "Sequence",
          "id": "SEQ",
          "scrollDirection": "vertical",
          "width": 200,
          "height": 500,
          "padding": 50,
          "items": {
            "type": "Frame",
            "id": "${data}",
            "width": 200,
            "height": 200
          },
          "data": [
            0,
            1,
            2,
            3,
            4
          ]
        }
      }
    }
)apl";

/**
 * Vertical Sequence with scroll and padding clips children
 * outside of scroll viewport.
 */
TEST_F(ComponentDrawTest, VerticalSequenceWPadding) {
    loadDocument(VERTICAL_SEQUENCE_PADDING);

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());

    // expect padding & first 3 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 2; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i), child->getId());
    }

    // scroll full "page"
    component->update(kUpdateScrollPosition, 500);
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    // expect last 3 children on screen
    ASSERT_EQ(3, component->getDisplayedChildCount());
    for (int i = 0; i < 3; i++) {
        auto child = component->getDisplayedChildAt(i);
        ASSERT_EQ(std::to_string(i + 2), child->getId());
    }
}

static const char *TRANSFORM =
        R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 100,
          "height": 100,
          "items": [
            {
              "type": "Frame",
              "id": "1",
              "position": "absolute",
              "x": 0,
              "y": 0,
              "width": "100",
              "height": "100",
              "transform": [
                {
                  "translateY": 100
                }
              ]
            },
            {
              "type": "Frame",
              "id": "2",
              "position": "absolute",
              "x": 0,
              "y": 0,
              "width": "100",
              "height": "100",
              "transform": [
                {
                  "translateX": 100
                }
              ]
            },
            {
              "type": "Frame",
              "id": "3",
              "position": "absolute",
              "x": 0,
              "y": 0,
              "width": "100",
              "height": "100",
              "transform": [
                {
                  "scale": 0.0
                }
              ]
            },
            {
              "type": "Frame",
              "id": "4",
              "position": "absolute",
              "x": 0,
              "y": 0,
              "width": "100",
              "height": "100",
              "transform": [
                {
                  "rotate": 45
                }
              ]
            },
            {
              "type": "Frame",
              "id": "5",
              "position": "absolute",
              "left": 100,
              "width": "100",
              "height": "100"
            }
          ]
        }
      }
    }
    )apl";

/**
 * Transformed components
 */
TEST_F(ComponentDrawTest, Transforms) {
    loadDocument(TRANSFORM);
    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());

    // 1 child displayed
    ASSERT_EQ(1, component->getDisplayedChildCount());
    auto child = as<CoreComponent>(component->getDisplayedChildAt(0));
    ASSERT_EQ("4", child->getId());

    // translated child "1" is just off bottom edge of parent
    // skew-ing it should bring the corner back into display
    child = as<CoreComponent>(component->findComponentById("1"));
    ASSERT_TRUE(CheckAABB(Rect(0, 100, 100, 100), child));
    TransformComponent(root, "1", "translateY", 100, "skewY", 45);
    ASSERT_TRUE(CheckDirty(child, kPropertyTransform));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckAABB(Rect(0, 50, 100, 200), child));

    // translated child "2" is just off right edge of parent
    // rotating it should bring the corner back into display
    child = as<CoreComponent>(component->findComponentById("2"));
    ASSERT_TRUE(CheckAABB(Rect(100, 0, 100, 100), child));
    TransformComponent(root, "2", "translateY", 100, "rotate", 45);
    ASSERT_TRUE(CheckDirty(child, kPropertyTransform));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckAABB(Rect(-20.7, 79.3, 141.4, 141.4), child));

    // child "3" is scaled to 0 size, reset transform to bring it back into display
    child = as<CoreComponent>(component->findComponentById("3"));
    ASSERT_TRUE(CheckAABB(Rect(50, 50, 0, 0), child));
    child->setProperty(kPropertyTransformAssigned, Object::EMPTY_ARRAY());
    ASSERT_TRUE(CheckDirty(child, kPropertyTransform));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckAABB(Rect(0, 0, 100, 100), child));

    // child "4" is rotated and visible, rotate more
    child = as<CoreComponent>(component->findComponentById("3"));
    ASSERT_TRUE(CheckAABB(Rect(0, 0, 100, 100), child));
    TransformComponent(root, "3", "rotate", 90);
    ASSERT_TRUE(CheckDirty(child, kPropertyTransform));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckAABB(Rect(0, 0, 100, 100), child));

    // child "5" has an absolute x value of 100, meaning it is not visible and lies
    // on the right edge of the parent
    // rotating it by 225 (effectively 45 degrees) should bring one of its corners
    // into view of the parent container
    child = as<CoreComponent>(component->findComponentById("5"));
    ASSERT_TRUE(CheckAABB(Rect(100, 0, 100, 100), child));
    TransformComponent(root, "5", "rotate", 225);
    ASSERT_TRUE(CheckDirty(child, kPropertyTransform));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckAABB(Rect(79.3, -20.7, 141.4, 141.4), child));

    // all children now displayed
    ASSERT_EQ(5, component->getDisplayedChildCount());
    for (int i = 0; i < 5; i++) {
        child = as<CoreComponent>(component->getDisplayedChildAt(i));
        ASSERT_EQ(std::to_string(i + 1), child->getId());
    }
}