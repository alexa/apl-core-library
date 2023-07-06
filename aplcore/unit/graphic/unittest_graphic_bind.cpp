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

    auto graphic = component->getCalculated(kPropertyGraphic).get<Graphic>();
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

    auto graphic = component->getCalculated(kPropertyGraphic).get<Graphic>();
    auto box = graphic->getRoot();
    ASSERT_EQ(kGraphicElementTypeContainer, box->getType());

    auto group = box->getChildAt(0);
    ASSERT_TRUE(IsEqual(Transform2D(), group->getValue(kGraphicPropertyTransform)));

    advanceTime(500);
    ASSERT_TRUE(CheckDirty(group, kGraphicPropertyTransform));
    ASSERT_TRUE(CheckDirty(graphic, group));
    ASSERT_TRUE(CheckDirty(component, kPropertyGraphic, kPropertyVisualHash));
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

    auto graphic = component->getCalculated(kPropertyGraphic).get<Graphic>();
    auto box = graphic->getRoot();
    ASSERT_EQ(kGraphicElementTypeContainer, box->getType());

    ASSERT_EQ(10, box->getChildCount());
    for (size_t i = 0 ; i < box->getChildCount() ; i++) {
        auto row = box->getChildAt(i);
        ASSERT_EQ(i+1, row->getChildCount());
        // Rows alternate blue and red
        auto first = row->getChildAt(0);
        ASSERT_TRUE(IsEqual(i % 2 == 0 ? Color::BLUE : Color::RED,
                            first->getValue(kGraphicPropertyFill).getColor()));
    }
}


static const char *BIND_NAMING = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "graphics": {
    "BOX": {
      "type": "AVG",
      "version": "1.2",
      "width": 100,
      "height": 100,
      "items": {
        "type": "text",
        "bind": { "name": "NAME", "value": "VALUE" },
        "text": "${NAME}"
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "VectorGraphic",
      "source": "BOX"
    }
  }
}
)apl";

static std::map<std::string, std::string> GOOD_NAME_TESTS = {
    {"_foo", "A"},
    {"__bar__", "B"},
    {"_234", "C"},
    {"a", "D"},
    {"a99_____", "E"},
    {"_", "F"},
};

TEST_F(GraphicBindTest, GoodNameCheck)
{
    for (const auto& it : GOOD_NAME_TESTS) {
        auto doc = std::regex_replace(BIND_NAMING, std::regex("NAME"), it.first);
        doc = std::regex_replace(doc, std::regex("VALUE"), it.second);
        loadDocument(doc.c_str());
        ASSERT_TRUE(component);

        auto graphic = component->getCalculated(apl::kPropertyGraphic).get<Graphic>();
        auto container = graphic->getRoot();
        ASSERT_EQ(kGraphicElementTypeContainer, container->getType());
        ASSERT_EQ(1, container->getChildCount());
        auto text = container->getChildAt(0);
        ASSERT_TRUE(IsEqual(it.second, text->getValue(apl::kGraphicPropertyText).asString()));
    }
}

static std::map<std::string, Object> BAD_NAME_TESTS = {
    { "234_foo", "${234_foo}" },
    { "åbc", "${åbc}" },
    { "abç", "${abç}" },
    { "a-b", "nan" },
    { "0", "0" },
    { "", "" },
};

TEST_F(GraphicBindTest, BadNameCheck)
{
    for (const auto& it : BAD_NAME_TESTS) {
        auto doc = std::regex_replace(BIND_NAMING, std::regex("NAME"), it.first);
        loadDocument(doc.c_str());
        auto graphic = component->getCalculated(apl::kPropertyGraphic).get<Graphic>();
        auto container = graphic->getRoot();
        ASSERT_EQ(kGraphicElementTypeContainer, container->getType());
        ASSERT_EQ(1, container->getChildCount());
        auto text = container->getChildAt(0);
        ASSERT_TRUE(IsEqual(it.second, text->getValue(apl::kGraphicPropertyText)));
        ASSERT_TRUE(ConsoleMessage());
    }
}

static const char *MISSING_VALUE = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "graphics": {
    "BOX": {
      "type": "AVG",
      "version": "1.2",
      "width": 100,
      "height": 100,
      "items": {
        "type": "text",
        "bind": { "name": "NAME" },
        "text": "${NAME}"
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "VectorGraphic",
      "source": "BOX"
    }
  }
}
)apl";

TEST_F(GraphicBindTest, MissingValue)
{
    loadDocument(MISSING_VALUE);
    auto graphic = component->getCalculated(apl::kPropertyGraphic).get<Graphic>();
    auto container = graphic->getRoot();
    ASSERT_EQ(kGraphicElementTypeContainer, container->getType());
    ASSERT_EQ(1, container->getChildCount());
    auto text = container->getChildAt(0);
    ASSERT_TRUE(IsEqual("", text->getValue(apl::kGraphicPropertyText)));
    ASSERT_TRUE(ConsoleMessage());
}