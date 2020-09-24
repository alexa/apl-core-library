/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

    Transform2D transform;
    ASSERT_TRUE(component->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D(), transform);

    ASSERT_TRUE(touchWrapper->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D::translate(-40, -50), transform);

    ASSERT_TRUE(frame->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D::translate(-40, -50), transform);
    ASSERT_TRUE(frame->getCoordinateTransformFromParent(touchWrapper, transform));
    ASSERT_EQ(Transform2D(), transform);
}

TEST_F(ComponentTransformTest, InvalidAncestor) {
    loadDocument(CHILD_IN_PARENT);

    auto touchWrapper = as<CoreComponent>(component->findComponentById("TouchWrapper"));
    auto frame = as<CoreComponent>(component->findComponentById("Frame"));

    auto transform = Transform2D::translate(1, 1);

    // frame is not an ancestor of touch wrapper
    ASSERT_FALSE(touchWrapper->getCoordinateTransformFromParent(frame, transform));
    // check that the output is not modified when false is returned
    ASSERT_EQ(Transform2D::translate(1, 1), transform);
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

    Transform2D transform;
    ASSERT_TRUE(touchWrapper->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D({2,0, 0, 2, -130, -150}), transform);

    ASSERT_TRUE(frame->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D({2,0, 0, 2, -155, -150}), transform);
    ASSERT_TRUE(frame->getCoordinateTransformFromParent(touchWrapper, transform));
    ASSERT_EQ(Transform2D::translateX(-25), transform);
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

    Transform2D transform;

    ASSERT_TRUE(component->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D(), transform);

    ASSERT_TRUE(container->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D(), transform);

    for (auto i = 0 ; i < container->getChildCount() ; i++) {
        auto child = as<CoreComponent>(container->getChildAt(i));
        ASSERT_TRUE(child->getCoordinateTransformFromGlobal(transform));
        ASSERT_EQ(Transform2D::translateY(-200 * i), transform);
        ASSERT_TRUE(child->getCoordinateTransformFromParent(container, transform));
        ASSERT_EQ(Transform2D::translateY(-200 * i), transform);
    }

    component->update(kUpdateScrollPosition, 300);

    ASSERT_TRUE(component->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D(), transform);

    ASSERT_TRUE(container->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D::translateY(300), transform);

    ASSERT_TRUE(container->getCoordinateTransformFromParent(component, transform));
    ASSERT_EQ(Transform2D::translateY(300), transform);

    for (auto i = 0 ; i < container->getChildCount() ; i++) {
        auto child = as<CoreComponent>(container->getChildAt(i));
        ASSERT_TRUE(child->getCoordinateTransformFromGlobal(transform));
        ASSERT_EQ(Transform2D::translateY(-200 * i + 300), transform);
        ASSERT_TRUE(child->getCoordinateTransformFromParent(container, transform));
        ASSERT_EQ(Transform2D::translateY(-200 * i), transform);
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

    Transform2D transform;

    ASSERT_TRUE(component->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D(), transform);

    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = as<CoreComponent>(component->getChildAt(i));
        ASSERT_TRUE(child->getCoordinateTransformFromGlobal(transform));
        ASSERT_EQ(Transform2D::translateY(-200 * i), transform);

        ASSERT_TRUE(child->getCoordinateTransformFromParent(component, transform));
        ASSERT_EQ(Transform2D::translateY(-200 * i), transform);
    }

    component->update(kUpdateScrollPosition, 300);

    ASSERT_TRUE(component->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D(), transform);

    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = as<CoreComponent>(component->getChildAt(i));
        ASSERT_TRUE(child->getCoordinateTransformFromGlobal(transform));
        ASSERT_EQ(Transform2D::translateY(-200 * i + 300), transform);

        ASSERT_TRUE(child->getCoordinateTransformFromParent(component, transform));
        ASSERT_EQ(Transform2D::translateY(-200 * i + 300), transform);
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

    Transform2D transform;

    ASSERT_TRUE(component->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D(), transform);

    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = as<CoreComponent>(component->getChildAt(i));
        ASSERT_TRUE(child->getCoordinateTransformFromGlobal(transform));
        ASSERT_EQ(Transform2D::translateX(-200 * i), transform);

        ASSERT_TRUE(child->getCoordinateTransformFromParent(component, transform));
        ASSERT_EQ(Transform2D::translateX(-200 * i), transform);
    }

    component->update(kUpdateScrollPosition, 300);

    ASSERT_TRUE(component->getCoordinateTransformFromGlobal(transform));
    ASSERT_EQ(Transform2D(), transform);

    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = as<CoreComponent>(component->getChildAt(i));
        ASSERT_TRUE(child->getCoordinateTransformFromGlobal(transform));
        ASSERT_EQ(Transform2D::translateX(-200 * i + 300), transform);

        ASSERT_TRUE(child->getCoordinateTransformFromParent(component, transform));
        ASSERT_EQ(Transform2D::translateX(-200 * i + 300), transform);
    }
}