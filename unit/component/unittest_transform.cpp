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

#include "rapidjson/document.h"
#include "gtest/gtest.h"

#include <cmath>

#include "apl/engine/evaluate.h"
#include "apl/engine/builder.h"

#include "../testeventloop.h"

using namespace apl;

class ComponentTransformTest : public DocumentWrapper {};

template<class T>
std::shared_ptr<T> as(const ComponentPtr &component) {
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

TEST_F(ComponentTransformTest, ChildInParent) {
    loadDocument(CHILD_IN_PARENT);

    auto touchWrapper = as<CoreComponent>(component->findComponentById("TouchWrapper"));
    auto frame = as<CoreComponent>(component->findComponentById("Frame"));

    ASSERT_TRUE(touchWrapper);
    ASSERT_TRUE(frame);

    ASSERT_EQ(Transform2D(), component->getGlobalToLocalTransform());
    ASSERT_EQ(Transform2D::translate(-40, -50), touchWrapper->getGlobalToLocalTransform());
    ASSERT_EQ(Transform2D::translate(-40, -50), frame->getGlobalToLocalTransform());
}

static const char *TRANSFORMATIONS =
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
              "transform": [
                {"scale": 0.5}
              ],
              "item": {
                "type": "Frame",
                "id": "Frame",
                "width": "100%",
                "height": "100%",
                "transform": [
                  {"translateX": 25}
                ]
              }
            }
          ]
        }
      }
    }
)apl";

TEST_F(ComponentTransformTest, Transformations) {
    loadDocument(TRANSFORMATIONS);

    auto touchWrapper = as<CoreComponent>(component->findComponentById("TouchWrapper"));
    auto frame = as<CoreComponent>(component->findComponentById("Frame"));

    ASSERT_TRUE(touchWrapper);
    ASSERT_TRUE(frame);

    ASSERT_EQ(Transform2D({2,0, 0, 2, -130, -150}), touchWrapper->getGlobalToLocalTransform());
    ASSERT_EQ(Transform2D({2,0, 0, 2, -155, -150}), frame->getGlobalToLocalTransform());
}

TEST_F(ComponentTransformTest, ToLocalPoint) {
    loadDocument(TRANSFORMATIONS);

    auto touchWrapper = as<CoreComponent>(component->findComponentById("TouchWrapper"));
    auto frame = as<CoreComponent>(component->findComponentById("Frame"));

    ASSERT_TRUE(touchWrapper);
    ASSERT_TRUE(frame);

    ASSERT_EQ(Transform2D({2,0, 0, 2, -130, -150}), touchWrapper->getGlobalToLocalTransform());
    ASSERT_EQ(Transform2D({2,0, 0, 2, -155, -150}), frame->getGlobalToLocalTransform());

    ASSERT_EQ(Point(-130, -150), touchWrapper->toLocalPoint({0,0}));
    ASSERT_EQ(Point(-110, -130), touchWrapper->toLocalPoint({10,10}));
    ASSERT_EQ(Point(-155, -150), frame->toLocalPoint({0,0}));
    ASSERT_EQ(Point(-135, -130), frame->toLocalPoint({10,10}));

    ASSERT_TRUE(TransformComponent(root, "Frame", "scale", 0));
    auto singularPoint = frame->toLocalPoint({0, 0});
    ASSERT_TRUE(std::isnan(singularPoint.getX()));
    ASSERT_TRUE(std::isnan(singularPoint.getY()));
}

static const char *SCROLL_VIEW =
    R"apl({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "parameters": [],
        "item": {
          "type": "ScrollView",
          "width": "100vw",
          "height": "100vh",
          "items": {
            "type": "Container",
            "items": {
              "type": "Frame",
              "width": 200,
              "height": 200
            },
            "data": [
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

TEST_F(ComponentTransformTest, ScrollView)
{
    loadDocument(SCROLL_VIEW);

    auto container = as<CoreComponent>(component->getChildAt(0));

    ASSERT_EQ(Transform2D(), component->getGlobalToLocalTransform());
    ASSERT_EQ(Transform2D(), container->getGlobalToLocalTransform());

    for (auto i = 0 ; i < container->getChildCount() ; i++) {
        auto child = as<CoreComponent>(container->getChildAt(i));
        ASSERT_EQ(Transform2D::translateY(-200 * i), child->getGlobalToLocalTransform());
    }

    component->update(kUpdateScrollPosition, 300);

    ASSERT_EQ(Transform2D(), component->getGlobalToLocalTransform());
    ASSERT_EQ(Transform2D::translateY(300), container->getGlobalToLocalTransform());

    for (auto i = 0 ; i < container->getChildCount() ; i++) {
        auto child = as<CoreComponent>(container->getChildAt(i));
        ASSERT_EQ(Transform2D::translateY(-200 * i + 300), child->getGlobalToLocalTransform());
    }
}

static const char *VERTICAL_SEQUENCE =
    R"apl({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "parameters": [],
        "item": {
          "type": "Sequence",
          "scrollDirection": "vertical",
          "width": 200,
          "height": 500,
          "items": {
            "type": "Frame",
            "width": 200,
            "height": 200
          },
          "data": [
            1,
            2,
            3,
            4,
            5
          ]
        }
      }
    }
)apl";

TEST_F(ComponentTransformTest, VerticalSequence)
{
    loadDocument(VERTICAL_SEQUENCE);
    advanceTime(10);

    ASSERT_EQ(Transform2D(), component->getGlobalToLocalTransform());

    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = as<CoreComponent>(component->getChildAt(i));
        ASSERT_EQ(Transform2D::translateY(-200 * i), child->getGlobalToLocalTransform());
    }

    component->update(kUpdateScrollPosition, 300);

    ASSERT_EQ(Transform2D(), component->getGlobalToLocalTransform());

    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = as<CoreComponent>(component->getChildAt(i));
        ASSERT_EQ(Transform2D::translateY(-200 * i + 300), child->getGlobalToLocalTransform());
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
          "scrollDirection": "horizontal",
          "width": 500,
          "height": 200,
          "items": {
            "type": "Frame",
            "width": 200,
            "height": 200
          },
          "data": [
            1,
            2,
            3,
            4,
            5
          ]
        }
      }
    }
)apl";

TEST_F(ComponentTransformTest, HorizontalSequence)
{
    loadDocument(HORIZONTAL_SEQUENCE);
    advanceTime(10);

    ASSERT_EQ(Transform2D(), component->getGlobalToLocalTransform());

    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = as<CoreComponent>(component->getChildAt(i));
        ASSERT_EQ(Transform2D::translateX(-200 * i), child->getGlobalToLocalTransform());
    }

    component->update(kUpdateScrollPosition, 300);

    ASSERT_EQ(Transform2D(), component->getGlobalToLocalTransform());

    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = as<CoreComponent>(component->getChildAt(i));
        ASSERT_EQ(Transform2D::translateX(-200 * i + 300), child->getGlobalToLocalTransform());
    }
}

static const char *STALENESS_PROPAGATION =
    R"apl({
      "type": "APL",
      "version": "1.4",
      "layouts": {
        "Subcontainer": {
          "parameters": [
            "containerIndex"
          ],
          "item": {
            "type": "Container",
            "width": 200,
            "height": 300,
            "items": {
              "type": "Text",
              "text": "${data}",
              "height": "50"
            },
            "data": [
              "item ${containerIndex}.1",
              "item ${containerIndex}.2",
              "item ${containerIndex}.3",
              "item ${containerIndex}.4",
              "item ${containerIndex}.5"
            ]
          }
        }
      },
      "mainTemplate": {
        "parameters": [],
        "item": {
          "type": "Sequence",
          "id": "top",
          "scrollDirection": "vertical",
          "width": 200,
          "height": 500,
          "items": [
            {
              "type": "Subcontainer",
              "containerIndex": "1"
            },
            {
              "type": "Subcontainer",
              "containerIndex": "2"
            },
            {
              "type": "Subcontainer",
              "containerIndex": "3"
            }
          ]
        }
      }
    }
)apl";

TEST_F(ComponentTransformTest, StalenessPropagation)
{
    loadDocument(STALENESS_PROPAGATION);
    advanceTime(10);

    ASSERT_EQ(Transform2D(), component->getGlobalToLocalTransform());

    ASSERT_EQ(3, component->getChildCount());
    for (auto i = 0 ; i < component->getChildCount(); i++) {
        auto subcontainer = component->getCoreChildAt(i);
        ASSERT_EQ(Transform2D::translateY(-300 * i), subcontainer->getGlobalToLocalTransform());

        ASSERT_EQ(5, subcontainer->getChildCount());
        for (auto j = 0; j < subcontainer->getChildCount(); j++) {
            auto text = subcontainer->getCoreChildAt(j);
            ASSERT_EQ(Transform2D::translateY(-300 * i - 50 * j), text->getGlobalToLocalTransform());
        }
    }

    component->update(kUpdateScrollPosition, 400);

    ASSERT_EQ(Transform2D(), component->getGlobalToLocalTransform());

    ASSERT_EQ(3, component->getChildCount());
    for (auto i = 0 ; i < component->getChildCount(); i++) {
        auto subcontainer = component->getCoreChildAt(i);
        ASSERT_EQ(Transform2D::translateY(-300 * i + 400), subcontainer->getGlobalToLocalTransform());

        ASSERT_EQ(5, subcontainer->getChildCount());
        for (auto j = 0; j < subcontainer->getChildCount(); j++) {
            auto text = subcontainer->getCoreChildAt(j);
            ASSERT_EQ(Transform2D::translateY(-300 * i + 400 - 50 * j), text->getGlobalToLocalTransform());
        }
    }

    ASSERT_TRUE(TransformComponent(root, "top", "translateX", 100));

    ASSERT_EQ(Transform2D::translateX(-100), component->getGlobalToLocalTransform());

    ASSERT_EQ(3, component->getChildCount());
    for (auto i = 0 ; i < component->getChildCount(); i++) {
        auto subcontainer = component->getCoreChildAt(i);
        ASSERT_EQ(Transform2D::translate(-100, -300 * i + 400),
            subcontainer->getGlobalToLocalTransform());

        ASSERT_EQ(5, subcontainer->getChildCount());
        for (auto j = 0; j < subcontainer->getChildCount(); j++) {
            auto text = subcontainer->getCoreChildAt(j);
            ASSERT_EQ(Transform2D::translate(-100, -300 * i + 400 - 50 * j),
                text->getGlobalToLocalTransform());
        }
    }
}