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
#include "test_sg.h"
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/node.h"

using namespace apl;

class SGGraphicTest : public DocumentWrapper {
public:
    SGGraphicTest() : DocumentWrapper() {
        config->measure(std::make_shared<MyTestMeasurement>());
    }

    void loadGraphic(const char *str, const StyleInstancePtr& style = nullptr) {
        gc = GraphicContent::create(session, str);
        ASSERT_TRUE(gc);
        auto jr = JsonResource(&gc->get(), Path());
        auto context = Context::createTestContext(metrics, *config);
        Properties properties;
        graphic = Graphic::create(context, jr, std::move(properties), nullptr);
        ASSERT_TRUE(graphic);
    }

    void TearDown() override {
        graphic = nullptr;
        DocumentWrapper::TearDown();
    }

    GraphicContentPtr gc;
    GraphicPtr graphic;
    sg::SceneGraphUpdates updates;
};


static const char *BASIC_RECT = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "items": {
        "type": "path",
        "fill": "red",
        "pathData": "M0,0 L100,0 L100,100 L0,100 z"
      }
    }
)apl";

TEST_F(SGGraphicTest, BasicRect)
{
    loadGraphic(BASIC_RECT);

    auto node = graphic->getSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(updates, node,
                                IsDrawNode()
                                    .path(IsGeneralPath("MLLLZ", {0, 0, 100, 0, 100, 100, 0, 100}))
                                    .pathOp(IsFillOp(IsColorPaint(Color::RED)))));
}


static const char *TWO_RECTS = R"apl(
{
    "type": "AVG",
    "version": "1.2",
    "height": 100,
    "width": 100,
    "items": [
        {
            "type": "path",
            "fill": "red",
            "pathData": "M0,0 L100,0 L100,100 L0,100 z"
        },
        {
            "type": "path",
            "fill": "blue",
            "pathData": "M20,20 L60,20 L60,60 L20,60 z"
        }
    ]
}
)apl";

TEST_F(SGGraphicTest, TwoRects)
{
    loadGraphic(TWO_RECTS);
    auto node = graphic->getSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsDrawNode()
            .path(IsGeneralPath("MLLLZ", {0, 0, 100, 0, 100, 100, 0, 100}))
            .pathOp(IsFillOp(IsColorPaint(Color::RED)))
            .next(IsDrawNode()
                      .path(IsGeneralPath("MLLLZ", {20, 20, 60, 20, 60, 60, 20, 60}))
                      .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))));


}




static const char *COMPLICATED_RECT = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "resources": {
        "gradients": {
          "FOO": {
            "type": "linear",
            "colorRange":
            [
              "red",
              "white"
            ],
            "inputRange":
            [
              0,
              1
            ],
            "angle": 90
          }
        }
      },
      "items": {
        "type": "path",
        "fill": "red",
        "fillOpacity": 0.5,
        "fillTransform": "translate(10,20)",
        "pathLength": 100,
        "stroke": "@FOO",
        "strokeDashArray": [1,2,3],
        "strokeDashOffset": 1,
        "strokeLineCap": "round",
        "strokeLineJoin": "round",
        "strokeMiterLimit": 10,
        "strokeOpacity": 0.25,
        "strokeWidth": 2,
        "strokeTransform": "rotate(90)",
        "pathData": "M0,0 L100,0 L100,100 L0,100 z"
      }
    }
)apl";

TEST_F(SGGraphicTest, ComplicatedRect)
{
    loadGraphic(COMPLICATED_RECT);
    auto node = graphic->getSceneGraph(updates);

    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsDrawNode()
            .path(IsGeneralPath("MLLLZ", {0, 0, 100, 0, 100, 100, 0, 100}))
            .pathOp(
                {IsFillOp(IsColorPaint(Color::RED, 0.5)),
                 IsStrokeOp(IsLinearGradientPaint({0, 1}, {Color::RED, Color::WHITE},
                                                  Gradient::GradientSpreadMethod::PAD, true, {0, 0},
                                                  {1, 1}, 0.25f, Transform2D::rotate(90.f)),
                            2.0f, 10.0f, 100.0f, 1.0f, GraphicLineCap::kGraphicLineCapRound,
                            GraphicLineJoin::kGraphicLineJoinRound,
                            {1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f})

                })));
}


static const char *PARAMETERIZED = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "resources": {
        "gradients": {
          "FOO": {
            "type": "linear",
            "colorRange":
            [
              "red",
              "white"
            ],
            "inputRange":
            [
              0,
              1
            ],
            "angle": 90
          }
        }
      },
      "parameters":
      [
        "color",
        {
          "name": "opacity",
          "default": 1.0
        },
        "transform",
        {
          "name": "pathLength",
          "default": 10.0
        },
        {
          "name": "dashArray",
          "default":
          []
        },
        {
          "name": "dashOffset",
          "default": 0
        },
        "lineCap",
        "lineJoin",
        {
          "name": "miterLimit",
          "default": "5.0"
        },
        "path"
      ],
      "items": {
        "type": "path",
        "stroke": "${color}",
        "strokeOpacity": "${opacity}",
        "strokeTransform": "${transform}",
        "pathLength": "${pathLength}",
        "strokeDashArray": "${dashArray}",
        "strokeDashOffset": "${dashOffset}",
        "strokeLineCap": "${lineCap}",
        "strokeLineJoin": "${lineJoin}",
        "strokeMiterLimit": "${miterLimit}",
        "pathData": "${path}"
      }
    }
)apl";

TEST_F(SGGraphicTest, Parameterized)
{
    loadGraphic(PARAMETERIZED);
    auto node = graphic->getSceneGraph(updates);

    ASSERT_FALSE(node->visible());

    graphic->setProperty("color", Color::GREEN);
    graphic->setProperty("path", "M0,0 L100,100");
    graphic->setProperty("opacity", 1.0f);

    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(
        CheckSceneGraph(updates, node,
                        IsDrawNode()
                            .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                            .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f, 10.0f, 0.0f,
                                               kGraphicLineCapButt, kGraphicLineJoinMiter, {}))));

    graphic->setProperty("lineJoin", "round");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(
        CheckSceneGraph(updates, node,
                        IsDrawNode()
                            .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                            .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f, 10.0f, 0.0f,
                                               kGraphicLineCapButt, kGraphicLineJoinRound, {}))));

    graphic->setProperty("pathLength", 20);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(
        CheckSceneGraph(updates, node,
                        IsDrawNode()
                            .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                            .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f, 20.0f, 0.0f,
                                               kGraphicLineCapButt, kGraphicLineJoinRound, {}))));

    graphic->setProperty("dashArray", std::vector<Object>(2, 2));
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsDrawNode()
            .path(IsGeneralPath("ML", {0, 0, 100, 100}))
            .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f, 20.0f, 0.0f,
                               kGraphicLineCapButt, kGraphicLineJoinRound, {2.0f, 2.0f}))));

    graphic->setProperty("dashOffset", 1.5f);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsDrawNode()
            .path(IsGeneralPath("ML", {0, 0, 100, 100}))
            .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f, 20.0f, 1.5f,
                               kGraphicLineCapButt, kGraphicLineJoinRound, {2.0f, 2.0f}))));

    graphic->setProperty("lineCap", "square");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsDrawNode()
            .path(IsGeneralPath("ML", {0, 0, 100, 100}))
            .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f, 20.0f, 1.5f,
                               kGraphicLineCapSquare, kGraphicLineJoinRound, {2.0f, 2.0f}))));

    graphic->setProperty("miterLimit", 23.0f);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsDrawNode()
            .path(IsGeneralPath("ML", {0, 0, 100, 100}))
            .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 23.0f, 20.0f, 1.5f,
                               kGraphicLineCapSquare, kGraphicLineJoinRound, {2.0f, 2.0f}))));

    graphic->setProperty("opacity", 0.5f);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsDrawNode()
            .path(IsGeneralPath("ML", {0, 0, 100, 100}))
            .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN, 0.5f), 1.0f, 23.0f, 20.0f, 1.5f,
                               kGraphicLineCapSquare, kGraphicLineJoinRound, {2.0f, 2.0f}))));

    // Update the transform - but color paint doesn't use transform, so nothing changes
    // However, the draw node is marked as modified because the transform did actually change
    graphic->setProperty("transform", "translate(1 2)");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsDrawNode()
            .path(IsGeneralPath("ML", {0, 0, 100, 100}))
            .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN, 0.5f), 1.0f, 23.0f, 20.0f, 1.5f,
                               kGraphicLineCapSquare, kGraphicLineJoinRound, {2.0f, 2.0f}))));

    // Assign a gradient.  This will pick up the translate
    graphic->setProperty("color", "@FOO");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsDrawNode()
            .path(IsGeneralPath("ML", {0, 0, 100, 100}))
            .pathOp(
                IsStrokeOp(IsLinearGradientPaint({0, 1}, {Color::RED, Color::WHITE},
                                                 Gradient::GradientSpreadMethod::PAD, true, {0, 0},
                                                 {1, 1}, 0.5f, Transform2D::translate({1, 2})),
                           1.0f, 23.0f, 20.0f, 1.5f, kGraphicLineCapSquare, kGraphicLineJoinRound,
                           {2.0f, 2.0f}))));

    // Clear the opacity
    graphic->setProperty("opacity", 0);
    graphic->updateSceneGraph(updates);

    ASSERT_FALSE(node->visible());
}


static const char *BASIC_GROUP = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "items": {
        "type": "group",
        "item": {
          "type": "path",
          "fill": "blue",
          "pathData": "M0,0 L100,50 L50,100 z"
        }
      }
    }
)apl";


TEST_F(SGGraphicTest, BasicGroup)
{
    loadGraphic(BASIC_GROUP);
    auto node = graphic->getSceneGraph(updates);

    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsOpacityNode("...opacity")
            .child(IsTransformNode().child(
                IsClipNode("...clip")
                    .path(IsGeneralPath("", {}))
                    .child(IsDrawNode("...draw")
                               .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                               .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));
}


static const char *FULL_GROUP = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "items": {
        "type": "group",
        "clipPath": "M0,50 l50,-50 l50,50 l-50,50 z",
        "opacity": 0.5,
        "transform": "rotate(45)",
        "item": {
          "type": "path",
          "fill": "blue",
          "pathData": "M0,0 L100,50 L50,100 z"
        }
      }
    }
)apl";

TEST_F(SGGraphicTest, FullGroup)
{
    loadGraphic(FULL_GROUP);
    auto node = graphic->getSceneGraph(updates);

    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsOpacityNode().opacity(0.5f).child(
            IsTransformNode()
                .transform(Transform2D::rotate(45))
                .child(IsClipNode()
                           .path(IsGeneralPath("MLLLZ", {0, 50, 50, 0, 100, 50, 50, 100}))
                           .child(IsDrawNode()
                                      .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                      .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));
}


static const char *PARAMETERIZED_GROUP = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "parameters":
      [
        "clipPath",
        {
          "name": "opacity",
          "default": 0.5
        },
        "transform"
      ],
      "items": {
        "type": "group",
        "clipPath": "${clipPath}",
        "opacity": "${opacity}",
        "transform": "${transform}",
        "item": {
          "type": "path",
          "fill": "blue",
          "pathData": "M0,0 L100,50 L50,100 z"
        }
      }
    }
)apl";

TEST_F(SGGraphicTest, ParameterizedGroup)
{
    loadGraphic(PARAMETERIZED_GROUP);
    auto node = graphic->getSceneGraph(updates);

    ASSERT_TRUE(
        CheckSceneGraph(updates, node,
                        IsOpacityNode().opacity(0.5f).child(IsTransformNode().child(
                            IsClipNode()
                                .path(IsGeneralPath("", {}))
                                .child(IsDrawNode()
                                           .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                           .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));

    graphic->setProperty("clipPath", "M50,0 L100,100 L0,50 z");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(
        CheckSceneGraph(updates, node,
                        IsOpacityNode().opacity(0.5f).child(IsTransformNode().child(
                            IsClipNode()
                                .path(IsGeneralPath("MLLZ", {50, 0, 100, 100, 0, 50}))
                                .child(IsDrawNode()
                                           .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                           .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));

    graphic->setProperty("transform", "scale(2)");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsOpacityNode("..opacity")
            .opacity(0.5f)
            .child(IsTransformNode("..transform")
                       .transform(Transform2D::scale(2.0f))
                       .child(IsClipNode("..clip")
                                  .path(IsGeneralPath("MLLZ", {50, 0, 100, 100, 0, 50}))
                                  .child(IsDrawNode("..draw")
                                             .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                             .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));

    graphic->setProperty("opacity", 1.0);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsOpacityNode().child(
            IsTransformNode()
                .transform(Transform2D::scale(2.0f))
                .child(IsClipNode()
                           .path(IsGeneralPath("MLLZ", {50, 0, 100, 100, 0, 50}))
                           .child(IsDrawNode()
                                      .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                      .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));

    graphic->setProperty("opacity", 0.0);
    graphic->updateSceneGraph(updates);
    ASSERT_FALSE(node->visible());
}


static const char *MULTI_CHILD_ONE = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "parameters":
      [
        {
          "name": "opacity",
          "default": 0.5
        }
      ],
      "items":
      [
        {
          "type": "group"
        },
        {
          "type": "group",
          "item": {
            "type": "path",
            "fillOpacity": "${opacity}",
            "fill": "blue",
            "pathData": "M0,0 L100,50 L50,100 z"
          }
        }
      ]
    }
)apl";

TEST_F(SGGraphicTest, MultiChild)
{
    loadGraphic(MULTI_CHILD_ONE);
    auto node = graphic->getSceneGraph(updates);

    ASSERT_TRUE(
        CheckSceneGraph(updates, node,
                        IsOpacityNode().child(IsTransformNode().child(
                            IsClipNode()
                                .path(IsGeneralPath("", {}))
                                .child(IsDrawNode()
                                           .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                           .pathOp(IsFillOp(IsColorPaint(Color::BLUE, 0.5f))))))));

    graphic->setProperty("opacity", 0.0f);
    graphic->updateSceneGraph(updates);

    ASSERT_FALSE(node->visible());
}


static const char *BASIC_TEXT = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "items": {
        "type": "text",
        "fill": "red",
        "text": "Hello, World!",
        "fontSize": 10
      }
    }
)apl";

TEST_F(SGGraphicTest, BasicText)
{
    loadGraphic(BASIC_TEXT);
    auto node = graphic->getSceneGraph(updates);

    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{0, -8})
            .child(IsTextNode().text("Hello, World!").pathOp(IsFillOp(IsColorPaint(Color::RED))))));
}


static const char *COMPLICATED_TEXT = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "resources": {
        "gradients": {
          "FOO": {
            "type": "linear",
            "colorRange": [
              "red",
              "white"
            ],
            "inputRange": [
              0,
              1
            ],
            "angle": 90
          }
        }
      },
      "items": {
        "type": "text",
        "fill": "red",
        "fillOpacity": 0.5,
        "fillTransform": "translate(10,20)",
        "stroke": "@FOO",
        "strokeOpacity": 0.25,
        "strokeWidth": 2,
        "strokeTransform": "rotate(90)",
        "text": "Fill and Stroke",
        "fontSize": 10
      }
    }
)apl";

TEST_F(SGGraphicTest, ComplicatedText)
{
    loadGraphic(COMPLICATED_TEXT);
    auto node = graphic->getSceneGraph(updates);

    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{0, -8})
            .child(IsTextNode()
                       .text("Fill and Stroke")
                       .pathOp(IsFillOp(IsColorPaint(Color::RED, 0.5)))
                       .pathOp(IsStrokeOp(IsLinearGradientPaint({0, 1}, {Color::RED, Color::WHITE},
                                                                Gradient::GradientSpreadMethod::PAD,
                                                                true, {0, 0}, {1, 1}, 0.25f,
                                                                Transform2D::rotate(90.f)),
                                          2.0f)))));
}


static const char *PARAMETERIZED_TEXT = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "resources": {
        "gradients": {
          "FOO": {
            "type": "linear",
            "colorRange": [
              "red",
              "white"
            ],
            "inputRange": [
              0,
              1
            ],
            "angle": 90
          }
        }
      },
      "parameters": [
        "color",
        { "name": "opacity", "default": 1.0 },
        "transform",
        "text",
        { "name": "anchor", "default": "start" },
        { "name": "x", "default": 0 },
        { "name": "y", "default": 0 }
      ],
      "items": {
        "type": "text",
        "fill": "${color}",
        "fillOpacity": "${opacity}",
        "fillTransform": "${transform}",
        "text": "${text}",
        "textAnchor": "${anchor}",
        "x": "${x}",
        "y": "${y}",
        "fontSize": 10
      }
    }
)apl";

TEST_F(SGGraphicTest, ParameterizedText)
{
    loadGraphic(PARAMETERIZED_TEXT);
    auto node = graphic->getSceneGraph(updates);

    ASSERT_FALSE(node->visible());

    graphic->setProperty("color", Color::GREEN);
    graphic->setProperty("text", "Woof!");

    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(updates, node,
                                IsTransformNode(".transform")
                                    .translate(Point{0, -8})
                                    .child(IsTextNode(".text").text("Woof!").pathOp(
                                        IsFillOp(IsColorPaint(Color::GREEN))))));

    graphic->setProperty("opacity", 0.5f);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{0, -8})
            .child(IsTextNode().text("Woof!").pathOp(IsFillOp(IsColorPaint(Color::GREEN, 0.5f))))));

    graphic->setProperty("color", "@FOO");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{0, -8})
            .child(IsTextNode().text("Woof!").pathOp(IsFillOp(IsLinearGradientPaint(
                {0, 1}, {Color::RED, Color::WHITE}, Gradient::GradientSpreadMethod::PAD, true,
                {0, 0}, {1, 1}, 0.5f, Transform2D()))))));

    graphic->setProperty("transform", "translate(1,2)");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{0, -8})
            .child(IsTextNode().text("Woof!").pathOp(IsFillOp(IsLinearGradientPaint(
                {0, 1}, {Color::RED, Color::WHITE}, Gradient::GradientSpreadMethod::PAD, true,
                {0, 0}, {1, 1}, 0.5f, Transform2D::translate({1, 2})))))));

    graphic->setProperty("text", "");
    graphic->updateSceneGraph(updates);
    ASSERT_FALSE(node->visible());

    graphic->setProperty("text", "Once upon a time");
    graphic->setProperty("opacity", 0);
    graphic->updateSceneGraph(updates);
    ASSERT_FALSE(node->visible());

    graphic->setProperty("opacity", 1.0f);
    graphic->setProperty("color", "blue");
    graphic->setProperty("text", "123");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{0, -8})
            .child(IsTextNode().text("123").pathOp(IsFillOp(IsColorPaint(Color::BLUE))))));

    graphic->setProperty("x", 10);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{10, -8})
            .child(IsTextNode().text("123").pathOp(IsFillOp(IsColorPaint(Color::BLUE))))));

    graphic->setProperty("y", 20);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{10, 12})
            .child(IsTextNode().text("123").pathOp(IsFillOp(IsColorPaint(Color::BLUE))))));

    graphic->setProperty("anchor", "end");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{-20, 12})
            .child(IsTextNode().text("123").pathOp(IsFillOp(IsColorPaint(Color::BLUE))))));
}


static const char *PARAMETERIZED_TEXT_STROKE = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "resources": {
        "gradients": {
          "FOO": {
            "type": "linear",
            "colorRange": [
              "red",
              "white"
            ],
            "inputRange": [
              0,
              1
            ],
            "angle": 90
          }
        }
      },
      "parameters": [
        "color",
        { "name": "opacity", "default": 1.0 },
        { "name": "swidth", "default": 1.0 },
        "transform"
      ],
      "items": {
        "type": "text",
        "stroke": "${color}",
        "strokeOpacity": "${opacity}",
        "strokeTransform": "${transform}",
        "strokeWidth": "${swidth}",
        "fill": "transparent",
        "text": "HELLO",
        "fontSize": 10
      }
    }
)apl";


TEST_F(SGGraphicTest, ParameterizedTextStroke)
{
    loadGraphic(PARAMETERIZED_TEXT_STROKE);
    auto node = graphic->getSceneGraph(updates);

    ASSERT_FALSE(node->visible());

    graphic->setProperty("color", Color::GREEN);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(updates, node,
                                IsTransformNode(".transform")
                                    .translate(Point{0, -8})
                                    .child(IsTextNode(".text").text("HELLO").pathOp(
                                        IsStrokeOp(IsColorPaint(Color::GREEN), 1)))));

    graphic->setProperty("opacity", 0.5f);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{0, -8})
            .child(IsTextNode().text("HELLO").pathOp(IsStrokeOp(IsColorPaint(Color::GREEN, 0.5f), 1)))));

    graphic->setProperty("color", "@FOO");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{0, -8})
            .child(IsTextNode().text("HELLO").pathOp(IsStrokeOp(IsLinearGradientPaint(
                {0, 1}, {Color::RED, Color::WHITE}, Gradient::GradientSpreadMethod::PAD, true,
                {0, 0}, {1, 1}, 0.5f, Transform2D()), 1.0f)))));

    graphic->setProperty("transform", "translate(1,2)");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{0, -8})
            .child(IsTextNode().text("HELLO").pathOp(IsStrokeOp(IsLinearGradientPaint(
                {0, 1}, {Color::RED, Color::WHITE}, Gradient::GradientSpreadMethod::PAD, true,
                {0, 0}, {1, 1}, 0.5f, Transform2D::translate({1, 2})), 1.0f)))));

    graphic->setProperty("opacity", 1.0f);
    graphic->setProperty("color", "blue");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{0, -8})
            .child(IsTextNode().text("HELLO").pathOp(IsStrokeOp(IsColorPaint(Color::BLUE), 1)))));

    graphic->setProperty("swidth", 0);
    graphic->updateSceneGraph(updates);
    ASSERT_FALSE(node->visible());
}


static const char *SHADOW = R"apl(
{
  "type": "AVG",
  "version": "1.2",
  "height": 100,
  "width": 100,
  "parameters": [
    {
      "name": "COLOR",
      "default": "blue"
    }
  ],
  "items": {
    "type": "path",
    "fill": "${COLOR}",
    "pathData": "M10,10 h80 v80 h-80 z",
    "filters": {
      "type": "DropShadow",
      "color": "${COLOR}",
      "horizontalOffset": 3,
      "verticalOffset": 3,
      "radius": 5
    }
  }
}
)apl";


TEST_F(SGGraphicTest, Shadow)
{
    loadGraphic(SHADOW);
    auto node = graphic->getSceneGraph(updates);

    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsShadowNode()
            .shadowTest(IsShadow(Color::BLUE, Point{3, 3}, 5))
            .child(IsDrawNode()
                       .path(IsGeneralPath("MLLLZ", {10, 10, 90, 10, 90, 90, 10, 90}))
                       .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))));

    graphic->setProperty("COLOR", "red");
    graphic->updateSceneGraph(updates);

    // Note: For now the filter is not dynamic
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsShadowNode()
            .shadowTest(IsShadow(Color::BLUE, Point{3, 3}, 5))
            .child(IsDrawNode()
                       .path(IsGeneralPath("MLLLZ", {10, 10, 90, 10, 90, 90, 10, 90}))
                       .pathOp(IsFillOp(IsColorPaint(Color::RED))))));
}

// Use a custom test for checking the nodes.  The regular test_sg.h code skips over operations
// that are not visible. But we want to verify that we have exactly the correct operations.
static ::testing::AssertionResult
checkOps(sg::PathOp *op, bool hasFill, Color fillColor, bool hasStroke, Color strokeColor) {
    if (hasFill) {
        if (!sg::FillPathOp::is_type(op))
            return ::testing::AssertionFailure() << "Expected a fill operation";
        auto *fill = sg::FillPathOp::cast(op);
        auto *paint = sg::ColorPaint::cast(fill->paint);
        if (!paint || paint->getColor() != fillColor)
            return ::testing::AssertionFailure() << "Fill color mismatch";
        op = op->nextSibling.get();
    }

    if (hasStroke) {
        if (!sg::StrokePathOp::is_type(op))
            return ::testing::AssertionFailure() << "Missing stroke operation";
        auto *stroke = sg::StrokePathOp::cast(op);
        auto *paint = sg::ColorPaint::cast(stroke->paint);
        if (!paint || paint->getColor() != strokeColor)
            return ::testing::AssertionFailure() << "Stroke color mismatch";
        op = op->nextSibling.get();
    }

    if (op)
        return ::testing::AssertionFailure() << "Extra operation";

    return ::testing::AssertionSuccess();
}

static ::testing::AssertionResult
checkDraw(sg::NodePtr node, float x, bool hasFill, Color fillColor, bool hasStroke, Color strokeColor) {
    auto *draw = sg::DrawNode::cast(node);
    if (!draw)
        return ::testing::AssertionFailure() << "not a draw node";

    auto *path = sg::GeneralPath::cast(draw->getPath());
    if (!path)
        return ::testing::AssertionFailure() << "missing path node";

    // negative x used to indicate no points
    if (path->getPoints() != (x < 0 ? std::vector<float>{} : std::vector<float>{0,0,0,x,x,x}))
        return ::testing::AssertionFailure() << "mismatched points";

    return checkOps(draw->getOp().get(), hasFill, fillColor, hasStroke, strokeColor);
}

static ::testing::AssertionResult
checkText(sg::NodePtr node, std::string textString, bool hasFill, Color fillColor, bool hasStroke, Color strokeColor) {
    auto *text = sg::TextNode::cast(node);
    if (!text)
        return ::testing::AssertionFailure() << "not a text node";

    if (text->getTextLayout()->toDebugString() != textString)
        return ::testing::AssertionFailure() << "text mismatch";

    return checkOps(text->getOp().get(), hasFill, fillColor, hasStroke, strokeColor);
}

static const char *DRAW_OPTIMIZATION = R"apl(
{
  "type": "AVG",
  "version": "1.2",
  "height": 100,
  "width": 100,
  "parameters": [
    {
      "name": "X",
      "default": false
    }
  ],
  "items": [
    {
      "type": "path",
      "description": "Empty path",
      "fill": "red",
      "pathData": "M10,10 M20,20"
    },
    {
      "type": "path",
      "description": "Just fill",
      "fill": "blue",
      "pathData": "M0,0 L0,1 L1,1 z"
    },
    {
      "type": "path",
      "description": "Just stroke",
      "stroke": "red",
      "pathData": "M0,0 L0,2 L2,2 z"
    },
    {
      "type": "path",
      "description": "Stroke, but no width",
      "stroke": "green",
      "strokeWidth": 0,
      "pathData": "M0,0 L0,3 L3,3 z"
    },
    {
      "type": "path",
      "description": "Stroke and fill",
      "stroke": "yellow",
      "fill": "black",
      "strokeWidth": 5,
      "pathData": "M0,0 L0,4 L4,4 z"
    },
    {
      "type": "path",
      "description": "Stroke and fill opacity zero",
      "stroke": "pink",
      "strokeOpacity": 0,
      "fill": "blue",
      "fillOpacity": 0,
      "strokeWidth": 5,
      "pathData": "M0,0 L0,5 L5,5 z"
    },
    {
      "type": "path",
      "description": "Path depends on X",
      "pathData": "${X ? 'M0,0 L0,6 L6,6 z' : 'M0,0'}",
      "fill": "purple"
    },
    {
      "type": "path",
      "description": "Fill depends on X",
      "pathData": "M0,0 L0,7 L7,7 z",
      "fill": "${X ? 'blue' : 'transparent'}"
    },
    {
      "type": "path",
      "description": "Stroke depends on X",
      "pathData": "M0,0 L0,8 L8,8 z",
      "stroke": "${X ? 'red' : 'transparent'}"
    }
  ]
}
)apl";

TEST_F(SGGraphicTest, DrawOptimization)
{
    loadGraphic(DRAW_OPTIMIZATION);
    auto node = graphic->getSceneGraph(updates);

    // Skip the empty path - there is no path data

    // Fill blue
    ASSERT_TRUE(checkDraw(node, 1, true, Color::BLUE, false, Color::TRANSPARENT));
    node = node->next();

    // Stroke red
    ASSERT_TRUE(checkDraw(node, 2, false, Color::TRANSPARENT, true, Color::RED));
    node = node->next();

    // Skip the stroke with green because there is no stroke width

    // Stroke yellow, fill black
    ASSERT_TRUE(checkDraw(node, 4, true, Color::BLACK, true, Color::YELLOW));
    node = node->next();

    // Skip stroke pink, fill blue because the opacities hide all colors

    // Allow the fill purple even though there is no path because the path is mutable
    ASSERT_TRUE(checkDraw(node, -1, true, Color::PURPLE, false, Color::TRANSPARENT));
    node = node->next();

    // Allow the 7th case - the fill color can be changed
    ASSERT_TRUE(checkDraw(node, 7, true, Color::TRANSPARENT, false, Color::TRANSPARENT));
    node = node->next();

    // Allow the 8th case - the stroke color can be changed
    ASSERT_TRUE(checkDraw(node, 8, false, Color::TRANSPARENT, true, Color::TRANSPARENT));
    node = node->next();

    ASSERT_FALSE(node);
}


static const char *TEXT_OPTIMIZATION = R"apl(
{
  "type": "AVG",
  "version": "1.2",
  "height": 100,
  "width": 100,
  "parameters": [
    {
      "name": "X",
      "default": false
    }
  ],
  "items": [
    {
      "type": "text",
      "text": "Just fill",
      "fill": "red"
    },
    {
      "type": "text",
      "text": "Just stroke",
      "stroke": "yellow",
      "fillOpacity": 0,
      "strokeWidth": 1
    },
    {
      "type": "text",
      "text": "Stroke and fill",
      "stroke": "green",
      "strokeWidth": 2,
      "fill": "blue"
    },
    {
      "type": "text",
      "text": "Nothing to draw",
      "fillOpacity": 0
    },
    {
      "type": "text",
      "text": "",
      "fill": "purple"
    },
    {
      "type": "text",
      "text": "Default"
    }
  ]
}
)apl";

TEST_F(SGGraphicTest, TextOptimization)
{
    loadGraphic(TEXT_OPTIMIZATION);
    auto node = graphic->getSceneGraph(updates);

    // Fill red (well, it's hidden under a transform)
    ASSERT_TRUE(checkText(node->child(), "Just fill", true, Color::RED, false, Color::TRANSPARENT));
    node = node->next();

    // Stroke yellow
    ASSERT_TRUE(checkText(node->child(), "Just stroke", false, Color::TRANSPARENT, true, Color::YELLOW));
    node = node->next();

    // Stroke green, fill blue
    ASSERT_TRUE(checkText(node->child(), "Stroke and fill", true, Color::BLUE, true, Color::GREEN));
    node = node->next();

    // Skip the "Nothing to draw" - there is no fill or stroke

    // Fill purple (even though there is no text to draw.  Fix this?)
    ASSERT_TRUE(checkText(node->child(), "", true, Color::PURPLE, false, Color::TRANSPARENT));
    node = node->next();

    // Fill with black (the default color)
    ASSERT_TRUE(checkText(node->child(), "Default", true, Color::BLACK, false, Color::TRANSPARENT));
    node = node->next();

    ASSERT_FALSE(node);
}
// TODO: Check style changes - they should update properties as appropriate