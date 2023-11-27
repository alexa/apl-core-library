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

class SGFrameTest : public DocumentWrapper {
public:
};

static const char* DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Frame"
    }
  }
})";

/**
 * A basic frame with no children, background, shadow, or fill should give an empty scene graph
 */
TEST_F(SGFrameTest, FrameDefaults)
{
    metrics.size(200,300);
    loadDocument(DEFAULT_DOC);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_FALSE(sg->getLayer()->visible());
}


static const char *BORDER = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Frame",
          "borderWidth": 10.0,
          "borderStrokeWidth": 4.0,
          "borderColor": "blue"
        }
      }
    }
)apl";

TEST_F(SGFrameTest, FrameWithBorder)
{
    metrics.size(200, 300);
    loadDocument(BORDER);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(sg->getLayer()->visible());
    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 200, 300})
                                .childClip(IsRoundRectPath(10, 10, 180, 280, 0))
                                .content(IsDrawNode()
                                             .path(IsFramePath(RoundedRect({0, 0, 200, 300}, 0), 4))
                                             .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))));
}


static const char *BORDER_AND_FILL = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Frame",
          "borderWidth": 10.0,
          "borderColor": "blue",
          "padding": 25,
          "backgroundColor": "white"
        }
      }
    }
)apl";

TEST_F(SGFrameTest, FrameWithBorderAndFill)
{
    metrics.size(200, 300);
    loadDocument(BORDER_AND_FILL);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 300}, "checking group")
                .childClip(IsRoundRectPath(10, 10, 180, 280, 0))
                .content(IsDrawNode()
                             .path(IsRoundRectPath(10, 10, 180, 280, 0))
                             .pathOp(IsFillOp(IsColorPaint(Color::WHITE)))
                             .next(IsDrawNode()
                                       .path(IsFramePath(RoundedRect({0, 0, 200, 300}, 0), 10))
                                       .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))));
}


static const char *NESTED_FRAMES = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Frame",
          "width": 200,
          "height": 400,
          "borderWidth": 10,
          "borderRadius": 4,
          "borderColor": "blue",
          "padding": 10,
          "item": {
            "type": "Frame",
            "width": "100%",
            "height": "100%",
            "backgroundColor": "green",
            "borderRadius": 15
          }
        }
      }
    }
)apl";

TEST_F(SGFrameTest, NestedFrames)
{
    loadDocument(NESTED_FRAMES);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 400}, " outer frame")
                .outline(IsRoundRectPath(0, 0, 200, 400, 4))
                .content(IsDrawNode()
                             .path(IsFramePath(RoundedRect({0, 0, 200, 400}, 4), 10))
                             .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))
                .childClip(IsRoundRectPath(10, 10, 180, 380, 0))
                .child(IsLayer(Rect{20, 20, 160, 360}, " inner frame")
                           .outline(IsRoundRectPath(0, 0, 160, 360, 15))
                           .content(IsDrawNode()
                                        .path(IsRoundRectPath(0, 0, 160, 360, 15))
                                        .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))))));
}

static const char *STACKED_FRAMES = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": 200,
          "height": 100,
          "items": {
            "type": "Frame",
            "padding": 10,
            "width": 40,
            "height": 50,
            "backgroundColor": "${data}"
          },
          "data": [
            "green",
            "blue",
            "red"
          ]
        }
      }
    }
)apl";

TEST_F(SGFrameTest, StackedFrames)
{
    loadDocument(STACKED_FRAMES);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 200, 100}, "...container")
                                .children({
                                    IsLayer(Rect{0, 0, 40, 50})
                                        .content(IsDrawNode()
                                                     .path(IsRoundRectPath(0, 0, 40, 50, 0))
                                                     .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))),
                                    IsLayer(Rect{0, 50, 40, 50})
                                        .content(IsDrawNode()
                                                     .path(IsRoundRectPath(0, 0, 40, 50, 0))
                                                     .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))),
                                    // Note that the third Frame is off the screen and is not drawn
                                })));
}

static const char *MODIFY_FRAMES = R"apl(
    {
      "type": "APL",
      "version": "1.8",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": 200,
          "height": 100,
          "id": "Container",
          "items": {
            "type": "Frame",
            "id": "${data}Frame",
            "padding": 10,
            "width": 40,
            "height": 50,
            "backgroundColor": "${data}"
          },
          "data": [
            "green",
            "blue",
            "red"
          ]
        }
      }
    }
)apl";

TEST_F(SGFrameTest, ModifyFrames) {
    loadDocument(MODIFY_FRAMES);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 200, 100}, "..container")
                                .children({
                                    IsLayer(Rect{0, 0, 40, 50}, "..frame1")
                                        .content(IsDrawNode()
                                                     .path(IsRoundRectPath(0, 0, 40, 50, 0))
                                                     .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))),
                                    IsLayer(Rect{0, 50, 40, 50}, "..frame2")
                                        .content(IsDrawNode()
                                                     .path(IsRoundRectPath(0, 0, 40, 50, 0))
                                                     .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))),
                                    // Note that the third Frame is off the screen and is not drawn
                                })));

    executeCommand(
        "SetValue",
        {{"componentId", "greenFrame"}, {"property", "backgroundColor"}, {"value", Color::FUCHSIA}},
        true);
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 100}, "..container")
                .children({
                    IsLayer(Rect{0, 0, 40, 50}, "..frame1")
                        .dirty(sg::Layer::kFlagRedrawContent)
                        .content(IsDrawNode()
                                     .path(IsRoundRectPath(0, 0, 40, 50, 0))
                                     .pathOp(IsFillOp(IsColorPaint(Color::FUCHSIA)))),
                    IsLayer(Rect{0, 50, 40, 50}, "..frame2")
                        .content(IsDrawNode()
                                     .path(IsRoundRectPath(0, 0, 40, 50, 0))
                                     .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))),
                    // Note that the third Frame is off the screen and is not drawn
                })));
}

static const char *MODIFY_GRADIENT_FRAMES = R"apl(
    {
      "type": "APL",
      "version": "1.8",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": 200,
          "height": 100,
          "id": "Container",
          "items": {
            "type": "Frame",
            "id": "${data}Frame",
            "padding": 10,
            "width": 40,
            "height": 50,
            "backgroundColor": "${data}"
          },
          "data": [
            "green",
            "blue",
            "red"
          ]
        }
      }
    }
)apl";

static const char *SWAP_TO_GRADIENT = R"apl([{
  "type": "SetValue",
  "componentId": "greenFrame",
  "property": "background",
  "value": {
    "type": "linear",
    "colorRange": [ "red", "white" ],
    "inputRange": [ 0, 1 ]
  }
}])apl";

TEST_F(SGFrameTest, ModifyGradientFrames) {
    loadDocument(MODIFY_GRADIENT_FRAMES);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(sg, IsLayer(Rect{0, 0, 200, 100}, "..container")
        .children({
            IsLayer(Rect{0, 0, 40, 50}, "..frame1")
                .content(IsDrawNode()
                             .path(IsRoundRectPath(0, 0, 40, 50, 0))
                             .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))),
            IsLayer(Rect{0, 50, 40, 50}, "..frame2")
                .content(IsDrawNode()
                             .path(IsRoundRectPath(0, 0, 40, 50, 0))
                             .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))),
            // Note that the third Frame is off the screen and is not drawn
        })));

    rapidjson::Document doc;
    doc.Parse(SWAP_TO_GRADIENT);
    executeCommands(apl::Object(doc), false);

    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(sg, IsLayer(Rect{0, 0, 200, 100}, "..container")
        .children({
            IsLayer(Rect{0, 0, 40, 50}, "..frame1")
                .dirty(sg::Layer::kFlagRedrawContent)
                .content(IsDrawNode()
                             .path(IsRoundRectPath(0, 0, 40, 50, 0))
                             .pathOp(IsFillOp(IsLinearGradientPaint({0, 1},
                                                                    {Color::RED, Color::WHITE},
                                                                    Gradient::GradientSpreadMethod::PAD,
                                                                    true, {0.5, 0}, {0.5, 1}, 1,
                                                                    Transform2D())))),
            IsLayer(Rect{0, 50, 40, 50}, "..frame2")
                .content(IsDrawNode()
                             .path(IsRoundRectPath(0, 0, 40, 50, 0))
                             .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))),
            // Note that the third Frame is off the screen and is not drawn
        })));
}


static const char *SHADOW = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Frame",
          "backgroundColor": "red",
          "borderRadius": 5,
          "width": 100,
          "height": 100,
          "shadowColor": "blue",
          "shadowVerticalOffset": 5,
          "shadowHorizontalOffset": 6,
          "shadowRadius": 10
        }
      }
    }
)apl";

TEST_F(SGFrameTest, Shadow)
{
    metrics.size(200, 300);
    loadDocument(SHADOW);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(sg->getLayer()->visible());
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 100, 100})
            .shadow(IsShadow(Color::BLUE, Point{6,5}, 10))
            .outline(IsRoundRectPath(0, 0, 100, 100, 5))
            .content(IsDrawNode()
                         .path(IsRoundRectPath(RoundedRect({0, 0, 100, 100}, 5)))
                         .pathOp(IsFillOp(IsColorPaint(Color::RED))))));
}


static const char * RESIZE = R"apl(
    {
      "type": "APL",
      "version": "1.9",
      "mainTemplate": {
        "item": {
          "type": "Frame",
          "borderWidth": 1,
          "borderColor": "red"
        }
      }
    }
)apl";

TEST_F(SGFrameTest, Resize) {
    metrics.size(300, 300);
    loadDocument(RESIZE);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 300, 300})
            .childClip(IsRoundRectPath(RoundedRect(Rect{1,1,298,298}, 0)))
            .content(IsDrawNode()
                         .path(IsFramePath(RoundedRect(Rect{0, 0, 300, 300}, 0), 1))
                         .pathOp(IsFillOp(IsColorPaint(Color::RED))))));

    // Resize the screen
    configChange(ConfigurationChange(200,200));
    root->clearPending();
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 200, 200})
            .dirty(sg::Layer::kFlagChildClipChanged | sg::Layer::kFlagSizeChanged | sg::Layer::kFlagRedrawContent)
            .childClip(IsRoundRectPath(RoundedRect(Rect{1,1,198,198}, 0)))
            .content(IsDrawNode()
                         .path(IsFramePath(RoundedRect(Rect{0, 0, 200, 200}, 0), 1))
                         .pathOp(IsFillOp(IsColorPaint(Color::RED))))));
}
