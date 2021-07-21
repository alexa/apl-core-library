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
        config->set({
            {RootProperty::kSwipeAwayAnimationEasing, "linear"},
            {RootProperty::kTapOrScrollTimeout, 0},
            {RootProperty::kPointerSlopThreshold, 5},
            {RootProperty::kSwipeVelocityThreshold, 200},
            {RootProperty::kPointerInactivityTimeout, 1000}
        });
    }

    template<class... Args>
    ::testing::AssertionResult
    CheckEvent(Args... args) {
        return CheckSendEvent(root, args...);
    }

    template<class... Args>
    ::testing::AssertionResult
    HandleAndCheckPointerEvent(PointerEventType type, const Point& point, Args... args) {
        return HandlePointerEvent(root, type, point, false, args...);
    }

    template<class... Args>
    ::testing::AssertionResult
    HandleAndCheckConsumedPointerEvent(PointerEventType type, const Point& point, Args... args) {
        return HandlePointerEvent(root, type, point, true, args...);
    }

    void swipeAwayLeftSlide();

    void swipeAwayRightSlide();

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
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    // Timeout Double press and ensure it reported single press
    advanceTime(600);
    ASSERT_TRUE(CheckEvent("onSinglePress", 10));

    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onCancel"));
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
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress", 15));

    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    advanceTime(700);
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
    advanceTime(600);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    advanceTime(50);

    // Single press triggered, as it was too slow
    ASSERT_TRUE(CheckEvent("onSinglePress", 10));

    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    advanceTime(50);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    advanceTime(300);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    advanceTime(1000);
    // Long here is fine
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onCancel"));
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
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    // Timeout Double press and ensure it reported single press.
    advanceTime(600);
    ASSERT_TRUE(CheckEvent("onSinglePress", 1));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerDown, Point(15,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(15,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress", 1));
    ASSERT_FALSE(root->hasEvent());
}

static const char *DOUBLE_PRESS_TARGETS = R"(
{
  "type": "APL",
  "version": "1.5",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "direction": "row",
      "items": [
        {
          "type": "TouchWrapper",
          "width": "100",
          "item": {
            "type": "Text",
            "id": "textLeft",
            "text": "Left"
          },
          "gestures": [
            {
              "type": "DoublePress",
              "onSinglePress": [
                {
                  "type": "SetValue",
                  "componentId": "textLeft",
                  "property": "text",
                  "value": "1x"
                },
                {
                  "type": "SendEvent",
                  "arguments": [
                    "onSinglePress",
                    "left"
                  ]
                }
              ],
              "onDoublePress": [
                {
                  "type": "SetValue",
                  "componentId": "textLeft",
                  "property": "text",
                  "value": "2x"
                },
                {
                  "type": "SendEvent",
                  "arguments": [
                    "onDoublePress",
                    "left"
                  ]
                }
              ]
            }
          ]
        },
        {
          "type": "TouchWrapper",
          "width": "150",
          "item": {
            "type": "Text",
            "id": "textMiddle",
            "text": "Middle"
          },
          "gestures": [
            {
              "type": "DoublePress",
              "onSinglePress": [
                {
                  "type": "SetValue",
                  "componentId": "textMiddle",
                  "property": "text",
                  "value": "1x"
                },
                {
                  "type": "SendEvent",
                  "arguments": [
                    "onSinglePress",
                    "middle"
                  ]
                }
              ],
              "onDoublePress": [
                {
                  "type": "SetValue",
                  "componentId": "textMiddle",
                  "property": "text",
                  "value": "2x"
                },
                {
                  "type": "SendEvent",
                  "arguments": [
                    "onDoublePress",
                    "middle"
                  ]
                }
              ]
            }
          ]
        },
        {
          "type": "Text",
          "id": "textRight",
          "text": "Right"
        }
      ]
    }
  }
}
)";

TEST_F(GesturesTest, DoublePressChangesTargetBetweenClicks) {
    loadDocument(DOUBLE_PRESS_TARGETS);

    // 1st click on the left touch wrapper
    ASSERT_TRUE(MouseDown(root, 50, 10));
    ASSERT_TRUE(MouseUp(root, 50, 10));

    // 2nd click on the middle touch wrapper
    advanceTime(200);
    ASSERT_TRUE(MouseDown(root, 150, 10));
    ASSERT_TRUE(MouseUp(root, 150, 10));

    advanceTime(600); // trigger timeout
    ASSERT_TRUE(CheckEvent("onSinglePress", "middle"));
    ASSERT_FALSE(root->hasEvent());

    // Click on the left touch wrapper again
    ASSERT_TRUE(MouseDown(root, 50, 10));
    ASSERT_TRUE(MouseUp(root, 50, 10));
    advanceTime(600);
    ASSERT_TRUE(CheckEvent("onSinglePress", "left"));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, DoublePressLosesTargetBetweenClicks) {
    loadDocument(DOUBLE_PRESS_TARGETS);

    // 1st click on the left touch wrapper
    ASSERT_TRUE(MouseDown(root, 50, 10));
    ASSERT_TRUE(MouseUp(root, 50, 10));

    // 2nd click on the right text
    advanceTime(200);
    ASSERT_FALSE(MouseDown(root, 250, 10)); // not a touchable component
    ASSERT_FALSE(MouseUp(root, 250, 10)); // not a touchable component

    advanceTime(600); // trigger timeout
    ASSERT_FALSE(root->hasEvent());

    // Click on the left touch wrapper again
    ASSERT_TRUE(MouseDown(root, 50, 10));
    ASSERT_TRUE(MouseUp(root, 50, 10));
    advanceTime(600);
    ASSERT_TRUE(CheckEvent("onSinglePress", "left"));
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
    advanceTime(500);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onUp"));
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(10,0), "onDown"));
    ASSERT_FALSE(root->hasEvent());

    // Not enough to fire onLongPressStart
    advanceTime(500);
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());
    ASSERT_FALSE(root->hasEvent());

    // This is enough
    advanceTime(500);
    ASSERT_EQ("Long ...", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onLongPressStart", 10, true));

    advanceTime(500);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(10,0), "onLongPressEnd", 10, true));
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
        "mode",
        "twWidth",
        "twHeight"
      ],
      "items": {
        "type": "TouchWrapper",
        "width": "${twWidth}",
        "height": "${twHeight}",
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
    "parameters": [ "direction", "mode", "w", "h"],
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
          "mode": "${mode}",
          "twWidth": "${w}",
          "twHeight": "${h}"
        }
      ]
    }
  }
})";

TEST_F(GesturesTest, SwipeAwayUnfinished)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up before fulfilled.
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    // Avoid flick triggered
    advanceTime(200);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(190,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));

    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());

    advanceTime(600);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(160,100), "onSwipeMove", 0.4, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-40), tw->getChildAt(1)));


    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(160,100)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-20), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.2, "left"));

    // Go to the end
    advanceTime(100);
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.0, "left"));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
}

TEST_F(GesturesTest, SwipeAwayUnfinishedMiddle)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up before fulfilled.
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(150,100), "onDown"));
    // Avoid flick triggered
    advanceTime(200);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));

    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());

    advanceTime(600);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(110,100), "onSwipeMove", 0.4, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-40), tw->getChildAt(1)));


    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(110,100)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-20), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.2, "left"));

    // Go to the end
    advanceTime(100);
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.0, "left"));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
}

TEST_F(GesturesTest, SwipeAwayCancelled)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.6, "left"));

    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));

    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());


    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerCancel, Point(140,100)));

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
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(200,110), "onMove"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(200,120), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_EQ(1, tw->getChildCount());

    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, SwipeAwayLeftReveal)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(190,100), "onDown"));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(180,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-10), tw->getChildAt(1)));


    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(130,100), "onSwipeMove", 0.6, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-60), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(130,100)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-80), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "left"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayLeftRevealTapOrScrollTimeout)
{
    config->set(RootProperty::kTapOrScrollTimeout, 60);
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(190,100), "onDown"));
    advanceTime(50);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(185,100), "onMove"));
    advanceTime(50);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(180,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));

    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(130,100), "onSwipeMove", 0.6, "left"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(130,100)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "left"));

    advanceTime(100);
    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));
}

TEST_F(GesturesTest, SwipeAwayLeftCover)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "cover", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(190,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1),
                           kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(90), tw->getChildAt(1)));

    advanceTime(500);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onSwipeMove", 0.60, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(40), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(140,100)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(20), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "left"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

void
GesturesTest::swipeAwayLeftSlide()
{
    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(190,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1),
                           kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-10), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(90), tw->getChildAt(1)));

    advanceTime(500);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onSwipeMove", 0.6, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-60), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(40), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(140,100)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(20), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "left"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayLeftSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide", "w": 100, "h": 100 })");
    swipeAwayLeftSlide();
}

TEST_F(GesturesTest, SwipeAwayBackwardSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "backward", "mode": "slide", "w": 100, "h": 100 })");
    swipeAwayLeftSlide();
}

TEST_F(GesturesTest, SwipeAwayLeftRightLeftSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    advanceTime(800);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(120,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1),
                           kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(20), tw->getChildAt(1)));

    advanceTime(200);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onSwipeMove", 0.60, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-60), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(40), tw->getChildAt(1)));


    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(140,100)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(20), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "left"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayLeftRightLeftSlideUnfinished)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    advanceTime(550);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(145,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.55, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1),
                           kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-55), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(45), tw->getChildAt(1)));

    advanceTime(50);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(160,100), "onSwipeMove", 0.40, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-40), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(60), tw->getChildAt(1)));


    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(160,100)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-20), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(80), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.2, "left"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.0, "left"));

    root->clearPending();
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
}

TEST_F(GesturesTest, SwipeAwayFlickLeftSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    // Advance time to something in flick range
    advanceTime(150);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(170,100), "onMove"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(170,100), "onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.3, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1),
                           kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-30), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(70), tw->getChildAt(1)));

    // Advance to half of remaining position.
    advanceTime(200);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-70), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(30), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.70, "left"));

    advanceTime(200);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayUnfinishedFlickLeftSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    // Advance time to something not in flick range
    advanceTime(250);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(180,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.2, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1),
                           kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-20), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(80), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(180,100)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-10), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(90), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.0, "left"));

    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
}

TEST_F(GesturesTest, SwipeAwayFlickTooFast)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    // This will actually give us 20000 dp/s, which would end up in 2 ms without a limit.
    advanceTime(1);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(180,100), "onMove"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(180,100), "onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.2, "left"));

    // Advance to half of remaining position.
    advanceTime(20);

    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.6, "left"));

    advanceTime(20);
    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    root->clearDirty();
}

TEST_F(GesturesTest, SwipeAwayLeftSlideNotEnoughDistance)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up before fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(195,100), "onMove"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(195,100), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    root->clearPending();
    ASSERT_FALSE(root->hasEvent());
}

void
GesturesTest::swipeAwayRightSlide() {
    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));

    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,100), "onDown"));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(110,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "right"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1),
                           kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(10), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-90), tw->getChildAt(1)));


    advanceTime(500);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(160,100), "onSwipeMove", 0.6, "right"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(60), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-40), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(160,100)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-20), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "right"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "right"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "right"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayRightSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "right", "mode": "slide", "w": 100, "h": 100 })");
    swipeAwayRightSlide();
}

TEST_F(GesturesTest, SwipeAwayForwardSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "forward", "mode": "slide", "w": 100, "h": 100 })");
    swipeAwayRightSlide();
}

TEST_F(GesturesTest, SwipeAwayUpSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "up", "mode": "slide", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,200), "onDown"));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(100,190), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "up"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1),
                           kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-10), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(90), tw->getChildAt(1)));

    advanceTime(500);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(100,140), "onSwipeMove", 0.60, "up"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-60), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(40), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(100,140)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(20), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "up"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "up"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "up"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayDownSlide)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "down", "mode": "slide", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,100), "onDown"));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(100,110), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "down"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1),
                           kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(10), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-90), tw->getChildAt(1)));

    advanceTime(500);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(100,160), "onSwipeMove", 0.60, "down"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(60), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-40), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(100,160)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(80), tw->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(-20), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "down"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());

    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "down"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "down"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateY(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayOverSwipe)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "cover", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.6, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ("swipy", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1),
                           kPropertyTransform, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(40), tw->getChildAt(1)));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(90,100), "onSwipeMove", 1.0, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(1)));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(210,100), "onSwipeMove", 0.0, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(100), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(210,100)));

    advanceTime(200);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.0, "left"));

    root->clearPending();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, SwipeAwayLeftPointerMovementTooVertical)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    advanceTime(100);
    // Move by 10 in X direction, but by 20 in the Y direction (too vertical). Gesture should not be triggered.
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(190,120), "onMove"));
    ASSERT_FALSE(root->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(140,120), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
}

TEST_F(GesturesTest, SwipeAwayRightPointerMovementTooVertical)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "right", "mode": "reveal", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,100), "onDown"));
    advanceTime(100);
    // Move by 10 in X direction, but by 20 in the Y direction (too vertical). Gesture should not be triggered.
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(110,120), "onMove"));
    ASSERT_FALSE(root->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(140,120), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
}

TEST_F(GesturesTest, SwipeAwayUpPointerMovementTooHorizontal)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "up", "mode": "reveal", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,200), "onDown"));
    advanceTime(100);
    // Move by 10 in Y direction, but by 20 in the X direction (too vertical). Gesture should not be triggered.
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(120,190), "onMove"));
    ASSERT_FALSE(root->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(120,140), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
}

TEST_F(GesturesTest, SwipeAwayDownPointerMovementTooHorizontal)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "down", "mode": "reveal", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,100), "onDown"));
    advanceTime(100);
    // Move by 10 in X direction, but by 20 in the Y direction (too vertical). Gesture should not be triggered.
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(120,110), "onMove"));
    ASSERT_FALSE(root->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(120,140), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
}

TEST_F(GesturesTest, SwipeAwayMaxDurationEnforced)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 400, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(500,100), "onDown"));
    advanceTime(1000);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(460,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-40), tw->getChildAt(1)));

    advanceTime(1000);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(260,100), "onSwipeMove", 0.6,
            "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-240), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(260,100)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-320), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "left"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayContext)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");

    rapidjson::Document document(rapidjson::kObjectType);
    // Retrieve context anch check the base
    auto context = root->serializeVisualContext(document.GetAllocator());

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
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(190,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-10), tw->getChildAt(1)));

    // While swiping there will be two with appropriate transforms
    ASSERT_TRUE(CheckDirtyVisualContext(root, tw->getChildAt(0),tw->getChildAt(1)));
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_FALSE(component->isVisualContextDirty());
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

    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(140,100), "onSwipeMove", 0.6, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-60), tw->getChildAt(1)));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(140,100)));

    advanceTime(200);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    // After swipe finished we have only 1 which is new one.
    ASSERT_TRUE(CheckDirtyVisualContext(root, tw));
    context = root->serializeVisualContext(document.GetAllocator());
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

TEST_F(GesturesTest, SwipeAwayLeftDisabled)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "slide", "w": 100, "h": 100 })");

    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    tw->setState(StateProperty::kStateDisabled, true);

    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_EQ(Rect(0, 0, 100, 100), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());

    // Up after fulfilled
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(200,100), false));
    advanceTime(100);
    ASSERT_FALSE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(190,100), true));
    ASSERT_FALSE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_FALSE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_FALSE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));

    advanceTime(500);
    ASSERT_FALSE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(140,100), true));

    ASSERT_FALSE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));

    ASSERT_FALSE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(140,100), false));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_FALSE(CheckEvent("onSwipeMove", 0.8, "left"));

    advanceTime(100);
    ASSERT_FALSE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());
    ASSERT_FALSE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_FALSE(CheckEvent("onSwipeDone", "left"));

    ASSERT_FALSE(CheckDirty(tw->getChildAt(0), kPropertyTransform));
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

    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress"));

    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());

    /************ Too short for long press but ok for single click ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));

    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    advanceTime(700);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());

    /************ Long press and single press instead of double ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));

    advanceTime(1000);

    ASSERT_EQ("Long ...", text->getCalculated(kPropertyText).asString());
    advanceTime(1000);
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onLongPressStart"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onLongPressEnd"));
    ASSERT_EQ("Long ... click", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    advanceTime(500);
    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    /************ Double press instead of long one ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    // Double tap consumed long press start.
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress"));

    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());

    /************ Insufficient move for swipe so single press only ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    advanceTime(50);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(5,0), "onMove"));

    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(5,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    // Wait out single press
    advanceTime(500);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    /************ Sufficient move for swipe ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    advanceTime(600);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(60,0), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.6, "right"));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(60,0)));
    advanceTime(200);
    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "right"));
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

    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onUp"));
    ASSERT_EQ("Lorem ipsum dolor", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress"));

    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());

    /************ Too short for long press but ok for single click ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));

    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onUp"));
    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    advanceTime(700);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());

    /************ Long press and single press instead of double ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));

    advanceTime(1000);

    ASSERT_EQ("Long ...", text->getCalculated(kPropertyText).asString());
    advanceTime(1000);
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onLongPressStart"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onLongPressEnd"));
    ASSERT_EQ("Long ... click", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    advanceTime(500);
    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    /************ Double press instead of long one ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onUp"));
    ASSERT_EQ("Click", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckEvent("onPress"));

    // Double tap consumed long press start.
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(50,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress"));

    ASSERT_EQ("Clicky click", text->getCalculated(kPropertyText).asString());

    /************ Insufficient move for swipe so single press only ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    advanceTime(50);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(55,0), "onMove"));

    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(55,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    // Wait out single press
    advanceTime(500);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    /************ Sufficient move for swipe ************/

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(50,0), "onDown"));
    advanceTime(600);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(100,0), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.5, "right"));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(100,0)));
    advanceTime(350);
    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "right"));
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
      "id": "MyGraphic",
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
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    // Timeout Double press and ensure it reported single press
    advanceTime(600);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));
    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onCancel"));
    ASSERT_TRUE(CheckEvent("onDoublePress"));
}

TEST_F(GesturesTest, DoublePressDisabledAVG)
{
    loadDocument(ALL_AVG);

    auto myGraphic = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("MyGraphic"));
    myGraphic->setState(StateProperty::kStateDisabled, true);

    ASSERT_EQ(kComponentTypeVectorGraphic, component->getType());

    ASSERT_FALSE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_FALSE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_FALSE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    // Timeout Double press and ensure it reported single press
    advanceTime(600);
    ASSERT_FALSE(CheckEvent("onSinglePress"));

    ASSERT_FALSE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_FALSE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_FALSE(CheckEvent("onPress"));
    advanceTime(400);
    ASSERT_FALSE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_FALSE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onCancel"));
    ASSERT_FALSE(CheckEvent("onDoublePress"));
}

TEST_F(GesturesTest, LongPressAVG)
{
    loadDocument(ALL_AVG);

    ASSERT_EQ(kComponentTypeVectorGraphic, component->getType());

    // Too short for long press
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    // Wait out single press
    advanceTime(500);
    ASSERT_TRUE(CheckEvent("onSinglePress"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_FALSE(root->hasEvent());

    // Not enough to fire onLongPressStart
    advanceTime(400);
    ASSERT_FALSE(root->hasEvent());

    // This is enough
    advanceTime(700);
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onLongPressStart"));

    advanceTime(500);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onLongPressEnd"));
}

TEST_F(GesturesTest, LongPressDisabledAVG)
{
    loadDocument(ALL_AVG);

    auto myGraphic = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("MyGraphic"));
    myGraphic->setState(StateProperty::kStateDisabled, true);

    ASSERT_EQ(kComponentTypeVectorGraphic, component->getType());

    // Too short for long press
    ASSERT_FALSE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    advanceTime(400);
    ASSERT_FALSE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_FALSE(CheckEvent("onPress"));

    // Wait out single press
    advanceTime(500);
    ASSERT_FALSE(CheckEvent("onSinglePress"));

    ASSERT_FALSE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(0,0), "onDown"));
    ASSERT_FALSE(root->hasEvent());

    // Not enough to fire onLongPressStart
    advanceTime(400);
    ASSERT_FALSE(root->hasEvent());

    // This is enough
    advanceTime(700);
    ASSERT_FALSE(CheckEvent("onCancel"));
    ASSERT_FALSE(CheckEvent("onLongPressStart"));

    advanceTime(500);
    ASSERT_FALSE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onLongPressEnd"));
}

TEST_F(GesturesTest, SwipeAwayAVG)
{
    loadDocument(ALL_AVG);

    ASSERT_EQ(kComponentTypeVectorGraphic, component->getType());

    // Not supported so just single press should happen
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,0), "onDown"));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerMove, Point(90,0), "onMove"));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(0,0), "onUp"));
    ASSERT_TRUE(CheckEvent("onPress"));

    // Wait out single press
    advanceTime(900);
    ASSERT_TRUE(CheckEvent("onSinglePress"));
}

static const char *SWIPE_TO_DELETE = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "layouts": {
    "swipeAway" : {
      "parameters": ["text1", "text2"],
      "item": {
        "type": "TouchWrapper",
        "width": 200,
        "item": {
          "type": "Frame",
          "backgroundColor": "blue",
          "height": 100,
          "items": {
            "type": "Text",
            "text": "${text1}",
            "fontSize": 60
          }
        },
        "gestures": [
          {
            "type": "SwipeAway",
            "direction": "left",
            "action":"reveal",
            "items": {
              "type": "Frame",
              "backgroundColor": "purple",
              "width": "100%",
              "items": {
                "type": "Text",
                "text": "${text2}",
                "fontSize": 60,
                "color": "white"
              }
            },
            "onSwipeDone": {
              "type": "SendEvent",
              "arguments": ["${event.source.uid}", "${index}"]
            }
          }
        ]
      }
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Sequence",
        "width": "100%",
        "height": 500,
        "alignItems": "center",
        "justifyContent": "spaceAround",
        "data": "${TestArray}",
        "items": [
          {
            "type": "swipeAway",
            "text1": "${data}",
            "text2": "${data}"
          }
        ]
      }
    ]
  }
})";

TEST_F(GesturesTest, SwipeToDelete)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2, 3, 4, 5});
    config->liveData("TestArray", myArray);

    loadDocument(SWIPE_TO_DELETE);

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());

    auto idToDelete = component->getChildAt(0)->getUniqueId();

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,1)));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(190,1)));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(140,1)));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(140,1)));

    advanceTime(800);
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    int indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(0, indexToDelete);

    myArray->remove(indexToDelete);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyBounds, kPropertyNotifyChildrenChanged));
    root->clearDirty();

    ASSERT_EQ(4, component->getChildCount());

    // Repeat for very first
    idToDelete = component->getChildAt(0)->getUniqueId();

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,1)));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(190,1)));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(140,1)));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(140,1)));

    advanceTime(800);
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(0, indexToDelete);
    root->clearDirty();

    myArray->remove(indexToDelete);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyBounds, kPropertyNotifyChildrenChanged));
    root->clearDirty();

    ASSERT_EQ(3, component->getChildCount());

    // Remove one at the end
    idToDelete = component->getChildAt(2)->getUniqueId();

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,201)));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(190,201)));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(140,201)));
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(140,201)));

    advanceTime(800);
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(2, indexToDelete);
    root->clearDirty();

    myArray->remove(indexToDelete);
    root->clearPending();
    root->clearDirty();

    ASSERT_EQ(2, component->getChildCount());
}
/*
 * Verify handling of transformations by gestures
 */

static const char *DOUBLE_PRESS_TRANSFORMATION = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "id": "tw",
      "width": 400,
      "height": 200,
      "item": {
        "type": "Frame",
        "id": "frame"
      },
      "gestures": [
        {
          "type": "DoublePress",
          "onSinglePress": [
            {
              "type": "SendEvent",
              "sequencer": "MAIN",
              "arguments": [
                "onSinglePress",
                "${event.component.x}",
                "${event.component.y}",
                "${event.component.width}",
                "${event.component.height}"
              ]
            }
          ],
          "onDoublePress": [
            {
              "type": "SendEvent",
              "sequencer": "MAIN",
              "arguments": [
                "onDoublePress",
                "${event.component.x}",
                "${event.component.y}",
                "${event.component.width}",
                "${event.component.height}"
              ]
            }
          ]
        }
      ],
      "onDown": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onDown",
          "${event.component.x}",
          "${event.component.y}",
          "${event.component.width}",
          "${event.component.height}"
        ]
      },
      "onMove": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onMove",
          "${event.component.x}",
          "${event.component.y}",
          "${event.component.width}",
          "${event.component.height}",
          "${event.inBounds}"
        ]
      },
      "onUp": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onUp",
          "${event.component.x}",
          "${event.component.y}",
          "${event.component.width}",
          "${event.component.height}",
          "${event.inBounds}"
        ]
      },
      "onCancel": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onCancel",
          "${event.component.x}",
          "${event.component.y}",
          "${event.component.width}",
          "${event.component.height}"
        ]
      },
      "onPress": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onPress"
        ]
      }
    }
  }
})";

TEST_F(GesturesTest, DoublePressTransformed) {
    loadDocument(DOUBLE_PRESS_TRANSFORMATION);

    // Scale by half, bounds will become (100,50)-(300,150)
    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0.5));

    // Verify transformation has been applied
    ASSERT_FALSE(MouseClick(root, 1, 1));

    ASSERT_TRUE(MouseClick(root, 110, 55));
    ASSERT_TRUE(CheckEvent("onDown", 20, 10, 400, 200));
    ASSERT_TRUE(CheckEvent("onUp", 20, 10, 400, 200, true));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    // Timeout double press
    advanceTime(600);
    ASSERT_TRUE(CheckEvent("onSinglePress", 20, 10, 400, 200));

    // Perform double press - first click
    advanceTime(100);
    ASSERT_TRUE(MouseClick(root, 110, 55));
    ASSERT_TRUE(CheckEvent("onDown", 20, 10, 400, 200));
    ASSERT_TRUE(CheckEvent("onUp", 20, 10, 400, 200, true));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    // Perform double press - second click
    advanceTime(100);
    ASSERT_TRUE(MouseClick(root, 110, 55));
    ASSERT_TRUE(CheckEvent("onDown", 20, 10, 400, 200));
    ASSERT_TRUE(CheckEvent("onCancel", 20, 10, 400, 200));
    ASSERT_TRUE(CheckEvent("onDoublePress", 20, 10, 400, 200));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, DoublePressTransformedBetweenClicks) {
    loadDocument(DOUBLE_PRESS_TRANSFORMATION);

    // Perform double press - first click
    ASSERT_TRUE(MouseClick(root, 10, 10));
    ASSERT_TRUE(CheckEvent("onDown", 10, 10, 400, 200));
    ASSERT_TRUE(CheckEvent("onUp", 10, 10, 400, 200, true));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    // Scale by half, bounds will become (100,50)-(300,150)
    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0.5));

    // Perform double press - first click
    ASSERT_FALSE(MouseClick(root, 10, 10));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, DoublePressTransformedBeforeLastMouseUp) {
    loadDocument(DOUBLE_PRESS_TRANSFORMATION);

    // Perform double press - first click
    ASSERT_TRUE(MouseClick(root, 10, 10));
    ASSERT_TRUE(CheckEvent("onDown", 10, 10, 400, 200));
    ASSERT_TRUE(CheckEvent("onUp", 10, 10, 400, 200, true));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    // Scale by half, bounds will become (100,50)-(300,150)
    ASSERT_TRUE(MouseDown(root, 10, 10));
    ASSERT_TRUE(CheckEvent("onDown", 10, 10, 400, 200));
    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0.5));
    ASSERT_FALSE(MouseUp(root, 10, 10));
    ASSERT_TRUE(CheckEvent("onCancel", -180, -80, 400, 200));
    ASSERT_TRUE(CheckEvent("onDoublePress", -180, -80, 400, 200));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, DoublePressSingularTransformDuringFirstPress) {
    loadDocument(DOUBLE_PRESS_TRANSFORMATION);

    // Perform double press - first click
    ASSERT_TRUE(MouseDown(root, 10, 10));
    ASSERT_TRUE(CheckEvent("onDown", 10, 10, 400, 200));
    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0));
    ASSERT_FALSE(MouseUp(root, 10, 10));
    ASSERT_TRUE(CheckEvent("onUp", NAN, NAN, 400, 200, false));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_FALSE(MouseDown(root, 10, 10));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, DoublePressSingularTransformDuringSecondPress) {
    loadDocument(DOUBLE_PRESS_TRANSFORMATION);

    // Perform double press - first click
    ASSERT_TRUE(MouseClick(root, 10, 10));
    ASSERT_TRUE(CheckEvent("onDown", 10, 10, 400, 200));
    ASSERT_TRUE(CheckEvent("onUp", 10, 10, 400, 200, true));
    ASSERT_TRUE(CheckEvent("onPress"));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(MouseDown(root, 10, 10));
    ASSERT_TRUE(CheckEvent("onDown", 10, 10, 400, 200));
    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0));
    ASSERT_FALSE(MouseUp(root, 10, 10));
    ASSERT_TRUE(CheckEvent("onCancel", NAN, NAN, 400, 200));
    ASSERT_TRUE(CheckEvent("onDoublePress", NAN, NAN, 400, 200));
    ASSERT_FALSE(root->hasEvent());
}

static const char *LONG_PRESS_TRANSFORMATION = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "id": "tw",
      "width": 400,
      "height": 200,
      "item": {
        "type": "Frame",
        "id": "frame"
      },
      "gestures": [
        {
          "type": "LongPress",
          "onLongPressStart": [
            {
              "type": "SendEvent",
              "sequencer": "MAIN",
              "arguments": [
                "onLongPressStart",
                "${event.component.x}",
                "${event.component.y}",
                "${event.component.width}",
                "${event.component.height}",
                "${event.inBounds}"
              ]
            }
          ],
          "onLongPressEnd": [
            {
              "type": "SendEvent",
              "sequencer": "MAIN",
              "arguments": [
                "onLongPressEnd",
                "${event.component.x}",
                "${event.component.y}",
                "${event.component.width}",
                "${event.component.height}",
                "${event.inBounds}"
              ]
            }
          ]
        }
      ],
      "onDown": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onDown",
          "${event.component.x}",
          "${event.component.y}",
          "${event.component.width}",
          "${event.component.height}"
        ]
      },
      "onMove": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onMove",
          "${event.component.x}",
          "${event.component.y}",
          "${event.component.width}",
          "${event.component.height}",
          "${event.inBounds}"
        ]
      },
      "onUp": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onUp",
          "${event.component.x}",
          "${event.component.y}",
          "${event.component.width}",
          "${event.component.height}",
          "${event.inBounds}"
        ]
      },
      "onCancel": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onCancel",
          "${event.component.x}",
          "${event.component.y}",
          "${event.component.width}",
          "${event.component.height}"
        ]
      },
      "onPress": {
        "type": "SendEvent",
        "sequencer": "MAIN",
        "arguments": [
          "onPress"
        ]
      }
    }
  }
})";

TEST_F(GesturesTest, LongPressTransformed) {
    loadDocument(LONG_PRESS_TRANSFORMATION);

    // Scale by half, bounds will become (100,50)-(300,150)
    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0.5));

    // Verify transformation has been applied
    ASSERT_FALSE(MouseClick(root, 1, 1));

    ASSERT_TRUE(MouseDown(root, 110, 55));
    ASSERT_TRUE(CheckEvent("onDown", 20, 10, 400, 200));
    ASSERT_FALSE(root->hasEvent());

    // Trigger long press start
    advanceTime(1000);
    ASSERT_TRUE(CheckEvent("onCancel", 20, 10, 400, 200));
    ASSERT_TRUE(CheckEvent("onLongPressStart", 20, 10, 400, 200, true));

    // Trigger long press end
    advanceTime(500);
    ASSERT_TRUE(MouseUp(root, 110, 55));
    ASSERT_TRUE(CheckEvent("onLongPressEnd", 20, 10, 400, 200, true));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, LongPressTransformedBetweenMouseEvents) {
    loadDocument(LONG_PRESS_TRANSFORMATION);

    ASSERT_TRUE(MouseDown(root, 10, 10));
    ASSERT_TRUE(CheckEvent("onDown", 10, 10, 400, 200));
    ASSERT_FALSE(root->hasEvent());

    // Trigger long press start
    advanceTime(1000);
    ASSERT_TRUE(CheckEvent("onCancel", 10, 10, 400, 200));
    ASSERT_TRUE(CheckEvent("onLongPressStart", 10, 10, 400, 200, true));

    // Scale by half, bounds will become (100,50)-(300,150)
    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0.5));

    // Trigger long press end
    advanceTime(500);
    ASSERT_FALSE(MouseUp(root, 10, 10));
    ASSERT_TRUE(CheckEvent("onLongPressEnd", -180, -80, 400, 200, false));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, LongPressSingularTransformAfterMouseDown) {
    loadDocument(LONG_PRESS_TRANSFORMATION);

    ASSERT_TRUE(MouseDown(root, 200, 100)); //center
    ASSERT_TRUE(CheckEvent("onDown", 200, 100, 400, 200));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0));

    // Trigger long press start
    advanceTime(1000);
    ASSERT_TRUE(CheckEvent("onCancel", NAN, NAN, 400, 200));
    ASSERT_TRUE(CheckEvent("onLongPressStart", NAN, NAN, 400, 200, false));

    // Trigger long press end
    advanceTime(500);
    ASSERT_FALSE(MouseUp(root, 200, 100)); // center
    ASSERT_TRUE(CheckEvent("onLongPressEnd", NAN, NAN, 400, 200, false));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, LongPressSingularTransformAfterStart) {
    loadDocument(LONG_PRESS_TRANSFORMATION);

    ASSERT_TRUE(MouseDown(root, 200, 100)); // center
    ASSERT_TRUE(CheckEvent("onDown", 200, 100, 400, 200));
    ASSERT_FALSE(root->hasEvent());

    // Trigger long press start
    advanceTime(1000);
    ASSERT_TRUE(CheckEvent("onCancel", 200, 100, 400, 200));
    ASSERT_TRUE(CheckEvent("onLongPressStart", 200, 100, 400, 200, true));

    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0));

    // Trigger long press end
    advanceTime(500);
    ASSERT_FALSE(MouseUp(root, 200, 100)); // center
    ASSERT_TRUE(CheckEvent("onLongPressEnd", NAN, NAN, 400, 200, false));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(GesturesTest, SwipeAwayScaled)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");
    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0.5, "rotate", 90));
    ASSERT_TRUE(CheckDirty(tw, kPropertyTransform));

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(150,175), "onDown"));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(150,165), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.2, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-20), tw->getChildAt(1)));


    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(150,145), "onSwipeMove", 0.6,
            "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-60), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(150,145)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-80), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "left"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayRotated)
{
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");
    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("texty", tw->getChildAt(0)->getId());

    ASSERT_TRUE(TransformComponent(root, "tw", "rotate", 90));
    ASSERT_TRUE(CheckDirty(tw, kPropertyTransform));

    // Up after min velocity reached
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(100,200), "onDown"));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(100,180), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.2, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-20), tw->getChildAt(1)));


    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(100,160), "onSwipeMove", 0.4,
            "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-40), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(100,160)));

    // Advance to half of remaining position.
    advanceTime(150);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-70), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.7, "left"));

    advanceTime(150);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwayTransformedDuringSwipe) {
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");
    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(190,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-10), tw->getChildAt(1)));

    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0.5));
    ASSERT_TRUE(CheckDirty(tw, kPropertyTransform));

    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(145,150), "onSwipeMove", 0.6, "left"));

    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-60), tw->getChildAt(1)));

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(145,150)));

    // Advance to half of remaining position.
    advanceTime(100);
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-80), tw->getChildAt(1)));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.8, "left"));

    advanceTime(100);
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_TRUE(CheckEvent("onSwipeMove", 1.0, "left"));
    ASSERT_TRUE(CheckEvent("onSwipeDone", "left"));

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), tw->getChildAt(0)));
    ASSERT_EQ(tw->getCalculated(kPropertyInnerBounds).getRect(), tw->getChildAt(0)->getCalculated(kPropertyBounds).getRect());
}

TEST_F(GesturesTest, SwipeAwaySingularTransformDuringSwipe) {
    loadDocument(SWIPE_AWAY, R"({ "direction": "left", "mode": "reveal", "w": 100, "h": 100 })");
    auto tw = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("tw"));

    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(200,100), "onDown"));
    advanceTime(100);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(190,100), "onMove"));
    ASSERT_TRUE(CheckEvent("onCancel"));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.1, "left"));
    ASSERT_TRUE(CheckDirty(tw, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, tw->getChildCount());
    ASSERT_EQ("swipy", tw->getChildAt(0)->getId());
    ASSERT_EQ("texty", tw->getChildAt(1)->getId());

    ASSERT_TRUE(CheckDirty(tw->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(tw->getChildAt(1), kPropertyTransform));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-10), tw->getChildAt(1)));

    ASSERT_TRUE(TransformComponent(root, "tw", "scale", 0.0));
    ASSERT_TRUE(CheckDirty(tw, kPropertyTransform));

    advanceTime(400);
    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(145,150), "onSwipeMove", 0.0, "left"));
    ASSERT_FALSE(root->hasEvent());

    advanceTime(100);
    ASSERT_FALSE(MouseUp(root, 145,150));
    ASSERT_TRUE(CheckEvent("onSwipeMove", 0.0, "left"));

    ASSERT_TRUE(session->checkAndClear());
}

static const char *SWIPE_RTL = R"(
{
  "type": "APL",
  "version": "1.7",
  "layouts": {
    "swipeAway" : {
      "parameters": ["text1", "text2", "action", "dir"],
      "item": {
        "type": "TouchWrapper",
        "width": "100%",
        "item": {
          "type": "Frame",
          "backgroundColor": "blue",
          "height": 100,
          "items": {
            "type": "Text",
            "text": "${text1}",
            "fontSize": 60
          }
        },
        "gestures": [
          {
            "type": "SwipeAway",
            "direction": "${dir}",
            "action":"${action}",
            "items": {
              "type": "Frame",
              "id": "internalFrame2",
              "backgroundColor": "purple",
              "width": "100%",
              "height": 100,
              "items": {
                "type": "Text",
                "text": "${text2}",
                "fontSize": 60,
                "color": "white"
              }
            }
          }
        ]
      }
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "width": 400,
        "height": 200,
        "justifyContent": "spaceAround",
        "items": [
          {
            "id": "forwardSwipe",
            "height": 100,
            "layoutDirection": "RTL",
            "type": "swipeAway",
            "text1": "Swipe with reveal",
            "text2": "You swiped with reveal",
            "action": "reveal",
            "dir": "forward"
          },
          {
            "id": "backwardSwipe",
            "height": 100,
            "layoutDirection": "RTL",
            "type": "swipeAway",
            "text1": "Swipe with reveal",
            "text2": "You swiped with reveal",
            "action": "reveal",
            "dir": "backward"
          }
        ]
      }
    ]
  }
}
)";

TEST_F(GesturesTest, SwipeAwayRTL) {
    loadDocument(SWIPE_RTL);
    auto f1 = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("forwardSwipe"));
    auto f2 = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("backwardSwipe"));

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(20,40)));
    advanceTime(100);

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(-100,40)));
    advanceTime(100);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), f1->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(-120), f1->getChildAt(1)));
    advanceTime(100);

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(-100,40)));

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(20,140)));
    advanceTime(100);

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(100,140)));
    advanceTime(100);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), f2->getChildAt(0)));
    ASSERT_TRUE(CheckTransform(Transform2D::translateX(80), f2->getChildAt(1)));
    advanceTime(100);

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(100,140)));
    advanceTime(200);

    loop->advanceToEnd();
}

TEST_F(GesturesTest, SwipeAwayWrongDirectionRTL) {
    loadDocument(SWIPE_RTL);
    auto f1 = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("forwardSwipe"));
    auto f2 = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("backwardSwipe"));

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(20,40)));
    advanceTime(100);

    ASSERT_FALSE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(100,40)));
    advanceTime(100);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), f1->getChildAt(0)));
    advanceTime(100);

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(100,40)));

    // Up after fulfilled
    ASSERT_TRUE(HandleAndCheckPointerEvent(PointerEventType::kPointerDown, Point(20,140)));
    advanceTime(100);

    ASSERT_FALSE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerMove, Point(-100,140)));
    advanceTime(100);

    ASSERT_TRUE(CheckTransform(Transform2D::translateX(0), f2->getChildAt(0)));
    advanceTime(100);

    ASSERT_TRUE(HandleAndCheckConsumedPointerEvent(PointerEventType::kPointerUp, Point(-100,140)));
}

