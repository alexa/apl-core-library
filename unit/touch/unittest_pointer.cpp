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

    ASSERT_TRUE(MouseClick(root, 50, 10));  // The center
    ASSERT_FALSE(MouseClick(root, 50, 40)); // Far Down
    ASSERT_TRUE(MouseClick(root, 10, 10));  // Left
    ASSERT_TRUE(MouseClick(root, 80, 10));  // Right
    ASSERT_FALSE(MouseClick(root, 120, 10));  // Far right

    ASSERT_TRUE(TransformComponent(root, "MyTouch", "translateX", 200));
    ASSERT_FALSE(MouseClick(root, 50, 10));  // The center
    ASSERT_FALSE(MouseClick(root, 50, 40));  // Far Down
    ASSERT_FALSE(MouseClick(root, 10, 10));  // Left
    ASSERT_FALSE(MouseClick(root, 80, 10));  // Right
    ASSERT_FALSE(MouseClick(root, 120, 10));  // Far right
    ASSERT_TRUE(MouseClick(root, 220, 10));  // Really far right

    ASSERT_TRUE(TransformComponent(root, "MyTouch", "rotate", 90));  // Rotates about the center
    ASSERT_FALSE(MouseClick(root, 10, 10));
    ASSERT_TRUE(MouseClick(root, 50, 10));
    ASSERT_TRUE(MouseClick(root, 50, 50));
    ASSERT_FALSE(MouseClick(root, 50, -40));  // Outside of the parent bounds

    ASSERT_TRUE(TransformComponent(root, "MyTouch", "scale", 0.5));  // Shrink it about the center
    ASSERT_TRUE(MouseClick(root, 50, 10));  // The center
    ASSERT_FALSE(MouseClick(root, 80, 10));
    ASSERT_FALSE(MouseClick(root, 20, 10));
    ASSERT_FALSE(MouseClick(root, 50, 16));
    ASSERT_FALSE(MouseClick(root, 50, 4));
}


/**
 * Singular matrix test - we shrink a component until it disappears
 */
TEST_F(PointerTest, Singularity)
{
    loadDocument(MOVING);

    ASSERT_TRUE(MouseClick(root, 50, 10));  // The center
    ASSERT_TRUE(MouseClick(root, 60, 10));

    // Shrink to 10% (occurs about the center)
    ASSERT_TRUE(TransformComponent(root, "MyTouch", "scale", 0.1));
    ASSERT_TRUE(MouseClick(root, 50, 10));
    ASSERT_FALSE(MouseClick(root, 60, 10));

    // Shrink it to 0% (occurs about the center)
    ASSERT_TRUE(TransformComponent(root, "MyTouch", "scale", 0));
    ASSERT_FALSE(MouseClick(root, 50, 10));
    ASSERT_FALSE(MouseClick(root, 60, 10));
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

    ASSERT_TRUE(MouseClick(root, 200, 200));  // The center
    ASSERT_TRUE(MouseClick(root, 100, 100));  // Top left
    ASSERT_TRUE(MouseClick(root, 300, 300)); // Bottom right

    // Shrink to 150% (occurs about the center)
    ASSERT_TRUE(TransformComponent(root, "MyTouch", "scale", 1.5));
    ASSERT_TRUE(MouseClick(root, 200, 200));
    ASSERT_TRUE(MouseClick(root, 51, 51)); // Top-left corner, after transformation
    ASSERT_FALSE(MouseClick(root, 25, 25));
    ASSERT_TRUE(MouseClick(root, 350, 350)); // Bottom-right corner, after transformation
    ASSERT_FALSE(MouseClick(root, 375, 375));
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
    ASSERT_FALSE(MouseClick(root, 150, 150));
    ASSERT_FALSE(root->hasEvent());

    // Now shift the TopFrame out of the way
    ASSERT_TRUE(TransformComponent(root, "HidingContainer", "translateX", 200));

    auto hiding = root->topComponent()->findComponentById("HidingContainer");
    auto transform = hiding->getCalculated(kPropertyTransform).getTransform2D();
    ASSERT_EQ(Point(200,0), transform * Point(0,0));

    // Poking the same point should hit the TouchWrapper
    ASSERT_TRUE(MouseClick(root, 150, 150));
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
    auto touch = std::static_pointer_cast<CoreComponent>(component->findComponentById("MyTouch"));

    // Verify you can hit the target at the starting location
    ASSERT_TRUE(MouseClick(root, touch, 25, 25));
    ASSERT_TRUE(MouseClick(root, component, 15, 15));  // The padding adds up to 20,20
    ASSERT_TRUE(MouseClick(root, touch, 115, 25));  // Right side of the component
    ASSERT_TRUE(MouseClick(root, component, 25, 45));  // Too far down

    // Scroll down
    component->update(kUpdateScrollPosition, 100);
    ASSERT_TRUE(MouseClick(root, component, 25, 25));

    // Move the touchwrapper down to compensate
    ASSERT_TRUE(TransformComponent(root, "MyTouch", "translateY", 100));  // Move it down by the scroll amount
    ASSERT_TRUE(MouseClick(root, touch, 25, 25));
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

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(0,100), false));
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
    ASSERT_TRUE(MouseClick(root, 250, 250));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // top left
    ASSERT_TRUE(MouseClick(root, 201, 201));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // bottom right
    ASSERT_TRUE(MouseClick(root, 299, 299));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // out of bounds
    ASSERT_FALSE(MouseClick(root, 301, 301));
    ASSERT_FALSE(root->hasEvent());

    // --- Release mouse inside component --

    ASSERT_TRUE(MouseDown(root, 250, 250));
    ASSERT_TRUE(MouseUp(root, 201, 201)); // within bounds
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // --- Release mouse outside component --

    ASSERT_TRUE(MouseDown(root, 250, 250));
    ASSERT_FALSE(MouseUp(root, 199, 199)); // outside bounds
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
    ASSERT_TRUE(MouseClick(root, 250, 250));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // top left
    ASSERT_TRUE(MouseClick(root, 176, 176));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // top left
    ASSERT_TRUE(MouseClick(root, 324, 324));
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // out of bounds
    ASSERT_FALSE(MouseClick(root, 326, 326));
    ASSERT_FALSE(root->hasEvent());

    // --- Release mouse inside component --

    ASSERT_TRUE(MouseDown(root, 250, 250));
    ASSERT_TRUE(MouseUp(root, 324, 324)); // within bounds
    ASSERT_TRUE(CheckSendEvent(root, "Pressed"));

    // --- Release mouse outside component --

    ASSERT_TRUE(MouseDown(root, 250, 250));
    ASSERT_FALSE(MouseUp(root, 326, 326)); // out of bounds
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
    ASSERT_FALSE(root->hasEvent()); // no 'onPress' event generated
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
    ASSERT_FALSE(MouseUp(root, 249, 249)); // no longer a hit due to translation
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
    ASSERT_FALSE(MouseClick(root, 150, 50));
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
              },
              "onMove": {
                "type": "SendEvent",
                "sequencer": "MAIN",
                "arguments": [
                  "MouseMove",
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

TEST_F(PointerTest, TouchWrapperUpEventProperties) {
    loadDocument(TOUCH_WRAPPER_MOUSE_EVENT);

    ASSERT_TRUE(MouseClick(root, 60, 40)); // center
    ASSERT_TRUE(CheckSendEvent(root, "MouseUp", 50, 30, 100, 60, true));

    ASSERT_TRUE(MouseDown(root, 60, 40)); // center
    ASSERT_FALSE(MouseUp(root, 120, 80)); // outside of bounds
    ASSERT_TRUE(CheckSendEvent(root, "MouseUp", 110, 70, 100, 60, false));
}

TEST_F(PointerTest, TouchWrapperUpEventPropertiesSingularity) {
    loadDocument(TOUCH_WRAPPER_MOUSE_EVENT);

    ASSERT_TRUE(MouseDown(root, 60, 40)); // center
    ASSERT_TRUE(TransformComponent(root, "TouchWrapper", "scale", 0));
    ASSERT_FALSE(MouseUp(root, 60, 40)); // center
    ASSERT_TRUE(CheckSendEvent(root, "MouseUp", NAN, NAN, 100, 60, false));
}

TEST_F(PointerTest, TouchWrapperMoveEventProperties) {
    loadDocument(TOUCH_WRAPPER_MOUSE_EVENT);

    ASSERT_TRUE(MouseDown(root, 60, 40)); // center
    ASSERT_TRUE(MouseMove(root, 50, 40));
    ASSERT_TRUE(CheckSendEvent(root, "MouseMove", 40, 30, 100, 60, true));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_FALSE(MouseMove(root, 410, 410));
    ASSERT_TRUE(CheckSendEvent(root, "MouseMove", 400, 400, 100, 60, false));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(PointerTest, TouchWrapperMoveEventPropertiesSingularity) {
    loadDocument(TOUCH_WRAPPER_MOUSE_EVENT);

    ASSERT_TRUE(MouseDown(root, 60, 40)); // center
    ASSERT_TRUE(TransformComponent(root, "TouchWrapper", "scale", 0));
    ASSERT_FALSE(MouseMove(root, 50, 40));
    ASSERT_TRUE(CheckSendEvent(root, "MouseMove", NAN, NAN, 100, 60, false));
    ASSERT_FALSE(root->hasEvent());
}

/**
 * Verify that the event properties when a TouchWrapper is transformed are relative to the
 * component's bounding box (i.e. original size), not the transformed/rendered size.
 */
TEST_F(PointerTest, TransformedTouchWrapperEventProperties) {
    loadDocument(TOUCH_WRAPPER_MOUSE_EVENT);
    ASSERT_TRUE(TransformComponent(root, "TouchWrapper", "scale", 0.5));

    ASSERT_TRUE(MouseClick(root, 60, 40)); // center
    ASSERT_TRUE(CheckSendEvent(root, "MouseUp", 50, 30, 100, 60, true));

    ASSERT_TRUE(MouseDown(root, 60, 40)); // center
    ASSERT_TRUE(MouseMove(root, 50, 40));
    ASSERT_TRUE(CheckSendEvent(root, "MouseMove", 30, 30, 100, 60, true));
    ASSERT_FALSE(MouseUp(root, 90, 60)); // outside of bounds after shrinking
    ASSERT_TRUE(CheckSendEvent(root, "MouseUp", 110, 70, 100, 60, false));
    ASSERT_FALSE(root->hasEvent());
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
          },
          "onMove": {
            "type": "SendEvent",
            "sequencer": "MAIN",
            "arguments": [
              "Move",
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

    ASSERT_TRUE(MouseDown(root, 20, 30));
    ASSERT_TRUE(CheckSendEvent(root, "Down",
                               10, 20, 100, 100, true,
                               20, 30, 220, 80));
    ASSERT_TRUE(MouseMove(root, 10, 20));
    ASSERT_TRUE(CheckSendEvent(root, "Move",
                               0, 10, 100, 100, true,
                               10, 20, 220, 80, true));
    ASSERT_TRUE(MouseUp(root, 10, 20));
    ASSERT_TRUE(CheckSendEvent(root, "Up",
                               0, 10, 100, 100, true,
                               10, 20, 220, 80, true));

    // Outside viewport but within vector graphics
    ASSERT_TRUE(MouseDown(root, 20, 30));
    ASSERT_TRUE(MouseMove(root, 200, 50));
    ASSERT_TRUE(MouseUp(root, 200, 50));
    ASSERT_TRUE(CheckSendEvent(root, "Down",
                               10, 20, 100, 100, true,
                               20, 30, 220, 80));
    ASSERT_TRUE(CheckSendEvent(root, "Move",
                               190, 40, 100, 100, false,
                               200, 50, 220, 80, true));
    ASSERT_TRUE(CheckSendEvent(root, "Up",
                               190, 40, 100, 100, false,
                               200, 50, 220, 80, true));

    // Outside vector graphics
    ASSERT_TRUE(MouseDown(root, 20, 30));
    ASSERT_FALSE(MouseMove(root, 230, 90));
    ASSERT_FALSE(MouseUp(root, 230, 90));
    ASSERT_TRUE(CheckSendEvent(root, "Down",
                               10, 20, 100, 100, true,
                               20, 30, 220, 80));
    ASSERT_TRUE(CheckSendEvent(root, "Move",
                               220, 80, 100, 100, false,
                               230, 90, 220, 80, false));
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

    ASSERT_TRUE(MouseClick(root, 110, 40)); // center
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
    ASSERT_FALSE(MouseUp(root, 170, 70));
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

TEST_F(PointerTest, VectorGraphicSingularity) {
    loadDocument(VECTOR_GRAPHIC_MOUSE_EVENT);

    ASSERT_TRUE(MouseDown(root, 20, 30));
    ASSERT_TRUE(CheckSendEvent(root, "Down",
                               10, 20, 100, 100, true,
                               20, 30, 220, 80));
    TransformComponent(root, "vg", "scale", 0);
    ASSERT_FALSE(MouseMove(root, 10, 20));
    ASSERT_TRUE(CheckSendEvent(root, "Move",
                               NAN, NAN, 100, 100, false,
                               NAN, NAN, 220, 80, false));
    ASSERT_FALSE(MouseUp(root, 10, 20));
    ASSERT_TRUE(CheckSendEvent(root, "Up",
                               NAN, NAN, 100, 100, false,
                               NAN, NAN, 220, 80, false));
}

static const char *DYNAMIC_SEQUENCE = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
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
            "type": "TouchWrapper",
            "width": 200,
            "item": {
              "type": "Frame",
              "backgroundColor": "blue",
              "height": 100,
              "items": {
                "type": "Text",
                "text": "${data}",
                "fontSize": 60
              }
            }
          }
        ]
      }
    ]
  }
})";

TEST_F(PointerTest, TouchEmptySequence)
{
    auto myArray = LiveArray::create(ObjectArray{});
    config->liveData("TestArray", myArray);

    loadDocument(DYNAMIC_SEQUENCE);

    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(MouseDown(root, component, 200, 1));
    ASSERT_TRUE(MouseUp(root, component, 200, 1));
}

TEST_F(PointerTest, TouchEmptiedSequence)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2, 3, 4, 5});
    config->liveData("TestArray", myArray);

    loadDocument(DYNAMIC_SEQUENCE);

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(MouseDown(root, 200, 1));
    ASSERT_TRUE(MouseUp(root, 200, 1));

    myArray->clear();

    root->clearPending();

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(MouseDown(root, component, 200, 1));
    ASSERT_TRUE(MouseUp(root, component, 200, 1));
}

static const char *DYNAMIC_PAGER = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Pager",
        "width": "100%",
        "height": 500,
        "alignItems": "center",
        "justifyContent": "spaceAround",
        "data": "${TestArray}",
        "items": [
          {
            "type": "TouchWrapper",
            "item": {
              "type": "Frame",
              "backgroundColor": "blue",
              "items": {
                "type": "Text",
                "text": "${data}",
                "fontSize": 60
              }
            }
          }
        ]
      }
    ]
  }
})";

TEST_F(PointerTest, TouchEmptyPager)
{
    auto myArray = LiveArray::create(ObjectArray{});
    config->liveData("TestArray", myArray);

    loadDocument(DYNAMIC_PAGER);

    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(MouseDown(root, component, 200, 1));
    ASSERT_TRUE(MouseUp(root, component, 200, 1));
}

TEST_F(PointerTest, TouchEmptiedPager)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2, 3, 4, 5});
    config->liveData("TestArray", myArray);

    loadDocument(DYNAMIC_PAGER);

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(MouseDown(root, 200, 1));
    ASSERT_TRUE(MouseUp(root, 200, 1));

    myArray->clear();

    root->clearPending();

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(MouseDown(root, component, 200, 1));
    ASSERT_TRUE(MouseUp(root, component, 200, 1));
}

static const char *DYNAMIC_CCONTAINER = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "width": "100%",
        "height": 500,
        "alignItems": "center",
        "justifyContent": "spaceAround",
        "data": "${TestArray}",
        "items": [
          {
            "type": "TouchWrapper",
            "width": "100%",
            "height": 500,
            "item": {
              "type": "Frame",
              "backgroundColor": "blue",
              "items": {
                "type": "Text",
                "text": "${data}",
                "fontSize": 60
              }
            }
          }
        ]
      }
    ]
  }
})";

TEST_F(PointerTest, TouchEmptyContainer)
{
    auto myArray = LiveArray::create(ObjectArray{});
    config->liveData("TestArray", myArray);

    loadDocument(DYNAMIC_CCONTAINER);

    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->getChildCount());

    ASSERT_FALSE(MouseDown(root, 200, 1));
    ASSERT_FALSE(MouseUp(root, 200, 1));
}

TEST_F(PointerTest, TouchEmptiedContainer)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2, 3, 4, 5});
    config->liveData("TestArray", myArray);

    loadDocument(DYNAMIC_CCONTAINER);

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(MouseDown(root, 200, 1));
    ASSERT_TRUE(MouseUp(root, 200, 1));

    myArray->clear();

    root->clearPending();

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_FALSE(MouseDown(root, 200, 1));
    ASSERT_FALSE(MouseUp(root, 200, 1));
}

static const char *NESTED_INHERITED_AVG = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "graphics": {
    "Icon": {
      "type": "AVG",
      "version": "1.0",
      "height": 50,
      "width": 50,
      "items": [
        {
          "type": "path",
          "pathData": "M16,22c-0.256,0-0.512-0.098-0.707-0.293l-9-9c-0.391-0.391-0.391-1.023,0-1.414l9-9c0.391-0.391,1.023-0.391,1.414,0s0.391,1.023,0,1.414L8.414,12l8.293,8.293c0.391,0.391,0.391,1.023,0,1.414C16.512,21.902,16.256,22,16,22z",
          "fill": "#FAFAFA"
        }
      ]
    }
  },
  "mainTemplate": {
    "item": [
      {
        "type": "TouchWrapper",
        "id": "tw",
        "height": 100,
        "width": 100,
        "onPress": {
          "type": "SendEvent",
          "arguments": [
            "sent!"
          ]
        },
        "items": [
          {
            "type": "Frame",
            "height": "100%",
            "width": "100%",
            "borderWidth": 1,
            "borderColor": "green",
            "inheritParentState": true,
            "item": {
              "type": "VectorGraphic",
              "id": "vg",
              "width": 50,
              "height": 50,
              "source": "Icon",
              "inheritParentState": true
            }
          }
        ]
      }
    ]
  }
})";

TEST_F(PointerTest, TouchNestedAVG) {
    loadDocument(NESTED_INHERITED_AVG);

    root->handlePointerEvent(PointerEvent(kPointerMove, Point(40, 40), 0, kMousePointer));
    advanceTime(50);
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(40, 40), 0, kMousePointer));
    advanceTime(50);
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(40, 40), 0, kMousePointer));
    advanceTime(50);
    root->clearPending();

    ASSERT_TRUE(root->hasEvent());

    auto event = root->popEvent();
    ASSERT_EQ(event.getType(), kEventTypeSendEvent);
    ASSERT_EQ("sent!", event.getValue(kEventPropertyArguments).at(0).getString());
}

static const char *NESTED_AVG = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "graphics": {
    "Icon": {
      "type": "AVG",
      "version": "1.0",
      "height": 50,
      "width": 50,
      "items": [
        {
          "type": "path",
          "pathData": "M16,22c-0.256,0-0.512-0.098-0.707-0.293l-9-9c-0.391-0.391-0.391-1.023,0-1.414l9-9c0.391-0.391,1.023-0.391,1.414,0s0.391,1.023,0,1.414L8.414,12l8.293,8.293c0.391,0.391,0.391,1.023,0,1.414C16.512,21.902,16.256,22,16,22z",
          "fill": "#FAFAFA"
        }
      ]
    }
  },
  "mainTemplate": {
    "item": [
      {
        "type": "TouchWrapper",
        "id": "tw",
        "height": 100,
        "width": 100,
        "onPress": {
          "type": "SendEvent",
          "arguments": [
            "sent!"
          ]
        },
        "items": [
          {
            "type": "Frame",
            "height": "100%",
            "width": "100%",
            "borderWidth": 1,
            "borderColor": "green",
            "inheritParentState": true,
            "item": {
              "type": "VectorGraphic",
              "id": "vg",
              "width": 50,
              "height": 50,
              "source": "Icon",
              "onPress": {
                "type": "SendEvent",
                "arguments": [
                  "Very sent!"
                ]
              }
            }
          }
        ]
      }
    ]
  }
})";

TEST_F(PointerTest, TouchAVG) {
    loadDocument(NESTED_AVG);

    root->handlePointerEvent(PointerEvent(kPointerMove, Point(40, 40), 0, kMousePointer));
    advanceTime(50);
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(40, 40), 0, kMousePointer));
    advanceTime(50);
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(40, 40), 0, kMousePointer));
    advanceTime(50);
    root->clearPending();

    ASSERT_TRUE(root->hasEvent());

    auto event = root->popEvent();
    ASSERT_EQ(event.getType(), kEventTypeSendEvent);
    ASSERT_EQ("Very sent!", event.getValue(kEventPropertyArguments).at(0).getString());
}

static const char *TW_INHERITS_STATE_OLD = R"({
  "type": "APL",
  "version": "1.5",
  "theme": "dark",
  "mainTemplate": {
    "item": [
      {
        "type": "TouchWrapper",
        "inheritParentState": true,
        "id": "tw",
        "height": 100,
        "width": 100,
        "onPress": {
          "type": "SendEvent"
        }
      }
    ]
  }
})";

TEST_F(PointerTest, TouchWrapperInheritsState) {
    loadDocument(TW_INHERITS_STATE_OLD);

    ASSERT_TRUE(MouseDown(root, 50, 50));
    ASSERT_EQ(false, component->getState().get(kStatePressed));
    ASSERT_TRUE(session->checkAndClear());
    ASSERT_TRUE(MouseUp(root, 50, 50));
    ASSERT_TRUE(session->checkAndClear());
    root->clearPending();

    ASSERT_TRUE(root->hasEvent());

    auto event = root->popEvent();
    ASSERT_EQ(event.getType(), kEventTypeSendEvent);
}