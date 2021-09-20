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

class DependantGraphicTest : public DocumentWrapper {};

static const char * SIMPLE_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"parameters\": ["
    "        \"BoxColor\""
    "      ],"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M0,0 h100 v100 h-100 z\","
    "        \"fill\": \"${BoxColor}\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"id\": \"myBox\","
    "      \"source\": \"box\","
    "      \"BoxColor\": \"blue\""
    "    }"
    "  }"
    "}";


TEST_F(DependantGraphicTest, Simple)
{
    loadDocument(SIMPLE_TEST);
    ASSERT_TRUE(component);

    // Verify that the graphic was created and that the color is blue
    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    auto box = graphic->getRoot();
    ASSERT_TRUE(box);
    ASSERT_EQ(kGraphicElementTypeContainer, box->getType());

    auto path = box->getChildAt(0);
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), path->getValue(kGraphicPropertyFill)));

    // There should be a dependant connection from the internal Graphic context to the graphic element
    ASSERT_EQ(1, graphic->getContext()->countDownstream("BoxColor"));
    ASSERT_EQ(1, path->countUpstream(kGraphicPropertyFill));

    // Now call SetValue on the component
    auto action = executeCommand("SetValue", {{"property",    "BoxColor"},
                                              {"componentId", "myBox"},
                                              {"value",       "red"}}, true);
    ASSERT_TRUE(IsEqual(Color(Color::RED), path->getValue(kGraphicPropertyFill)));
}

static const char *BINDING_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"width\": 10,"
    "      \"height\": 10,"
    "      \"parameters\": ["
    "        {"
    "          \"name\": \"FillColor\","
    "          \"default\": \"green\""
    "        },"
    "        {"
    "          \"name\": \"StrokeColor\","
    "          \"default\": \"black\""
    "        }"
    "      ],"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M0,0 h10 v10 h-10 z\","
    "        \"fill\": \"${FillColor}\","
    "        \"stroke\": \"${StrokeColor}\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"id\": \"boxy\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"CompanyRed\","
    "          \"value\": \"red\""
    "        }"
    "      ],"
    "      \"source\": \"box\","
    "      \"FillColor\": \"${CompanyRed}\""
    "    }"
    "  }"
    "}";

TEST_F(DependantGraphicTest, Binding)
{
    loadDocument(BINDING_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(CheckDirty(component));

    // Verify that the graphic was created
    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);
    ASSERT_TRUE(CheckDirty(graphic));

    auto box = graphic->getRoot();
    ASSERT_TRUE(box);
    ASSERT_EQ(kGraphicElementTypeContainer, box->getType());
    ASSERT_TRUE(CheckDirty(box));

    auto path = box->getChildAt(0);
    ASSERT_TRUE(IsEqual(Color(Color::RED), path->getValue(kGraphicPropertyFill)));
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), path->getValue(kGraphicPropertyStroke)));
    ASSERT_TRUE(CheckDirty(path));

    // Set the value of CompanyRed.  The fill color should update
    executeCommand("SetValue", {{"componentId", "boxy"},
                                {"property",    "CompanyRed"},
                                {"value",       "yellow"}}, true);

    ASSERT_TRUE(IsEqual(Color(Color::YELLOW), path->getValue(kGraphicPropertyFill)));
    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyFill));
    ASSERT_TRUE(CheckDirty(graphic, path));
    ASSERT_TRUE(CheckDirty(root, component));

    // Now set the FillColor property directly.  This changes the fill color and detaches from CompanyRed
    executeCommand("SetValue", {{"componentId", "boxy"},
                                {"property",    "FillColor"},
                                {"value",       "white"}}, true);
    ASSERT_TRUE(IsEqual(Color(Color::WHITE), path->getValue(kGraphicPropertyFill)));
    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyFill));
    ASSERT_TRUE(CheckDirty(graphic, path));
    ASSERT_TRUE(CheckDirty(root, component));

    // Changing "CompanyRed" no longer affects the graphic
    executeCommand("SetValue", {{"componentId", "boxy"},
                                {"property",    "CompanyRed"},
                                {"value",       "red"}}, true);
    ASSERT_TRUE(IsEqual(Color(Color::WHITE), path->getValue(kGraphicPropertyFill)));
    ASSERT_TRUE(CheckDirty(path));
    ASSERT_TRUE(CheckDirty(graphic));
    ASSERT_TRUE(CheckDirty(root));
}

static const char *MANY_BINDINGS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"graphics\": {"
    "    \"triangle\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"parameters\": ["
    "        \"TriColor\""
    "      ],"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M50,0 L100,100 L0,100 z\","
    "        \"fill\": \"${TriColor}\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"id\": \"myContainer\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"HappyRed\","
    "          \"value\": \"blue\""
    "        }"
    "      ],"
    "      \"items\": {"
    "        \"type\": \"VectorGraphic\","
    "        \"id\": \"myTriangle\","
    "        \"source\": \"triangle\","
    "        \"bind\": ["
    "          {"
    "            \"name\": \"CompanyRed\","
    "            \"value\": \"${HappyRed}\","
    "            \"default\": \"black\""
    "          }"
    "        ],"
    "        \"TriColor\": \"${CompanyRed}\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DependantGraphicTest, ManyBindings)
{
    loadDocument(MANY_BINDINGS);
    ASSERT_TRUE(component);
    auto vg = component->getChildAt(0);

    // Verify that the graphic was created and that the color is blue
    auto graphic = vg->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);
    ASSERT_TRUE(CheckDirty(graphic));

    auto triangle = graphic->getRoot();
    ASSERT_TRUE(triangle);
    ASSERT_EQ(kGraphicElementTypeContainer, triangle->getType());
    ASSERT_TRUE(CheckDirty(triangle));

    auto path = triangle->getChildAt(0);
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), path->getValue(kGraphicPropertyFill)));
    ASSERT_TRUE(CheckDirty(path));

    // Change the HappyRed value and watch it trickle down
    executeCommand("SetValue", {{"componentId", "myContainer"},
                                {"property",    "HappyRed"},
                                {"value",       "#ee0000ff"}}, true);

    ASSERT_TRUE(IsEqual(Color(0xee0000ff), path->getValue(kGraphicPropertyFill)));
    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyFill));
    ASSERT_TRUE(CheckDirty(graphic, path));
    ASSERT_TRUE(CheckDirty(vg, kPropertyGraphic, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, vg));

    // Change CompanyRed.  This will disconnect from HappyRed
    executeCommand("SetValue", {{"componentId", "myTriangle"},
                                {"property",    "CompanyRed"},
                                {"value",       "#dd0000ff"}}, true);

    ASSERT_TRUE(IsEqual(Color(0xdd0000ff), path->getValue(kGraphicPropertyFill)));
    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyFill));
    ASSERT_TRUE(CheckDirty(graphic, path));
    ASSERT_TRUE(CheckDirty(vg, kPropertyGraphic, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, vg));

    // HappyRed is no longer attached.
    executeCommand("SetValue", {{"componentId", "myContainer"},
                                {"property",    "HappyRed"},
                                {"value",       "#330000ff"}}, true);

    ASSERT_TRUE(IsEqual(Color(0xdd0000ff), path->getValue(kGraphicPropertyFill)));
    ASSERT_TRUE(CheckDirty(path));
    ASSERT_TRUE(CheckDirty(graphic));
    ASSERT_TRUE(CheckDirty(vg));
    ASSERT_TRUE(CheckDirty(root));
}

static const char *TRANSFORMED_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "id": "gc",
      "height": 100,
      "width": 100,
      "source": "box",
      "groupTransform": "translate(-36 45.5) ",
      "fillSkew": 40
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "parameters": [
        "groupTransform",
        "fillSkew"
      ],
      "items": {
        "type": "group",
        "style": "expanded",
        "transform": "${groupTransform}",
        "items": [
          {
            "type": "path",
            "fill": "green",
            "fillTransform": "skewX(${fillSkew}) ",
            "style": "expanded",
            "stroke": "red",
            "strokeTransform": "scale(0.7 0.5) ",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          }
        ]
      }
    }
  }
})";

TEST_F(DependantGraphicTest, Transformed)
{
    loadDocument(TRANSFORMED_DOC);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();

    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());

    auto transform = group->getValue(kGraphicPropertyTransform).getTransform2D();
    ASSERT_EQ(Transform2D::translate(-36, 45.5), transform);

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    auto fill = path->getValue(kGraphicPropertyFill);
    ASSERT_EQ(Color(Color::GREEN), fill.asColor());

    auto fillTransform = path->getValue(kGraphicPropertyFillTransform).getTransform2D();
    ASSERT_EQ(Transform2D::skewX(40), fillTransform);

    ASSERT_TRUE(path->getValue(kGraphicPropertyStroke).isColor());
    auto strokeTransform = path->getValue(kGraphicPropertyStrokeTransform).getTransform2D();
    ASSERT_EQ(Transform2D::scale(0.7, 0.5), strokeTransform);

    executeCommand("SetValue", {{"componentId", "gc"},
                                {"property",    "groupTransform"},
                                {"value",       "scale(0.7 0.5)"}}, true);

    executeCommand("SetValue", {{"componentId", "gc"},
                                {"property",    "fillSkew"},
                                {"value",       7}}, true);

    transform = group->getValue(kGraphicPropertyTransform).getTransform2D();
    ASSERT_EQ(Transform2D::scale(0.7, 0.5), transform);

    fillTransform = path->getValue(kGraphicPropertyFillTransform).getTransform2D();
    ASSERT_EQ(Transform2D::skewX(7), fillTransform);
}

static const char *CHANGING_GRADIENT = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "id": "gc",
      "height": 100,
      "width": 100,
      "source": "box",
      "gradientColor": "red"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "parameters": [
        "gradientColor"
      ],
      "items": {
        "type": "group",
        "items": [
          {
            "type": "path",
            "fill": {
              "type": "linear",
              "colorRange": [ "${gradientColor}", "white" ],
              "inputRange": [0, 1]
            },
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          {
            "type": "text",
            "stroke": {
              "type": "linear",
              "colorRange": [ "${gradientColor}", "white" ],
              "inputRange": [0, 1]
            },
            "text": "Text"
          }
        ]
      }
    }
  }
})";

TEST_F(DependantGraphicTest, ChangingGradient)
{
    loadDocument(CHANGING_GRADIENT);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();

    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    auto pathGrad = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(pathGrad.isGradient());
    ASSERT_EQ(Color(Color::RED), pathGrad.getGradient().getColorRange().at(0));

    auto text = group->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());

    auto textGrad = text->getValue(kGraphicPropertyStroke);
    ASSERT_TRUE(textGrad.isGradient());
    ASSERT_EQ(Color(Color::RED), textGrad.getGradient().getColorRange().at(0));

    executeCommand("SetValue", {{"componentId", "gc"},
                                {"property",    "gradientColor"},
                                {"value",       "green"}}, true);

    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyFill));
    ASSERT_TRUE(CheckDirty(text, kGraphicPropertyStroke));

    pathGrad = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(pathGrad.isGradient());
    ASSERT_EQ(Object(Color(Color::GREEN)), pathGrad.getGradient().getProperty(kGradientPropertyColorRange).at(0));

    textGrad = text->getValue(kGraphicPropertyStroke);
    ASSERT_TRUE(textGrad.isGradient());
    ASSERT_EQ(Object(Color(Color::GREEN)), textGrad.getGradient().getProperty(kGradientPropertyColorRange).at(0));
}

static const char *STROKE_VARIATION_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "graphics": {
        "box": {
          "type": "AVG",
          "version": "1.0",
          "width": 100,
          "height": 100,
          "parameters": [
            {
              "name": "a",
              "default": 50
            },
            {
              "name": "b",
              "default": 25
            }
          ],
          "items": {
            "type": "path",
            "pathData": "M0,0 L100,100",
            "stroke": "black",
            "strokeDashArray": [
              "${a}",
              100
            ],
            "strokeDashOffset": "${b}"
          }
        }
      },
      "mainTemplate": {
        "item": {
          "type": "VectorGraphic",
          "id": "vg",
          "source": "box",
          "a": 10,
          "b": 20
        }
      }
    }
)apl";

// Test that the stroke dash array can be dynamically updated
TEST_F(DependantGraphicTest, StrokeVariation) {
    loadDocument(STROKE_VARIATION_TEST);
    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    auto path = container->getChildAt(0);
    ASSERT_TRUE(IsEqual(20, path->getValue(kGraphicPropertyStrokeDashOffset)));
    ASSERT_TRUE(IsEqual(std::vector<Object>{10,100}, path->getValue(kGraphicPropertyStrokeDashArray)));

    // Update the stroke dash offset and verify it is dirty/changed
    executeCommand("SetValue", {{"componentId", "vg"},
                                {"property",    "b"},
                                {"value",       33}}, true);

    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyStrokeDashOffset));
    ASSERT_TRUE(CheckDirty(graphic, path));
    ASSERT_TRUE(CheckDirty(root, component));

    ASSERT_TRUE(IsEqual(33, path->getValue(kGraphicPropertyStrokeDashOffset)));

    // Update the stroke dash array and verify it is dirty/changed
    executeCommand("SetValue", {{"componentId", "vg"},
                                {"property",    "a"},
                                {"value",       33}}, true);

    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyStrokeDashArray));
    ASSERT_TRUE(CheckDirty(graphic, path));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(IsEqual(std::vector<Object>{33,100}, path->getValue(kGraphicPropertyStrokeDashArray)));
}


static const char * PARAMETER_TEST =
    R"apl(
    {
      "graphics": {
        "box": {
          "height": 100,
          "items": {
            "text": "a=${a} b=${b}",
            "type": "text"
          },
          "parameters": [
            {
              "default": 50,
              "name": "a"
            },
            {
              "default": "${a*2}",
              "name": "b"
            }
          ],
          "type": "AVG",
          "version": "1.0",
          "width": 100
        }
      },
      "mainTemplate": {
        "items": {
          "source": "box",
          "type": "VectorGraphic"
        }
      },
      "type": "APL",
      "version": "1.4"
    }
)apl";

// Test that the default value in a parameter list can depend on a prior parameter
TEST_F(DependantGraphicTest, Parameter)
{
    loadDocument(PARAMETER_TEST);
    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    auto text = container->getChildAt(0);
    ASSERT_TRUE(IsEqual("a=50 b=100", text->getValue(kGraphicPropertyText)));
}
