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
#include "apl/scenegraph/utilities.h"
#include "test_sg.h"
#include "testedittext.h"

using namespace apl;

class SGEditTextTest : public DocumentWrapper {
public:
    SGEditTextTest()
        : DocumentWrapper()
    {
        config->measure(std::make_shared<MyTestMeasurement>());
        etlFactory = std::make_shared<TestEditTextFactory>();
        config->editTextFactory(etlFactory);
    }

    std::shared_ptr<TestEditTextFactory> etlFactory;
};

static const char *BASIC = R"apl(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "text": "Hello",
      "fontSize": 40,
      "size": 10,
      "color": "blue"
    }
  }
}
)apl";

/**
 * Basic layout.  In this case the edit text box expands to fill the screen.
 */
TEST_F(SGEditTextTest, Basic)
{
    metrics.size(600, 700);
    loadDocument(BASIC);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 600, 700})
                .child(IsLayer(Rect{0, 0, 600, 700})
                           .content(IsEditNode("edit node")
                                        .text("Hello")
                                        .color(Color::BLUE)))));
}


static const char *NESTED = R"apl(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "justifyContent": "center",
      "alignItems": "start",
      "item": {
        "type": "EditText",
        "text": "Hello",
        "fontSize": 40,
        "size": 10,
        "color": "red"
      }
    }
  }
}
)apl";

/**
 * Basic layout.  Note that we ignore the lineHeight property, so we expect the height to be 40
 * (fontSize) and the width to be 400 (fontSize * size).  The position is centered
 * in the box.
 */
TEST_F(SGEditTextTest, Nested)
{
    metrics.size(600, 700);
    loadDocument(NESTED);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 600, 700})
                .child(IsLayer(Rect{0, 330, 400, 40})
                           .child(IsLayer(Rect{0, 0, 400, 40})
                                      .content(IsEditNode("edit node")
                                                   .text("Hello")
                                                   .color(Color::RED))))));
}


static const char *BORDER = R"apl(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "text": "Hello",
      "fontSize": 40,
      "size": 10,
      "borderWidth": 10,
      "borderStrokeWidth": 4,
      "borderColor": "red",
      "color": "blue"
    }
  }
}
)apl";

TEST_F(SGEditTextTest, Border)
{
    metrics.size(600, 700);
    loadDocument(BORDER);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 600, 700})
                .content(IsDrawNode()
                             .path(IsFramePath(RoundedRect{Rect{0, 0, 600, 700}, 0}, 4))
                             .pathOp(IsFillOp(IsColorPaint(Color::RED))))
                .child(IsLayer(Rect{10, 10, 580, 680})
                           .content(IsEditNode("edit node")
                                        .text("Hello")
                                        .color(Color::BLUE)))));
}

static const char *EVERYTHING = R"apl(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "id": "TEST",
      "text": "$$foo@bar.org",
      "fontSize": 40,
      "size": 10,
      "borderWidth": 10,
      "borderStrokeWidth": 4,
      "borderColor": "red",
      "color": "purple",
      "fontFamily": "Helvetica",
      "fontStyle": "italic",
      "fontWeight": 200,
      "highlightColor": "yellow",
      "hint": "e-mail address",
      "hintColor": "blue",
      "hintStyle": "italic",
      "hintWeight": 500,
      "keyboardType": "emailAddress",
      "lang": "es-US",
      "maxLength": 8,
      "secureInput": true,
      "selectOnFocus": true,
      "submitKeyType": "go",
      "validCharacters": "a-zA-Z@."
    }
  }
}
)apl";

/**
 * This test sets all of the EditText properties. Howver, checking the scene graph doesn't
 * verify the following properties:
 *
 *   color, fontFamily, fontStyle, fontWeight, highlightColor,
 *   hintColor, hintStyle, hintWeight, keyboardType, lang,
 *   secureInput, selectOnFocus, submitKeyType
 */
TEST_F(SGEditTextTest, Everything)
{
    metrics.size(1000, 1000);
    loadDocument(EVERYTHING);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 1000, 1000})
                .content(IsDrawNode()
                             .path(IsFramePath(RoundedRect{Rect{0, 0, 1000, 1000}, 0}, 4))
                             .pathOp(IsFillOp(IsColorPaint(Color::RED))))
                .child(IsLayer(Rect{10, 10, 980, 980})
                           .content(IsEditNode().text("foo@bar.").color(Color::PURPLE)))));

    // Change the text
    executeCommand("SetValue", {{"componentId", "TEST"}, {"property", "text"}, {"value", "a"}},
                   false);
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 1000, 1000})
                .content(IsDrawNode()
                             .path(IsFramePath(RoundedRect{Rect{0, 0, 1000, 1000}, 0}, 4))
                             .pathOp(IsFillOp(IsColorPaint(Color::RED))))
                .child(IsLayer(Rect{10, 10, 980, 980})
                           .dirty(sg::Layer::kFlagRedrawContent)
                           .content(IsEditNode().text("a").color(Color::PURPLE)))));

    // Clear the text.  The hint should be displayed
    executeCommand("SetValue", {{"componentId", "TEST"}, {"property", "text"}, {"value", ""}},
                   false);

    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 1000, 1000})
                .dirty(sg::Layer::kFlagRedrawContent)
                .content(IsDrawNode()
                             .path(IsFramePath(RoundedRect{Rect{0, 0, 1000, 1000}, 0}, 4))
                             .pathOp(IsFillOp(IsColorPaint(Color::RED)))
                             .next(IsTransformNode()
                                       .translate(Point{10, 480})
                                       .child(IsTextNode()
                                                  .text("e-mail address")
                                                  .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))
                .child(IsLayer(Rect{10, 10, 980, 980})
                           .dirty(sg::Layer::kFlagRedrawContent)
                           .content(IsEditNode().text("").color(Color::PURPLE)))));
}


static const char *USER_TYPING = R"apl(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "justifyContent": "center",
      "alignItems": "center",
      "items": {
        "type": "EditText",
        "text": "Hello",
        "fontSize": 40,
        "size": 10,
        "color": "green"
      }
    }
  }
}
)apl";

TEST_F(SGEditTextTest, UserTyping)
{
    metrics.size(600, 700);
    loadDocument(USER_TYPING);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 600, 700})
            .child(IsLayer(Rect{100, 330, 400, 40})
                       .child(IsLayer(Rect{0, 0, 400, 40})
                                  .content(IsEditNode("edit node")
                                               .text("Hello")
                                               .color(Color::GREEN))))));

    // Simulate typing.  This should NOT cause a redraw of the hint
    etlFactory->changeText("Goodbye");
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 600, 700})
            .child(IsLayer(Rect{100, 330, 400, 40})
                       .child(IsLayer(Rect{0, 0, 400, 40})
                                  .dirty(sg::Layer::kFlagRedrawContent)
                                  .content(IsEditNode("edit node")
                                               .text("Goodbye")
                                               .color(Color::GREEN))))));

    auto editText = component->getChildAt(0);
    ASSERT_TRUE(IsEqual("Goodbye", editText->getCalculated(kPropertyText)));
}


static const char *USER_DELETE = R"apl(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "text": "Hello",
      "hint": "Type Here",
      "hintColor": "blue",
      "fontSize": 40,
      "size": 10,
      "color": "blue"
    }
  }
}
)apl";

TEST_F(SGEditTextTest, UserDelete)
{
    metrics.size(600, 700);
    loadDocument(USER_DELETE);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 600, 700})
                .child(IsLayer(Rect{0, 0, 600, 700})
                           .content(IsEditNode("edit node")
                                        .text("Hello")
                                        .color(Color::BLUE)))));

    // Simulate deleting the word.  This causes a redraw of the hint (because the color changed).
    etlFactory->changeText("");
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 600, 700})
            .dirty(sg::Layer::kFlagRedrawContent)  // Hint color changed
            .content(
                IsTransformNode()
                    .translate(Point{0, 330})
                    .child(
                        IsTextNode().text("Type Here").pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))
            .child(IsLayer(Rect{0, 0, 600, 700})
                       .dirty(sg::Layer::kFlagRedrawContent)
                       .content(IsEditNode("edit node")
                                    .text("")
                                    .color(Color::BLUE)))));

    ASSERT_TRUE(IsEqual("", component->getCalculated(kPropertyText)));
}



static const char * RESIZE = R"apl(
    {
      "type": "APL",
      "version": "1.9",
      "mainTemplate": {
        "item": {
          "type": "EditText",
          "text": "Hello",
          "color": "blue",
          "borderWidth": 1,
          "borderColor": "red"
        }
      }
    }
)apl";

TEST_F(SGEditTextTest, Resize) {
    metrics.size(300, 300);
    loadDocument(RESIZE);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300})
                .content(IsDrawNode()
                             .path(IsFramePath(RoundedRect(Rect{0, 0, 300, 300}, 0), 1))
                             .pathOp(IsFillOp(IsColorPaint(Color::RED))))
                .child(IsLayer{Rect{1, 1, 298, 298}}.content(
                    IsEditNode().text("Hello").color(Color::BLUE)))));

    // Resize the screen
    configChange(ConfigurationChange(200, 200));
    root->clearPending();
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .dirty(sg::Layer::kFlagSizeChanged | sg::Layer::kFlagRedrawContent)
                .content(IsDrawNode()
                             .path(IsFramePath(RoundedRect(Rect{0, 0, 200, 200}, 0), 1))
                             .pathOp(IsFillOp(IsColorPaint(Color::RED))))
                .child(IsLayer{Rect{1, 1, 198, 198}}
                           .dirty(sg::Layer::kFlagSizeChanged)
                           .content(IsEditNode().text("Hello").color(Color::BLUE)))));
}

static const char *CHANGE_SUBMIT = R"apl(
    {
      "type": "APL",
      "version": "1.9",
      "mainTemplate": {
        "items": {
          "type": "EditText",
          "color": "black",
          "onTextChange": {
            "type": "SendEvent",
            "arguments": [
              "${event.source.source}",
              "${event.source.handler}",
              "${event.source.value}"
            ]
          },
          "onSubmit": {
            "type": "SendEvent",
            "arguments": [
              "${event.source.source}",
              "${event.source.handler}",
              "${event.source.value}"
            ]
          }
        }
      }
    }
)apl";

TEST_F(SGEditTextTest, ChangeSubmit)
{
    metrics.size(300, 300);
    loadDocument(CHANGE_SUBMIT);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(sg, IsLayer(Rect{0, 0, 300, 300})
                                        .child(IsLayer{Rect{0, 0, 300, 300}}.content(
                                            IsEditNode().text("").color(Color::BLACK)))));

    // Change the text
    etlFactory->changeText("Foobar");
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300})
                .dirty(sg::Layer::kFlagRedrawContent) // Toggled because the hint color changes
                .child(IsLayer{Rect{0, 0, 300, 300}}
                           .dirty(sg::Layer::kFlagRedrawContent)
                           .content(IsEditNode().text("Foobar").color(Color::BLACK)))));

    ASSERT_TRUE(IsEqual("Foobar", component->getCalculated(kPropertyText)));
    ASSERT_TRUE(CheckSendEvent(root, "EditText", "TextChange", "Foobar"));

    // Call "submit"
    etlFactory->submit();
    ASSERT_TRUE(CheckSendEvent(root, "EditText", "Submit", "Foobar"));
}

static const char *FOCUS_STYLE = R"apl(
{
  "type": "APL",
  "version": "1.9",
  "styles": {
    "EditTextStyle": {
      "values": [
        {
          "borderWidth": 2,
          "borderColor": "blue",
          "fontSize": 10,
          "color": "black"
        },
        {
          "when": "${state.focused}",
          "borderColor": "red"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": {
        "type": "EditText",
        "style": "EditTextStyle",
        "text": "${data}"
      },
      "data": ["Alpha", "Beta"]
    }
  }
}
)apl";

TEST_F(SGEditTextTest, FocusStyle)
{
    metrics.size(300, 300);
    loadDocument(FOCUS_STYLE);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300})
                .child(IsLayer{Rect{0, 0, 300, 14}}
                           .content(IsDrawNode()
                                        .path(IsFramePath(RoundedRect(Rect{0, 0, 300, 14}, 0), 2))
                                        .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))
                           .child(IsLayer(Rect{2, 2, 296, 10})
                                      .content(IsEditNode().text("Alpha").color(Color::BLACK))))
                .child(IsLayer{Rect{0, 14, 300, 14}}
                           .content(IsDrawNode()
                                        .path(IsFramePath(RoundedRect(Rect{0, 0, 300, 14}, 0), 2))
                                        .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))
                           .child(IsLayer(Rect{2, 2, 296, 10})
                                      .content(IsEditNode().text("Beta").color(Color::BLACK))))));

    // Pull out the two edit nodes
    auto edit1 = sg::EditTextNode::cast(sg->getLayer()->children().at(0)->children().at(0)->content());
    auto edit2 = sg::EditTextNode::cast(sg->getLayer()->children().at(1)->children().at(0)->content());
    ASSERT_TRUE(edit1);
    ASSERT_TRUE(edit2);

    auto test1 = static_cast<TestEditText *>(edit1->getEditText().get());
    auto test2 = static_cast<TestEditText *>(edit2->getEditText().get());
    ASSERT_TRUE(test1);
    ASSERT_TRUE(test2);

    // Focus the first edit text box.  It should change the color of the border
    test1->focus(true);
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300})
                .child(IsLayer{Rect{0, 0, 300, 14}}
                           .dirty(sg::Layer::kFlagRedrawContent)   // Content needs to be redrawn
                           .content(IsDrawNode()
                                        .path(IsFramePath(RoundedRect(Rect{0, 0, 300, 14}, 0), 2))
                                        .pathOp(IsFillOp(IsColorPaint(Color::RED))))   // Frame changes color
                           .child(IsLayer(Rect{2, 2, 296, 10})
                                      .content(IsEditNode().text("Alpha").color(Color::BLACK))))
                .child(IsLayer{Rect{0, 14, 300, 14}}
                           .content(IsDrawNode()
                                        .path(IsFramePath(RoundedRect(Rect{0, 0, 300, 14}, 0), 2))
                                        .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))
                           .child(IsLayer(Rect{2, 2, 296, 10})
                                      .content(IsEditNode().text("Beta").color(Color::BLACK))))));

    // Focus the second edit text box.  This should remove focus from the first
    test2->focus(true);
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300})
                .child(IsLayer{Rect{0, 0, 300, 14}}
                           .dirty(sg::Layer::kFlagRedrawContent)   // Content needs to be redrawn
                           .content(IsDrawNode()
                                        .path(IsFramePath(RoundedRect(Rect{0, 0, 300, 14}, 0), 2))
                                        .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))   // Frame changes back
                           .child(IsLayer(Rect{2, 2, 296, 10})
                                      .content(IsEditNode().text("Alpha").color(Color::BLACK))))
                .child(IsLayer{Rect{0, 14, 300, 14}}
                           .dirty(sg::Layer::kFlagRedrawContent)   // Content needs to be redrawn
                           .content(IsDrawNode()
                                        .path(IsFramePath(RoundedRect(Rect{0, 0, 300, 14}, 0), 2))
                                        .pathOp(IsFillOp(IsColorPaint(Color::RED))))   // Frame changes to red
                           .child(IsLayer(Rect{2, 2, 296, 10})
                                      .content(IsEditNode().text("Beta").color(Color::BLACK))))));

    // Drop focus
    test2->focus(false);
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300})
                .child(IsLayer{Rect{0, 0, 300, 14}}
                           .content(IsDrawNode()
                                        .path(IsFramePath(RoundedRect(Rect{0, 0, 300, 14}, 0), 2))
                                        .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))
                           .child(IsLayer(Rect{2, 2, 296, 10})
                                      .content(IsEditNode().text("Alpha").color(Color::BLACK))))
                .child(IsLayer{Rect{0, 14, 300, 14}}
                           .dirty(sg::Layer::kFlagRedrawContent)   // Content needs to be redrawn
                           .content(IsDrawNode()
                                        .path(IsFramePath(RoundedRect(Rect{0, 0, 300, 14}, 0), 2))
                                        .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))   // Frame changes to red
                           .child(IsLayer(Rect{2, 2, 296, 10})
                                      .content(IsEditNode().text("Beta").color(Color::BLACK))))));
}


// TODO: Style-change all properties and verify that they update correctly