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

using namespace apl;

class GraphicTest : public MemoryWrapper {
public:
    GraphicTest() : MemoryWrapper()
    {
        propertyValues = std::make_shared<ObjectMap>();
        metrics = Metrics().size(1024, 800);
    }

    void addToProperties(const char *str, Object value) {
        (*propertyValues)[str] = value;
    }

    void loadGraphic(const char *str, const StyleInstancePtr& style = nullptr) {
        GraphicContentPtr gc = GraphicContent::create(session, str);
        ASSERT_TRUE(gc);

        auto context = Context::create(metrics, session);
        Properties properties;
        properties.emplace(propertyValues);
        graphic = Graphic::create(context, gc, std::move(properties), style);
        ASSERT_TRUE(graphic);
    }

    void loadGraphic(const rapidjson::Value& json, const StyleInstancePtr& style = nullptr) {
        auto context = Context::create(metrics, session);
        Properties properties;
        properties.emplace(propertyValues);
        graphic = Graphic::create(context, json, std::move(properties), style);
        ASSERT_TRUE(graphic);
    }

    void TearDown() override {
        graphic = nullptr;
    }

    Metrics metrics;
    GraphicPtr graphic;
    SharedMapPtr propertyValues;
};

static const char * HEART =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"description\": \"Partially filled heart with rotation\","
    "  \"height\": 157,"
    "  \"width\": 171,"
    "  \"viewportHeight\": 157,"
    "  \"viewportWidth\": 171,"
    "  \"parameters\": ["
    "    {"
    "      \"default\": \"green\","
    "      \"type\": \"color\","
    "      \"name\": \"fillColor\""
    "    },"
    "    {"
    "      \"default\": 15.0,"
    "      \"type\": \"number\","
    "      \"name\": \"rotation\""
    "    }"
    "  ],"
    "  \"items\": ["
    "    {"
    "      \"pivotX\": 85.5,"
    "      \"pivotY\": 78.5,"
    "      \"type\": \"group\","
    "      \"rotation\": \"${rotation}\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"path\","
    "          \"pathData\": \"M85.7106781,155.714249 L85.3571247,156.067803 L86.0642315,156.067803 L85.7106781,155.714249 Z M155.714249,85.7106781 L156.067803,86.0642315 L156.421356,85.7106781 L156.067803,85.3571247 L155.714249,85.7106781 Z\","
    "          \"fillOpacity\": 0.3,"
    "          \"fill\": \"${fillColor}\""
    "        },"
    "        {"
    "          \"type\": \"path\","
    "          \"pathData\": \"M169.384239,39.5 L169.786098,39.5 L169.298242,39.1095251 C169.327433,39.2395514 169.356099,39.3697105 169.384239,39.5 Z M155.714249,85.7106781 L156.067803,86.0642315 L156.421356,85.7106781 L156.067803,85.3571247 L155.714249,85.7106781 Z M85.7106781,155.714249 L85.3571247,156.067803 L86.0642315,156.067803 L85.7106781,155.714249 Z M1.61576082,39.5 C1.64390105,39.3697105 1.67256715,39.2395514 1.70175839,39.1095251 L1.21390159,39.5 L1.61576071,39.5 Z\","
    "          \"fill\": \"${fillColor}\""
    "        }"
    "      ]"
    "    }"
    "  ]"
    "}";

TEST_F(GraphicTest, Basic)
{
    loadGraphic(HEART);
    auto container = graphic->getRoot();

    ASSERT_EQ(Object(Dimension(157)), container->getValue(kGraphicPropertyHeightOriginal));
    ASSERT_EQ(Object(Dimension(171)), container->getValue(kGraphicPropertyWidthOriginal));
    ASSERT_EQ(Object(157), container->getValue(kGraphicPropertyViewportHeightOriginal));
    ASSERT_EQ(Object(171), container->getValue(kGraphicPropertyViewportWidthOriginal));
    ASSERT_EQ(Object(kGraphicScaleNone), container->getValue(kGraphicPropertyScaleTypeHeight));
    ASSERT_EQ(Object(kGraphicScaleNone), container->getValue(kGraphicPropertyScaleTypeWidth));

    ASSERT_EQ(1, container->getChildCount());
    auto child = container->getChildAt(0);

    ASSERT_EQ(kGraphicElementTypeGroup, child->getType());
    ASSERT_EQ(Object(1), child->getValue(kGraphicPropertyOpacity));
    ASSERT_EQ(Object(15), child->getValue(kGraphicPropertyRotation));
    ASSERT_EQ(Object(85.5), child->getValue(kGraphicPropertyPivotX));
    ASSERT_EQ(Object(78.5), child->getValue(kGraphicPropertyPivotY));
    ASSERT_EQ(Object(1), child->getValue(kGraphicPropertyScaleX));
    ASSERT_EQ(Object(1), child->getValue(kGraphicPropertyScaleY));
    ASSERT_EQ(Object(0), child->getValue(kGraphicPropertyTranslateX));
    ASSERT_EQ(Object(0), child->getValue(kGraphicPropertyTranslateY));

    ASSERT_EQ(2, child->getChildCount());

    auto path = child->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());
    ASSERT_TRUE(path->getValue(kGraphicPropertyPathData).size() > 30);
    ASSERT_EQ(Object(0.3), path->getValue(kGraphicPropertyFillOpacity));
    ASSERT_EQ(Object(Color(Color::GREEN)), path->getValue(kGraphicPropertyFill));

    path = child->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());
    ASSERT_TRUE(path->getValue(kGraphicPropertyPathData).size() > 30);
    ASSERT_EQ(Object(1.0), path->getValue(kGraphicPropertyFillOpacity));
    ASSERT_EQ(Object(Color(Color::GREEN)), path->getValue(kGraphicPropertyFill));
}

// Verify default properties get set correctly

static const char *MINIMAL =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 200"
    "}";

TEST_F(GraphicTest, Minimal)
{
    loadGraphic(MINIMAL);
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(kGraphicElementTypeContainer, container->getType());

    ASSERT_EQ(Object(Dimension(100)), container->getValue(kGraphicPropertyHeightOriginal));
    ASSERT_EQ(Object(Dimension(200)), container->getValue(kGraphicPropertyWidthOriginal));
    ASSERT_EQ(kGraphicScaleNone, container->getValue(kGraphicPropertyScaleTypeHeight).asInt());
    ASSERT_EQ(kGraphicScaleNone, container->getValue(kGraphicPropertyScaleTypeWidth).asInt());
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightOriginal));
    ASSERT_EQ(Object(200), container->getValue(kGraphicPropertyViewportWidthOriginal));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(Object(200), container->getValue(kGraphicPropertyViewportWidthActual));

    ASSERT_EQ(0, container->getChildCount());
}

static const char *MINIMAL_VIEWPORT =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 200,"
    "  \"viewportHeight\": 300,"
    "  \"viewportWidth\": 400,"
    "  \"scaleTypeHeight\": \"stretch\","
    "  \"scaleTypeWidth\": \"grow\""
    "}";

TEST_F(GraphicTest, MinimalViewport)
{
    loadGraphic(MINIMAL_VIEWPORT);
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(Object(Dimension(100)), container->getValue(kGraphicPropertyHeightOriginal));
    ASSERT_EQ(Object(Dimension(200)), container->getValue(kGraphicPropertyWidthOriginal));
    ASSERT_EQ(kGraphicScaleStretch, container->getValue(kGraphicPropertyScaleTypeHeight).asInt());
    ASSERT_EQ(kGraphicScaleGrow, container->getValue(kGraphicPropertyScaleTypeWidth).asInt());
    ASSERT_EQ(Object(300), container->getValue(kGraphicPropertyViewportHeightOriginal));
    ASSERT_EQ(Object(400), container->getValue(kGraphicPropertyViewportWidthOriginal));
    ASSERT_EQ(Object(300), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(Object(400), container->getValue(kGraphicPropertyViewportWidthActual));

    ASSERT_EQ(0, container->getChildCount());
}

static const char *MINIMAL_GROUP =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 200,"
    "  \"item\": {"
    "    \"type\": \"group\""
    "  }"
    "}";

TEST_F(GraphicTest, MinimalGroup)
{
    loadGraphic(MINIMAL_GROUP);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto group = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());

    ASSERT_EQ(Object(1.0), group->getValue(kGraphicPropertyOpacity));
    ASSERT_EQ(Object(0), group->getValue(kGraphicPropertyRotation));
    ASSERT_EQ(Object(0), group->getValue(kGraphicPropertyPivotX));
    ASSERT_EQ(Object(0), group->getValue(kGraphicPropertyPivotY));
    ASSERT_EQ(Object(1.0), group->getValue(kGraphicPropertyScaleX));
    ASSERT_EQ(Object(1.0), group->getValue(kGraphicPropertyScaleY));
    ASSERT_EQ(Object(0), group->getValue(kGraphicPropertyTranslateX));
    ASSERT_EQ(Object(0), group->getValue(kGraphicPropertyTranslateY));
    ASSERT_EQ(0, group->getChildCount());
}

static const char *GROUP_PROPERTIES =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 200,"
    "  \"item\": {"
    "    \"type\": \"group\","
    "    \"opacity\": 0.5,"
    "    \"rotation\": 23,"
    "    \"pivotX\": 50,"
    "    \"pivotY\": 60,"
    "    \"scaleX\": 0.5,"
    "    \"scaleY\": 2.0,"
    "    \"translateX\": 100,"
    "    \"translateY\": -50"
    "  }"
    "}";

TEST_F(GraphicTest, GroupProperties)
{
    loadGraphic(GROUP_PROPERTIES);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto group = container->getChildAt(0);

    ASSERT_EQ(Object(0.5), group->getValue(kGraphicPropertyOpacity));
    ASSERT_EQ(Object(23), group->getValue(kGraphicPropertyRotation));
    ASSERT_EQ(Object(50), group->getValue(kGraphicPropertyPivotX));
    ASSERT_EQ(Object(60), group->getValue(kGraphicPropertyPivotY));
    ASSERT_EQ(Object(0.5), group->getValue(kGraphicPropertyScaleX));
    ASSERT_EQ(Object(2.0), group->getValue(kGraphicPropertyScaleY));
    ASSERT_EQ(Object(100), group->getValue(kGraphicPropertyTranslateX));
    ASSERT_EQ(Object(-50), group->getValue(kGraphicPropertyTranslateY));
    ASSERT_EQ(0, group->getChildCount());
}

static const char *MINIMAL_PATH =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 200,"
    "  \"item\": {"
    "    \"type\": \"path\","
    "    \"pathData\": \"M0,0\""
    "  }"
    "}";

TEST_F(GraphicTest, MinimalPath)
{
    loadGraphic(MINIMAL_PATH);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto path = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    ASSERT_EQ(Object(Color()), path->getValue(kGraphicPropertyFill));
    ASSERT_EQ(Object(1), path->getValue(kGraphicPropertyFillOpacity));
    ASSERT_EQ(Object("M0,0"), path->getValue(kGraphicPropertyPathData));
    ASSERT_EQ(Object(Color()), path->getValue(kGraphicPropertyStroke));
    ASSERT_EQ(Object(1), path->getValue(kGraphicPropertyStrokeOpacity));
    ASSERT_EQ(Object(1), path->getValue(kGraphicPropertyStrokeWidth));

    ASSERT_EQ(0, path->getChildCount());
}

static const char *PATH_PROPERTIES =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 200,"
    "  \"item\": {"
    "    \"type\": \"path\","
    "    \"pathData\": \"M0,0\","
    "    \"fill\": \"red\","
    "    \"fillOpacity\": 0.5,"
    "    \"stroke\": \"green\","
    "    \"strokeWidth\": 4,"
    "    \"strokeOpacity\": 0.25"
    "  }"
    "}";

TEST_F(GraphicTest, PathProperties)
{
    loadGraphic(PATH_PROPERTIES);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto path = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    ASSERT_EQ(Object(Color(Color::RED)), path->getValue(kGraphicPropertyFill));
    ASSERT_EQ(Object(0.5), path->getValue(kGraphicPropertyFillOpacity));
    ASSERT_EQ(Object("M0,0"), path->getValue(kGraphicPropertyPathData));
    ASSERT_EQ(Object(Color(Color::GREEN)), path->getValue(kGraphicPropertyStroke));
    ASSERT_EQ(Object(0.25), path->getValue(kGraphicPropertyStrokeOpacity));
    ASSERT_EQ(Object(4.0), path->getValue(kGraphicPropertyStrokeWidth));

    ASSERT_EQ(0, path->getChildCount());
}

// Unit test verifying that we fail if required properties aren't provided

static std::vector<std::string> BAD_CONTENT = {
    "{\"version\": \"1.0\", \"height\": 100, \"width\": 200}",     // Missing type
    "{\"type\": \"AVG\", \"height\": 100, \"width\": 200}",        // Missing version
    "{\"type\": \"AVG\", \"version\": \"1.0\", \"width\": 200}",   // Missing height
    "{\"type\": \"AVG\", \"version\": \"1.0\", \"height\": 100 }", // Missing width
    "{\"type\": \"AVS\", \"version\": \"1.0\", \"height\": 100, \"width\": 200}",   // Bad type
    "{\"type\": \"AVG\", \"version\": \"0.8\", \"height\": 100, \"width\": 200}",   // Bad version
};

TEST_F(GraphicTest, BadContent)
{
    for (auto& s : BAD_CONTENT) {
        auto gc = GraphicContent::create(session, s);
        ASSERT_FALSE(gc);
        ASSERT_TRUE(ConsoleMessage());
        ASSERT_FALSE(LogMessage());
    }
}


TEST_F(GraphicTest, BadContentNoSession)
{
    for (auto& s : BAD_CONTENT) {
        auto gc = GraphicContent::create(s);
        ASSERT_FALSE(gc);
        ASSERT_FALSE(ConsoleMessage());
        ASSERT_TRUE(LogMessage());
    }
}

static std::vector<std::string> BAD_CONTAINER_PROPERTIES = {
    "{\"type\": \"AVG\", \"version\": \"1.0\", \"height\": 0, \"width\": 200}",     // Zero height
    "{\"type\": \"AVG\", \"version\": \"1.0\", \"height\": 100, \"width\": 0}",     // Zero width
    "{\"type\": \"AVG\", \"version\": \"1.0\", \"height\": -20, \"width\": 200}",   // Negative height
    "{\"type\": \"AVG\", \"version\": \"1.0\", \"height\": 100, \"width\": -33}",   // Negative width
};

TEST_F(GraphicTest, BadContainerProperty)
{
    for (auto& s : BAD_CONTAINER_PROPERTIES) {
        loadGraphic(s.c_str());
        auto container = graphic->getRoot();
        ASSERT_FALSE(container);
        ASSERT_TRUE(ConsoleMessage());
    }
}

static std::vector<std::string> BAD_CHILD_PROPERTIES = {
    "{\"type\":\"AVG\",\"version\":\"1.0\",\"height\":100,\"width\":200,\"item\":{\"fill\":\"white\"}}",  // No type
    "{\"type\":\"AVG\",\"version\":\"1.0\",\"height\":100,\"width\":200,\"item\":{\"type\":\"\"}}",       // No name
    "{\"type\":\"AVG\",\"version\":\"1.0\",\"height\":100,\"width\":200,\"item\":{\"type\":\"math\"}}",   // Misspelled
    "{\"type\":\"AVG\",\"version\":\"1.0\",\"height\":100,\"width\":200,\"item\":{\"type\":\"path\"}}",   // No pathData
};

TEST_F(GraphicTest, BadChildProperties)
{
    for (auto& s : BAD_CHILD_PROPERTIES) {
        loadGraphic(s.c_str());
        auto container = graphic->getRoot();
        ASSERT_TRUE(container);
        ASSERT_EQ(0, container->getChildCount());
        ASSERT_TRUE(ConsoleMessage());
    }
}

// Unit test verifying scaling modes

static const char * SCALE_NONE =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 100"
    "}";

TEST_F(GraphicTest, ScaleTypeNone)
{
    loadGraphic(SCALE_NONE);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    graphic->layout(200,300,false);
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportWidthActual));
    ASSERT_EQ(0, graphic->getDirty().size());
}

static const char *SCALE_GROW_SHRINK =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 100,"
    "  \"scaleTypeHeight\": \"grow\","
    "  \"scaleTypeWidth\": \"shrink\""
    "}";

TEST_F(GraphicTest, ScaleTypeGrowShrink)
{
    loadGraphic(SCALE_GROW_SHRINK);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    graphic->layout(50,75,false);
    ASSERT_EQ(Object(50), container->getValue(kGraphicPropertyViewportWidthActual));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(0, graphic->getDirty().size());

    graphic->layout(200,300,false);
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportWidthActual));
    ASSERT_EQ(Object(300), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(0, graphic->getDirty().size());
}


static const char *SCALE_GROW_SHRINK_2 =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 100,"
    "  \"scaleTypeHeight\": \"shrink\","
    "  \"scaleTypeWidth\": \"grow\""
    "}";

TEST_F(GraphicTest, ScaleTypeGrowShrink2)
{
    loadGraphic(SCALE_GROW_SHRINK_2);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    graphic->layout(50,75,false);
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportWidthActual));
    ASSERT_EQ(Object(75), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(0, graphic->getDirty().size());

    graphic->layout(200,300,false);
    ASSERT_EQ(Object(200), container->getValue(kGraphicPropertyViewportWidthActual));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(0, graphic->getDirty().size());
}

static const char *SCALE_STRETCH =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 100,"
    "  \"scaleTypeHeight\": \"stretch\","
    "  \"scaleTypeWidth\": \"stretch\""
    "}";

TEST_F(GraphicTest, ScaleTypeGrowStretch)
{
    loadGraphic(SCALE_STRETCH);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    graphic->layout(50,75,false);
    ASSERT_EQ(Object(50), container->getValue(kGraphicPropertyViewportWidthActual));
    ASSERT_EQ(Object(75), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(0, graphic->getDirty().size());

    graphic->layout(200,300,false);
    ASSERT_EQ(Object(200), container->getValue(kGraphicPropertyViewportWidthActual));
    ASSERT_EQ(Object(300), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(0, graphic->getDirty().size());
}

// Pass arguments into parameters

static const char *PARAMETER_TEST =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 100,"
    "  \"parameters\": ["
    "    {"
    "      \"name\": \"myColor\","
    "      \"type\": \"color\","
    "      \"default\": \"red\""
    "    }"
    "  ],"
    "  \"items\": {"
    "    \"type\": \"path\","
    "    \"pathData\": \"M0,0 h100 v100 h-100 z\","
    "    \"fill\": \"${myColor}\""
    "  }"
    "}";

TEST_F(GraphicTest, DefaultParameters)
{
    loadGraphic(PARAMETER_TEST);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(1, container->getChildCount());

    auto path = container->getChildAt(0);
    ASSERT_EQ(Object(Color(Color::RED)), path->getValue(kGraphicPropertyFill));
}

TEST_F(GraphicTest, AssignedParameters)
{
    addToProperties("myColor", "blue");  // This isn't right - we should pass this as a Property!
    loadGraphic(PARAMETER_TEST);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(1, container->getChildCount());

    auto path = container->getChildAt(0);
    ASSERT_EQ(Object(Color(Color::BLUE)), path->getValue(kGraphicPropertyFill));
}

static const char *STYLED_DOC =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\""
    "    }"
    "  },"
    "  \"resources\": [],"
    "  \"styles\": {"
    "    \"base\": {"
    "      \"values\": ["
    "        {"
    "          \"myColor\": \"olive\","
    "          \"width\": 400"
    "        },"
    "        {"
    "          \"myColor\": \"blue\","
    "          \"when\": \"${state.disabled}\""
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"parameters\": ["
    "        {"
    "          \"name\": \"myColor\","
    "          \"type\": \"color\","
    "          \"default\": \"red\""
    "        }"
    "      ],"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M0,0 h100 v100 h-100 z\","
    "        \"fill\": \"${myColor}\""
    "      }"
    "    }"
    "  }"
    "}";

// Test styled parameters.  This example starts with no style.

TEST_F(GraphicTest, StyledParameters)
{
    auto content = Content::create(STYLED_DOC, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(box.json());
    auto path = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(Object(Color(Color::RED)), path->getValue(kGraphicPropertyFill));
    ASSERT_EQ(0, graphic->getDirty().size());

    auto style = root->context().getStyle("base", State());
    ASSERT_TRUE(style);

    graphic->updateStyle(style);
    ASSERT_EQ(1, graphic->getDirty().size());
    ASSERT_EQ(1, graphic->getDirty().count(path));
    ASSERT_EQ(1, path->getDirtyProperties().size());
    ASSERT_EQ(1, path->getDirtyProperties().count(kGraphicPropertyFill));
    ASSERT_EQ(Object(Color(Color::OLIVE)), path->getValue(kGraphicPropertyFill));

    path->clearDirtyProperties();
    graphic->clearDirty();
    ASSERT_EQ(0, path->getDirtyProperties().size());
    ASSERT_EQ(0, graphic->getDirty().size());

    graphic->updateStyle( root->context().getStyle("base", State().emplace(kStateDisabled)) );
    ASSERT_EQ(1, graphic->getDirty().size());
    ASSERT_EQ(1, graphic->getDirty().count(path));
    ASSERT_EQ(1, path->getDirtyProperties().size());
    ASSERT_EQ(1, path->getDirtyProperties().count(kGraphicPropertyFill));
    ASSERT_EQ(Object(Color(Color::BLUE)), path->getValue(kGraphicPropertyFill));
}

// This test STARTS the graphic with a style and then toggles it

TEST_F(GraphicTest, StyledParameters2)
{
    auto content = Content::create(STYLED_DOC, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box");
    ASSERT_FALSE(box.empty());

    loadGraphic(box.json(), root->context().getStyle("base", State()));
    auto path = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(Object(Color(Color::OLIVE)), path->getValue(kGraphicPropertyFill));
    ASSERT_EQ(0, graphic->getDirty().size());

    // Toggle the disabled state
    graphic->updateStyle( root->context().getStyle("base", State().emplace(kStateDisabled)) );
    ASSERT_EQ(1, graphic->getDirty().size());
    ASSERT_EQ(1, graphic->getDirty().count(path));
    ASSERT_EQ(1, path->getDirtyProperties().size());
    ASSERT_EQ(1, path->getDirtyProperties().count(kGraphicPropertyFill));
    ASSERT_EQ(Object(Color(Color::BLUE)), path->getValue(kGraphicPropertyFill));

    // Clear dirty
    path->clearDirtyProperties();
    graphic->clearDirty();
    ASSERT_EQ(0, path->getDirtyProperties().size());
    ASSERT_EQ(0, graphic->getDirty().size());

    // Untoggle the disabled state
    graphic->updateStyle( root->context().getStyle("base", State()) );
    ASSERT_EQ(1, graphic->getDirty().size());
    ASSERT_EQ(1, graphic->getDirty().count(path));
    ASSERT_EQ(1, path->getDirtyProperties().size());
    ASSERT_EQ(1, path->getDirtyProperties().count(kGraphicPropertyFill));
    ASSERT_EQ(Object(Color(Color::OLIVE)), path->getValue(kGraphicPropertyFill));
}


static const char *TIME_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"graphics\": {"
    "    \"clock\": {"
    "      \"description\": \"Live analog clock\","
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"item\": {"
    "        \"type\": \"group\","
    "        \"rotation\": \"${Time.seconds(localTime)*6}\","
    "        \"pivotX\": 50,"
    "        \"pivotY\": 50,"
    "        \"items\": {"
    "          \"type\": \"path\","
    "          \"pathData\": \"M50,0 l0,50\","
    "          \"stroke\": \"red\""
    "        }"
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"source\": \"clock\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"scale\": \"best-fit\","
    "      \"align\": \"center\""
    "    }"
    "  }"
    "}";

/**
 * A popular use of a vector graphic is to create a clock.  This clock example uses
 * the "localTime" global property to move the second hand directly.
 */
TEST_F(GraphicTest, Time)
{
    auto content = Content::create(TIME_TEST, session);
    ASSERT_TRUE(content);

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->topComponent();
    ASSERT_TRUE(box);

    auto graphic = box->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_EQ(100, graphic->getViewportWidth());
    ASSERT_EQ(100, graphic->getViewportHeight());

    auto container = graphic->getRoot();
    ASSERT_EQ(kGraphicElementTypeContainer, container->getType());

    auto group = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());
    ASSERT_EQ(0, group->getValue(kGraphicPropertyRotation).asNumber());

    // Now advance local time by 3 seconds
    root->updateTime(3000);
    ASSERT_EQ(18, group->getValue(kGraphicPropertyRotation).asNumber());
    ASSERT_TRUE(CheckDirty(group, kGraphicPropertyRotation));
    ASSERT_TRUE(CheckDirty(graphic, group));
    ASSERT_TRUE(CheckDirty(box, kPropertyGraphic));
    ASSERT_TRUE(CheckDirty(root, box));
}


static const char *PARAMETERIZED_TIME =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"graphics\": {"
    "    \"clock\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"parameters\": ["
    "        \"time\""
    "      ],"
    "      \"item\": {"
    "        \"type\": \"group\","
    "        \"rotation\": \"${Time.seconds(time)*6}\","
    "        \"pivotX\": 50,"
    "        \"pivotY\": 50,"
    "        \"items\": {"
    "          \"type\": \"path\","
    "          \"pathData\": \"M50,0 l0,50\","
    "          \"stroke\": \"red\""
    "        }"
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"source\": \"clock\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"scale\": \"best-fit\","
    "      \"align\": \"center\","
    "      \"time\": \"${localTime + 30000}\""
    "    }"
    "  }"
    "}";

/**
 * This clock test passes the time as a parameter in from the mainTemplate
 */
TEST_F(GraphicTest, ParameterizedTime)
{
    auto content = Content::create(PARAMETERIZED_TIME, session);
    ASSERT_TRUE(content);

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->topComponent();
    ASSERT_TRUE(box);

    auto graphic = box->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_EQ(100, graphic->getViewportWidth());
    ASSERT_EQ(100, graphic->getViewportHeight());

    auto container = graphic->getRoot();
    ASSERT_EQ(kGraphicElementTypeContainer, container->getType());

    auto group = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());
    ASSERT_EQ(180, group->getValue(kGraphicPropertyRotation).asNumber());

    // Now advance local time by 3 seconds
    root->updateTime(3000);
    ASSERT_EQ(198, group->getValue(kGraphicPropertyRotation).asNumber());
    ASSERT_TRUE(CheckDirty(group, kGraphicPropertyRotation));
    ASSERT_TRUE(CheckDirty(graphic, group));
    ASSERT_TRUE(CheckDirty(box, kPropertyGraphic));
    ASSERT_TRUE(CheckDirty(root, box));
}

static const char *FULL_CLOCK =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.2\","
    "  \"graphics\": {"
    "    \"clock\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"parameters\": ["
    "        \"time\""
    "      ],"
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"items\": ["
    "        {"
    "          \"type\": \"group\","
    "          \"description\": \"MinuteHand\","
    "          \"rotation\": \"${Time.minutes(time) * 6}\","
    "          \"pivotX\": 50,"
    "          \"pivotY\": 50,"
    "          \"items\": {"
    "            \"type\": \"path\","
    "            \"pathData\": \"M48.5,7 L51.5,7 L51.5,50 L48.5,50 L48.5,7 Z\","
    "            \"fill\": \"orange\""
    "          }"
    "        },"
    "        {"
    "          \"type\": \"group\","
    "          \"description\": \"HourHand\","
    "          \"rotation\": \"${Time.hours(time) * 30}\","
    "          \"pivotX\": 50,"
    "          \"pivotY\": 50,"
    "          \"items\": {"
    "            \"type\": \"path\","
    "            \"pathData\": \"M48.5,17 L51.5,17 L51.5,50 L48.5,50 L48.5,17 Z\","
    "            \"fill\": \"black\""
    "          }"
    "        },"
    "        {"
    "          \"type\": \"group\","
    "          \"description\": \"SecondHand\","
    "          \"rotation\": \"${Time.seconds(time) * 6}\","
    "          \"pivotX\": 50,"
    "          \"pivotY\": 50,"
    "          \"items\": {"
    "            \"type\": \"path\","
    "            \"pathData\": \"M49.5,15 L50.5,15 L50.5,60 L49.5,60 L49.5,15 Z\","
    "            \"fill\": \"red\""
    "          }"
    "        },"
    "        {"
    "          \"type\": \"path\","
    "          \"description\": \"Cap\","
    "          \"pathData\": \"M50,53 C51.656854,53 53,51.6568542 53,50 C53,48.3431458 51.656854,47 50,47 C48.343146,47 47,48.3431458 47,50 C47,51.6568542 48.343146,53 50,53 Z\","
    "          \"fill\": \"#d8d8d8ff\","
    "          \"stroke\": \"#e6e6e6ff\","
    "          \"strokeWidth\": 1"
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"source\": \"clock\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"scale\": \"best-fit\","
    "      \"align\": \"center\","
    "      \"time\": \"${localTime + 1000 * (payload.seconds + 60 * payload.minutes + 3600 * payload.hours)}\""
    "    }"
    "  }"
    "}";

/**
 * Sanity check a clock with a second, minute, and hour hand.  We pass in a payload that specifies the
 * exact hours, minutes, and seconds we wish to set
 */
TEST_F(GraphicTest, FullClock)
{
    auto content = Content::create(FULL_CLOCK, session);
    ASSERT_TRUE(content);

    content->addData("payload", R"({"hours": 1, "minutes": 20, "seconds": 30})");
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->topComponent();
    ASSERT_TRUE(box);

    auto graphic = box->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_EQ(100, graphic->getViewportWidth());
    ASSERT_EQ(100, graphic->getViewportHeight());

    auto container = graphic->getRoot();
    ASSERT_EQ(kGraphicElementTypeContainer, container->getType());
    ASSERT_EQ(4, container->getChildCount());

    // The first child should be the minute hand
    auto minuteHand = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, minuteHand->getType());
    ASSERT_EQ(120, minuteHand->getValue(kGraphicPropertyRotation).asNumber());  // 20 minutes = 120 degrees rotation

    // The second child is the hour hand
    auto hourHand = container->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeGroup, hourHand->getType());
    ASSERT_EQ(30, hourHand->getValue(kGraphicPropertyRotation).asNumber());  // 1 o'clock = 30 degrees rotation

    // The third child is the second hand
    auto secondHand = container->getChildAt(2);
    ASSERT_EQ(kGraphicElementTypeGroup, secondHand->getType());
    ASSERT_EQ(180, secondHand->getValue(kGraphicPropertyRotation).asNumber());  // 30 seconds = 180 degrees rotation

    // Now advance local time by one hour, one minute, and one second
    root->updateTime((3600 + 60 + 1) * 1000);
    ASSERT_EQ(126, minuteHand->getValue(kGraphicPropertyRotation).asNumber());  // 21 minutes = 126 degrees rotation
    ASSERT_EQ(60, hourHand->getValue(kGraphicPropertyRotation).asNumber());  // 2 o'clock = 60 degrees rotation
    ASSERT_EQ(186, secondHand->getValue(kGraphicPropertyRotation).asNumber());  // 31 seconds = 186 degrees rotation

    ASSERT_TRUE(CheckDirty(minuteHand, kGraphicPropertyRotation));
    ASSERT_TRUE(CheckDirty(hourHand, kGraphicPropertyRotation));
    ASSERT_TRUE(CheckDirty(secondHand, kGraphicPropertyRotation));
    ASSERT_TRUE(CheckDirty(graphic, minuteHand, hourHand, secondHand));
    ASSERT_TRUE(CheckDirty(box, kPropertyGraphic));
    ASSERT_TRUE(CheckDirty(root, box));
}


/**
 * Viewhost-like clock impl with a second, minute, and hour hand. This test avoids the use of CheckDirty
 * utilities and calls isDirty() and clearDirty() in a manner like the viewhost.
 * In a loop the test specifies the exact hours, minutes, and seconds we wish to set, verifies and
 * clears the dirty state.
 */
TEST_F(GraphicTest, ClearDirty)
{
    auto content = Content::create(FULL_CLOCK, session);
    ASSERT_TRUE(content);

    content->addData("payload", R"({"hours": 1, "minutes": 20, "seconds": 30})");
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->topComponent();
    ASSERT_TRUE(box);
    ASSERT_EQ(0,box->getChildCount());

    auto graphic = box->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);
    ASSERT_TRUE(graphic->isValid());

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(4, container->getChildCount());

    // The first child should be the minute hand
    auto minuteHand = container->getChildAt(0);
    ASSERT_TRUE(minuteHand);

    // The second child is the hour hand
    auto hourHand = container->getChildAt(1);
    ASSERT_TRUE(hourHand);

    // The third child is the second hand
    auto secondHand = container->getChildAt(2);
    ASSERT_TRUE(secondHand);

    // The third child is the second hand
    auto cap = container->getChildAt(3);
    ASSERT_TRUE(cap);

    // Now advance local time by one hour, one minute, and one second
    for (int i=1; i<10; i++) {
        root->updateTime((3600 + 60 + 1) * 1000*i);

        LOG_IF(true) << "LOOP:"<<i;

        // verify root is dirty
        ASSERT_TRUE(root->isDirty());
        ASSERT_TRUE(root->getDirty().size() > 0);

        // verify component is dirty
        ASSERT_TRUE(box->getDirty().count(kPropertyGraphic) > 0);
        ASSERT_EQ(3, graphic->getDirty().size());

        // verify elements are dirty
        ASSERT_TRUE(hourHand->getDirtyProperties().count(kGraphicPropertyRotation) > 0);
        ASSERT_TRUE(minuteHand->getDirtyProperties().count(kGraphicPropertyRotation) > 0);
        ASSERT_TRUE(secondHand->getDirtyProperties().count(kGraphicPropertyRotation) > 0);
        ASSERT_TRUE(cap->getDirtyProperties().count(kGraphicPropertyRotation) == 0);

        // clear dirty state at root context and verify everything is clean
        root->clearDirty();

        ASSERT_TRUE(root->getDirty().size() == 0);

        // verify component is clean
        ASSERT_TRUE(box->getDirty().count(kPropertyGraphic) == 0);
        ASSERT_EQ(0, graphic->getDirty().size());

        // verify elements are clean
        ASSERT_TRUE(hourHand->getDirtyProperties().count(kGraphicPropertyRotation) == 0);
        ASSERT_TRUE(minuteHand->getDirtyProperties().count(kGraphicPropertyRotation) == 0);
        ASSERT_TRUE(secondHand->getDirtyProperties().count(kGraphicPropertyRotation) == 0);
        ASSERT_TRUE(cap->getDirtyProperties().count(kGraphicPropertyRotation) == 0);

    }
}
