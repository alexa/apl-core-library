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
        gc = nullptr;
        updates.clear();
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
        "pathData": "M10,10 L100,10 L100,100 L10,100 z"
      }
    }
)apl";

TEST_F(SGGraphicTest, BasicRect)
{
    loadGraphic(BASIC_RECT);

    auto node = graphic->getSceneGraph(false, updates)->node();
    ASSERT_TRUE(CheckSceneGraph(updates, node,
                                IsDrawNode()
                                    .path(IsGeneralPath("MLLLZ", {10, 10, 100, 10, 100, 100, 10, 100}))
                                    .pathOp(IsFillOp(IsColorPaint(Color::RED)))));
}

TEST_F(SGGraphicTest, BasicRectLayers)
{
    loadGraphic(BASIC_RECT);

    auto sg = graphic->getSceneGraph(true, updates);
    updates.clear();
    ASSERT_TRUE(CheckSceneGraph(
        updates, sg->layer(),
        IsLayer(Rect(10, 10, 90, 90))
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .contentOffset(Point(10, 10))
            .content(IsDrawNode()
                         .path(IsGeneralPath("MLLLZ", {10, 10, 100, 10, 100, 100, 10, 100}))
                         .pathOp(IsFillOp(IsColorPaint(Color::RED))))));
}

static const char *ILLEGAL_GRAPHIC = R"apl(
    {
      "type": "AVG",
      "version": "2.3",
      "height": 100,
      "width": 100,
      "items": {
        "type": "path",
        "fill": "red",
        "pathData": "M10,10 L100,10 L100,100 L10,100 z"
      }
    }
)apl";

TEST_F(SGGraphicTest, IllegalGraphic)
{
    gc = GraphicContent::create(session, ILLEGAL_GRAPHIC);
    ASSERT_FALSE(gc);
    ASSERT_TRUE(ConsoleMessage());
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
    auto node = graphic->getSceneGraph(false, updates)->node();
    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsDrawNode()
            .path(IsGeneralPath("MLLLZ", {0, 0, 100, 0, 100, 100, 0, 100}))
            .pathOp(IsFillOp(IsColorPaint(Color::RED)))
            .next(IsDrawNode()
                      .path(IsGeneralPath("MLLLZ", {20, 20, 60, 20, 60, 60, 20, 60}))
                      .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))));
}

TEST_F(SGGraphicTest, TwoRectsLayers)
{
    loadGraphic(TWO_RECTS);

    auto sg = graphic->getSceneGraph(true, updates);
    updates.clear();
    ASSERT_TRUE(CheckSceneGraph(
        updates, sg->layer(),
        IsLayer(Rect(0, 0, 100, 100))
          .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
          .content(IsDrawNode()
                .path(IsGeneralPath("MLLLZ", {0, 0, 100, 0, 100, 100, 0, 100}))
                .pathOp(IsFillOp(IsColorPaint(Color::RED)))
                .next(IsDrawNode()
                          .path(IsGeneralPath("MLLLZ", {20, 20, 60, 20, 60, 60, 20, 60}))
                          .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))));
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
    auto node = graphic->getSceneGraph(false, updates)->node();

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

TEST_F(SGGraphicTest, ComplicatedRectLayers)
{
    loadGraphic(COMPLICATED_RECT);
    auto sg = graphic->getSceneGraph(true, updates);
    updates.clear();
    ASSERT_TRUE(CheckSceneGraph(
        updates, sg->layer(),
        IsLayer(Rect(-1, -1, 102, 102))
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .contentOffset(Point(-1,-1))
            .content(
            IsDrawNode()
                .path(IsGeneralPath("MLLLZ", {0, 0, 100, 0, 100, 100, 0, 100}))
                .pathOp({IsFillOp(IsColorPaint(Color::RED, 0.5)),
                         IsStrokeOp(IsLinearGradientPaint({0, 1}, {Color::RED, Color::WHITE},
                                                          Gradient::GradientSpreadMethod::PAD, true,
                                                          {0, 0}, {1, 1}, 0.25f,
                                                          Transform2D::rotate(90.f)),
                                    2.0f, 10.0f, 100.0f, 1.0f, GraphicLineCap::kGraphicLineCapRound,
                                    GraphicLineJoin::kGraphicLineJoinRound,
                                    {1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f})

                }))));
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
        { "name": "strokeWidth", "default": 1.0 },
        { "name": "opacity", "default": 1.0 },
        "transform",
        { "name": "pathLength", "default": 10.0 },
        { "name": "dashArray", "default": [] },
        { "name": "dashOffset", "default": 0 },
        "lineCap",
        "lineJoin",
        { "name": "miterLimit", "default": "5.0" },
        "path"
      ],
      "items": {
        "type": "path",
        "stroke": "${color}",
        "strokeWidth": "${strokeWidth}",
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
    auto result = graphic->getSceneGraph(false, updates);
    auto node = result->node();
    ASSERT_FALSE(node->visible());

    graphic->setProperty("color", Color::GREEN);
    graphic->setProperty("path", "M0,0 L100,100");
    graphic->setProperty("opacity", 1.0f);
    graphic->setProperty("strokeWidth", 1.0f);

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

TEST_F(SGGraphicTest, ParameterizedLayers)
{
    loadGraphic(PARAMETERIZED);
    auto result = graphic->getSceneGraph(true, updates);
    updates.clear();
    auto layer = result->layer();

    graphic->setProperty("color", Color::GREEN);
    graphic->setProperty("path", "M0,0 L100,100");
    graphic->setProperty("opacity", 1.0f);

    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-2.5f, -2.5f, 105, 105),
                           "...path") // Miter limit leaves 5 unit padding
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent | sg::Layer::kFlagPositionChanged |
                              sg::Layer::kFlagSizeChanged | sg::Layer::kFlagChildOffsetChanged)
                       .contentOffset(Point(-2.5f, -2.5f))
                       .childOffset(Point(-2.5f, -2.5f))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                                    .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f,
                                                       10.0f, 0.0f, kGraphicLineCapButt,
                                                       kGraphicLineJoinMiter, {}))))));

    graphic->setProperty("lineJoin", "round");  // This will change the size of the layer
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-0.5f, -0.5f, 101, 101), "...path")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent | sg::Layer::kFlagPositionChanged |
                              sg::Layer::kFlagSizeChanged | sg::Layer::kFlagChildOffsetChanged)
                       .contentOffset(Point(-0.5f, -0.5f))
                       .childOffset(Point(-0.5f, -0.5f))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                                    .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f,
                                                       10.0f, 0.0f, kGraphicLineCapButt,
                                                       kGraphicLineJoinRound, {}))))));

    graphic->setProperty("pathLength", 20);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-0.5f, -0.5f, 101, 101), "...path")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(Point(-0.5f, -0.5f))
                       .childOffset(Point(-0.5f, -0.5f))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                                    .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f,
                                                       20.0f, 0.0f, kGraphicLineCapButt,
                                                       kGraphicLineJoinRound, {}))))));

    graphic->setProperty("dashArray", std::vector<Object>(2, 2));
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-0.5f, -0.5f, 101, 101), "...path")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(Point(-0.5f, -0.5f))
                       .childOffset(Point(-0.5f, -0.5f))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                                    .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f,
                                                       20.0f, 0.0f, kGraphicLineCapButt,
                                                       kGraphicLineJoinRound, {2.0f, 2.0f}))))));

    graphic->setProperty("dashOffset", 1.5f);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-0.5f, -0.5f, 101, 101), "...path")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(Point(-0.5f, -0.5f))
                       .childOffset(Point(-0.5f, -0.5f))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                                    .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f,
                                                       20.0f, 1.5f, kGraphicLineCapButt,
                                                       kGraphicLineJoinRound, {2.0f, 2.0f}))))));

    graphic->setProperty("lineCap", "square");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-0.5f, -0.5f, 101, 101), "...path")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(Point(-0.5f, -0.5f))
                       .childOffset(Point(-0.5f, -0.5f))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                                    .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 5.0f,
                                                       20.0f, 1.5f, kGraphicLineCapSquare,
                                                       kGraphicLineJoinRound, {2.0f, 2.0f}))))));

    graphic->setProperty("miterLimit", 23.0f);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-0.5f, -0.5f, 101, 101), "...path")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(Point(-0.5f, -0.5f))
                       .childOffset(Point(-0.5f, -0.5f))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                                    .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN), 1.0f, 23.0f,
                                                       20.0f, 1.5f, kGraphicLineCapSquare,
                                                       kGraphicLineJoinRound, {2.0f, 2.0f}))))));

    graphic->setProperty("opacity", 0.5f);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-0.5f, -0.5f, 101, 101), "...path")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(Point(-0.5f, -0.5f))
                       .childOffset(Point(-0.5f, -0.5f))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                                    .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN, 0.5f), 1.0f,
                                                       23.0f, 20.0f, 1.5f, kGraphicLineCapSquare,
                                                       kGraphicLineJoinRound, {2.0f, 2.0f}))))));

    // Update the transform - but color paint doesn't use transform, so nothing changes
    // However, the draw node is marked as modified because the transform did actually change
    graphic->setProperty("transform", "translate(1 2)");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-0.5f, -0.5f, 101, 101), "...path")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(Point(-0.5f, -0.5f))
                       .childOffset(Point(-0.5f, -0.5f))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                                    .pathOp(IsStrokeOp(IsColorPaint(Color::GREEN, 0.5f), 1.0f,
                                                       23.0f, 20.0f, 1.5f, kGraphicLineCapSquare,
                                                       kGraphicLineJoinRound, {2.0f, 2.0f}))))));

    // Assign a gradient.  This will pick up the translate
    graphic->setProperty("color", "@FOO");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-0.5f, -0.5f, 101, 101), "...path")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(Point(-0.5f, -0.5f))
                       .childOffset(Point(-0.5f, -0.5f))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("ML", {0, 0, 100, 100}))
                                    .pathOp(IsStrokeOp(
                                        IsLinearGradientPaint({0, 1}, {Color::RED, Color::WHITE},
                                                              Gradient::GradientSpreadMethod::PAD,
                                                              true, {0, 0}, {1, 1}, 0.5f,
                                                              Transform2D::translate({1, 2})),
                                        1.0f, 23.0f, 20.0f, 1.5f, kGraphicLineCapSquare,
                                        kGraphicLineJoinRound, {2.0f, 2.0f}))))));

    // Clear the opacity
    graphic->setProperty("opacity", 0);
    graphic->updateSceneGraph(updates);

    ASSERT_FALSE(layer->visible());
}


static const char *BASIC_GROUP = R"apl(
    {
      "type": "AVG",
      "version": "1.2",
      "height": 100,
      "width": 100,
      "items": {
        "type": "group",
        "item": [
          {
            "type": "path",
            "fill": "blue",
            "pathData": "M0,0 L100,50 L50,100 z"
          },
          {
            "type": "path",
            "fill": "red",
            "pathData": "M10,10 L110,60 L60,110 z"
          }
        ]
      }
    }
)apl";


TEST_F(SGGraphicTest, BasicGroup)
{
    loadGraphic(BASIC_GROUP);
    auto node = graphic->getSceneGraph(false, updates)->node();

    ASSERT_TRUE(
        CheckSceneGraph(updates, node,
                        IsDrawNode("...draw")
                            .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                            .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))
                            .next(IsDrawNode("...path2")
                                      .path(IsGeneralPath("MLLZ", {10, 10, 110, 60, 60, 110}))
                                      .pathOp(IsFillOp(IsColorPaint(Color::RED))))));
}

TEST_F(SGGraphicTest, BasicGroupLayers)
{
    loadGraphic(BASIC_GROUP);
    auto layer = graphic->getSceneGraph(true, updates)->layer();
    updates.clear();
    ASSERT_TRUE(
        CheckSceneGraph(updates, layer,
                        IsLayer(Rect(0,0,110,110))
                          .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                          .content(IsDrawNode("...draw")
                                .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))
                                .next(IsDrawNode("...path2")
                                          .path(IsGeneralPath("MLLZ", {10, 10, 110, 60, 60, 110}))
                                          .pathOp(IsFillOp(IsColorPaint(Color::RED)))))));
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
    auto node = graphic->getSceneGraph(false, updates)->node();

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

TEST_F(SGGraphicTest, FullGroupLayer) {
    loadGraphic(FULL_GROUP);
    auto layer = graphic->getSceneGraph(true, updates)->layer();
    updates.clear();

    auto p = 50.0f / M_SQRT2; // The length of one-half of the side of the clipping square

    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(-p, p, 2 * p, 2 * p))
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .contentOffset(Point(-p, p))
            .content(IsOpacityNode().opacity(0.5f).child(
                IsTransformNode()
                    .transform(Transform2D::rotate(45))
                    .child(IsClipNode()
                               .path(IsGeneralPath("MLLLZ", {0, 50, 50, 0, 100, 50, 50, 100}))
                               .child(IsDrawNode()
                                          .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                          .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))))));
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
    auto node = graphic->getSceneGraph(false, updates)->node();

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

TEST_F(SGGraphicTest, ParameterizedGroupLayouts)
{
    loadGraphic(PARAMETERIZED_GROUP);
    auto layer = graphic->getSceneGraph(true, updates)->layer();
    updates.clear();

    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0,0,0,0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(0,0,100,100), "...group")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .opacity(0.5f) // Opacity pulled into the layer
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                    .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))));

    graphic->setProperty("clipPath", "M50,0 L100,100 L0,50 z");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0,0,0,0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(0,0,100,100), "...group")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagOutlineChanged)
                       .opacity(0.5f) // Opacity pulled into the layer
                       .outline(IsGeneralPath("MLLZ", {50, 0, 100, 100, 0, 50}))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                    .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))));

    graphic->setProperty("transform", "scale(2)");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0,0,0,0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(0,0,100,100), "...group")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagTransformChanged)
                       .opacity(0.5f) // Opacity pulled into the layer
                       .outline(IsGeneralPath("MLLZ", {50, 0, 100, 100, 0, 50}))
                       .transform(Transform2D::scale(2.0f))
                       .content(IsDrawNode("..draw")
                                    .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                    .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))));

    graphic->setProperty("opacity", 1.0);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0,0,0,0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(0,0,100,100), "...group")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagOpacityChanged)
                       .opacity(1.0f) // Opacity pulled into the layer
                       .outline(IsGeneralPath("MLLZ", {50, 0, 100, 100, 0, 50}))
                       .transform(Transform2D::scale(2.0f))
                       .content(IsDrawNode("..draw")
                                    .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                    .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))));

    graphic->setProperty("opacity", 0.0);
    graphic->updateSceneGraph(updates);
    ASSERT_FALSE(layer->visible());
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
    auto node = graphic->getSceneGraph(false, updates)->node();

    ASSERT_TRUE(CheckSceneGraph(updates, node,
                                IsDrawNode()
                                    .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                    .pathOp(IsFillOp(IsColorPaint(Color::BLUE, 0.5f)))));

    graphic->setProperty("opacity", 0.0f);
    graphic->updateSceneGraph(updates);

    ASSERT_FALSE(node->visible());
}

TEST_F(SGGraphicTest, MultiChildLayer)
{
    loadGraphic(MULTI_CHILD_ONE);
    auto layer = graphic->getSceneGraph(true, updates)->layer();
    updates.clear();
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0,0,0,0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(
                IsLayer(Rect(0,0,0,0), "...group")
                    .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                    .child(IsLayer(Rect(0,0,100,100), "...path")
                               .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                               .content(IsDrawNode()
                                            .path(IsGeneralPath("MLLZ", {0, 0, 100, 50, 50, 100}))
                                            .pathOp(IsFillOp(IsColorPaint(Color::BLUE, 0.5f))))))));

    graphic->setProperty("opacity", 0.0f);
    graphic->updateSceneGraph(updates);

    ASSERT_FALSE(layer->visible());
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
    auto node = graphic->getSceneGraph(false, updates)->node();

    ASSERT_TRUE(CheckSceneGraph(
        updates, node,
        IsTransformNode()
            .translate(Point{0, -8})
            .child(IsTextNode().text("Hello, World!").pathOp(IsFillOp(IsColorPaint(Color::RED))))));
}

TEST_F(SGGraphicTest, BasicTextLayer)
{
    loadGraphic(BASIC_TEXT);
    auto layer = graphic->getSceneGraph(true, updates)->layer();
    updates.clear();
    ASSERT_TRUE(
        CheckSceneGraph(updates, layer,
                        IsLayer(Rect(0, -8, 130, 10))
                            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                            .contentOffset(Point(0, -8))
                            .childOffset(Point(0, -8))
                            .content(IsTransformNode()
                                         .translate(Point{0, -8})
                                         .child(IsTextNode()
                                                    .text("Hello, World!")
                                                    .pathOp(IsFillOp(IsColorPaint(Color::RED)))))));
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
    auto node = graphic->getSceneGraph(false, updates)->node();

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

TEST_F(SGGraphicTest, ComplicatedTextLayer)
{
    loadGraphic(COMPLICATED_TEXT);
    auto layer = graphic->getSceneGraph(true, updates)->layer();
    updates.clear();
     // Text width = 15 characters * 10 per character + 2 strokeWidth * 4 miterlimit = 158
    // Text height = 10 + 2 strokeWidth * 4 miterLimit = 18
    // Vertical offset = 0.5 * (2 strokeWidth * 4 miterLimit) + 8 ascender = 12
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(-4, -12, 158, 18))
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .contentOffset(Point(-4, -12))
            .childOffset(Point(-4, -8))
            .content(IsTransformNode()
                         .translate(Point{0, -8})
                         .child(IsTextNode()
                                    .text("Fill and Stroke")
                                    .pathOp(IsFillOp(IsColorPaint(Color::RED, 0.5)))
                                    .pathOp(IsStrokeOp(
                                        IsLinearGradientPaint({0, 1}, {Color::RED, Color::WHITE},
                                                              Gradient::GradientSpreadMethod::PAD,
                                                              true, {0, 0}, {1, 1}, 0.25f,
                                                              Transform2D::rotate(90.f)),
                                        2.0f))))));
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
    auto node = graphic->getSceneGraph(false, updates)->node();

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


TEST_F(SGGraphicTest, ParameterizedTextLayout)
{
    loadGraphic(PARAMETERIZED_TEXT);
    auto layer = graphic->getSceneGraph(true, updates)->layer();
    updates.clear();

    ASSERT_FALSE(layer->visible());

    graphic->setProperty("color", Color::GREEN);
    graphic->setProperty("text", "Woof!");

    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(0, -8, 50, 10), "...text")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent | sg::Layer::kFlagPositionChanged |
                              sg::Layer::kFlagSizeChanged | sg::Layer::kFlagChildOffsetChanged)
                       .contentOffset(0, -8)
                       .content(IsTransformNode()
                                    .translate(Point(0, -8))
                                    .child(IsTextNode(".text").text("Woof!").pathOp(
                                        IsFillOp(IsColorPaint(Color::GREEN))))))));

    graphic->setProperty("opacity", 0.5f);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(0, -8, 50, 10), "...text")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(0, -8)
                       .content(IsTransformNode()
                                    .translate(Point(0, -8))
                                    .child(IsTextNode(".text").text("Woof!").pathOp(
                                        IsFillOp(IsColorPaint(Color::GREEN, 0.5f))))))));

    graphic->setProperty("color", "@FOO");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(0, -8, 50, 10), "...text")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(0, -8)
                       .content(IsTransformNode()
                                    .translate(Point(0, -8))
                                    .child(IsTextNode(".text").text("Woof!").pathOp(IsFillOp(
                                        IsLinearGradientPaint({0, 1}, {Color::RED, Color::WHITE},
                                                              Gradient::GradientSpreadMethod::PAD,
                                                              true, {0, 0}, {1, 1}, 0.5f,
                                                              Transform2D()))))))));

    graphic->setProperty("transform", "translate(1,2)");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(0, -8, 50, 10), "...text")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(0, -8)
                       .content(IsTransformNode()
                                    .translate(Point(0, -8))
                                    .child(IsTextNode(".text").text("Woof!").pathOp(IsFillOp(
                                        IsLinearGradientPaint({0, 1}, {Color::RED, Color::WHITE},
                                                              Gradient::GradientSpreadMethod::PAD,
                                                              true, {0, 0}, {1, 1}, 0.5f,
                                                              Transform2D::translate(1, 2)))))))));

    graphic->setProperty("text", "");
    graphic->updateSceneGraph(updates);
    ASSERT_FALSE(layer->visible());

    graphic->setProperty("text", "Once upon a time");
    graphic->setProperty("opacity", 0);
    graphic->updateSceneGraph(updates);
    ASSERT_FALSE(layer->visible());

    graphic->setProperty("opacity", 1.0f);
    graphic->setProperty("color", "blue");
    graphic->setProperty("text", "123");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(0, -8, 30, 10), "...text")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagChildOffsetChanged | sg::Layer::kFlagRedrawContent |
                              sg::Layer::kFlagSizeChanged | sg::Layer::kFlagPositionChanged)
                       .contentOffset(0, -8)
                       .content(IsTransformNode().translate(0, -8).child(
                           IsTextNode(".text").text("123").pathOp(
                               IsFillOp(IsColorPaint(Color::BLUE))))))));

    graphic->setProperty("x", 10);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(10, -8, 30, 10), "...text")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagChildOffsetChanged | sg::Layer::kFlagRedrawContent |
                              sg::Layer::kFlagPositionChanged)
                       .contentOffset(10, -8)
                       .content(IsTransformNode().translate(10, -8).child(
                           IsTextNode(".text").text("123").pathOp(
                               IsFillOp(IsColorPaint(Color::BLUE))))))));

    graphic->setProperty("y", 20);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(10, 12, 30, 10), "...text")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagChildOffsetChanged | sg::Layer::kFlagRedrawContent |
                              sg::Layer::kFlagPositionChanged)
                       .contentOffset(10, 12)
                       .content(IsTransformNode().translate(10, 12).child(
                           IsTextNode(".text").text("123").pathOp(
                               IsFillOp(IsColorPaint(Color::BLUE))))))));

    graphic->setProperty("anchor", "end");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-20, 12, 30, 10), "...text")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagChildOffsetChanged | sg::Layer::kFlagRedrawContent |
                              sg::Layer::kFlagPositionChanged)
                       .contentOffset(-20, 12)
                       .content(IsTransformNode().translate(-20, 12).child(
                           IsTextNode(".text").text("123").pathOp(
                               IsFillOp(IsColorPaint(Color::BLUE))))))));
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
    auto node = graphic->getSceneGraph(false, updates)->node();

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

TEST_F(SGGraphicTest, ParameterizedTextStrokeLayouts)
{
    loadGraphic(PARAMETERIZED_TEXT_STROKE);
    auto layer = graphic->getSceneGraph(true, updates)->layer();
    updates.clear();

    ASSERT_FALSE(layer->visible());

    graphic->setProperty("color", Color::GREEN);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(
        CheckSceneGraph(updates, layer,
                        IsLayer(Rect(0, 0, 0, 0), "...container")
                            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                            .child(IsLayer(Rect(-2, -10, 54, 14), "....text")
                                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                                       .dirty(sg::Layer::kFlagRedrawContent)
                                       .contentOffset(Point(-2, -10))
                                       .content(IsTransformNode().translate(0, -8).child(
                                           IsTextNode(".text").text("HELLO").pathOp(
                                               IsStrokeOp(IsColorPaint(Color::GREEN), 1)))))));

    graphic->setProperty("opacity", 0.5f);
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(
        CheckSceneGraph(updates, layer,
                        IsLayer(Rect(0, 0, 0, 0), "...container")
                            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                            .child(IsLayer(Rect(-2, -10, 54, 14), "....text")
                                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                                       .dirty(sg::Layer::kFlagRedrawContent)
                                       .contentOffset(Point(-2, -10))
                                       .content(IsTransformNode().translate(0, -8).child(
                                           IsTextNode(".text").text("HELLO").pathOp(IsStrokeOp(
                                               IsColorPaint(Color::GREEN, 0.5f), 1)))))));

    graphic->setProperty("color", "@FOO");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(-2, -10, 54, 14), "....text")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .contentOffset(Point(-2, -10))
                       .content(IsTransformNode().translate(0, -8).child(
                           IsTextNode(".text").text("HELLO").pathOp(IsStrokeOp(
                               IsLinearGradientPaint({0, 1}, {Color::RED, Color::WHITE},
                                                     Gradient::GradientSpreadMethod::PAD, true,
                                                     {0, 0}, {1, 1}, 0.5f, Transform2D()),
                               1.0f)))))));

    graphic->setProperty("transform", "translate(1,2)");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0, 0, 0, 0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(
                IsLayer(Rect(-2, -10, 54, 14), "....text")
                    .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                    .dirty(sg::Layer::kFlagRedrawContent)
                    .contentOffset(Point(-2, -10))
                    .content(IsTransformNode().translate(0, -8).child(
                        IsTextNode(".text").text("HELLO").pathOp(IsStrokeOp(
                            IsLinearGradientPaint({0, 1}, {Color::RED, Color::WHITE},
                                                  Gradient::GradientSpreadMethod::PAD, true, {0, 0},
                                                  {1, 1}, 0.5f, Transform2D::translate(1, 2)),
                            1.0f)))))));

    graphic->setProperty("opacity", 1.0f);
    graphic->setProperty("color", "blue");
    graphic->updateSceneGraph(updates);
    ASSERT_TRUE(
        CheckSceneGraph(updates, layer,
                        IsLayer(Rect(0, 0, 0, 0), "...container")
                            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                            .child(IsLayer(Rect(-2, -10, 54, 14), "....text")
                                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                                       .dirty(sg::Layer::kFlagRedrawContent)
                                       .contentOffset(Point(-2, -10))
                                       .content(IsTransformNode().translate(0, -8).child(
                                           IsTextNode(".text").text("HELLO").pathOp(
                                               IsStrokeOp(IsColorPaint(Color::BLUE), 1)))))));

    graphic->setProperty("swidth", 0);
    graphic->updateSceneGraph(updates);
    ASSERT_FALSE(layer->visible());
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
    auto node = graphic->getSceneGraph(false, updates)->node();

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

TEST_F(SGGraphicTest, ShadowLayers)
{
    loadGraphic(SHADOW);
    auto layer = graphic->getSceneGraph(true, updates)->layer();
    updates.clear();
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0,0,0,0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(10,10,80,80), "...path")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .shadow(IsShadow(Color::BLUE, Point{3, 3}, 5))
                       .contentOffset(Point(10,10))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("MLLLZ", {10, 10, 90, 10, 90, 90, 10, 90}))
                                    .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))));

    graphic->setProperty("COLOR", "red");
    graphic->updateSceneGraph(updates);

    // Note: The shadow color does not change despite the data-binding.  That is because graphic
    //       element filters are not dynamic.  This may change in the future.
    ASSERT_TRUE(CheckSceneGraph(
        updates, layer,
        IsLayer(Rect(0,0,0,0), "...container")
            .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
            .child(IsLayer(Rect(10,10,80,80), "...path")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .shadow(IsShadow(Color::BLUE, Point{3, 3}, 5))
                       .contentOffset(Point(10,10))
                       .content(IsDrawNode()
                                    .path(IsGeneralPath("MLLLZ", {10, 10, 90, 10, 90, 90, 10, 90}))
                                    .pathOp(IsFillOp(IsColorPaint(Color::RED)))))));
}

// TODO: Check style changes - they should update properties as appropriate