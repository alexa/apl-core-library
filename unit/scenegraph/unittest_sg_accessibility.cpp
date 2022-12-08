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

class SGAccessibilityTest : public DocumentWrapper {
public:
    SGAccessibilityTest() : DocumentWrapper() {
        config->measure(std::make_shared<MyTestMeasurement>());
    }
};


static const char * BASIC = R"apl(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "items": {
      "type": "ScrollView",
      "accessibilityLabel": "Master Scroll",
      "items": {
        "type": "Container",
        "width": "100%",
        "items": {
          "type": "Text",
          "id": "TEXT${index}",
          "text": "Item ${index}",
          "color": "black",
          "accessibilityLabel": "Text item ${index}"
        },
        "data": "${Array.range(3)}"
      }
    }
  }
}
)apl";

TEST_F(SGAccessibilityTest, Basic) {
    metrics.size(300, 300);
    loadDocument(BASIC);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 300, 100})
            .vertical()
            .accessibility(IsAccessibility().label("Master Scroll"))
            .child(
                IsLayer(Rect{0, 0, 300, 120})
                    .child(IsLayer(Rect{0, 0, 300, 40})
                               .accessibility(IsAccessibility().label("Text item 0"))
                               .content(IsTransformNode().child(IsTextNode().text("Item 0").pathOp(
                                   IsFillOp(IsColorPaint(Color::BLACK))))))
                    .child(IsLayer(Rect{0, 40, 300, 40})
                               .accessibility(IsAccessibility().label("Text item 1"))
                               .content(IsTransformNode().child(IsTextNode().text("Item 1").pathOp(
                                   IsFillOp(IsColorPaint(Color::BLACK))))))
                    .child(IsLayer(Rect{0, 80, 300, 40})
                               .accessibility(IsAccessibility().label("Text item 2"))
                               .content(IsTransformNode().child(IsTextNode().text("Item 2").pathOp(
                                   IsFillOp(IsColorPaint(Color::BLACK))))))

                    )));

    executeCommand("SetValue",
                   {{"componentId", "TEXT2"}, {"property", "accessibilityLabel"}, {"value", "FOO"}}, true);
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 300, 100})
            .vertical()
            .accessibility(IsAccessibility().label("Master Scroll"))
            .child(
                IsLayer(Rect{0, 0, 300, 120})
                    .child(IsLayer(Rect{0, 0, 300, 40})
                               .accessibility(IsAccessibility().label("Text item 0"))
                               .content(IsTransformNode().child(IsTextNode().text("Item 0").pathOp(
                                   IsFillOp(IsColorPaint(Color::BLACK))))))
                    .child(IsLayer(Rect{0, 40, 300, 40})
                               .accessibility(IsAccessibility().label("Text item 1"))
                               .content(IsTransformNode().child(IsTextNode().text("Item 1").pathOp(
                                   IsFillOp(IsColorPaint(Color::BLACK))))))
                    .child(IsLayer(Rect{0, 80, 300, 40})
                               .dirty(sg::Layer::kFlagAccessibilityChanged)
                               .accessibility(IsAccessibility().label("FOO"))
                               .content(IsTransformNode().child(IsTextNode().text("Item 2").pathOp(
                                   IsFillOp(IsColorPaint(Color::BLACK))))))

                )));
}


static const char *ROLE = R"apl(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": 200,
      "height": 200,
      "role": "list",
      "items": [
        {
          "type": "Text",
          "text": "Hello",
          "role": "listitem",
          "width": 100,
          "height": 100,
          "color": "black"
        },
        {
          "type": "Image",
          "role": "image",
          "width": 100,
          "height": 100
        }
      ]
    }
  }
}
)apl";

TEST_F(SGAccessibilityTest, Role) {
    metrics.size(300, 300);
    loadDocument(ROLE);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .accessibility(IsAccessibility().role(apl::kRoleList))
                .child(IsLayer(Rect{0, 0, 100, 100})
                           .accessibility(IsAccessibility().role(apl::kRoleListItem))
                           .content(IsTransformNode().child(IsTextNode().text("Hello").pathOp(
                               IsFillOp(IsColorPaint(Color::BLACK))))))
                .child(IsLayer(Rect{0, 100, 100, 100})
                           .accessibility(IsAccessibility().role(apl::kRoleImage)))));
}


static const char *ACTIONS = R"apl(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "actions": [
        {
          "name": "activate",
          "label": "Message to Server",
          "commands": {
            "type": "SendEvent",
            "arguments": [
              "alpha"
            ]
          }
        },
        {
          "name": "deactivate",
          "label": "Different message",
          "enabled": false,
          "commands": {
            "type": "SendEvent",
            "arguments": [
              "beta"
            ]
          }
        }
      ]
    }
  }
}
)apl";

TEST_F(SGAccessibilityTest, Actions)
{
    metrics.size(300, 300);
    loadDocument(ACTIONS);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300})
                .pressable()
                .accessibility(IsAccessibility()
                                   .action("activate", "Message to Server", true)
                                   .action("deactivate", "Different message", false))));

    // Execute the first action
    sg->getLayer()->getAccessibility()->executeCallback("activate");
    CheckSendEvent(root, "alpha");

    // Try to execute the second (it is disabled)
    sg->getLayer()->getAccessibility()->executeCallback("deactivate");
    ASSERT_FALSE(root->hasEvent());
}


static const char *INTERACTION_CHECKED_ENABLED = R"apl(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "id": "TOUCH"
    }
  }
}
)apl";

TEST_F(SGAccessibilityTest, InteractionCheckedEnabled)
{
    metrics.size(100, 100);
    loadDocument(INTERACTION_CHECKED_ENABLED);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(sg, IsLayer(Rect{0, 0, 100, 100}).pressable()));

    // Set "checked" state
    executeCommand("SetValue", {{"componentId", "TOUCH"}, {"property", "checked"}, {"value", true}},
                   false);
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(sg, IsLayer(Rect{0, 0, 100, 100})
                                        .pressable()
                                        .checked()
                                        .dirty(sg::Layer::kFlagInteractionChanged)));

    // Toggle "disabled" state
    executeCommand("SetValue", {{"componentId", "TOUCH"}, {"property", "disabled"}, {"value", true}},
                   false);
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(sg, IsLayer(Rect{0, 0, 100, 100})
                                        .pressable()
                                        .checked()
                                        .disabled()
                                        .dirty(sg::Layer::kFlagInteractionChanged)));

    // Unset "checked" state
    executeCommand("SetValue", {{"componentId", "TOUCH"}, {"property", "checked"}, {"value", false}},
                   false);
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(sg, IsLayer(Rect{0, 0, 100, 100})
                                        .pressable()
                                        .disabled()
                                        .dirty(sg::Layer::kFlagInteractionChanged)));
}

TEST_F(SGAccessibilityTest, Serialize)
{
    auto a = std::make_shared<sg::Accessibility>([](const std::string&){});
    a->setLabel("The Label");
    a->setRole(Role::kRoleAlert);
    a->appendAction("bounce", "this is a bounce", true);
    a->appendAction("debounce", "this is not a bounce", false);

    rapidjson::Document document;
    ASSERT_TRUE(IsEqual(a->serialize(document.GetAllocator()), StringToMapObject(R"apl(
        {
            "label": "The Label",
            "role": "alert",
            "actions": [
                {
                    "name": "bounce",
                    "label": "this is a bounce",
                    "enabled": true
                },
                {
                    "name": "debounce",
                    "label": "this is not a bounce",
                    "enabled": false
                }
            ]
        }
    )apl")));
}

TEST_F(SGAccessibilityTest, Comparisons)
{
    auto a = std::make_shared<sg::Accessibility>([](const std::string&){});
    auto b = std::make_shared<sg::Accessibility>([](const std::string&){});

    ASSERT_EQ(*a, *b);
    b->setRole(Role::kRoleAlert);
    ASSERT_NE(*a, *b);
    a->setRole(Role::kRoleAlert);
    ASSERT_EQ(*a, *b);

    b->setLabel("I am an alert");
    ASSERT_NE(*a, *b);
    a->setLabel("I am an alert");
    ASSERT_EQ(*a, *b);

    b->appendAction("bounce", "this is a bounce", true);
    a->appendAction("bounce", "this is a bounce", false);
    ASSERT_NE(*a, *b);
}