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
