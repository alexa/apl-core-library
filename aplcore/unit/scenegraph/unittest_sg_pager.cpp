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

class SGPagerTest : public DocumentWrapper {
public:
    SGPagerTest() {
        config->set(RootProperty::kDefaultPagerAnimationDuration, 200);
    }
};

static const char* DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Pager"
    }
  }
})";

/**
 * A basic pager with no children should give an empty scene graph
 */
TEST_F(SGPagerTest, PagerDefaults)
{
    loadDocument(DEFAULT_DOC);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_FALSE(sg->getLayer()->visible());
}

static const char *BASIC_PAGER = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Pager",
          "id": "MyPager",
          "width": 300,
          "height": 300,
          "items": {
            "type": "Frame",
            "width": "100%",
            "height": "100%",
            "backgroundColor": "${data}"
          },
          "data": [
            "red",
            "blue",
            "green"
          ]
        }
      }
    }
)apl";

TEST_F(SGPagerTest, BasicPager)
{
    loadDocument(BASIC_PAGER);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300}, "...Pager")
                .horizontal()
                .child(IsLayer(Rect{0, 0, 300, 300}, "...Child1")
                           .content(IsDrawNode()
                                        .path(IsRoundRectPath(0, 0, 300, 300, 0))
                                        .pathOp(IsFillOp(IsColorPaint(Color::RED)))))
                .accessibility(IsAccessibility()
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   true)
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   true))));

    auto ptr = executeCommand("AutoPage",
                              {{"componentId", "MyPager"}, {"count", 4}, {"duration", 100}}, false);
    root->updateTime(100); // This should be halfway through the pager animation
    root->clearPending();

    sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300}, "...Pager")
                .horizontal()
                .dirty(sg::Layer::kFlagChildrenChanged)
                .child(IsLayer(Rect{0, 0, 300, 300}, "....Child1")
                           .dirty(sg::Layer::kFlagTransformChanged)   // Transform changed
                           .transform(Transform2D::translate(-150, 0))
                           .content(IsDrawNode()
                                        .path(IsRoundRectPath(0, 0, 300, 300, 0))
                                        .pathOp(IsFillOp(IsColorPaint(Color::RED)))))
                .child(IsLayer(Rect{0, 0, 300, 300}, "....Child2")
                           .transform(Transform2D::translate(150, 0))
                           .content(IsDrawNode()
                                        .path(IsRoundRectPath(0, 0, 300, 300, 0))
                                        .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))
                .accessibility(IsAccessibility()
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   true)
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   true))));

    root->updateTime(250); // This should be in the pause between auto page animations
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300}, "...Pager")
                .horizontal()
                .dirty(sg::Layer::kFlagChildrenChanged)
                .child(IsLayer(Rect{0, 0, 300, 300}, "....Child2")
                           .dirty(sg::Layer::kFlagTransformChanged)  // Transform changed
                           .content(IsDrawNode()
                                        .path(IsRoundRectPath(0, 0, 300, 300, 0))
                                        .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))
                .accessibility(IsAccessibility()
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   true)
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   true))));
}

static const char *NORMAL_PAGER = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Pager",
          "id": "MyPager",
          "width": 300,
          "height": 300,
          "items": {
            "type": "Frame",
            "width": "100%",
            "height": "100%",
            "backgroundColor": "${data}"
          },
          "data": [
            "red",
            "blue",
            "green"
          ]
        }
      }
    }
)apl";

TEST_F(SGPagerTest, NormalPager)
{
    loadDocument(NORMAL_PAGER);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300}, "...Pager")
                .horizontal()
                .child(IsLayer(Rect{0, 0, 300, 300}, "...Child1")
                           .content(IsDrawNode()
                                        .path(IsRoundRectPath(0, 0, 300, 300, 0))
                                        .pathOp(IsFillOp(IsColorPaint(Color::RED)))))
                .accessibility(IsAccessibility()
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   true)
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   true))));

    executeCommand("SetPage", {{"componentId", "MyPager"}, {"position", "relative"}, {"value", 1}}, false);
    advanceTime(1000);

    sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300}, "...Pager")
                .horizontal()
                .dirty(sg::Layer::kFlagChildrenChanged)
                .child(IsLayer(Rect{0, 0, 300, 300}, "...Child1")
                           .content(IsDrawNode()
                                        .path(IsRoundRectPath(0, 0, 300, 300, 0))
                                        .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))
                .accessibility(IsAccessibility()
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   true)
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   true))));

    executeCommand("SetPage", {{"componentId", "MyPager"}, {"position", "relative"}, {"value", 1}}, false);
    advanceTime(1000);

    sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300}, "...Pager")
                .horizontal()
                .dirty(sg::Layer::kFlagChildrenChanged)
                .child(IsLayer(Rect{0, 0, 300, 300}, "...Child1")
                           .content(IsDrawNode()
                                        .path(IsRoundRectPath(0, 0, 300, 300, 0))
                                        .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))))
                .accessibility(IsAccessibility()
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD,
                                   true)
                           .action(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD,
                                   true))));
}