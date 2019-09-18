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

#include "testeventloop.h"
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

    // Verify that the graphic was created and that the color is blue
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
    ASSERT_TRUE(CheckDirty(vg, kPropertyGraphic));
    ASSERT_TRUE(CheckDirty(root, vg));

    // Change CompanyRed.  This will disconnect from HappyRed
    executeCommand("SetValue", {{"componentId", "myTriangle"},
                                {"property",    "CompanyRed"},
                                {"value",       "#dd0000ff"}}, true);

    ASSERT_TRUE(IsEqual(Color(0xdd0000ff), path->getValue(kGraphicPropertyFill)));
    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyFill));
    ASSERT_TRUE(CheckDirty(graphic, path));
    ASSERT_TRUE(CheckDirty(vg, kPropertyGraphic));
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

