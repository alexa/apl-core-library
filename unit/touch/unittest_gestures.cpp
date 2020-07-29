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

class GesturesTest : public DocumentWrapper {
public:
    GesturesTest() : DocumentWrapper() {
        config.swipeAwayAnimationEasing("linear");
        config.swipeAwayTriggerDistanceThreshold(5);
    }

    static
    ::testing::AssertionResult
    compareTransformApprox(const Transform2D& left, const Transform2D& right, float delta = 0.001F) {
        auto leftComponents = left.get();
        auto rightComponents = right.get();

        for (int i = 0; i < 6; i++) {
            auto diff = std::abs(leftComponents.at(i) - rightComponents.at(i));
            if (diff > delta) {
                return ::testing::AssertionFailure()
                        << "transorms is not equal: "
                        << " Expected: " << left.toDebugString()
                        << ", actual: " << right.toDebugString();
            }
        }
        return ::testing::AssertionSuccess();
    }

    static
    ::testing::AssertionResult
    CheckTransform(const Transform2D& expected, const ComponentPtr& component) {
        return compareTransformApprox(expected, component->getCalculated(kPropertyTransform).getTransform2D());
    }

    template<class... Args>
    ::testing::AssertionResult
    CheckEvent(Args... args) {
        return CheckSendEvent(root, args...);
    }

    template<class... Args>
    ::testing::AssertionResult
    HandleAndCheckPointerEvent(PointerEventType type, const Point& point, Args... args) {
        if (!root->handlePointerEvent(PointerEvent(type, point)))
            return ::testing::AssertionFailure() << "Event processing failed.";

        if (sizeof...(Args) > 0) {
            return CheckSendEvent(root, args...);
        }

        return ::testing::AssertionSuccess();
    }
};

static const char *DOUBLE_PRESS = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "item": {
        "type": "Text",
        "id": "texty",
        "text": "Lorem ipsum dolor",
        "fontSize": "50"
      },
      "gestures": [
        {
          "type": "DoublePress",
          "onSinglePress": [
            {
              "type": "SetValue",
              "componentId": "texty",
              "property": "text",
              "value": "Click"
            },
            {
              "type": "SendEvent",
              "arguments": [ "onSinglePress", "${event.component.x}" ]
            }
          ],
          "onDoublePress": [
            {
              "type": "SetValue",
              "componentId": "texty",
              "property": "text",
              "value": "Clicky click"
            },
            {
              "type": "SendEvent",
              "arguments": [ "onDoublePress", "${event.component.x}" ]
            }
          ]
        }
      ],
      "onDown": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onDown" ]
      },
      "onMove": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onMove" ]
      },
      "onUp": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onUp" ]
      },
      "onCancel": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onCancel" ]
      },
      "onPress": {
        "type": "SendEvent",
        "arguments": [ "onPress" ]
      }
    }
  }
})";


TEST_F(GesturesTest, DoublePress)
{
    loadDocument(DOUBLE_PRESS);

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component);
    auto text = tw->getChildAt(0);
    ASSERT_EQ(kComponentTypeText, text->getType());
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(10,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    // Timeout Double press and ensure it reported single press
    root->updateTime(600);
    ASSERT_TRUE(CheckEvent("onSinglePress", 10));

    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    root->updateTime(1000);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress", 15));

    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());
}

TEST_F(GesturesTest, DoublePressThree)
{
    loadDocument(DOUBLE_PRESS);

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component);
    auto text = tw->getChildAt(0);
    ASSERT_EQ(kComponentTypeText, text->getType());
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());

    // "press", "short wait", "press", "short wait", "press"
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(10,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    root->updateTime(400);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress", 15));

    root->updateTime(800);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    root->updateTime(1500);
    ASSERT_TRUE(CheckEvent("onSinglePress", 15));
}

TEST_F(GesturesTest, DoublePressTooLong)
{
    loadDocument(DOUBLE_PRESS);

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component);
    auto text = tw->getChildAt(0);
    ASSERT_EQ(kComponentTypeText, text->getType());
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(10,0), "onDown"));
    root->updateTime(600);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    root->updateTime(650);

    // Single press triggered, as it was too slow
    ASSERT_TRUE(CheckEvent("onSinglePress", 10));

    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    root->updateTime(700);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    root->updateTime(1000);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    root->updateTime(2000);
    // Long here is fine
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress", 15));

    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());
}

static const char *DOUBLE_PRESS_TWICE = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "item": {
        "type": "Text",
        "id": "texty",
        "text": "Lorem ipsum dolor",
        "fontSize": "50"
      },
      "gestures": [
        {
          "type": "DoublePress",
          "onSinglePress": [
            {
              "type": "SendEvent",
              "arguments": [ "onSinglePress", "1" ]
            }
          ],
          "onDoublePress": [
            {
              "type": "SendEvent",
              "arguments": [ "onDoublePress", "1" ]
            }
          ]
        },
        {
          "type": "DoublePress",
          "onSinglePress": [
            {
              "type": "SendEvent",
              "arguments": [ "onSinglePress", "2" ]
            }
          ],
          "onDoublePress": [
            {
              "type": "SendEvent",
              "arguments": [ "onDoublePress", "2" ]
            }
          ]
        }
      ],
      "onDown": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onDown" ]
      },
      "onMove": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onMove" ]
      },
      "onUp": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onUp" ]
      },
      "onCancel": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onCancel" ]
      },
      "onPress": {
        "type": "SendEvent",
        "arguments": [ "onPress" ]
      }
    }
  }
})";

TEST_F(GesturesTest, DoublePressDefinedTwice)
{
    loadDocument(DOUBLE_PRESS_TWICE);

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component);
    auto text = tw->getChildAt(0);
    ASSERT_EQ(kComponentTypeText, text->getType());
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(10,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    // Timeout Double press and ensure it reported single press.
    root->updateTime(600);
    ASSERT_TRUE(CheckEvent("onSinglePress", 1));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    root->updateTime(1000);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress", 1));
    ASSERT_FALSE(root->hasEvent());
}

static const char *LONG_PRESS = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "item": {
        "type": "Text",
        "id": "texty",
        "text": "Lorem ipsum dolor",
        "fontSize": "50"
      },
      "gestures": [
        {
          "type": "LongPress",
          "onLongPressStart": [
            {
              "type": "SetValue",
              "componentId": "texty",
              "property": "text",
              "value": "Long ..."
            },
            {
              "type": "SendEvent",
              "sequencer": "MAIN",
              "arguments": [ "onLongPressStart", "${event.component.x}", "${event.inBounds}" ]
            }
          ],
          "onLongPressEnd": [
            {
              "type": "SetValue",
              "componentId": "texty",
              "property": "text",
              "value": "Long ... click"
            },
            {
              "type": "SendEvent",
              "arguments": [ "onLongPressEnd", "${event.component.x}", "${event.inBounds}" ]
            }
          ]
        }
      ],
      "onDown": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onDown" ]
      },
      "onMove": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onMove" ]
      },
      "onUp": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onUp" ]
      },
      "onCancel": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [ "onCancel" ]
      },
      "onPress": {
        "type": "SendEvent",
        "arguments": [ "onPress" ]
      }
    }
  }
})";

TEST_F(GesturesTest, LongPress)
{
    loadDocument(LONG_PRESS);

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component);
    auto text = tw->getChildAt(0);
    ASSERT_EQ(kComponentTypeText, text->getType());
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());

    // Too short for long press
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(10,0), "onDown"));
    root->updateTime(500);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onUp"));
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(10,0), "onDown"));
    ASSERT_FALSE(root->hasEvent());

    // Not enough to fire onLongPressStart
    root->updateTime(1000);
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());
    ASSERT_FALSE(root->hasEvent());

    // This is enough
    root->updateTime(1500);
    ASSERT_EQ("Long ...", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onLongPressStart", 10, true));

    root->updateTime(2000);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onLongPressEnd", 10, true));
    ASSERT_EQ("Long ... click", text->getCalculated(kPropertyText).asString());
}

static const char *SWIPE_AWAY = R"({
  "type": "APL",
  "version": "1.4",
  "layouts": {
    "Swipable": {
      "parameters": [
        "case",
        "dir",
        "mode"
      ],
      "items": {
        "type": "TouchWrapper",
        "width": "100",
        "height": "100",
        "id": "tw",
        "item": {
          "type": "Text",
          "entities": ["entity"],
          "id": "texty",
          "text": "Some very texty text",
          "width": "100%",
          "height": "100%"
        },
        "gestures": [
          {
            "type": "SwipeAway",
            "direction": "${dir}",
            "action": "${mode}",
            "items": {
              "type": "Frame",
              "entities": ["entity"],
              "id": "swipy",
              "backgroundColor": "purple"
            },
            "onSwipeMove": [
              {
                "type": "SendEvent",
                "sequencer": "MAIN",
                "arguments": [ "onSwipeMove", "${event.position}", "${event.direction}" ]
              }
            ],
            "onSwipeDone": {
              "type": "SendEvent",
              "arguments": [ "onSwipeDone", "${event.direction}" ]
            }
          }
        ],
        "onDown": {
          "type": "SendEvent",
          "sequencer": "MAIN",
          "arguments": [ "onDown" ]
        },
        "onMove": {
          "type": "SendEvent",
          "sequencer": "MAIN",
          "arguments": [ "onMove" ]
        },
        "onUp": {
          "type": "SendEvent",
          "sequencer": "MAIN",
          "arguments": [ "onUp" ]
        },
        "onCancel": {
          "type": "SendEvent",
          "sequencer": "MAIN",
          "arguments": [ "onCancel" ]
        },
        "onPress": {
          "type": "SendEvent",
          "arguments": [ "onPress" ]
        }
      }
    }
  },
  "mainTemplate": {
    "parameters": [ "direction", "mode" ],
    "item": {
      "type": "Container",
      "position": "absolute",
      "width": "100%",
      "height": "100%",
      "items": [
        {
          "type": "Swipable",
          "left": 100,
          "top": 100,
          "dir": "${direction}",
          "mode": "${mode}"
        }
      ]
    }
  }
})";

TEST_F(GesturesTest, SwipeAwayUnfinished)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up before fulfilled.
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    // Avoid flick triggered
    root->updateTime(200);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(190,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));

    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());

    root->updateTime(800);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(160,100), "onSwipeMove", 0.4, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-40), tw->getChildAt(1)));


    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(160,100)));

    // Advance to half of remaining position.
    root->updateTime(900);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-20), tw->getChildAt(1)));

    // Go to the end
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
}

TEST_F(GesturesTest, SwipeAwayCancelled)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.6, "left"));

    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());


    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerCancel, Point(140,100)));

    // Go to the end
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
}

TEST_F(GesturesTest, SwipeAwayWrongDirection)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(200,110), "onMove"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(200,120), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_EQ(1, tw->getChildCount());

    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, SwipeAwayLeftReveal)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    root->updateTime(100);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(190,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-10), tw->getChildAt(1)));


    root->updateTime(500);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onSwipeMove", 0.6, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-60), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(140,100)));

    // Advance to half of remaining position.
    root->updateTime(600);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-80), tw->getChildAt(1)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayLeftCover)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "cover" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    root->updateTime(100);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(190,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(90), tw->getChildAt(1)));

    root->updateTime(600);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onSwipeMove", 0.60, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(40), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(140,100)));

    // Advance to half of remaining position.
    root->updateTime(700);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(20), tw->getChildAt(1)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayLeftSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    root->updateTime(100);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(190,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-10), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(90), tw->getChildAt(1)));

    root->updateTime(600);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onSwipeMove", 0.6, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-60), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(40), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(140,100)));

    // Advance to half of remaining position.
    root->updateTime(700);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(20), tw->getChildAt(1)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayLeftRightLeftSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    root->updateTime(800);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(120,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(20), tw->getChildAt(1)));

    root->updateTime(1000);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onSwipeMove", 0.60, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-60), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(40), tw->getChildAt(1)));


    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(140,100)));

    // Advance to half of remaining position.
    root->updateTime(1100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(20), tw->getChildAt(1)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayLeftRightLeftSlideUnfinished)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    root->updateTime(550);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(145,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.55, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-55), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(45), tw->getChildAt(1)));

    root->updateTime(600);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(160,100), "onSwipeMove", 0.40, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-40), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(60), tw->getChildAt(1)));


    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(160,100)));

    // Advance to half of remaining position.
    root->updateTime(700);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-20), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(80), tw->getChildAt(1)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    root->clearPending();
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
}

TEST_F(GesturesTest, SwipeAwayFlickLeftSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    // Advance time to something in flick range
    root->updateTime(150);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(170,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.3, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-30), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(70), tw->getChildAt(1)));

    // Advance to half of remaining position.
    root->updateTime(325);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-65), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(35), tw->getChildAt(1)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayUnfinishedFlickLeftSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    // Advance time to something not in flick range
    root->updateTime(250);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(180,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.2, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-20), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(80), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(180,100)));

    // Advance to half of remaining position.
    root->updateTime(300);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-10), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(90), tw->getChildAt(1)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
}

TEST_F(GesturesTest, SwipeAwayLeftSlideNotEnoughDistance)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up before fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(195,100), "onMove"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(195,100), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    root->clearPending();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, SwipeAwayRightSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "right", "mode": "slide" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,100), "onDown"));
    root->updateTime(100);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(110,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "right"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(10), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-90), tw->getChildAt(1)));


    root->updateTime(600);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(160,100), "onSwipeMove", 0.6, "right"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(60), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-40), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(160,100)));

    // Advance to half of remaining position.
    root->updateTime(700);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-20), tw->getChildAt(1)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeDone", "right"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayUpSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "up", "mode": "slide" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,200), "onDown"));
    root->updateTime(100);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(100,190), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "up"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-10), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(90), tw->getChildAt(1)));

    root->updateTime(600);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(100,140), "onSwipeMove", 0.60, "up"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-60), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(40), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(100,140)));

    // Advance to half of remaining position.
    root->updateTime(700);
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(20), tw->getChildAt(1)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeDone", "up"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayDownSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "down", "mode": "slide" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,100), "onDown"));
    root->updateTime(100);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(100,110), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "down"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(10), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-90), tw->getChildAt(1)));

    root->updateTime(600);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(100,160), "onSwipeMove", 0.60, "down"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(60), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-40), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(100,160)));

    // Advance to half of remaining position.
    root->updateTime(700);
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-20), tw->getChildAt(1)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeDone", "down"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayOverSwipe)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "cover" })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.6, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(40), tw->getChildAt(1)));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(90,100), "onSwipeMove", 1.0, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(1)));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(210,100), "onSwipeMove", 0.0, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(210,100)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    root->clearPending();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, SwipeAwayContext)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal" })");

    rapidjson::Document document(rapidjson::kObjectType);
    // Retrieve context anch check the base
    auto context = component->serializeVisualContext(document.GetAllocator());
    ASSERT_EQ(1, context["children"].GetArray().Size());
    auto& twCtx = context["children"][0];

    // Check parent
    ASSERT_TRUE(twCtx.HasMember("tags"));
    ASSERT_FALSE(twCtx.HasMember("transform"));
    ASSERT_TRUE(twCtx.HasMember("id"));
    ASSERT_STREQ("tw", twCtx["id"].GetString());
    ASSERT_TRUE(twCtx.HasMember("uid"));
    ASSERT_TRUE(twCtx["tags"].HasMember("clickable"));
    ASSERT_FALSE(twCtx.HasMember("visibility"));
    ASSERT_STREQ("text", twCtx["type"].GetString());

    // Check children
    ASSERT_EQ(1, twCtx["children"].GetArray().Size());
    auto& child = twCtx["children"][0];
    ASSERT_FALSE(child.HasMember("transform"));
    ASSERT_STREQ("texty", child["id"].GetString());
    ASSERT_STREQ("text", child["type"].GetString());
    ASSERT_FALSE(child.HasMember("tags"));
    //////////////////

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    root->updateTime(100);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(190,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-10), tw->getChildAt(1)));

    // While swiping there will be two with appropriate transforms
    context = component->serializeVisualContext(document.GetAllocator());
    ASSERT_EQ(1, context["children"].GetArray().Size());
    twCtx = context["children"][0];

    ASSERT_EQ(2, twCtx["children"].GetArray().Size());
    child = twCtx["children"][0];
    ASSERT_FALSE(child.HasMember("transform"));
    ASSERT_STREQ("swipy", child["id"].GetString());
    ASSERT_STREQ("empty", child["type"].GetString());
    ASSERT_FALSE(child.HasMember("tags"));

    child = twCtx["children"][1];
    ASSERT_TRUE(child.HasMember("transform"));
    ASSERT_STREQ("texty", child["id"].GetString());
    ASSERT_STREQ("text", child["type"].GetString());
    ASSERT_FALSE(child.HasMember("tags"));
    //////////////////

    root->updateTime(500);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onSwipeMove", 0.6, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-60), tw->getChildAt(1)));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(140,100)));

    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    // After swipe finished we have only 1 which is new one.
    context = component->serializeVisualContext(document.GetAllocator());
    ASSERT_EQ(1, context["children"].GetArray().Size());
    twCtx = context["children"][0];

    ASSERT_EQ(1, twCtx["children"].GetArray().Size());
    child = twCtx["children"][0];
    ASSERT_FALSE(child.HasMember("transform"));
    ASSERT_STREQ("swipy", child["id"].GetString());
    ASSERT_STREQ("empty", child["type"].GetString());
    ASSERT_FALSE(child.HasMember("tags"));
    //////////////////

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

static const char *TOUCH_ALL = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "item": {
        "type": "TouchWrapper",
        "id": "tw",
        "width": 100,
        "height": "50",
        "item": {
          "type": "Text",
          "id": "texty",
          "text": "Lorem ipsum dolor",
          "fontSize": "50"
        },
        "gestures": [
          {
            "type": "LongPress",
            "onLongPressStart": [
              {
                "type": "SetValue",
                "componentId": "texty",
                "property": "text",
                "value": "Long ..."
              },
              {
                "type": "SendEvent",
                "sequencer": "MAIN",
                "arguments": [ "onLongPressStart" ]
              }
            ],
            "onLongPressEnd": [
              {
                "type": "SetValue",
                "componentId": "texty",
                "property": "text",
                "value": "Long ... click"
              },
              {
                "type": "SendEvent",
                "arguments": [ "onLongPressEnd" ]
              }
            ]
          },
          {
            "type": "DoublePress",
            "onSinglePress": [
              {
                "type": "SetValue",
                "componentId": "texty",
                "property": "text",
                "value": "Click"
              },
              {
                "type": "SendEvent",
                "arguments": [ "onSinglePress" ]
              }
            ],
            "onDoublePress": [
              {
                "type": "SetValue",
                "componentId": "texty",
                "property": "text",
                "value": "Clicky click"
              },
              {
                "type": "SendEvent",
                "arguments": [ "onDoublePress" ]
              }
            ]
          },
          {
            "type": "SwipeAway",
            "direction": "right",
            "action": "reveal",
            "items": {
              "type": "Frame",
              "id": "swipy",
              "backgroundColor": "purple",
              "items": {
                "componentId": "MyCheck",
                "type": "Text",
                "text": "✓",
                "fontSize": 60,
                "color": "white",
                "width": 60,
                "height": "100%",
                "textAlign": "center",
                "textVerticalAlign": "center"
              }
            },
            "onSwipeMove": {
              "type": "SendEvent",
              "sequencer": "MAIN",
              "arguments": ["onSwipeMove", "${event.position}", "${event.direction}"]
            },
            "onSwipeDone": {
              "type": "SendEvent",
              "arguments": ["onSwipeDone", "${event.direction}"]
            }
          }
        ],
        "onDown": {
          "type": "SendEvent",
          "sequencer": "MAIN",
          "arguments": [ "onDown" ]
        },
        "onMove": {
          "type": "SendEvent",
          "sequencer": "MAIN",
          "arguments": [ "onMove" ]
        },
        "onUp": {
          "type": "SendEvent",
          "sequencer": "MAIN",
          "arguments": [ "onUp" ]
        },
        "onCancel": {
          "type": "SendEvent",
          "sequencer": "MAIN",
          "arguments": [ "onCancel" ]
        },
        "onPress": {
          "type": "SendEvent",
          "arguments": [ "onPress" ]
        }
      }
    }
  }
})";

TEST_F(GesturesTest, GestureCombo)
{
    loadDocument(TOUCH_ALL);

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    auto text = tw->getChildAt(0);
    ASSERT_EQ(kComponentTypeText, text->getType());
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());

    /************ Too short for long press but could be ok for double click ************/
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));

    root->updateTime(400);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress"));

    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());

    /************ Too short for long press but ok for single click ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));

    root->updateTime(800);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    root->updateTime(1500);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());

    /************ Long press and single press instead of double ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));

    root->updateTime(2500);

    ASSERT_EQ("Long ...", text->getCalculated(kPropertyText).asString());
    root->updateTime(3500);
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onLongPressStart"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onLongPressEnd"));
    ASSERT_EQ("Long ... click", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    root->updateTime(4000);
    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    /************ Double press instead of long one ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    // Double tap consumed long press start.
    root->updateTime(4100);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    root->updateTime(4500);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress"));

    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());

    /************ Insufficient move for swipe so single press only ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    root->updateTime(4550);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(5,0), "onMove"));

    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(5,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    // Wait out single press
    root->updateTime(5050);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    /************ Sufficient move for swipe ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    root->updateTime(5650);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(60,0), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.6, "right"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(60,0)));
    loop->advanceToEnd();
    ASSERT_TRUE(CheckEvent("onSwipeDone", "right"));

    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, SwipeAwayMiddle)
{
    loadDocument(TOUCH_ALL);

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    auto text = tw->getChildAt(0);
    ASSERT_EQ(kComponentTypeText, text->getType());
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());

    /************ Too short for long press but could be ok for double click ************/
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));

    root->updateTime(400);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onUp"));
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress"));

    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());

    /************ Too short for long press but ok for single click ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));

    root->updateTime(800);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onUp"));
    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    root->updateTime(1500);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());

    /************ Long press and single press instead of double ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));

    root->updateTime(2500);

    ASSERT_EQ("Long ...", text->getCalculated(kPropertyText).asString());
    root->updateTime(3500);
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onLongPressStart"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onLongPressEnd"));
    ASSERT_EQ("Long ... click", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    root->updateTime(4000);
    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    /************ Double press instead of long one ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onUp"));
    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    // Double tap consumed long press start.
    root->updateTime(4100);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    root->updateTime(4500);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress"));

    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());

    /************ Insufficient move for swipe so single press only ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    root->updateTime(4550);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(55,0), "onMove"));

    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(55,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    // Wait out single press
    root->updateTime(5050);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    /************ Sufficient move for swipe ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    root->updateTime(5650);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(60,0), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.6, "right"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(60,0)));
    loop->advanceToEnd();
    ASSERT_TRUE(CheckEvent("onSwipeDone", "right"));

    ASSERT_FALSE(root->hasEvent());
}

static const char *ALL_AVG = R"({
  "type": "APL",
  "version": "1.4",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "items": {
        "type": "group",
        "style": "expanded",
        "items": [
          {
            "type": "path",
            "fill": "red",
            "stroke": "blue",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          {
            "type": "text",
            "fill": "red",
            "fontFamily": "amazon-ember, sans-serif",
            "fontSize": 40,
            "text": "Diamond",
            "x": 25,
            "y": 25,
            "textAnchor": "middle"
          }
        ]
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "VectorGraphic",
      "source": "box",
      "gestures": [
        {
          "type": "LongPress",
          "onLongPressStart": [
            {
              "type": "SendEvent",
              "sequencer": "MAIN",
              "arguments": [
                "onLongPressStart"
              ]
            }
          ],
          "onLongPressEnd": [
            {
              "type": "SendEvent",
              "arguments": [
                "onLongPressEnd"
              ]
            }
          ]
        },
        {
          "type": "DoublePress",
          "onSinglePress": [
            {
              "type": "SendEvent",
              "arguments": [
                "onSinglePress"
              ]
            }
          ],
          "onDoublePress": [
            {
              "type": "SendEvent",
              "arguments": [ "onDoublePress" ]
            }
          ]
        },
        {
          "type": "SwipeAway",
          "direction": "left",
          "action": "reveal",
          "items": {
            "type": "Frame",
            "id": "swipy",
            "backgroundColor": "purple",
            "items": {
              "componentId": "MyCheck",
              "type": "Text",
              "text": "✓",
              "fontSize": 60,
              "color": "white",
              "width": 60,
              "height": "100%",
              "textAlign": "center",
              "textVerticalAlign": "center"
            }
          },
          "onSwipeMove": {
            "type": "SendEvent",
            "sequencer": "MAIN",
            "arguments": [
              "onSwipeMove",
              "${event.position}",
              "${event.direction}"
            ]
          },
          "onSwipeDone": {
            "type": "SendEvent",
            "arguments": [
              "onSwipeDone",
              "${event.direction}"
            ]
          }
        }
      ],
      "onDown": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onDown"
        ]
      },
      "onMove": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onMove"
        ]
      },
      "onUp": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onUp"
        ]
      },
      "onCancel": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onCancel"
        ]
      },
      "onPress": {
        "type": "SendEvent",
        "arguments": [
          "onPress"
        ]
      }
    }
  }
})";

TEST_F(GesturesTest, DoublePressAVG)
{
    loadDocument(ALL_AVG);

    ASSERT_EQ(kComponentTypeVectorGraphic, component->getType());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    // Timeout Double press and ensure it reported single press
    root->updateTime(600);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    root->updateTime(1000);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress"));
}

TEST_F(GesturesTest, LongPressAVG)
{
    loadDocument(ALL_AVG);

    ASSERT_EQ(kComponentTypeVectorGraphic, component->getType());

    // Too short for long press
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    root->updateTime(400);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    // Wait out single press
    root->updateTime(900);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_FALSE(root->hasEvent());

    // Not enough to fire onLongPressStart
    root->updateTime(1300);
    ASSERT_FALSE(root->hasEvent());

    // This is enough
    root->updateTime(2000);
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onLongPressStart"));

    root->updateTime(2500);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onLongPressEnd"));
}

TEST_F(GesturesTest, SwipeAwayAVG)
{
    loadDocument(ALL_AVG);

    ASSERT_EQ(kComponentTypeVectorGraphic, component->getType());

    // Not supported so just single press should happen
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,0), "onDown"));
    root->updateTime(100);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(90,0), "onMove"));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    // Wait out single press
    root->updateTime(1000);
    ASSERT_TRUE(CheckEvent("onSinglePress"));
}
