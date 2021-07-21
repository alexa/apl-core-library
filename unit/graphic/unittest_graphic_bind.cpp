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
#include "apl/graphic/graphicdependant.h"

using namespace apl;

class GraphicBindTest : public DocumentWrapper {};

static const char *BIND_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "graphics": {
        "Boxy": {
          "type": "AVG",
          "version": "1.2",
          "width": 100,
          "height": 100,
          "parameters": [
            "BoxColor"
          ],
          "items": {
            "type": "path",
            "bind": {
              "name": "InternalBoxColor",
              "value": "${BoxColor}"
            },
            "pathData": "M0,0 L100,0 100,100 0,100 z",
            "fill": "${InternalBoxColor}"
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "id": "MyBox",
          "source": "Boxy",
          "BoxColor": "blue"
        }
      }
    }
)apl";

// Bind a value to a passed-in property.  Calling SetValue should cause the bind to update.
TEST_F(GraphicBindTest, BindTest)
{
    loadDocument(BIND_TEST);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    auto box = graphic->getRoot();
    ASSERT_EQ(kGraphicElementTypeContainer, box->getType());

    auto path = box->getChildAt(0);
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), path->getValue(kGraphicPropertyFill)));

    executeCommand("SetValue", {{"property", "BoxColor"},
                                {"componentId", "MyBox"},
                                {"value", "red"}}, true);

    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyFill));
    ASSERT_TRUE(CheckDirty(graphic, path));
    ASSERT_TRUE(IsEqual(Color(Color::RED), path->getValue(kGraphicPropertyFill)));
}


static const char *BIND_TO_TIME_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "graphics": {
        "Bouncy": {
          "type": "AVG",
          "version": "1.2",
          "width": 200,
          "height": 100,
          "items": {
            "type": "group",
            "bind": {
              "name": "X",
              "value": "${utcTime % 1000 / 10}"
            },
            "items": {
              "type": "path",
              "pathData": "M0,50 l50,-50 50,50 -50,50 Z",
              "fill": "blue"
            },
            "transform": "translate(${X})"
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "id": "MyBouncy",
          "source": "Bouncy"
        }
      }
    }
)apl";

// Bind a variable to an external property (like time) and verify that it updates correctly inside
TEST_F(GraphicBindTest, BindToTime)
{
    loadDocument(BIND_TO_TIME_TEST);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    auto box = graphic->getRoot();
    ASSERT_EQ(kGraphicElementTypeContainer, box->getType());

    auto group = box->getChildAt(0);
    ASSERT_TRUE(IsEqual(Transform2D(), group->getValue(kGraphicPropertyTransform)));

    advanceTime(500);
    ASSERT_TRUE(CheckDirty(group, kGraphicPropertyTransform));
    ASSERT_TRUE(CheckDirty(graphic, group));
    ASSERT_TRUE(CheckDirty(component, kPropertyGraphic));
    ASSERT_TRUE(CheckDirty(root, component));

    ASSERT_TRUE(IsEqual(Transform2D::translateX(50), group->getValue(kGraphicPropertyTransform)));
}


static const char *NESTED = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "graphics": {
        "Pyramid": {
          "type": "AVG",
          "version": "1.2",
          "width": 200,
          "height": 100,
          "data": "${Array.range(10)}",
          "items": {
            "type": "group",
            "bind": {
              "name": "COUNT",
              "value": "${data + 1}"
            },
            "transform": "translate(${100-COUNT*10},${data*10})",
            "data": "${Array.range(COUNT)}",
            "item": {
              "type": "path",
              "pathData": "M${data * 20} 0 l10,0 0,10 -10,0 z",
              "fill": "${COUNT % 2 ? 'blue' : 'red'}"
            }
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "source": "Pyramid"
        }
      }
    }
)apl";

// Verify that nested use of "data" inflation works when you bind to the outer data value
TEST_F(GraphicBindTest, Nested)
{
    loadDocument(NESTED);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    auto box = graphic->getRoot();
    ASSERT_EQ(kGraphicElementTypeContainer, box->getType());

    ASSERT_EQ(10, box->getChildCount());
    for (size_t i = 0 ; i < box->getChildCount() ; i++) {
        auto row = box->getChildAt(i);
        ASSERT_EQ(i+1, row->getChildCount());
        // Rows alternate blue and red
        auto first = row->getChildAt(0);
        ASSERT_TRUE(IsEqual(Color(i % 2 == 0 ? Color::BLUE : Color::RED),
                            first->getValue(kGraphicPropertyFill).asColor()));
    }
}
