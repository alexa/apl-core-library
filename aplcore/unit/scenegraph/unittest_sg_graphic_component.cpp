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

using namespace apl;

class SGGraphicComponentTest : public DocumentWrapper {
public:
    SGGraphicComponentTest() : DocumentWrapper() {
        config->measure(std::make_shared<MyTestMeasurement>());
    }
};

static const char * BASIC = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "graphics": {
        "Diamond": {
          "type": "AVG",
          "version": "1.2",
          "height": 100,
          "width": 100,
          "items": {
            "type": "path",
            "fill": "green",
            "pathData": "M0,50 L50,0 L100,50 L50,100 z"
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "id": "VG",
          "width": 200,
          "height": 200,
          "source": "Diamond"
        }
      }
    }
)apl";

TEST_F(SGGraphicComponentTest, Basic)
{
    loadDocument(BASIC);
    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 200, 200}, ".VectorGraphic")
            .child(IsLayer(Rect{50, 50, 100, 100}, ".MediaLayer")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly)
                       .content(IsTransformNode(".transform")
                                    .child(IsDrawNode(".draw")
                                               .path(IsGeneralPath(
                                                   "MLLLZ", {0, 50, 50, 0, 100, 50, 50, 100}))
                                               .pathOp(IsFillOp(IsColorPaint(Color::GREEN))))))));

    executeCommand("SetValue",
                   {{"componentId", "VG"}, {"property", "align"}, {"value", "top-right"}}, true);
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200}, ".VectorGraphic")
                .child(IsLayer(Rect{100, 0, 100, 100}, ".MediaLayer")
                           .characteristic(sg::Layer::kCharacteristicRenderOnly)
                           .dirty(sg::Layer::kFlagPositionChanged)
                           .content(IsTransformNode().child(
                               IsDrawNode()
                                   .path(IsGeneralPath("MLLLZ", {0, 50, 50, 0, 100, 50, 50, 100}))
                                   .pathOp(IsFillOp(IsColorPaint(Color::GREEN))))))));
}


static const char * MISSING = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "id": "VG",
          "width": 200,
          "height": 200,
          "source": "Wrong"
        }
      }
    }
)apl";

TEST_F(SGGraphicComponentTest, Missing)
{
    loadDocument(MISSING);
    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 200, 200}, ".VectorGraphic")
            .child(IsLayer(Rect{0, 0, 1, 1}, ".MediaLayer")
                    .characteristic(sg::Layer::kCharacteristicRenderOnly))));  // No content is visible
}

static const char * TOGGLE_OFF_AND_ON = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "graphics": {
        "Diamond": {
          "type": "AVG",
          "version": "1.2",
          "height": 100,
          "width": 100,
          "items": {
            "type": "path",
            "fill": "green",
            "pathData": "M0,50 L50,0 L100,50 L50,100 z"
          }
        },
        "Bad": {
          "type": "AVG",
          "version": "1.2",
          "height": 0,
          "width": 0
        },
        "Empty": {
          "type": "AVG",
          "version": "1.2",
          "height": 100,
          "width": 100
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "id": "VG",
          "width": 200,
          "height": 200,
          "source": "Diamond"
        }
      }
    }
)apl";

TEST_F(SGGraphicComponentTest, ToggleOffAndOn)
{
    loadDocument(TOGGLE_OFF_AND_ON);
    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 200, 200}, ".VectorGraphic")
            .child(IsLayer(Rect{50, 50, 100, 100}, ".MediaLayer")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly)
                       .content(IsTransformNode(".transform")
                                    .child(IsDrawNode(".draw")
                                               .path(IsGeneralPath(
                                                   "MLLLZ", {0, 50, 50, 0, 100, 50, 50, 100}))
                                               .pathOp(IsFillOp(IsColorPaint(Color::GREEN))))))));

    // Set an invalid vector graphic
    executeCommand("SetValue",
                   {{"componentId", "VG"}, {"property", "source"}, {"value", "Missing"}}, true);
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200}, ".VectorGraphic")
                .child(IsLayer(Rect{50, 50, 100, 100}, ".MediaLayer")
                           .characteristic(sg::Layer::kCharacteristicRenderOnly)
                           .dirty(sg::Layer::kFlagRedrawContent)))); // No content is visible

    // Set to a bad vector graphic - one with an illegal height/width
    executeCommand("SetValue",
                   {{"componentId", "VG"}, {"property", "source"}, {"value", "Bad"}}, true);
    ASSERT_TRUE(ConsoleMessage());  // It should complain about the invalid graphic
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200}, ".VectorGraphic")
                .child(IsLayer(Rect{50, 50, 100, 100}, ".MediaLayer")
                        .characteristic(sg::Layer::kCharacteristicRenderOnly))));

    // Set to an empty vector graphic - one with no content
    executeCommand("SetValue",
                   {{"componentId", "VG"}, {"property", "source"}, {"value", "Empty"}}, true);
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200}, ".VectorGraphic")
                .child(IsLayer(Rect{50, 50, 100, 100}, ".MediaLayer")
                          .characteristic(sg::Layer::kCharacteristicRenderOnly))));

    // Set it back to the original
    executeCommand("SetValue",
                   {{"componentId", "VG"}, {"property", "source"}, {"value", "Diamond"}}, true);
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 200, 200}, ".VectorGraphic")
            .child(IsLayer(Rect{50, 50, 100, 100}, ".MediaLayer")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly)
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .content(IsTransformNode(".transform")
                                    .child(IsDrawNode(".draw")
                                               .path(IsGeneralPath(
                                                   "MLLLZ", {0, 50, 50, 0, 100, 50, 50, 100}))
                                               .pathOp(IsFillOp(IsColorPaint(Color::GREEN))))))));
}

static const char *MULTI_TEXT = R"apl(
{
  "type": "APL",
  "version": "1.6",
  "graphics": {
    "arcs": {
      "type": "AVG",
      "version": "1.1",
      "width": 600,
      "height": 600,
      "items": {
        "type": "text",
        "stroke": "blue",
        "strokeWidth": 2,
        "text": "Hello, world!",
        "x": 10,
        "y": 100,
        "fontSize": 10
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "width": "800",
      "height": "800",
      "items": {
        "type": "VectorGraphic",
        "source": "arcs",
        "scale": "best-fit",
        "width": "100%",
        "height": "100%"
      }
    }
  }
}
)apl";


TEST_F(SGGraphicComponentTest, MultiText)
{
    loadDocument(MULTI_TEXT);
    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 800, 800}, "...Frame")
            .child(IsLayer(Rect{0, 0, 800, 800}, "...VectorGraphic")
                       .child(IsLayer(Rect{0, 0, 800, 800}, "...Graphic")
                                  .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicHasText)
                                  .content(IsTransformNode()
                                               .transform(Transform2D::scale(4. / 3.))
                                               .child(IsTransformNode()
                                                          .translate(Point{10, 92})
                                                          .child(IsTextNode()
                                                                     .text("Hello, world!")
                                                                     .pathOp(IsFillOp(IsColorPaint(
                                                                         Color::BLACK)))
                                                                     .pathOp(IsStrokeOp(
                                                                         IsColorPaint(Color::BLUE),
                                                                         2)))))))));
}

static const char *MOVING = R"apl(
{
  "type": "APL",
  "version": "1.8",
  "graphics": {
    "Box": {
      "type": "AVG",
      "version": "1.1",
      "parameters": [
        "X"
      ],
      "width": 200,
      "height": 200,
      "items": {
        "type": "group",
        "transform": "translate(${X},0)",
        "items": {
          "type": "path",
          "fill": "blue",
          "pathData": "M0,0 h10 v10 h-10 z"
        }
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "Box",
      "width": 200,
      "height": 200,
      "X": "${elapsedTime}"
    }
  }
}
)apl";

TEST_F(SGGraphicComponentTest, Moving)
{
    loadDocument(MOVING);
    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 200, 200})
            .child(
                IsLayer(Rect{0, 0, 200, 200})
                    .characteristic(sg::Layer::kCharacteristicRenderOnly)
                    .content(IsTransformNode(".alignment")
                                 .child(IsTransformNode(".group").child(
                                     IsDrawNode()
                                         .path(IsGeneralPath("MLLLZ", {0, 0, 10, 0, 10, 10, 0, 10}))
                                         .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))))));

    root->updateTime(100);
    root->clearPending();

    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 200, 200}, "...vector graphic")
            .child(
                IsLayer(Rect{0, 0, 200, 200}, "...media layer")
                    .characteristic(sg::Layer::kCharacteristicRenderOnly)
                    .dirty(sg::Layer::kFlagRedrawContent)
                    .content(IsTransformNode(".alignment")
                                 .child(IsTransformNode(".group").translate({100, 0}).child(
                                     IsDrawNode()
                                         .path(IsGeneralPath("MLLLZ", {0, 0, 10, 0, 10, 10, 0, 10}))
                                         .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))))));
}

/**
 * Turning on the experimental feature to generate layers for parameterized AVG puts the
 * group in its own layer.
 */
TEST_F(SGGraphicComponentTest, MovingLayers)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureGraphicLayers);
    loadDocument(MOVING);
    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200}, "...vector graphic")
                .child(IsLayer(Rect{0, 0, 200, 200}, "...media layer")
                           .characteristic(sg::Layer::kCharacteristicRenderOnly)
                           .child(IsLayer(Rect(0,0,0,0), "...container")
                                      .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                                      .child(IsLayer(Rect(0,0,10,10), "...group")
                                                 .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                                                 .content(
                                                     IsDrawNode()
                                                         .path(IsGeneralPath(
                                                             "MLLLZ", {0, 0, 10, 0, 10, 10, 0, 10}))
                                                         .pathOp(IsFillOp(
                                                             IsColorPaint(Color::BLUE)))))))));

    root->updateTime(100);
    root->clearPending();

    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .child(IsLayer(Rect{0, 0, 200, 200})
                           .characteristic(sg::Layer::kCharacteristicRenderOnly)
                           .child(IsLayer(Rect(0,0,0,0), "...container")
                                      .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                                      .child(IsLayer(Rect(0,0,10,10), "...group")
                                                 .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                                                 .dirty(sg::Layer::kFlagTransformChanged)
                                                 .transform(Transform2D::translate({100,0}))
                                                 .content(
                                                     IsDrawNode()
                                                         .path(IsGeneralPath(
                                                             "MLLLZ", {0, 0, 10, 0, 10, 10, 0, 10}))
                                                         .pathOp(IsFillOp(
                                                             IsColorPaint(Color::BLUE)))))))));
}


static const char *REPLACE_SOURCE = R"apl(
{
  "type": "APL",
  "version": "2022.1",
  "graphics": {
    "BlueBox": {
      "type": "AVG",
      "version": "1.2",
      "width": 200,
      "height": 200,
      "items": {
        "type": "path",
        "fill": "blue",
        "pathData": "h200 v200 h-200 z"
      }
    },
    "RedBox": {
      "type": "AVG",
      "version": "1.2",
      "width": 200,
      "height": 200,
      "items": {
        "type": "path",
        "fill": "red",
        "pathData": "h200 v200 h-200 z"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "BlueBox",
      "scale": "best-fit",
      "width": "200",
      "height": "200"
    }
  }
}
)apl";

TEST_F(SGGraphicComponentTest, ReplaceSource)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureGraphicLayers);
    loadDocument(REPLACE_SOURCE);
    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 200, 200}, "...vector graphic")
            .child(IsLayer(Rect{0, 0, 200, 200}, "...media layer")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly)
                       .child(IsLayer(Rect(0, 0, 200, 200), "...container")
                                  .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                                  .content(IsDrawNode()
                                               .path(IsGeneralPath(
                                                   "MLLLZ", {0, 0, 200, 0, 200, 200, 0, 200}))
                                               .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));

    component->setProperty(apl::kPropertySource, "RedBox");
    ASSERT_TRUE(CheckDirty(component, kPropertySource, kPropertyGraphic, kPropertyVisualHash));

    root->clearPending();
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 200, 200}, "...vector graphic")
            .child(IsLayer(Rect{0, 0, 200, 200}, "...media layer")
                       .characteristic(sg::Layer::kCharacteristicRenderOnly)
                       .dirty(sg::Layer::kFlagChildrenChanged)
                       .child(IsLayer(Rect(0, 0, 200, 200), "...container")
                                  .characteristic(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren)
                                  .content(IsDrawNode()
                                               .path(IsGeneralPath(
                                                   "MLLLZ", {0, 0, 200, 0, 200, 200, 0, 200}))
                                               .pathOp(IsFillOp(IsColorPaint(Color::RED))))))));
}