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

class GraphicTest : public DocumentWrapper {
public:
    GraphicTest() : DocumentWrapper()
    {
        propertyValues = std::make_shared<ObjectMap>();
    }

    void addToProperties(const char *str, Object value) {
        (*propertyValues)[str] = value;
    }

    void loadGraphic(const char *str, const StyleInstancePtr& style = nullptr) {
        GraphicContentPtr gc = GraphicContent::create(session, str);
        ASSERT_TRUE(gc);
        auto jr = JsonResource(&gc->get(), Path());
        auto context = Context::createTestContext(metrics, *config);
        Properties properties;
        properties.emplace(propertyValues);
        graphic = Graphic::create(context, jr, std::move(properties), nullptr);
        ASSERT_TRUE(graphic);
    }

    void loadGraphic(const rapidjson::Value& json, const StyleInstancePtr& style = nullptr) {
        auto context = Context::createTestContext(metrics, session);
        loadGraphic(context, json, style);
    }

    void loadGraphic(const ContextPtr& context, const rapidjson::Value& json, const StyleInstancePtr& style = nullptr) {
        Properties properties;
        properties.emplace(propertyValues);
        graphic = Graphic::create(context, json, std::move(properties), nullptr, Path(), style);
        ASSERT_TRUE(graphic);
    }

    void TearDown() override {
        graphic = nullptr;
        DocumentWrapper::TearDown();
    }

    GraphicPtr graphic;
    SharedMapPtr propertyValues;
};

static const char * HEART =
    R"({
      "type": "AVG",
      "version": "1.0",
      "lang": "en-US",
      "layoutDirection": "RTL",
      "description": "Partially filled heart with rotation",
      "height": 157,
      "width": 171,
      "viewportHeight": 157,
      "viewportWidth": 171,
      "parameters": [
        {
          "default": "green",
          "type": "color",
          "name": "fillColor"
        },
        {
          "default": 15.0,
          "type": "number",
          "name": "rotation"
        }
      ],
      "items": [
        {
          "pivotX": 85.5,
          "pivotY": 78.5,
          "type": "group",
          "rotation": "${rotation}",
          "items": [
            {
              "type": "path",
              "pathData": "M85.7106781,155.714249 L85.3571247,156.067803 L86.0642315,156.067803 L85.7106781,155.714249 Z M155.714249,85.7106781 L156.067803,86.0642315 L156.421356,85.7106781 L156.067803,85.3571247 L155.714249,85.7106781 Z",
              "fillOpacity": 0.3,
              "fill": "${fillColor}"
            },
            {
              "type": "path",
              "pathData": "M169.384239,39.5 L169.786098,39.5 L169.298242,39.1095251 C169.327433,39.2395514 169.356099,39.3697105 169.384239,39.5 Z M155.714249,85.7106781 L156.067803,86.0642315 L156.421356,85.7106781 L156.067803,85.3571247 L155.714249,85.7106781 Z M85.7106781,155.714249 L85.3571247,156.067803 L86.0642315,156.067803 L85.7106781,155.714249 Z M1.61576082,39.5 C1.64390105,39.3697105 1.67256715,39.2395514 1.70175839,39.1095251 L1.21390159,39.5 L1.61576071,39.5 Z",
              "fill": "${fillColor}"
            }
          ]
        }
      ]
    })";

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
    ASSERT_EQ(Object("en-US"), container->getValue(kGraphicPropertyLang));
    ASSERT_EQ(kGraphicLayoutDirectionRTL, container->getValue(kGraphicPropertyLayoutDirection).asInt());

    ASSERT_EQ(1, container->getChildCount());
    auto child = container->getChildAt(0);

    ASSERT_EQ(kGraphicElementTypeGroup, child->getType());
    auto filterArray = child->getValue(kGraphicPropertyFilters);
    ASSERT_EQ(rapidjson::kArrayType, filterArray.getType());
    ASSERT_EQ(Object::EMPTY_ARRAY(), filterArray);
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
    filterArray = path->getValue(kGraphicPropertyFilters);
    ASSERT_EQ(rapidjson::kArrayType, filterArray.getType());
    ASSERT_EQ(Object::EMPTY_ARRAY(), filterArray);
    ASSERT_TRUE(path->getValue(kGraphicPropertyPathData).size() > 30);
    ASSERT_EQ(Object(0.3), path->getValue(kGraphicPropertyFillOpacity));
    ASSERT_EQ(Object(Color(Color::GREEN)), path->getValue(kGraphicPropertyFill));

    path = child->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());
    filterArray = path->getValue(kGraphicPropertyFilters);
    ASSERT_EQ(rapidjson::kArrayType, filterArray.getType());
    ASSERT_EQ(Object::EMPTY_ARRAY(), filterArray);
    ASSERT_TRUE(path->getValue(kGraphicPropertyPathData).size() > 30);
    ASSERT_EQ(Object(1.0), path->getValue(kGraphicPropertyFillOpacity));
    ASSERT_EQ(Object(Color(Color::GREEN)), path->getValue(kGraphicPropertyFill));
}

// Verify default properties get set correctly

static const char *MINIMAL =
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 200
    })";

TEST_F(GraphicTest, Minimal)
{
    loadGraphic(MINIMAL);
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(kGraphicElementTypeContainer, container->getType());

    ASSERT_EQ(Object(Dimension(100)), container->getValue(kGraphicPropertyHeightOriginal));
    ASSERT_EQ(Object(Dimension(200)), container->getValue(kGraphicPropertyWidthOriginal));
    ASSERT_EQ(kGraphicScaleNone, container->getValue(kGraphicPropertyScaleTypeHeight).getInteger());
    ASSERT_EQ(kGraphicScaleNone, container->getValue(kGraphicPropertyScaleTypeWidth).getInteger());
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightOriginal));
    ASSERT_EQ(Object(200), container->getValue(kGraphicPropertyViewportWidthOriginal));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(Object(200), container->getValue(kGraphicPropertyViewportWidthActual));

    ASSERT_EQ(0, container->getChildCount());
}

static const char *MINIMAL_11 = R"({
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 200
    })";

TEST_F(GraphicTest, Minimal11)
{
    loadGraphic(MINIMAL_11);
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(kGraphicElementTypeContainer, container->getType());

    ASSERT_EQ(Object(Dimension(100)), container->getValue(kGraphicPropertyHeightOriginal));
    ASSERT_EQ(Object(Dimension(200)), container->getValue(kGraphicPropertyWidthOriginal));
    ASSERT_EQ(kGraphicScaleNone, container->getValue(kGraphicPropertyScaleTypeHeight).getInteger());
    ASSERT_EQ(kGraphicScaleNone, container->getValue(kGraphicPropertyScaleTypeWidth).getInteger());
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightOriginal));
    ASSERT_EQ(Object(200), container->getValue(kGraphicPropertyViewportWidthOriginal));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(Object(200), container->getValue(kGraphicPropertyViewportWidthActual));

    ASSERT_EQ(0, container->getChildCount());
}

static const char *MINIMAL_BAD_VERSION = R"({
      "type": "AVG",
      "version": "0.9",
      "height": 100,
      "width": 200
    })";

TEST_F(GraphicTest, MinimalBadVersion)
{
    GraphicContentPtr gc = GraphicContent::create(session, MINIMAL_BAD_VERSION);
    ASSERT_TRUE(ConsoleMessage());
    ASSERT_FALSE(gc);
}

static const char *MINIMAL_VIEWPORT =
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 200,
      "viewportHeight": 300,
      "viewportWidth": 400,
      "scaleTypeHeight": "stretch",
      "scaleTypeWidth": "grow"
    })";

TEST_F(GraphicTest, MinimalViewport)
{
    loadGraphic(MINIMAL_VIEWPORT);
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(Object(Dimension(100)), container->getValue(kGraphicPropertyHeightOriginal));
    ASSERT_EQ(Object(Dimension(200)), container->getValue(kGraphicPropertyWidthOriginal));
    ASSERT_EQ(kGraphicScaleStretch, container->getValue(kGraphicPropertyScaleTypeHeight).getInteger());
    ASSERT_EQ(kGraphicScaleGrow, container->getValue(kGraphicPropertyScaleTypeWidth).getInteger());
    ASSERT_EQ(Object(300), container->getValue(kGraphicPropertyViewportHeightOriginal));
    ASSERT_EQ(Object(400), container->getValue(kGraphicPropertyViewportWidthOriginal));
    ASSERT_EQ(Object(300), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(Object(400), container->getValue(kGraphicPropertyViewportWidthActual));

    ASSERT_EQ(0, container->getChildCount());
}

static const char *MINIMAL_RESOURCES =
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 200,
      "resources": [
        {
          "strings": {
            "test": "A"
          }
        }
      ]
    })";

TEST_F(GraphicTest, MinimalResources)
{
    loadGraphic(MINIMAL_RESOURCES);
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(kGraphicElementTypeContainer, container->getType());

    ASSERT_EQ("A", graphic->getContext()->opt("@test").asString());
}

static const char *MINIMAL_DOCUMENT =
    R"({
      "type": "APL",
      "lang": "en-US",
      "layoutDirection": "RTL",
      "version": "1.1",
      "graphics": {
        "box": {
          "type": "AVG",
          "version": "1.0",
          "height": 100,
          "width": 100,
          "parameters": [
            "BoxColor"
          ],
          "resources": [
            {
              "strings": {
                "test": "A"
              }
            }
          ],
          "items": {
            "type": "path",
            "pathData": "M0,0 h100 v100 h-100 z",
            "fill": "${BoxColor}"
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "id": "myBox",
          "source": "box",
          "BoxColor": "blue"
        }
      }
    })";

TEST_F(GraphicTest, MinimalProvenance)
{
    loadDocument(MINIMAL_DOCUMENT);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    ASSERT_EQ("A", graphic->getContext()->opt("@test").asString());

    ASSERT_STREQ("_main/graphics/box/resources/0/strings/test", graphic->getContext()->provenance("@test").c_str());
    ASSERT_EQ(Object::NULL_OBJECT(), context->opt("@test"));

    // Make sure we don't shadow the document lang
    ASSERT_EQ(Object(""), graphic->getRoot()->getValue(kGraphicPropertyLang));
    ASSERT_EQ(kGraphicLayoutDirectionLTR, graphic->getRoot()->getValue(kGraphicPropertyLayoutDirection).asInt());
}

static const char *GRAPHIC_RESOURCES =
    R"({
      "type": "APL",
      "version": "1.1",
      "resources": [
        {
          "strings": {
            "firstName": "john",
            "lastName": "smith",
            "duplicated": "base"
          },
          "colors": {
            "myColor": "red"
          }
        }
      ],
      "graphics": {
        "box": {
          "type": "AVG",
          "version": "1.0",
          "height": 100,
          "width": 100,
          "parameters": [
            "TextColor"
          ],
          "resources": [
            {
              "strings": {
                "name": "${@firstName + @lastName}",
                "duplicated": "overridden"
              },
              "colors": {
                 "myColor": "blue"
              }
            },
            {
              "when": "${viewport.width < 200}",
              "strings": {
                "name": "@firstName"
              }
            }
          ],
          "items": {
            "type": "text",
            "text": "${@name + @duplicated}",
            "fill": "${TextColor}"
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "id": "myBox",
          "source": "box",
          "TextColor": "@myColor"
        }
      }
    })";

TEST_F(GraphicTest, GraphicResources)
{
    loadDocument(GRAPHIC_RESOURCES);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto text = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());

    ASSERT_EQ("johnsmithoverridden", text->getValue(kGraphicPropertyText).asString());

    ASSERT_EQ(0, text->getChildCount());
}

TEST_F(GraphicTest, GraphicResourceComponentContextScoping)
{
    loadDocument(GRAPHIC_RESOURCES);

    auto object = context->opt("@myColor");
    ASSERT_TRUE(object.isColor());
    ASSERT_EQ(Color(Color::RED), object.asColor());

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto text = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());
    ASSERT_EQ(Color(Color::RED), text->getValue(kGraphicPropertyFill).asColor());

}

TEST_F(GraphicTest, GraphicResourcesSmallPort)
{
    metrics.size(100,100);
    loadDocument(GRAPHIC_RESOURCES);
    ASSERT_TRUE(component);


    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto text = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());

    ASSERT_EQ("johnoverridden", text->getValue(kGraphicPropertyText).asString());

    ASSERT_EQ(0, text->getChildCount());
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
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 200,
      "item": {
        "type": "group",
        "clipPath": "M0,0",
        "opacity": 0.5,
        "rotation": 23,
        "pivotX": 50,
        "pivotY": 60,
        "scaleX": 0.5,
        "scaleY": 2.0,
        "translateX": 100,
        "translateY": -50
      }
    })";

TEST_F(GraphicTest, GroupProperties)
{
    loadGraphic(GROUP_PROPERTIES);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto group = container->getChildAt(0);

    ASSERT_EQ(Object("M0,0"), group->getValue(kGraphicPropertyClipPath));
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

static const char *MINIMAL_TEXT =
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 200,
      "item": {
        "type": "text",
        "text": "test text"
      }
    })";

TEST_F(GraphicTest, MinimalText)
{
    loadGraphic(MINIMAL_TEXT);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(Object(""), container->getValue(kGraphicPropertyLang));
    ASSERT_EQ(kGraphicLayoutDirectionLTR, container->getValue(kGraphicPropertyLang).asInt());

    ASSERT_EQ(1, container->getChildCount());
    auto text = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());

    ASSERT_EQ(Object(Color(Color::BLACK)), text->getValue(kGraphicPropertyFill));
    ASSERT_EQ(Object(1), text->getValue(kGraphicPropertyFillOpacity));
    ASSERT_EQ(Object("sans-serif"), text->getValue(kGraphicPropertyFontFamily));
    ASSERT_EQ(Object(40), text->getValue(kGraphicPropertyFontSize));
    ASSERT_EQ(kFontStyleNormal, text->getValue(kGraphicPropertyFontStyle).getInteger());
    ASSERT_EQ(Object(400), text->getValue(kGraphicPropertyFontWeight));
    ASSERT_EQ(Object(0), text->getValue(kGraphicPropertyLetterSpacing));
    ASSERT_EQ(Object(Color()), text->getValue(kGraphicPropertyStroke));
    ASSERT_EQ(Object(1), text->getValue(kGraphicPropertyStrokeOpacity));
    ASSERT_EQ(Object(0), text->getValue(kGraphicPropertyStrokeWidth));
    ASSERT_EQ(Object("test text"), text->getValue(kGraphicPropertyText));
    ASSERT_EQ(kGraphicTextAnchorStart, text->getValue(kGraphicPropertyTextAnchor).getInteger());
    ASSERT_EQ(Object(0), text->getValue(kGraphicPropertyCoordinateX));
    ASSERT_EQ(Object(0), text->getValue(kGraphicPropertyCoordinateY));

    ASSERT_EQ(0, text->getChildCount());
}

TEST_F(GraphicTest, MinimalTextDefaultFontFamily)
{
    config->set(RootProperty::kDefaultFontFamily, "potato");

    loadGraphic(MINIMAL_TEXT);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto text = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());

    ASSERT_EQ(Object("potato"), text->getValue(kGraphicPropertyFontFamily));
}

static const char *TEXT_PROPERTIES =
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 200,
      "item": {
        "type": "text",
        "text": "test text",
        "fill": "red",
        "fillOpacity": 0.5,
        "fontFamily": "monospace",
        "fontSize": 14,
        "fontStyle": "italic",
        "fontWeight": "300",
        "letterSpacing": 71,
        "stroke": "green",
        "strokeOpacity": 0.25,
        "strokeWidth": 4,
        "textAnchor": "middle",
        "x": 14.9,
        "y": 31.7
      }
    })";

TEST_F(GraphicTest, TextProperties)
{
    loadGraphic(TEXT_PROPERTIES);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto text = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());

    ASSERT_EQ(Object(Color(Color::RED)), text->getValue(kGraphicPropertyFill));
    ASSERT_EQ(Object(0.5), text->getValue(kGraphicPropertyFillOpacity));
    ASSERT_EQ(Object("monospace"), text->getValue(kGraphicPropertyFontFamily));
    ASSERT_EQ(Object(14), text->getValue(kGraphicPropertyFontSize));
    ASSERT_EQ(kFontStyleItalic, text->getValue(kGraphicPropertyFontStyle).getInteger());
    ASSERT_EQ(Object(300), text->getValue(kGraphicPropertyFontWeight));
    ASSERT_EQ(Object(71), text->getValue(kGraphicPropertyLetterSpacing));
    ASSERT_EQ(Object(Color(Color::GREEN)), text->getValue(kGraphicPropertyStroke));
    ASSERT_EQ(Object(0.25), text->getValue(kGraphicPropertyStrokeOpacity));
    ASSERT_EQ(Object(4), text->getValue(kGraphicPropertyStrokeWidth));
    ASSERT_EQ(Object("test text"), text->getValue(kGraphicPropertyText));
    ASSERT_EQ(kGraphicTextAnchorMiddle, text->getValue(kGraphicPropertyTextAnchor).getInteger());
    ASSERT_EQ(Object(14.9), text->getValue(kGraphicPropertyCoordinateX));
    ASSERT_EQ(Object(31.7), text->getValue(kGraphicPropertyCoordinateY));

    ASSERT_EQ(0, text->getChildCount());
}

static const char *MINIMAL_PATH =
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 200,
      "item": {
        "type": "path",
        "pathData": "M0,0"
      }
    })";

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
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 200,
      "item": {
        "type": "path",
        "pathData": "M0,0",
        "pathLength": 42,
        "fill": "red",
        "fillOpacity": 0.5,
        "stroke": "green",
        "strokeDashArray": [1, 2],
        "strokeDashOffset": 2,
        "strokeLineCap": "butt",
        "strokeLineJoin": "bevel",
        "strokeMiterLimit": 3,
        "strokeWidth": 4,
        "strokeOpacity": 0.25
      }
    })";

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
    ASSERT_EQ(Object(42), path->getValue(kGraphicPropertyPathLength));
    ASSERT_EQ(Object(Color(Color::GREEN)), path->getValue(kGraphicPropertyStroke));
    ASSERT_EQ(Object::kArrayType, path->getValue(kGraphicPropertyStrokeDashArray).getType());
    ASSERT_EQ(2, path->getValue(kGraphicPropertyStrokeDashArray).getArray().size());
    ASSERT_EQ(Object(1), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[0]);
    ASSERT_EQ(Object(2), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[1]);
    ASSERT_EQ(Object(2), path->getValue(kGraphicPropertyStrokeDashOffset));
    ASSERT_EQ(kGraphicLineCapButt, path->getValue(kGraphicPropertyStrokeLineCap).getInteger());
    ASSERT_EQ(kGraphicLineJoinBevel, path->getValue(kGraphicPropertyStrokeLineJoin).getInteger());
    ASSERT_EQ(Object(3), path->getValue(kGraphicPropertyStrokeMiterLimit));
    ASSERT_EQ(Object(0.25), path->getValue(kGraphicPropertyStrokeOpacity));
    ASSERT_EQ(Object(4.0), path->getValue(kGraphicPropertyStrokeWidth));

    ASSERT_EQ(0, path->getChildCount());
}

static const char *ODD_DASH_ARRAY =
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 200,
      "item": {
        "type": "path",
        "pathData": "M0,0",
        "strokeDashArray": [1, 2, 3]
      }
    })";

TEST_F(GraphicTest, OddDashArray)
{
    loadGraphic(ODD_DASH_ARRAY);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto path = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    ASSERT_EQ(6, path->getValue(kGraphicPropertyStrokeDashArray).getArray().size());
    ASSERT_EQ(Object(1), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[0]);
    ASSERT_EQ(Object(2), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[1]);
    ASSERT_EQ(Object(3), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[2]);
    ASSERT_EQ(Object(1), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[3]);
    ASSERT_EQ(Object(2), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[4]);
    ASSERT_EQ(Object(3), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[5]);
}

static const char *EVEN_DASH_ARRAY =
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 200,
      "item": {
        "type": "path",
        "pathData": "M0,0",
        "strokeDashArray": [1, 2, 3, 4]
      }
    })";

TEST_F(GraphicTest, EvenDashArray)
{
    loadGraphic(EVEN_DASH_ARRAY);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto path = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    ASSERT_EQ(4, path->getValue(kGraphicPropertyStrokeDashArray).getArray().size());
    ASSERT_EQ(Object(1), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[0]);
    ASSERT_EQ(Object(2), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[1]);
    ASSERT_EQ(Object(3), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[2]);
    ASSERT_EQ(Object(4), path->getValue(kGraphicPropertyStrokeDashArray).getArray()[3]);
}

// Unit test verifying that we fail if required properties aren't provided

static std::vector<std::string> BAD_CONTENT = {
    R"({"version": "1.0", "height": 100, "width": 200})",     // Missing type
    R"({"type": "AVG", "height": 100, "width": 200})",        // Missing version
    R"({"type": "AVG", "version": "1.0", "width": 200})",   // Missing height
    R"({"type": "AVG", "version": "1.0", "height": 100 })", // Missing width
    R"({"type": "AVS", "version": "1.0", "height": 100, "width": 200})",   // Bad type
    R"({"type": "AVG", "version": "0.8", "height": 100, "width": 200})",   // Bad version
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
    R"({"type": "AVG", "version": "1.0", "height": 0, "width": 200})",     // Zero height
    R"({"type": "AVG", "version": "1.0", "height": 100, "width": 0})",     // Zero width
    R"({"type": "AVG", "version": "1.0", "height": -20, "width": 200})",   // Negative height
    R"({"type": "AVG", "version": "1.0", "height": 100, "width": -33})",   // Negative width
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
    R"({"type":"AVG","version":"1.0","height":100,"width":200,"item":{"fill":"white"}})",  // No type
    R"({"type":"AVG","version":"1.0","height":100,"width":200,"item":{"type":""}})",       // No name
    R"({"type":"AVG","version":"1.0","height":100,"width":200,"item":{"type":"math"}})",   // Misspelled
    R"({"type":"AVG","version":"1.0","height":100,"width":200,"item":{"type":"path"}})",   // No pathData
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

static const char* PILL_DOCUMENT =
    R"(
{
    "type": "APL",
    "version": "1.0",
    "graphics": {
        "myOtherGraphic": {
            "type": "AVG",
            "version": "1.0",
            "height": 100,
            "width": 100,
            "parameters": [
                "myScaleType"
            ],
            "scaleTypeHeight": "${myScaleType}",
            "items": [
                {
                    "type": "group",
                    "items": [
                        {
                            "type": "path",
                            "pathData": "M25,50 a25,25 0 1 1 50,0 l0 ${height-100} a25,25 0 1 1 -50,0 z",
                            "stroke": "black",
                            "strokeWidth": 20
                        }
                    ]
                }
            ]
        }
    },
    "mainTemplate": {
        "item": {
            "type": "Container",
            "direction": "row",
            "items": {
                "type": "VectorGraphic",
                "source": "http://myPillShape",
                "width": 100,
                "height": 200,
                "scale": "fill",
                "myScaleType": "${data}",
                "id": "${data}"
            },
            "data": [
                "none",
                "stretch"
            ]
        }
    }
}
)";

static const char* PILL_AVG =
    R"({
        "type": "AVG",
        "version": "1.0",
        "height": 100,
        "width": 100,
        "parameters": [
            "myScaleType"
        ],
        "resources": [
            {
                "strings": {
                    "test": "A"
                }
            }
        ],
        "scaleTypeHeight": "${myScaleType}",
        "items": [
            {
                "type": "group",
                "items": [
                    {
                        "type": "path",
                        "pathData": "M25,50 a25,25 0 1 1 50,0 l0 ${height-100} a25,25 0 1 1 -50,0 z",
                        "stroke": "black",
                        "strokeWidth": 20
                    }
                ]
            }
        ]
    }
})";

TEST_F(GraphicTest, InvalidUpdateWithInvalidJson) {
    loadDocument(PILL_DOCUMENT);
    ASSERT_TRUE(component);

    auto none = component->findComponentById("none");
    ASSERT_TRUE(none);
    ASSERT_EQ(Object::NULL_OBJECT(), none->getCalculated(kPropertyGraphic));
    auto stretch = component->findComponentById("stretch");
    ASSERT_TRUE(stretch);
    ASSERT_EQ(Object::NULL_OBJECT(), stretch->getCalculated(kPropertyGraphic));

    auto json = JsonData(R"(abcd)");
    auto graphicContent = GraphicContent::create(std::move(json));
    ASSERT_EQ(nullptr, graphicContent);

    none = component->findComponentById("none");
    ASSERT_EQ(Object::NULL_OBJECT(), none->getCalculated(kPropertyGraphic));

    auto result = stretch->updateGraphic(graphicContent);
    ASSERT_FALSE(result);

    stretch = component->findComponentById("stretch");
    ASSERT_EQ(Object::NULL_OBJECT(), stretch->getCalculated(kPropertyGraphic));
}

TEST_F(GraphicTest, InvalidUpdateWithValidJson) {
    loadDocument(PILL_DOCUMENT);
    ASSERT_TRUE(component);

    auto none = component->findComponentById("none");
    ASSERT_TRUE(none);
    ASSERT_EQ(Object::NULL_OBJECT(), none->getCalculated(kPropertyGraphic));
    auto stretch = component->findComponentById("stretch");
    ASSERT_TRUE(stretch);
    ASSERT_EQ(Object::NULL_OBJECT(), stretch->getCalculated(kPropertyGraphic));

    auto json = JsonData(PILL_AVG);
    auto graphicContent = GraphicContent::create(std::move(json));
    stretch->updateGraphic(graphicContent);

    none = component->findComponentById("none");
    ASSERT_EQ(Object::NULL_OBJECT(), none->getCalculated(kPropertyGraphic));
    stretch = component->findComponentById("stretch");
    auto graphic = stretch->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    ASSERT_EQ("A", graphic->getContext()->opt("@test").asString());

    ASSERT_STREQ("_url/http%3A%2F%2FmyPillShape/resources/0/strings/test", graphic->getContext()->provenance("@test").c_str());
}


// Unit test verifying scaling modes

static const char * SCALE_NONE =
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100
    })";

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
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "scaleTypeHeight": "grow",
      "scaleTypeWidth": "shrink"
    })";

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
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "scaleTypeHeight": "shrink",
      "scaleTypeWidth": "grow"
    })";

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
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "scaleTypeHeight": "stretch",
      "scaleTypeWidth": "stretch"
    })";

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
    R"({
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "parameters": [
        {
          "name": "myColor",
          "type": "color",
          "default": "red"
        }
      ],
      "items": {
        "type": "path",
        "pathData": "M0,0 h100 v100 h-100 z",
        "fill": "${myColor}"
      }
    })";

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
    R"({
      "type": "APL",
      "version": "1.0",
      "mainTemplate": {
        "items": {
          "type": "Container"
        }
      },
      "resources": [],
      "styles": {
        "base": {
          "values": [
            {
              "myColor": "olive",
              "width": 400
            },
            {
              "myColor": "blue",
              "when": "${state.disabled}"
            }
          ]
        }
      },
      "graphics": {
        "box": {
          "type": "AVG",
          "version": "1.0",
          "height": 100,
          "width": 100,
          "parameters": [
            {
              "name": "myColor",
              "type": "color",
              "default": "red"
            }
          ],
          "items": {
            "type": "path",
            "pathData": "M0,0 h100 v100 h-100 z",
            "fill": "${myColor}"
          }
        }
      }
    })";

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
    R"({
      "type": "APL",
      "version": "1.3",
      "graphics": {
        "clock": {
          "description": "Live analog clock",
          "type": "AVG",
          "version": "1.0",
          "height": 100,
          "width": 100,
          "item": {
            "type": "group",
            "rotation": "${Time.seconds(localTime)*6}",
            "pivotX": 50,
            "pivotY": 50,
            "items": {
              "type": "path",
              "pathData": "M50,0 l0,50",
              "stroke": "red"
            }
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "source": "clock",
          "width": "100%",
          "height": "100%",
          "scale": "best-fit",
          "align": "center"
        }
      }
    })";

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
    ASSERT_EQ(0, group->getValue(kGraphicPropertyRotation).getDouble());

    // Now advance local time by 3 seconds
    root->updateTime(3000);
    ASSERT_EQ(18, group->getValue(kGraphicPropertyRotation).getDouble());
    ASSERT_TRUE(CheckDirty(group, kGraphicPropertyTransform));
    ASSERT_TRUE(CheckDirty(graphic, group));
    ASSERT_TRUE(CheckDirty(box, kPropertyGraphic));
    ASSERT_TRUE(CheckDirty(root, box));
}


static const char *PARAMETERIZED_TIME =
    R"({
      "type": "APL",
      "version": "1.3",
      "graphics": {
        "clock": {
          "type": "AVG",
          "version": "1.0",
          "height": 100,
          "width": 100,
          "parameters": [
            "time"
          ],
          "item": {
            "type": "group",
            "rotation": "${Time.seconds(time)*6}",
            "pivotX": 50,
            "pivotY": 50,
            "items": {
              "type": "path",
              "pathData": "M50,0 l0,50",
              "stroke": "red"
            }
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "source": "clock",
          "width": "100%",
          "height": "100%",
          "scale": "best-fit",
          "align": "center",
          "time": "${localTime + 30000}"
        }
      }
    })";

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
    ASSERT_EQ(180, group->getValue(kGraphicPropertyRotation).getDouble());

    // Now advance local time by 3 seconds
    root->updateTime(3000);
    ASSERT_EQ(198, group->getValue(kGraphicPropertyRotation).getDouble());
    ASSERT_TRUE(CheckDirty(group, kGraphicPropertyTransform));
    ASSERT_TRUE(CheckDirty(graphic, group));
    ASSERT_TRUE(CheckDirty(box, kPropertyGraphic));
    ASSERT_TRUE(CheckDirty(root, box));
}

static const char *FULL_CLOCK =
    R"({
      "type": "APL",
      "version": "1.2",
      "graphics": {
        "clock": {
          "type": "AVG",
          "version": "1.0",
          "parameters": [
            "time"
          ],
          "width": 100,
          "height": 100,
          "items": [
            {
              "type": "group",
              "description": "MinuteHand",
              "rotation": "${Time.minutes(time) * 6}",
              "pivotX": 50,
              "pivotY": 50,
              "items": {
                "type": "path",
                "pathData": "M48.5,7 L51.5,7 L51.5,50 L48.5,50 L48.5,7 Z",
                "fill": "orange"
              }
            },
            {
              "type": "group",
              "description": "HourHand",
              "rotation": "${Time.hours(time) * 30}",
              "pivotX": 50,
              "pivotY": 50,
              "items": {
                "type": "path",
                "pathData": "M48.5,17 L51.5,17 L51.5,50 L48.5,50 L48.5,17 Z",
                "fill": "black"
              }
            },
            {
              "type": "group",
              "description": "SecondHand",
              "rotation": "${Time.seconds(time) * 6}",
              "pivotX": 50,
              "pivotY": 50,
              "items": {
                "type": "path",
                "pathData": "M49.5,15 L50.5,15 L50.5,60 L49.5,60 L49.5,15 Z",
                "fill": "red"
              }
            },
            {
              "type": "path",
              "description": "Cap",
              "pathData": "M50,53 C51.656854,53 53,51.6568542 53,50 C53,48.3431458 51.656854,47 50,47 C48.343146,47 47,48.3431458 47,50 C47,51.6568542 48.343146,53 50,53 Z",
              "fill": "#d8d8d8ff",
              "stroke": "#e6e6e6ff",
              "strokeWidth": 1
            }
          ]
        }
      },
      "mainTemplate": {
        "parameters": [
          "payload"
        ],
        "items": {
          "type": "VectorGraphic",
          "source": "clock",
          "width": "100%",
          "height": "100%",
          "scale": "best-fit",
          "align": "center",
          "time": "${localTime + 1000 * (payload.seconds + 60 * payload.minutes + 3600 * payload.hours)}"
        }
      }
    })";

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
    ASSERT_EQ(120, minuteHand->getValue(kGraphicPropertyRotation).getDouble());  // 20 minutes = 120 degrees rotation

    // The second child is the hour hand
    auto hourHand = container->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeGroup, hourHand->getType());
    ASSERT_EQ(30, hourHand->getValue(kGraphicPropertyRotation).getDouble());  // 1 o'clock = 30 degrees rotation

    // The third child is the second hand
    auto secondHand = container->getChildAt(2);
    ASSERT_EQ(kGraphicElementTypeGroup, secondHand->getType());
    ASSERT_EQ(180, secondHand->getValue(kGraphicPropertyRotation).getDouble());  // 30 seconds = 180 degrees rotation

    // Now advance local time by one hour, one minute, and one second
    root->updateTime((3600 + 60 + 1) * 1000);
    ASSERT_EQ(126, minuteHand->getValue(kGraphicPropertyRotation).getDouble());  // 21 minutes = 126 degrees rotation
    ASSERT_EQ(60, hourHand->getValue(kGraphicPropertyRotation).getDouble());  // 2 o'clock = 60 degrees rotation
    ASSERT_EQ(186, secondHand->getValue(kGraphicPropertyRotation).getDouble());  // 31 seconds = 186 degrees rotation

    ASSERT_TRUE(CheckDirty(minuteHand, kGraphicPropertyTransform));
    ASSERT_TRUE(CheckDirty(hourHand, kGraphicPropertyTransform));
    ASSERT_TRUE(CheckDirty(secondHand, kGraphicPropertyTransform));
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

        // verify root is dirty
        ASSERT_TRUE(root->isDirty());
        ASSERT_TRUE(root->getDirty().size() > 0);

        // verify component is dirty
        ASSERT_TRUE(box->getDirty().count(kPropertyGraphic) > 0);
        ASSERT_EQ(3, graphic->getDirty().size());

        // verify elements are dirty
        ASSERT_TRUE(hourHand->getDirtyProperties().count(kGraphicPropertyTransform) > 0);
        ASSERT_TRUE(minuteHand->getDirtyProperties().count(kGraphicPropertyTransform) > 0);
        ASSERT_TRUE(secondHand->getDirtyProperties().count(kGraphicPropertyTransform) > 0);
        ASSERT_TRUE(cap->getDirtyProperties().count(kGraphicPropertyTransform) == 0);

        // clear dirty state at root context and verify everything is clean
        root->clearDirty();

        ASSERT_TRUE(root->getDirty().size() == 0);

        // verify component is clean
        ASSERT_TRUE(box->getDirty().count(kPropertyGraphic) == 0);
        ASSERT_EQ(0, graphic->getDirty().size());

        // verify elements are clean
        ASSERT_TRUE(hourHand->getDirtyProperties().count(kGraphicPropertyTransform) == 0);
        ASSERT_TRUE(minuteHand->getDirtyProperties().count(kGraphicPropertyTransform) == 0);
        ASSERT_TRUE(secondHand->getDirtyProperties().count(kGraphicPropertyTransform) == 0);
        ASSERT_TRUE(cap->getDirtyProperties().count(kGraphicPropertyTransform) == 0);

    }
}

static const char *LOCAL_STYLING_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "styles": {
        "base": {
          "values": [
            {
              "fill": "red",
              "opacity": 0.7
            }
          ]
        }
      },
      "items": {
        "type": "group",
        "style": "base",
        "items": [
          {
            "type": "path",
            "style": "base",
            "stroke": "blue",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          {
            "type": "text",
            "style": "base",
            "fontFamily": "amazon-ember, sans-serif",
            "fontSize": 40,
            "text": "Diamond",
            "x": 25,
            "y": 25,
            "textAnchor": "middle"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicTest, LocalStyling)
{
    auto content = Content::create(LOCAL_STYLING_DOC, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());
    ASSERT_EQ(0.7, group->getValue(kGraphicPropertyOpacity).getDouble());

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());
    ASSERT_EQ(Object(Color(Color::RED)), path->getValue(kGraphicPropertyFill));

    auto text = group->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());
    ASSERT_EQ(Object(Color(Color::RED)), text->getValue(kGraphicPropertyFill));
}

static const char *LOCAL_EXPANDED_STYLING_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "styles": {
        "base": {
          "values": [
            {
              "opacity": 0.7
            }
          ]
        },
        "expanded": {
          "extends": "base",
          "values": [
            {
              "fill": "red"
            }
          ]
        }
      },
      "items": {
        "type": "group",
        "style": "expanded",
        "items": [
          {
            "type": "path",
            "style": "expanded",
            "stroke": "blue",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          {
            "type": "text",
            "style": "expanded",
            "fontFamily": "amazon-ember, sans-serif",
            "fontSize": 40,
            "text": "Diamond",
            "x": 25,
            "y": 25,
            "textAnchor": "middle"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicTest, LocalExpandedStyling)
{
    auto content = Content::create(LOCAL_EXPANDED_STYLING_DOC, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());
    ASSERT_EQ(0.7, group->getValue(kGraphicPropertyOpacity).getDouble());

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());
    ASSERT_EQ(Object(Color(Color::RED)), path->getValue(kGraphicPropertyFill));

    auto text = group->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());
    ASSERT_EQ(Object(Color(Color::RED)), text->getValue(kGraphicPropertyFill));
}

static const char *EXTERNAL_STYLING_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "styles": {
    "base": {
      "values": [
        {
          "opacity": 0.7,
          "fill": "red"
        }
      ]
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "items": {
        "type": "group",
        "style": "base",
        "items": [
          {
            "type": "path",
            "style": "base",
            "stroke": "blue",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          {
            "type": "text",
            "style": "base",
            "fontFamily": "amazon-ember, sans-serif",
            "fontSize": 40,
            "text": "Diamond",
            "x": 25,
            "y": 25,
            "textAnchor": "middle"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicTest, ExternalStyling)
{
    auto content = Content::create(EXTERNAL_STYLING_DOC, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());
    ASSERT_EQ(0.7, group->getValue(kGraphicPropertyOpacity).getDouble());

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());
    ASSERT_EQ(Object(Color(Color::RED)), path->getValue(kGraphicPropertyFill));

    auto text = group->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());
    ASSERT_EQ(Object(Color(Color::RED)), text->getValue(kGraphicPropertyFill));
}

static const char *EXTERNAL_EXPANDED_STYLING_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "styles": {
    "base": {
      "values": [
        {
          "opacity": 0.7
        }
      ]
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "styles": {
        "expanded": {
          "extends": "base",
          "values": [
            {
              "fill": "red"
            }
          ]
        }
      },
      "items": {
        "type": "group",
        "style": "expanded",
        "items": [
          {
            "type": "path",
            "style": "expanded",
            "stroke": "blue",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          {
            "type": "text",
            "style": "expanded",
            "fontFamily": "amazon-ember, sans-serif",
            "fontSize": 40,
            "text": "Diamond",
            "x": 25,
            "y": 25,
            "textAnchor": "middle"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicTest, ExternalExpandedStyling)
{
    auto content = Content::create(EXTERNAL_EXPANDED_STYLING_DOC, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());
    ASSERT_EQ(0.7, group->getValue(kGraphicPropertyOpacity).getDouble());

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());
    ASSERT_EQ(Object(Color(Color::RED)), path->getValue(kGraphicPropertyFill));

    auto text = group->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());
    ASSERT_EQ(Object(Color(Color::RED)), text->getValue(kGraphicPropertyFill));
}

static const char *TRANSFORMED_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "resources": [
        {
          "gradients": {
            "fillGradient": {
              "type": "linear",
              "colorRange": [ "red", "white" ],
              "inputRange": [0, 1],
              "spreadMethod": "repeat",
              "x1": 0.76,
              "y1": 0.99,
              "x2": 0.16,
              "y2": 0.89
            },
            "strokeGradient": {
              "type": "radial",
              "colorRange": [ "blue", "green" ],
              "inputRange": [0, 1],
              "centerX": 0.6,
              "centerY": 0.3,
              "radius": 1.2
            }
          }
        }
      ],
      "items": {
        "type": "group",
        "style": "expanded",
        "transform": "rotate(-10 50 75) ",
        "items": [
          {
            "type": "path",
            "fill": "@fillGradient",
            "fillTransform": "translate(-36 45.5) skewX(40) ",
            "style": "expanded",
            "stroke": "@strokeGradient",
            "strokeTransform": "skewY(5) scale(0.7 0.5) ",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicTest, Transformed)
{
    auto content = Content::create(TRANSFORMED_DOC, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());

    auto transform = group->getValue(kGraphicPropertyTransform).getTransform2D();
    Transform2D expected;
    expected *= Transform2D::translate(50, 75);
    expected *= Transform2D::rotate(-10);
    expected *= Transform2D::translate(-50, -75);
    ASSERT_EQ(expected, transform);

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    ASSERT_TRUE(path->getValue(kGraphicPropertyFill).isGradient());
    auto fill = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isGradient());
    auto& fillGrad = fill.getGradient();
    ASSERT_EQ(Gradient::LINEAR, fillGrad.getProperty(kGradientPropertyType).getInteger());
    auto colorRange = fillGrad.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::RED), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::WHITE), colorRange.at(1).asColor());

    auto inputRange = fillGrad.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).getDouble());
    ASSERT_EQ(1, inputRange.at(1).getDouble());

    ASSERT_EQ(Gradient::kGradientUnitsBoundingBox, fillGrad.getProperty(kGradientPropertyUnits).getInteger());
    ASSERT_EQ(Object::NULL_OBJECT(), fillGrad.getProperty(kGradientPropertyAngle));

    auto spreadMethod = fillGrad.getProperty(kGradientPropertySpreadMethod);
    ASSERT_EQ(Gradient::REPEAT, static_cast<Gradient::GradientSpreadMethod>(spreadMethod.getInteger()));

    ASSERT_EQ(0.76, fillGrad.getProperty(kGradientPropertyX1).getDouble());
    ASSERT_EQ(0.99, fillGrad.getProperty(kGradientPropertyY1).getDouble());
    ASSERT_EQ(0.16, fillGrad.getProperty(kGradientPropertyX2).getDouble());
    ASSERT_EQ(0.89, fillGrad.getProperty(kGradientPropertyY2).getDouble());


    auto fillTransform = path->getValue(kGraphicPropertyFillTransform).getTransform2D();
    Transform2D expectedFill;
    expectedFill *= Transform2D::translate(-36, 45.5);
    expectedFill *= Transform2D::skewX(40);

    ASSERT_EQ(expectedFill, fillTransform);

    ASSERT_TRUE(path->getValue(kGraphicPropertyStroke).isGradient());
    auto stroke = path->getValue(kGraphicPropertyStroke);
    ASSERT_TRUE(stroke.isGradient());
    auto& strokeGrad = stroke.getGradient();
    ASSERT_EQ(Gradient::RADIAL, strokeGrad.getProperty(kGradientPropertyType).getInteger());
    colorRange = strokeGrad.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::BLUE), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::GREEN), colorRange.at(1).asColor());

    inputRange = strokeGrad.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).getDouble());
    ASSERT_EQ(1, inputRange.at(1).getDouble());

    ASSERT_EQ(Gradient::kGradientUnitsBoundingBox, strokeGrad.getProperty(kGradientPropertyUnits).getInteger());

    ASSERT_EQ(0.6, strokeGrad.getProperty(kGradientPropertyCenterX).getDouble());
    ASSERT_EQ(0.3, strokeGrad.getProperty(kGradientPropertyCenterY).getDouble());
    ASSERT_EQ(1.2, strokeGrad.getProperty(kGradientPropertyRadius).getDouble());

    auto strokeTransform = path->getValue(kGraphicPropertyStrokeTransform).getTransform2D();
    Transform2D expectedStroke;
    expectedStroke *= Transform2D::skewY(5);
    expectedStroke *= Transform2D::scale(0.7, 0.5);
    ASSERT_EQ(expectedStroke, strokeTransform);
}

static const char* RESOURCE_TYPES = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "resources": [
        {
          "boolean": {
            "condition": true
          },
          "color": {
            "strokeColor": "green"
          },
          "gradient": {
            "gradientFill": {
              "type": "linear",
              "units": "userSpace",
              "x1": 25,
              "y1": 15,
              "x2": 75,
              "y2": 85,
              "colorRange": [
                "red",
                "transparent"
              ],
              "inputRange": [
                0,
                0.4
              ]
            }
          },
          "string": {
            "pathString": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          "number": {
            "length": 2
          },
          "dimension": {
            "sw": 4
          }
        }
      ],
      "items": {
        "type": "group",
        "items": [
          {
            "type": "path",
            "fill": "@gradientFill",
            "stroke": "@strokeColor",
            "strokeWidth": "@sw",
            "pathData": "${@condition ? @pathString : M}",
            "pathLength": "@length"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicTest, AVGResourceTypes)
{
    auto content = Content::create(RESOURCE_TYPES, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    // Patterns checked separately
    auto fill = path->getValue(kGraphicPropertyFill).getGradient();
    ASSERT_EQ(Gradient::LINEAR, fill.getType());
    ASSERT_EQ(Gradient::kGradientUnitsUserSpace, fill.getProperty(kGradientPropertyUnits).getInteger());
    ASSERT_EQ(std::vector<Color>({Color::RED, Color::TRANSPARENT}), fill.getColorRange());
    ASSERT_EQ(std::vector<double>({0, 0.4}), fill.getInputRange());
    ASSERT_EQ(25, fill.getProperty(kGradientPropertyX1).getDouble());
    ASSERT_EQ(75, fill.getProperty(kGradientPropertyX2).getDouble());
    ASSERT_EQ(15, fill.getProperty(kGradientPropertyY1).getDouble());
    ASSERT_EQ(85, fill.getProperty(kGradientPropertyY2).getDouble());

    ASSERT_EQ(Color(Color::GREEN), path->getValue(kGraphicPropertyStroke).getColor());
    ASSERT_EQ("M 50 0 L 100 50 L 50 100 L 0 50 z", path->getValue(kGraphicPropertyPathData).asString());
    ASSERT_EQ(2, path->getValue(kGraphicPropertyPathLength).getDouble());

    // Dimension is not allowed in AVG local resources.
    ASSERT_TRUE(path->getValue(kGraphicPropertyStrokeWidth).isNaN());
}

static const char* LOCALLY_RESOURCED_PATTERN = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "resources": [
        {
          "patterns": {
            "fillPattern": {
              "height": 18,
              "width": 18,
              "item": {
                "type": "path",
                "pathData": "M0,9 a9,9 0 1 1 18,0 a9,9 0 1 1 -18,0",
                "fill": "red"
              }
            },
            "strokePattern": {
              "height": 9,
              "width": 9,
              "item": {
                "type": "path",
                "pathData": "M0,9 a9,9 0 1 1 18,0 a9,9 0 1 1 -18,0",
                "fill": "green"
              }
            }
          }
        }
      ],
      "items": {
        "type": "group",
        "items": [
          {
            "type": "path",
            "fill": "@fillPattern",
            "stroke": "green",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          {
            "type": "text",
            "fill": "red",
            "stroke": "@strokePattern",
            "strokeWidth": 4,
            "text": "TEXT"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicTest, LocalResourcedPattern)
{
    auto content = Content::create(LOCALLY_RESOURCED_PATTERN, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    auto fillPattern = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fillPattern.isGraphicPattern());
    auto fillPatternId = fillPattern.getGraphicPattern()->getId();

    auto fillPath = fillPattern.getGraphicPattern()->getItems().at(0);
    ASSERT_EQ(kGraphicElementTypePath, fillPath->getType());
    ASSERT_EQ(Object(Color(Color::RED)), fillPath->getValue(kGraphicPropertyFill));

    auto text = group->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());

    auto strokePattern = text->getValue(kGraphicPropertyStroke);
    ASSERT_TRUE(strokePattern.isGraphicPattern());
    auto strokePatternId = strokePattern.getGraphicPattern()->getId();

    auto strokePath = strokePattern.getGraphicPattern()->getItems().at(0);
    ASSERT_EQ(kGraphicElementTypePath, strokePath->getType());
    ASSERT_EQ(Object(Color(Color::GREEN)), strokePath->getValue(kGraphicPropertyFill));

    // Ensure pattern IDs are unique
    ASSERT_NE(fillPatternId, strokePatternId);
}

static const char* EXTERNAL_RESOURCED_PATTERN = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "resources": [
    {
      "patterns": {
        "fillPattern": {
          "height": 18,
          "width": 18,
          "item": {
            "type": "path",
            "pathData": "M0,9 a9,9 0 1 1 18,0 a9,9 0 1 1 -18,0",
            "fill": "red"
          }
        }
      }
    }
  ],
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "items": {
        "type": "path",
        "fill": "@fillPattern",
        "stroke": "green",
        "strokeWidth": 4,
        "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
      }
    }
  }
})";

TEST_F(GraphicTest, ExternalResourcedPattern)
{
    auto content = Content::create(EXTERNAL_RESOURCED_PATTERN, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto path = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    // External resources has no definition for pattern(s) resource type
    auto pattern = root->context().opt("@fillPattern");
    ASSERT_TRUE(pattern.isNull());
    ASSERT_TRUE(ConsoleMessage());

    auto fillPattern = path->getValue(kGraphicPropertyFill);
    ASSERT_FALSE(fillPattern.isGraphicPattern());
}

static const char* PATTERN_INLINE = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "items": {
        "type": "path",
        "fill": {
          "type": "pattern",
          "height": 18,
          "width": 18,
          "item": {
            "type": "path",
            "pathData": "M0,9 a9,9 0 1 1 18,0 a9,9 0 1 1 -18,0",
            "fill": "red"
          }
        },
        "stroke": "green",
        "strokeWidth": 4,
        "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
      }
    }
  }
})";

TEST_F(GraphicTest, PatternInline)
{
    auto content = Content::create(PATTERN_INLINE, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto path = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    auto fillPattern = path->getValue(kGraphicPropertyFill);
    // Inline not supported
    ASSERT_TRUE(fillPattern.isColor());
    ASSERT_EQ(Object(Color()), fillPattern);
    ASSERT_TRUE(ConsoleMessage());
}

static const char* LOCALLY_RESOURCED_GRADIENT = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "resources": [
        {
          "gradients": {
            "fillGradient": {
              "type": "linear",
              "colorRange": [ "red", "white" ],
              "inputRange": [0, 1],
              "spreadMethod": "repeat",
              "x1": 0.76,
              "y1": 0.99,
              "x2": 0.16,
              "y2": 0.89
            },
            "strokeGradient": {
              "type": "radial",
              "colorRange": [ "blue", "green" ],
              "inputRange": [0, 1],
              "centerX": 0.6,
              "centerY": 0.3,
              "radius": 1.2
            }
          }
        }
      ],
      "items": {
        "type": "group",
        "items": [
          {
            "type": "path",
            "fill": "@fillGradient",
            "stroke": "green",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          {
            "type": "text",
            "fill": "red",
            "stroke": "@strokeGradient",
            "strokeWidth": 4,
            "text": "TEXT"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicTest, LocalResourcedGradient)
{
    auto content = Content::create(LOCALLY_RESOURCED_GRADIENT, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    auto fill = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isGradient());
    auto& fillGrad = fill.getGradient();
    ASSERT_EQ(Gradient::LINEAR, fillGrad.getProperty(kGradientPropertyType).getInteger());
    auto colorRange = fillGrad.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::RED), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::WHITE), colorRange.at(1).asColor());

    auto inputRange = fillGrad.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).getDouble());
    ASSERT_EQ(1, inputRange.at(1).getDouble());

    ASSERT_EQ(Object::NULL_OBJECT(), fillGrad.getProperty(kGradientPropertyAngle));

    auto spreadMethod = fillGrad.getProperty(kGradientPropertySpreadMethod);
    ASSERT_EQ(Gradient::REPEAT, static_cast<Gradient::GradientSpreadMethod>(spreadMethod.getInteger()));

    ASSERT_EQ(0.76, fillGrad.getProperty(kGradientPropertyX1).getDouble());
    ASSERT_EQ(0.99, fillGrad.getProperty(kGradientPropertyY1).getDouble());
    ASSERT_EQ(0.16, fillGrad.getProperty(kGradientPropertyX2).getDouble());
    ASSERT_EQ(0.89, fillGrad.getProperty(kGradientPropertyY2).getDouble());



    auto text = group->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());

    auto stroke = text->getValue(kGraphicPropertyStroke);
    ASSERT_TRUE(stroke.isGradient());
    auto& strokeGrad = stroke.getGradient();
    ASSERT_EQ(Gradient::RADIAL, strokeGrad.getProperty(kGradientPropertyType).getInteger());
    colorRange = strokeGrad.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::BLUE), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::GREEN), colorRange.at(1).asColor());

    inputRange = strokeGrad.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).getDouble());
    ASSERT_EQ(1, inputRange.at(1).getDouble());

    ASSERT_EQ(0.6, strokeGrad.getProperty(kGradientPropertyCenterX).getDouble());
    ASSERT_EQ(0.3, strokeGrad.getProperty(kGradientPropertyCenterY).getDouble());
    ASSERT_EQ(1.2, strokeGrad.getProperty(kGradientPropertyRadius).getDouble());
}

static const char* EXTERNAL_RESOURCED_GRADIENT = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "resources": [
    {
      "gradients": {
        "fillGradient": {
          "type": "linear",
          "colorRange": [ "blue", "white" ],
          "inputRange": [0, 1],
          "angle": 30
        },
        "strokeGradient": {
          "type": "radial",
          "colorRange": [ "red", "green" ],
          "inputRange": [0, 1]
        }
      }
    }
  ],
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "items": {
        "type": "group",
        "items": [
          {
            "type": "path",
            "fill": "@fillGradient",
            "stroke": "green",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          {
            "type": "text",
            "fill": "red",
            "stroke": "@strokeGradient",
            "strokeWidth": 4,
            "text": "TEXT"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicTest, ExternalResourcedGradient)
{
    auto content = Content::create(EXTERNAL_RESOURCED_GRADIENT, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    auto fill = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isGradient());
    auto& fillGrad = fill.getGradient();
    ASSERT_EQ(Gradient::LINEAR, fillGrad.getProperty(kGradientPropertyType).getInteger());
    auto colorRange = fillGrad.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::BLUE), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::WHITE), colorRange.at(1).asColor());

    auto inputRange = fillGrad.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).getDouble());
    ASSERT_EQ(1, inputRange.at(1).getDouble());

    ASSERT_EQ(30, fillGrad.getProperty(kGradientPropertyAngle).getDouble());

    auto spreadMethod = fillGrad.getProperty(kGradientPropertySpreadMethod);
    ASSERT_EQ(Gradient::PAD, static_cast<Gradient::GradientSpreadMethod>(spreadMethod.getInteger()));

    ASSERT_NEAR(0.1585, fillGrad.getProperty(kGradientPropertyX1).getDouble(), 0.0001);
    ASSERT_NEAR(-0.0915, fillGrad.getProperty(kGradientPropertyY1).getDouble(), 0.0001);
    ASSERT_NEAR(0.8415, fillGrad.getProperty(kGradientPropertyX2).getDouble(), 0.0001);
    ASSERT_NEAR(1.0915, fillGrad.getProperty(kGradientPropertyY2).getDouble(), 0.0001);



    auto text = group->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());

    auto stroke = text->getValue(kGraphicPropertyStroke);
    ASSERT_TRUE(stroke.isGradient());
    auto& strokeGrad = stroke.getGradient();
    ASSERT_EQ(Gradient::RADIAL, strokeGrad.getProperty(kGradientPropertyType).getInteger());
    colorRange = strokeGrad.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::RED), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::GREEN), colorRange.at(1).asColor());

    inputRange = strokeGrad.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).getDouble());
    ASSERT_EQ(1, inputRange.at(1).getDouble());

    ASSERT_EQ(0.5, strokeGrad.getProperty(kGradientPropertyCenterX).getDouble());
    ASSERT_EQ(0.5, strokeGrad.getProperty(kGradientPropertyCenterY).getDouble());
    ASSERT_EQ(0.7071, strokeGrad.getProperty(kGradientPropertyRadius).getDouble());
}

static const char* GRADIENT_INLINE = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "items": {
        "type": "path",
        "fill": {
          "type": "linear",
          "colorRange": [ "blue", "white" ],
          "inputRange": [0, 1],
          "angle": 5
        },
        "stroke": "green",
        "strokeWidth": 4,
        "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
      }
    }
  }
})";

TEST_F(GraphicTest, GradientInline)
{
    auto content = Content::create(GRADIENT_INLINE, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto path = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    auto fill = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isGradient());
    auto& fillGrad = fill.getGradient();
    ASSERT_EQ(Gradient::LINEAR, fillGrad.getProperty(kGradientPropertyType).getInteger());
    auto colorRange = fillGrad.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::BLUE), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::WHITE), colorRange.at(1).asColor());
}

static const char* MIXED_RESOURCES = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "resources": [
    {
      "color": {
        "fillColor1": "red",
        "fillColor2": "green"
      }
    }
  ],
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "resources": [
        {
          "patterns": {
            "fillPattern": {
              "height": 18,
              "width": 18,
              "item": {
                "type": "path",
                "pathData": "M0,9 a9,9 0 1 1 18,0 a9,9 0 1 1 -18,0",
                "fill": "@fillColor1"
              }
            }
          },
          "gradients": {
            "strokeGradient": {
              "type": "radial",
              "colorRange": [ "@fillColor1", "@fillColor2" ],
              "inputRange": [0, 1]
            }
          }
        }
      ],
      "items": {
        "type": "group",
        "items": [
          {
            "type": "path",
            "fill": "@fillPattern",
            "stroke": "green",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          {
            "type": "text",
            "fill": "red",
            "stroke": "@strokeGradient",
            "strokeWidth": 4,
            "text": "TEXT"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicTest, MixedResources)
{
    auto content = Content::create(MIXED_RESOURCES, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());


    auto fillPattern = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fillPattern.isGraphicPattern());

    auto fillPath = fillPattern.getGraphicPattern()->getItems().at(0);
    ASSERT_EQ(kGraphicElementTypePath, fillPath->getType());
    ASSERT_EQ(Object(Color(Color::RED)), fillPath->getValue(kGraphicPropertyFill));


    auto text = group->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());

    auto stroke = text->getValue(kGraphicPropertyStroke);
    ASSERT_TRUE(stroke.isGradient());
    auto& strokeGrad = stroke.getGradient();
    ASSERT_EQ(Gradient::RADIAL, strokeGrad.getProperty(kGradientPropertyType).getInteger());
    auto colorRange = strokeGrad.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::RED), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::GREEN), colorRange.at(1).asColor());

    auto inputRange = strokeGrad.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).getDouble());
    ASSERT_EQ(1, inputRange.at(1).getDouble());

    ASSERT_EQ(0.5, strokeGrad.getProperty(kGradientPropertyCenterX).getDouble());
    ASSERT_EQ(0.5, strokeGrad.getProperty(kGradientPropertyCenterY).getDouble());
    ASSERT_EQ(0.7071, strokeGrad.getProperty(kGradientPropertyRadius).getDouble());
}

static const char* TRANSFORM_TEST = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "box"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "items": {
        "type": "group",
        "translateX": 100,
        "translateY": 50,
        "rotation": 90,
        "pivotX": 20,
        "pivotY": 10,
        "scaleX": 2,
        "scaleY": 0.5
      }
    }
  }
})";

TEST_F(GraphicTest, Transform)
{
    auto content = Content::create(TRANSFORM_TEST, session);
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
    ASSERT_EQ(90, group->getValue(kGraphicPropertyRotation).getDouble());
    ASSERT_EQ(100, group->getValue(kGraphicPropertyTranslateX).getDouble());
    ASSERT_EQ(50, group->getValue(kGraphicPropertyTranslateY).getDouble());
    ASSERT_EQ(20, group->getValue(kGraphicPropertyPivotX).getDouble());
    ASSERT_EQ(10, group->getValue(kGraphicPropertyPivotY).getDouble());
    ASSERT_EQ(2, group->getValue(kGraphicPropertyScaleX).getDouble());
    ASSERT_EQ(0.5, group->getValue(kGraphicPropertyScaleY).getDouble());

    auto transform = group->getValue(kGraphicPropertyTransform);
    ASSERT_TRUE(transform.isTransform2D());

    // Start       -Pivot        Scaled       Rotate     +Pivot     Translated
    // ( 0, 0) -> (-20,-10) -> (-40, -5) -> ( 5,-40) -> (25,-30) -> (125, 20)
    ASSERT_EQ(Point(125,20), transform.getTransform2D() * Point(0,0));

    // (20,10) -> (  0,  0) -> (  0,  0) -> ( 0,  0) -> (20, 10) -> (120, 60)
    ASSERT_EQ(Point(120,60), transform.getTransform2D() * Point(20,10));

    // (30,20) -> ( 10, 10) -> ( 20,  5) -> (-5, 20) -> (15, 30) -> (115, 80)
    ASSERT_EQ(Point(115,80), transform.getTransform2D() * Point(30,20));
}

static const char* GRADIENT_REQUIRED = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "items": [
        {
          "type": "path",
          "fill": {
            "type": "linear",
            "description": "Type, no color range."
          },
          "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
        },
        {
          "type": "path",
          "fill": {
            "description": "No type, color range.",
            "colorRange": [ "blue", "white" ]
          },
          "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
        },
        {
          "type": "path",
          "fill": {
            "description": "No type, no color range."
          },
          "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
        },
        {
          "type": "path",
          "fill": {
            "type": "linear",
            "description": "Default linear.",
            "colorRange": [ "blue", "white" ]
          },
          "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
        },
        {
          "type": "path",
          "fill": {
            "type": "radial",
            "description": "Default radial.",
            "colorRange": [ "blue", "white" ]
          },
          "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
        }
      ]
    }
  }
})";

TEST_F(GraphicTest, GradientChecks)
{
    auto content = Content::create(GRADIENT_REQUIRED, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box") ;
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());
    ASSERT_TRUE(ConsoleMessage());

    // Defaults to default color when gradient is incorrect (no color range)
    auto path = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    auto fill = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isColor());
    ASSERT_EQ(Color(Color::TRANSPARENT), fill.getColor());

    // Defaults to default color when gradient is incorrect (no type)
    path = graphic->getRoot()->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    fill = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isColor());
    ASSERT_EQ(Color(Color::TRANSPARENT), fill.getColor());

    // Defaults to default color when gradient is incorrect (no type, no color range)
    path = graphic->getRoot()->getChildAt(2);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    fill = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isColor());
    ASSERT_EQ(Color(Color::TRANSPARENT), fill.getColor());

    // Default values on linear gradient
    path = graphic->getRoot()->getChildAt(3);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    fill = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isGradient());
    auto& fillLinearGrad = fill.getGradient();
    ASSERT_EQ(Gradient::LINEAR, fillLinearGrad.getProperty(kGradientPropertyType).getInteger());
    auto colorRange = fillLinearGrad.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::BLUE), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::WHITE), colorRange.at(1).asColor());
    auto inputRange = fillLinearGrad.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).getDouble());
    ASSERT_EQ(1, inputRange.at(1).getDouble());
    ASSERT_EQ(Gradient::GradientSpreadMethod::PAD, fillLinearGrad.getProperty(kGradientPropertySpreadMethod).getInteger());
    ASSERT_EQ(0, fillLinearGrad.getProperty(kGradientPropertyX1).getDouble());
    ASSERT_EQ(1, fillLinearGrad.getProperty(kGradientPropertyX2).getDouble());
    ASSERT_EQ(0, fillLinearGrad.getProperty(kGradientPropertyY1).getDouble());
    ASSERT_EQ(1, fillLinearGrad.getProperty(kGradientPropertyY2).getDouble());

    // Default values on radial gradient
    path = graphic->getRoot()->getChildAt(4);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    fill = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isGradient());
    auto& fillRadialGrad = fill.getGradient();
    ASSERT_EQ(Gradient::RADIAL, fillRadialGrad.getProperty(kGradientPropertyType).getInteger());
    colorRange = fillRadialGrad.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::BLUE), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::WHITE), colorRange.at(1).asColor());
    inputRange = fillRadialGrad.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).getDouble());
    ASSERT_EQ(1, inputRange.at(1).getDouble());
    ASSERT_EQ(0.5, fillRadialGrad.getProperty(kGradientPropertyCenterX).getDouble());
    ASSERT_EQ(0.5, fillRadialGrad.getProperty(kGradientPropertyCenterY).getDouble());
    ASSERT_EQ(0.7071, fillRadialGrad.getProperty(kGradientPropertyRadius).getDouble());
}

static const char* FILTERED_TEXT = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 500,
      "width": 500,
      "items": [
        {
          "type": "text",
          "fill": "white",
          "text": "<b>Ignored bold.</b> &amp; - &lt; - &gt; - &#169; - &#xa9;",
          "y": 100
        }
      ]
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "VectorGraphic",
        "source": "box",
        "width": "100%",
        "height": "100%"
      }
    ]
  }
})";

TEST_F(GraphicTest, FilteredText) {
    auto content = Content::create(FILTERED_TEXT, session);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content);
    ASSERT_TRUE(root);

    auto box = root->context().getGraphic("box");
    ASSERT_FALSE(box.empty());

    loadGraphic(root->contextPtr(), box.json());

    auto textElement = graphic->getRoot()->getChildAt(0);
    auto text = textElement->getValue(kGraphicPropertyText).asString();

    ASSERT_EQ("Ignored bold. & - < - > - © - ©", text);
}

static const char *DEFAULT_FILTERS = R"apl(
{
  "type":"AVG",
  "version":"1.1",
  "height":157,
  "width":171,
  "viewportHeight":157,
  "viewportWidth":171,
  "parameters":[
    {
      "default":"green",
      "type":"color",
      "name":"fillColor"
    },
    {
      "default":15.0,
      "type":"number",
      "name":"rotation"
    }
  ],
  "items":[
    {
      "pivotX":85.5,
      "pivotY":78.5,
      "type":"group",
      "filter": {
        "type":"DropShadow"
      },
      "rotation":"${rotation}",
      "items":[
        {
          "type":"path",
          "pathData":"M85.7106781,155.714249 L85.3571247,156.067803 L86.0642315,156.067803 L85.7106781,155.714249 Z M155.714249,85.7106781 L156.067803,86.0642315 L156.421356,85.7106781 L156.067803,85.3571247 L155.714249,85.7106781 Z",
          "fillOpacity":0.3,
          "fill":"${fillColor}"
        },
        {
          "type":"text",
          "text":"Hello",
          "filters":[
            {
              "type":"DropShadow"
            }
          ],
          "fill":"${fillColor}"
        }
      ]
    }
  ]
}
)apl";

TEST_F(GraphicTest, DefaultGraphicFilter)
{
    loadGraphic(DEFAULT_FILTERS);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto child = container->getChildAt(0);
    ASSERT_EQ(2, child->getChildCount());

    ASSERT_EQ(kGraphicElementTypeGroup, child->getType());
    auto filterArray = child->getValue(kGraphicPropertyFilters);
    ASSERT_EQ(rapidjson::kArrayType, filterArray.getType());
    ASSERT_EQ(1, filterArray.size());
    auto graphicFilter = filterArray.at(0).getGraphicFilter();
    ASSERT_EQ(kGraphicFilterTypeDropShadow, graphicFilter.getType());
    ASSERT_TRUE(IsEqual(Color::BLACK, graphicFilter.getValue(kGraphicPropertyFilterColor)));
    ASSERT_TRUE(IsEqual(0, graphicFilter.getValue(kGraphicPropertyFilterHorizontalOffset)));
    ASSERT_TRUE(IsEqual(0, graphicFilter.getValue(kGraphicPropertyFilterRadius)));
    ASSERT_TRUE(IsEqual(0, graphicFilter.getValue(kGraphicPropertyFilterVerticalOffset)));

    auto path = child->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());
    filterArray = path->getValue(kGraphicPropertyFilters);
    ASSERT_EQ(rapidjson::kArrayType, filterArray.getType());
    ASSERT_EQ(Object::EMPTY_ARRAY(), filterArray);

    auto text = child->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());
    filterArray = text->getValue(kGraphicPropertyFilters);
    ASSERT_EQ(rapidjson::kArrayType, filterArray.getType());
    graphicFilter = filterArray.at(0).getGraphicFilter();
    ASSERT_EQ(kGraphicFilterTypeDropShadow, graphicFilter.getType());
    ASSERT_TRUE(IsEqual(Color::BLACK, graphicFilter.getValue(kGraphicPropertyFilterColor)));
    ASSERT_TRUE(IsEqual(0, graphicFilter.getValue(kGraphicPropertyFilterHorizontalOffset)));
    ASSERT_TRUE(IsEqual(0, graphicFilter.getValue(kGraphicPropertyFilterRadius)));
    ASSERT_TRUE(IsEqual(0, graphicFilter.getValue(kGraphicPropertyFilterVerticalOffset)));
}

static const char *GRAPHIC_FILTER_ARRAY = R"apl(
{
  "type":"AVG",
  "version":"1.1",
  "lang": "ja-JP",
  "layoutDirection": "RTL",
  "height":157,
  "width":171,
  "viewportHeight":157,
  "viewportWidth":171,
  "parameters":[
    {
      "default":"green",
      "type":"color",
      "name":"fillColor"
    },
    {
      "default":15.0,
      "type":"number",
      "name":"rotation"
    }
  ],
  "items":[
    {
      "pivotX":85.5,
      "pivotY":78.5,
      "type":"group",
      "filters":[
        {
          "type":"DropShadow",
          "color":"${fillColor}",
          "horizontalOffset":1,
          "radius":2,
          "verticalOffset":3
        }
      ],
      "rotation":"${rotation}",
      "items":[
        {
          "type":"path",
          "pathData":"M85.7106781,155.714249 L85.3571247,156.067803 L86.0642315,156.067803 L85.7106781,155.714249 Z M155.714249,85.7106781 L156.067803,86.0642315 L156.421356,85.7106781 L156.067803,85.3571247 L155.714249,85.7106781 Z",
          "fillOpacity":0.3,
          "fill":"${fillColor}",
          "filters":[
            {
              "type":"DropShadow"
            },
            {

            },
            {
              "type":"DropShadow",
              "color":"blue",
              "horizontalOffset":-1,
              "radius":-2,
              "verticalOffset":-3
            }
          ]
        }
      ]
    }
  ]
}
)apl";

TEST_F(GraphicTest, GraphicFilterArray)
{
    loadGraphic(GRAPHIC_FILTER_ARRAY);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto child = container->getChildAt(0);
    ASSERT_EQ(1, child->getChildCount());

    ASSERT_EQ(kGraphicElementTypeGroup, child->getType());
    auto filterArray = child->getValue(kGraphicPropertyFilters);
    ASSERT_EQ(rapidjson::kArrayType, filterArray.getType());
    ASSERT_EQ(1, filterArray.size());
    auto graphicFilter = filterArray.at(0).getGraphicFilter();
    ASSERT_EQ(kGraphicFilterTypeDropShadow, graphicFilter.getType());
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), graphicFilter.getValue(kGraphicPropertyFilterColor)));
    ASSERT_TRUE(IsEqual(1, graphicFilter.getValue(kGraphicPropertyFilterHorizontalOffset)));
    ASSERT_TRUE(IsEqual(2, graphicFilter.getValue(kGraphicPropertyFilterRadius)));
    ASSERT_TRUE(IsEqual(3, graphicFilter.getValue(kGraphicPropertyFilterVerticalOffset)));

    auto path = child->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());
    filterArray = path->getValue(kGraphicPropertyFilters);
    ASSERT_EQ(rapidjson::kArrayType, filterArray.getType());
    ASSERT_EQ(2, filterArray.size());

    // check value of first filter
    graphicFilter = filterArray.at(0).getGraphicFilter();
    ASSERT_EQ(kGraphicFilterTypeDropShadow, graphicFilter.getType());
    ASSERT_TRUE(IsEqual(Color::BLACK, graphicFilter.getValue(kGraphicPropertyFilterColor)));
    ASSERT_TRUE(IsEqual(0, graphicFilter.getValue(kGraphicPropertyFilterHorizontalOffset)));
    ASSERT_TRUE(IsEqual(0, graphicFilter.getValue(kGraphicPropertyFilterRadius)));
    ASSERT_TRUE(IsEqual(0, graphicFilter.getValue(kGraphicPropertyFilterVerticalOffset)));

    // check value of second filter
    graphicFilter = filterArray.at(1).getGraphicFilter();
    ASSERT_EQ(kGraphicFilterTypeDropShadow, graphicFilter.getType());
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), graphicFilter.getValue(kGraphicPropertyFilterColor)));
    ASSERT_TRUE(IsEqual(-1, graphicFilter.getValue(kGraphicPropertyFilterHorizontalOffset)));
    ASSERT_TRUE(IsEqual(0, graphicFilter.getValue(kGraphicPropertyFilterRadius)));
    ASSERT_TRUE(IsEqual(-3, graphicFilter.getValue(kGraphicPropertyFilterVerticalOffset)));

    // empty filter will throw a console log of missing 'type' property
    ASSERT_EQ("No 'type' property defined for graphic filter", session->getLast());
    session->clear();
}

// Verify that filters serialize correctly
TEST_F(GraphicTest, Serialize)
{
    loadGraphic(GRAPHIC_FILTER_ARRAY);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(1, container->getChildCount());
    auto child = container->getChildAt(0);
    ASSERT_EQ(1, child->getChildCount());

    ASSERT_EQ(kGraphicElementTypeGroup, child->getType());
    auto filters = child->getValue(kGraphicPropertyFilters);
    ASSERT_TRUE(filters.isArray());
    ASSERT_EQ(1, filters.size());

    rapidjson::Document doc;
    auto json = filters.serialize(doc.GetAllocator());

    ASSERT_TRUE(json.IsArray());
    ASSERT_EQ(1, json.GetArray().Size());

    // Check the first filter
    ASSERT_EQ(5, json[0].MemberCount());  // Five members: type, color, horizontalOffset, radius, verticalOffset
    ASSERT_EQ(kGraphicFilterTypeDropShadow, json[0]["type"].GetDouble());
    ASSERT_STREQ("#008000ff", json[0]["color"].GetString());
    ASSERT_EQ(1, json[0]["horizontalOffset"].GetDouble());
    ASSERT_EQ(2, json[0]["radius"].GetDouble());
    ASSERT_EQ(3, json[0]["verticalOffset"].GetDouble());

    ASSERT_EQ("No 'type' property defined for graphic filter", session->getLast());

    const auto& graphicJson = container->serialize(doc.GetAllocator()).FindMember("props")->value;
    auto lang = graphicJson.FindMember("lang")->value.GetString();
    ASSERT_STREQ("ja-JP", lang);

    auto layoutDirection = graphicJson.FindMember("layoutDirection")->value.GetDouble();
    ASSERT_EQ(kGraphicLayoutDirectionRTL, layoutDirection);

    session->clear();
}

static const char *GRAPHIC_ELEMENT_MISSING_WIDTH = R"apl(
{
  "type": "APL",
  "version": "1.6",
  "graphics": {
    "ToggleButton": {
      "type": "AVG",
  "version": "1.0",
"parameters": [
"On"   ],
     "itedth": 150,
      "height": 90,
      "items": [
        {   "type": "path",
          "deption": "Background shape",
  "pathData": "M45,88 A4L105,2 A43,43,0,0,1,105,88 Z",
  "stroke": "#97On ? 'green' : '#d8d8d8' }",
          "strokeWidth": 2
    },
        {
  "type": "group",
          "description": "Button",
          "translateX": "${On ? 60: 0}",
          "items": {
    "type": "path",
            "pathData": "M45,82 A36,36,0,0,1,45,8 A36,36,0,1,1,45,82 Z",
            "fill": "white",
            "stroke": "#979797",
            "strokeWidth": 2
          }
}
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "bind": {
        "name": "IsOn",
        "value": false
      },
      "onPress": [ {
          "type": "SetValue",
          "property": "IsOn",
          "value": "${!IsOn}"
        },
        {
          "type": "SendEvent"
}
      ],
      "items": [
        {
  "type": "VectorGraphic",
          "source": "ToggleButton",
          "On": "${IsOn}",
          "inheritParentState": true,
          "onPress": [
            {
      "type": "SetValue",
              "property": "On",
              "value": "${!On}"
    }
          ]
        }
      ]
    }
  }
}
)apl";

// Verify that filters serialize correctly
TEST_F(GraphicTest, MissingWidthDoesntStop)
{
    auto content = apl::Content::create(GRAPHIC_ELEMENT_MISSING_WIDTH);
    ASSERT_TRUE(content->isReady());
    ASSERT_TRUE(apl::RootContext::create(
        apl::Metrics().size(1280, 800).dpi(160).shape(apl::ScreenShape::ROUND), content));
}