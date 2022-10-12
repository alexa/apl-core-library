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

using namespace apl;

class SGTouchTest : public DocumentWrapper {
};

static const char *TOUCH_WRAPPER = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "FRAME": {
          "values":
          [
            {
              "backgroundColor": "red"
            },
            {
              "when": "${state.pressed}",
              "backgroundColor": "green"
            }
          ]
        }
      },
      "mainTemplate": {
        "item": {
          "type": "TouchWrapper",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 100,
            "style": "FRAME",
            "inheritParentState": true
          }
        }
      }
    }
)apl";

TEST_F(SGTouchTest, TouchWrapper)
{
    loadDocument(TOUCH_WRAPPER);
    ASSERT_TRUE(component);

    auto frame = component->getChildAt(0);
    ASSERT_TRUE(frame);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 100}, "...Touch")
                .pressable()
                .child(IsLayer(Rect{0, 0, 100, 100}, "...Frame")
                           .content(IsDrawNode()
                                        .path(IsRoundRectPath(0, 0, 100, 100, 0))
                                        .pathOp(IsFillOp(IsColorPaint(Color::RED)))))));

    // Mouse down
    ASSERT_TRUE(MouseDown(root, 50, 50));
    ASSERT_TRUE(CheckDirtyDoNotClear(frame, kPropertyBackgroundColor, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirtyDoNotClear(root, frame));

    sg = root->getSceneGraph();

    // Extracting the scene graph cleans all dirty properties.
    ASSERT_TRUE(CheckDirty(root));

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 100}, "...Touch")
                .pressable()
                .child(IsLayer(Rect{0, 0, 100, 100}, "...Frame")
                           .dirty(sg::Layer::kFlagRedrawContent)
                           .content(IsDrawNode()
                                        .path(IsRoundRectPath(0, 0, 100, 100, 0))
                                        .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))))));

    // Mouse up
    ASSERT_TRUE(MouseUp(root, 60, 60));
    ASSERT_TRUE(CheckDirtyDoNotClear(frame, kPropertyBackgroundColor, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirtyDoNotClear(root, frame));

    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 100}, "...Touch")
                .pressable()
                .child(IsLayer(Rect{0, 0, 100, 100}, "...Frame")
                           .dirty(sg::Layer::kFlagRedrawContent)
                           .content(IsDrawNode()
                                        .path(IsRoundRectPath(0, 0, 100, 100, 0))
                                        .pathOp(IsFillOp(IsColorPaint(Color::RED)))))));
}