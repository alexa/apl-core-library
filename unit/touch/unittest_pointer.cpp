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

::testing::AssertionResult PokeTarget(const RootContextPtr& root, double x, double y) {
    auto point = Point(x,y);
    auto down = root->handlePointerEvent(PointerEvent(kPointerDown, point));
    auto up = root->handlePointerEvent(PointerEvent(kPointerUp, point));

    if (!down)
        return ::testing::AssertionFailure() << "Down failed to hit target at " << x << "," << y;
    if (!up)
        return ::testing::AssertionFailure() << "Up failed to hit target at " << x << "," << y;

    return ::testing::AssertionSuccess();
}

template<class... Args>
::testing::AssertionResult TransformComponent(const RootContextPtr& root, const std::string& id, Args... args) {
    auto component = root->topComponent()->findComponentById(id);
    if (!component)
        return ::testing::AssertionFailure() << "Unable to find component " << id;

    std::vector<Object> values = {args...};
    if (values.size() % 2 != 0)
        return ::testing::AssertionFailure() << "Must provide an even number of transformations";

    ObjectArray transforms;
    for (auto it = values.begin() ; it != values.end() ; ) {
        auto key = *it++;
        auto value = *it++;
        auto map = std::make_shared<ObjectMap>();
        map->emplace(key.asString(), value);
        transforms.emplace_back(std::move(map));
    }

    auto event = std::make_shared<ObjectMap>();
    event->emplace("type", "SetValue");
    event->emplace("componentId", id);
    event->emplace("property", "transform");
    event->emplace("value", std::move(transforms));

    root->executeCommands(ObjectArray{event}, true);
    root->clearPending();

    return ::testing::AssertionSuccess();
}

/**
 * A set of tests to verify that we find the correct component in the hierarchy when there is
 * a touch or mouse event.  These tests check to see that transformed, scrolled, and otherwise
 * positioning of the component hierarchy still results in the correct component being selected.
 */
class PointerTest : public DocumentWrapper {};

static const char * MOVING =
    R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 400,
          "height": 400,
          "items": {
            "type": "TouchWrapper",
            "id": "MyTouch",
            "width": 100,
            "height": 20
          }
        }
      }
    }
)apl";

/*
 * Move a component around on the screen and verify that you can hit it.
 */
TEST_F(PointerTest, Moving) {
    loadDocument(MOVING);

    ASSERT_TRUE(PokeTarget(root, 50, 10));  // The center
    ASSERT_FALSE(PokeTarget(root, 50, 40)); // Far Down
    ASSERT_TRUE(PokeTarget(root, 10, 10));  // Left
    ASSERT_TRUE(PokeTarget(root, 80, 10));  // Right
    ASSERT_FALSE(PokeTarget(root, 120, 10));  // Far right

    ASSERT_TRUE(TransformComponent(root, "MyTouch", "translateX", 200));
    ASSERT_FALSE(PokeTarget(root, 50, 10));  // The center
    ASSERT_FALSE(PokeTarget(root, 50, 40));  // Far Down
    ASSERT_FALSE(PokeTarget(root, 10, 10));  // Left
    ASSERT_FALSE(PokeTarget(root, 80, 10));  // Right
    ASSERT_FALSE(PokeTarget(root, 120, 10));  // Far right
    ASSERT_TRUE(PokeTarget(root, 220, 10));  // Really far right

    ASSERT_TRUE(TransformComponent(root, "MyTouch", "rotate", 90));  // Rotates about the center
    ASSERT_FALSE(PokeTarget(root, 10, 10));
    ASSERT_TRUE(PokeTarget(root, 50, 10));
    ASSERT_TRUE(PokeTarget(root, 50, 50));
    ASSERT_FALSE(PokeTarget(root, 50, -40));  // Outside of the parent bounds

    ASSERT_TRUE(TransformComponent(root, "MyTouch", "scale", 0.5));  // Shrink it about the center
    ASSERT_TRUE(PokeTarget(root, 50, 10));  // The center
    ASSERT_FALSE(PokeTarget(root, 80, 10));
    ASSERT_FALSE(PokeTarget(root, 20, 10));
    ASSERT_FALSE(PokeTarget(root, 50, 16));
    ASSERT_FALSE(PokeTarget(root, 50, 4));
}


/**
 * Singular matrix test - we shrink a component until it disappears
 */
TEST_F(PointerTest, Singularity)
{
    loadDocument(MOVING);

    ASSERT_TRUE(PokeTarget(root, 50, 10));  // The center
    ASSERT_TRUE(PokeTarget(root, 60, 10));

    // Shrink to 10% (occurs about the center)
    ASSERT_TRUE(TransformComponent(root, "MyTouch", "scale", 0.1));
    ASSERT_TRUE(PokeTarget(root, 50, 10));
    ASSERT_FALSE(PokeTarget(root, 60, 10));

    // Shrink it to 0% (occurs about the center)
    ASSERT_TRUE(TransformComponent(root, "MyTouch", "scale", 0));
    ASSERT_FALSE(PokeTarget(root, 50, 10));
    ASSERT_FALSE(PokeTarget(root, 60, 10));
}

static const char * PADDING =
    R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 400,
          "height": 400,
          "paddingTop": 100,
          "paddingLeft": 100,
          "items": {
            "type": "TouchWrapper",
            "id": "MyTouch",
            "width": 200,
            "height": 200
          }
        }
      }
    }
)apl";

/*
 * Make sure we are applying inverse transformations in the correct coordinate space.
 * If we don't, the padding applied here will cause some miscalculations in the region
 * near the touch wrapper.
 */
TEST_F(PointerTest, Padding)
{
    loadDocument(PADDING);

    ASSERT_TRUE(PokeTarget(root, 200, 200));  // The center
    ASSERT_TRUE(PokeTarget(root, 100, 100));  // Top left
    ASSERT_TRUE(PokeTarget(root, 300, 300)); // Bottom right

    // Shrink to 150% (occurs about the center)
    ASSERT_TRUE(TransformComponent(root, "MyTouch", "scale", 1.5));
    ASSERT_TRUE(PokeTarget(root, 200, 200));
    ASSERT_TRUE(PokeTarget(root, 51, 51)); // Top-left corner, after transformation
    ASSERT_FALSE(PokeTarget(root, 25, 25));
    ASSERT_TRUE(PokeTarget(root, 350, 350)); // Bottom-right corner, after transformation
    ASSERT_FALSE(PokeTarget(root, 375, 375));
}


static const char * OVERLAP =
    R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 400,
          "height": 400,
          "items": [
            {
              "type": "Frame",
              "id": "BottomFrame",
              "width": 100,
              "height": 100,
              "position": "absolute",
              "top": 100,
              "left": 100,
              "items": {
                "type": "TouchWrapper",
                "id": "BottomTouch",
                "width": "100%",
                "height": "100%",
                "onPress": {
                  "type": "SendEvent",
                  "arguments": [
                    "Right"
                  ]
                }
              }
            },
            {
              "type": "Container",
              "id": "HidingContainer",
              "description": "This container exactly overlaps the first frame",
              "width": 100,
              "height": 100,
              "position": "absolute",
              "top": 100,
              "left": 100
            }
          ]
        }
      }
    }
)apl";

/*
 * Start with a component OVERLAPPING the target component.  Then *transform* that component
 * to move it out of the way.
 */
TEST_F(PointerTest, Overlapping) {
    loadDocument(OVERLAP);

    // Poking the container doesn't result in a pointer event
    ASSERT_FALSE(PokeTarget(root, 150, 150));
    ASSERT_FALSE(root->hasEvent());

    // Now shift the TopFrame out of the way
    ASSERT_TRUE(TransformComponent(root, "HidingContainer", "translateX", 200));

    auto hiding = root->topComponent()->findComponentById("HidingContainer");
    auto transform = hiding->getCalculated(kPropertyTransform).getTransform2D();
    ASSERT_EQ(Point(200,0), transform * Point(0,0));

    // Poking the same point should hit the TouchWrapper
    ASSERT_TRUE(PokeTarget(root, 150, 150));
    ASSERT_TRUE(CheckSendEvent(root, "Right"));
}

static const char *SCROLLING_CONTAINER =
    R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "ScrollView",
          "width": 400,
          "height": 400,
          "paddingLeft": 10,
          "paddingTop": 10,
          "items": {
            "type": "Container",
            "id": "MyContainer",
            "width": "100%",
            "height": "200%",
            "paddingLeft": 10,
            "paddingRight": 10,
            "items": {
              "type": "TouchWrapper",
              "id": "MyTouch",
              "width": 100,
              "height": 20
            }
          }
        }
      }
    }
)apl";

/**
 * Test moving around a touch wrapper in a scroll view
 */
TEST_F(PointerTest, ScrollView)
{
    loadDocument(SCROLLING_CONTAINER);

    // Verify you can hit the target at the starting location
    ASSERT_TRUE(PokeTarget(root, 25, 25));
    ASSERT_FALSE(PokeTarget(root, 15, 15));  // The padding adds up to 20,20
    ASSERT_TRUE(PokeTarget(root, 115, 25));  // Right side of the component
    ASSERT_FALSE(PokeTarget(root, 25, 45));  // Too far down

    // Scroll down
    component->update(kUpdateScrollPosition, 100);
    ASSERT_FALSE(PokeTarget(root, 25, 25));

    // Move the touchwrapper down to compensate
    ASSERT_TRUE(TransformComponent(root, "MyTouch", "translateY", 100));  // Move it down by the scroll amount
    ASSERT_TRUE(PokeTarget(root, 25, 25));
}

static const char* SEQUENCE_AND_PAGER = R"({
  "type": "APL",
  "version": "1.4",
  "layouts": {
    "Potato": {
      "parameters": [
        "w",
        "h",
        "c",
        "i"
      ],
      "item": [
        {
          "type": "TouchWrapper",
          "width": "${w}",
          "height": "${h}",
          "id": "${c}${i}",
          "item": {
            "type": "Frame",
            "backgroundColor": "${c}",
            "width": "${w}",
            "height": "${h}",
            "item": {
              "type": "Text",
              "text": "${i}"
            }
          },
          "onDown": {
            "type": "SendEvent",
            "sequencer": "SE",
            "arguments": [ "onDown:${event.source.id}" ]
          }
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "direction": "row",
      "items": [
        {
          "type": "Sequence",
          "id": "scrollings",
          "width": 200,
          "height": "100%",
          "data": ["red", "yellow"],
          "items": [
            {
              "type": "Potato",
              "w": 200,
              "h": 400,
              "c": "${data}",
              "i": "_sequence${index}"
            }
          ]
        },
        {
          "type": "Container",
          "width": 100
        },
        {
          "type": "Pager",
          "id": "pagers",
          "width": 500,
          "height": 500,
          "items": [
            {
              "type": "Potato",
              "w": "100%",
              "h": "100%",
              "c": "green",
              "i": "_pager"
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(PointerTest, SequenceAndPager)
{
    loadDocument(SEQUENCE_AND_PAGER);

    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(0,100))));
    ASSERT_TRUE(CheckSendEvent(root, "onDown:red_sequence0"));
}

static const char *ON_PRESS =
    R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 400,
          "height": 400,
          "items": [
            {
              "type": "Frame",
              "id": "Frame",
              "width": 300,
              "height": 300,
              "position": "absolute",
              "top": 100,
              "left": 100,
              "paddingTop": 100,
              "paddingLeft": 100,
              "items": {
                "type": "TouchWrapper",
                "id": "TouchWrapper",
                "width": "100",
                "height": "100",
                "onPress": {
                  "type": "SendEvent",
                  "arguments": [
                    "Pressed"
                  ]
                }
              }
            }
          ]
        }
      }
    }
)apl";


TEST_F(PointerTest, OnPress)
{
    loadDocument(ON_PRESS);

    // center
    ASSERT_TRUE(PokeTarget(root, 250, 250));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // top left
    ASSERT_TRUE(PokeTarget(root, 201, 201));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // bottom right
    ASSERT_TRUE(PokeTarget(root, 299, 299));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // out of bounds
    ASSERT_FALSE(PokeTarget(root, 301, 301));
    ASSERT_FALSE(root->hasEvent());

    // --- Release mouse inside component --

    ASSERT_TRUE(MouseDown(root, 250, 250));
    ASSERT_TRUE(MouseUp(root, 201, 201)); // within bounds
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // --- Release mouse outside component --

    ASSERT_TRUE(MouseDown(root, 250, 250));
    ASSERT_TRUE(MouseUp(root, 199, 199)); // within bounds
    ASSERT_FALSE(root->hasEvent());
}

/**
 * Test that the onPress event correctly accounts for applied transformations.
 */
TEST_F(PointerTest, TransformedOnPress)
{
    loadDocument(ON_PRESS);

    // --- Grow by 50% ---

    ASSERT_TRUE(TransformComponent(root, "TouchWrapper", "scale", 1.5));

    // center
    ASSERT_TRUE(PokeTarget(root, 250, 250));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // top left
    ASSERT_TRUE(PokeTarget(root, 176, 176));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // top left
    ASSERT_TRUE(PokeTarget(root, 324, 324));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // out of bounds
    ASSERT_FALSE(PokeTarget(root, 326, 326));
    ASSERT_FALSE(root->hasEvent());

    // --- Release mouse inside component --

    ASSERT_TRUE(MouseDown(root, 250, 250));
    ASSERT_TRUE(MouseUp(root, 324, 324)); // within bounds
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // --- Release mouse outside component --

    ASSERT_TRUE(MouseDown(root, 250, 250));
    ASSERT_TRUE(MouseUp(root, 326, 326)); // out of bounds
    ASSERT_FALSE(root->hasEvent());
}

/**
 * Check that we correctly handle a singular transformation after
 * a target is acquired on mouse down.
 */
TEST_F(PointerTest, SingularTransformDuringPress)
{
    loadDocument(ON_PRESS);

    // center
    ASSERT_TRUE(MouseDown(root, 250, 250));
    ASSERT_TRUE(TransformComponent(root, "TouchWrapper", "scale", 0));
    ASSERT_FALSE(MouseUp(root, 250, 250));
    ASSERT_FALSE(root->hasEvent());
}

/**
 * Test that the onPress event correctly accounts for applied transformations
 * that occur between mouse down and mouse up events
 */
TEST_F(PointerTest, TransformedDuringPress)
{
    loadDocument(ON_PRESS);

    // Grow component during mouse press
    ASSERT_TRUE(MouseDown(root, 250, 250));
    ASSERT_TRUE(TransformComponent(root, "TouchWrapper", "scale", 1.5));
    ASSERT_TRUE(MouseUp(root, 324, 324)); // now a hit because of scaling
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // Move component away during mouse press
    ASSERT_TRUE(MouseDown(root, 201, 201));
    ASSERT_TRUE(TransformComponent(root, "TouchWrapper", "translateX", 100));
    ASSERT_TRUE(MouseUp(root, 249, 249)); // no longer a hit due to translation
    ASSERT_FALSE(root->hasEvent());
}

static const char *PRUNED_TRAVERSAL =
    R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "direction": "row",
          "width": "400",
          "height": "400",
          "items": [
            {
              "type": "TouchWrapper",
              "width": 100,
              "height": 100,
              "item": {
                "type": "Frame",
                "width": "100%",
                "height": "100%"
              },
              "onDown": {
                "type": "SendEvent",
                "arguments": [
                  "Down"
                ]
              }
            },
            {
              "type": "Frame",
              "width": 100,
              "height": 100
            }
          ]
        }
      }
    }
)apl";

/**
 * Test that the traversal of the component hierarchy is correctly pruned.
 */
TEST_F(PointerTest, PruneTraversal) {
    loadDocument(PRUNED_TRAVERSAL);

    // Poke the frame to the right of the touch wrapper. This should trigger no event.
    // If the search is incorrectly pruned, the same local coordinates might trigger an event in
    // the touch wrapper since they have the same local coordinates.
    ASSERT_FALSE(PokeTarget(root, 150, 50));
    ASSERT_FALSE(root->hasEvent());
}

static const char *TOUCH_WRAPPER_MOUSE_EVENT =
    R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 400,
          "height": 400,
          "paddingLeft": 10,
          "paddingTop": 10,
          "items": [
            {
              "type": "TouchWrapper",
              "id": "TouchWrapper",
              "width": "100",
              "height": "60",
              "onUp": {
                "type": "SendEvent",
                "sequencer": "MAIN",
                "arguments": [
                  "MouseUp",
                  "${event.component.x}",
                  "${event.component.y}",
                  "${event.component.width}",
                  "${event.component.height}",
                  "${event.inBounds}"
                ]
              }
            }
          ]
        }
      }
    }
)apl";

TEST_F(PointerTest, TouchWrapperEventProperties) {
    loadDocument(TOUCH_WRAPPER_MOUSE_EVENT);

    ASSERT_TRUE(PokeTarget(root, 60, 40)); // center
    ASSERT_TRUE(CheckSendEvent(root, "MouseUp", 50, 30, 100, 60, true));

    ASSERT_TRUE(MouseDown(root, 60, 40)); // center
    ASSERT_TRUE(MouseUp(root, 120, 80)); // outside of bounds
    ASSERT_TRUE(CheckSendEvent(root, "MouseUp", 110, 70, 100, 60, false));
}

/**
 * Verify that the event properties when a TouchWrapper is transformed are relative to the
 * component's bounding box (i.e. original size), not the transformed/rendered size.
 */
TEST_F(PointerTest, TransformedTouchWrapperEventProperties) {
    loadDocument(TOUCH_WRAPPER_MOUSE_EVENT);
    ASSERT_TRUE(TransformComponent(root, "TouchWrapper", "scale", 0.5));

    ASSERT_TRUE(PokeTarget(root, 60, 40)); // center
    ASSERT_TRUE(CheckSendEvent(root, "MouseUp", 50, 30, 100, 60, true));

    ASSERT_TRUE(MouseDown(root, 60, 40)); // center
    ASSERT_TRUE(MouseUp(root, 90, 60)); // outside of bounds after shrinking
    ASSERT_TRUE(CheckSendEvent(root, "MouseUp", 110, 70, 100, 60, false));
}

static const char * VECTOR_GRAPHIC_MOUSE_EVENT =
    R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "graphics": {
        "box": {
          "type": "AVG",
          "version": "1.0",
          "height": 100,
          "width": 100
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "id": "vg",
          "align": "top-left",
          "paddingLeft": 10,
          "paddingRight": 10,
          "paddingTop": 10,
          "paddingBottom": 10,
          "source": "box",
          "width": 220,
          "height": 80,
          "onDown": {
            "type": "SendEvent",
            "sequencer": "MAIN",
            "arguments": [
              "Down",
              "${event.viewport.x}",
              "${event.viewport.y}",
              "${event.viewport.width}",
              "${event.viewport.height}",
              "${event.viewport.inBounds}",
              "${event.component.x}",
              "${event.component.y}",
              "${event.component.width}",
              "${event.component.height}"
            ]
          },
          "onUp": {
            "type": "SendEvent",
            "sequencer": "MAIN",
            "arguments": [
              "Up",
              "${event.viewport.x}",
              "${event.viewport.y}",
              "${event.viewport.width}",
              "${event.viewport.height}",
              "${event.viewport.inBounds}",
              "${event.component.x}",
              "${event.component.y}",
              "${event.component.width}",
              "${event.component.height}",
              "${event.inBounds}"
            ]
          }
        }
      }
})apl";

TEST_F(PointerTest, VectorGraphicEventProperties) {
    loadDocument(VECTOR_GRAPHIC_MOUSE_EVENT);

    ASSERT_TRUE(PokeTarget(root, 20, 30));
    ASSERT_TRUE(CheckSendEvent(root, "Down",
                               10, 20, 100, 100, true,
                               20, 30, 220, 80));
    ASSERT_TRUE(CheckSendEvent(root, "Up",
                               10, 20, 100, 100, true,
                               20, 30, 220, 80, true));

    // Release outside viewport but within vector graphics
    ASSERT_TRUE(MouseDown(root, 20, 30));
    ASSERT_TRUE(MouseUp(root, 200, 50));
    ASSERT_TRUE(CheckSendEvent(root, "Down",
                               10, 20, 100, 100, true,
                               20, 30, 220, 80));
    ASSERT_TRUE(CheckSendEvent(root, "Up",
                               190, 40, 100, 100, false,
                               200, 50, 220, 80, true));

    // Release outside vector graphics
    ASSERT_TRUE(MouseDown(root, 20, 30));
    ASSERT_TRUE(MouseUp(root, 230, 90));
    ASSERT_TRUE(CheckSendEvent(root, "Down",
                               10, 20, 100, 100, true,
                               20, 30, 220, 80));
    ASSERT_TRUE(CheckSendEvent(root, "Up",
                               220, 80, 100, 100, false,
                               230, 90, 220, 80, false));
}

/**
 * Verify that the event properties when a VectorGraphic component is transformed are relative to
 * the component's bounding box (i.e. original size), not the transformed/rendered size.
 */
TEST_F(PointerTest, TransformedVectorGraphicEventProperties) {
    loadDocument(VECTOR_GRAPHIC_MOUSE_EVENT);
    ASSERT_TRUE(TransformComponent(root, "vg", "scale", 0.5));

    ASSERT_TRUE(PokeTarget(root, 110, 40)); // center
    ASSERT_TRUE(CheckSendEvent(root, "Down",
                               100, 30, 100, 100, true,
                               110, 40, 220, 80));
    ASSERT_TRUE(CheckSendEvent(root, "Up",
                               100, 30, 100, 100, true,
                               110, 40, 220, 80, true));


    // Release outside viewport but within vector graphics
    ASSERT_TRUE(MouseDown(root, 110, 40));
    ASSERT_TRUE(MouseUp(root, 150, 50));
    EXPECT_TRUE(CheckSendEvent(root, "Down",
                               100, 30, 100, 100, true,
                               110, 40, 220, 80));
    EXPECT_TRUE(CheckSendEvent(root, "Up",
                               /*
                                * |2 0 -110|   | 1 0 -10|
                                * |0 2  -40| * | 0 1 -10| * (150, 50, 1)
                                * |0 0    1|   | 0 0   1|
                                */
                               180, 50, 100, 100, false,

                               /*
                                * |2 0 -110|
                                * |0 2  -40| * (150, 50, 1)
                                * |0 0    1|
                                */
                               190, 60, 220, 80, true));

    // Release outside vector graphics
    ASSERT_TRUE(MouseDown(root, 110, 40));
    ASSERT_TRUE(MouseUp(root, 170, 70));
    EXPECT_TRUE(CheckSendEvent(root, "Down",
                               100, 30, 100, 100, true,
                               110, 40, 220, 80));
    EXPECT_TRUE(CheckSendEvent(root, "Up",
                               /*
                                * |2 0 -110|   | 1 0 -10|
                                * |0 2  -40| * | 0 1 -10| * (170, 70, 1)
                                * |0 0    1|   | 0 0   1|
                                */
                               220, 90, 100, 100, false,

                               /*
                                * |2 0 -110|
                                * |0 2  -40| * (170, 70, 1)
                                * |0 0    1|
                                */
                               230, 100, 220, 80, false));
}