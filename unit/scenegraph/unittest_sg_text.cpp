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

using namespace apl;

class SGTextTest : public DocumentWrapper {
public:
    SGTextTest() : DocumentWrapper() {
        config->measure(std::make_shared<MyTestMeasurement>());
    }
};

struct SplitTestCase {
    std::string input;
    std::vector<std::string> expected;
};

static const auto SPLIT_TEST_CASES = std::vector<SplitTestCase>{
    {"",                               {"sans-serif"}},
    {"           ",                    {"sans-serif"}},
    {"Arial",                          {"Arial", "sans-serif"}},
    {"amazon-ember",                   {"amazon-ember", "sans-serif"}},
    {"_amazon-Ember234",               {"_amazon-Ember234", "sans-serif"}},
    {"Amazon Ember Bold",              {"Amazon Ember Bold", "sans-serif"}},
    {"Helvetica, Verdana, sans-serif", {"Helvetica", "Verdana", "sans-serif"}},
    {"   Helvetica   ",                {"Helvetica", "sans-serif"}},
    {"   Helvetica   ,  Arial    ",    {"Helvetica", "Arial", "sans-serif"}},
    {"   Helvetica,Arial,Beruit",      {"Helvetica", "Arial",   "Beruit", "sans-serif"}},
    {"Avenir Next Condensed",          {"Avenir Next Condensed", "sans-serif"}},
    {"  Avenir   Next     Condensed",  {"Avenir Next Condensed", "sans-serif"}},
    {"'#Test!'",                       {"#Test!", "sans-serif"}},
    {"'  spaces  '",                   {"  spaces  ", "sans-serif"}},
    {"  '$one' , \" %two \"  ",        {"$one",      " %two ", "sans-serif"}},
};

TEST_F(SGTextTest, SplitString)
{
    config->set(RootProperty::kDefaultFontFamily, "sans-serif");

    for (const auto& m : SPLIT_TEST_CASES)
        ASSERT_EQ(m.expected, sg::splitFontString(*config, m.input)) << m.input;
}


static const auto BAD_TEST_CASES = std::vector<std::string>{
    "%",                     // Illegal character
    "this is a long font!",  // Another illegal character
    " 'Harvey ",             // Unterminated quotation
    "Arial 'BOLD'",          // Quoted string appended to unquoted region
};

TEST_F(SGTextTest, SplitStringBad)
{
    config->set(RootProperty::kDefaultFontFamily, "fail");
    const std::vector<std::string> expected{"fail"};

    for (const auto& m : BAD_TEST_CASES) {
        ASSERT_EQ(expected, sg::splitFontString(*config, m)) << m;
        ASSERT_TRUE(ConsoleMessage());
    }
}


static const char * BASIC_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Text",
          "text": "This is my text",
          "color": "red"
        }
      }
    }
)apl";

TEST_F(SGTextTest, Basic)
{
    metrics.size(300, 300);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 300, 300})
                .content(IsTransformNode()
                             .child(IsTextNode()
                                        .text("This is my text")
                                        .pathOp(IsFillOp(IsColorPaint(Color::RED)))))));
}

static const char * FRAMED = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Frame",
          "items": {
            "type": "Text",
            "text": "This is my text",
            "color": "red",
            "fontSize": 10
          }
        }
      }
    }
)apl";

TEST_F(SGTextTest, Framed)
{
    metrics.size(300, 300);
    loadDocument(FRAMED);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 300, 300}, "...Frame")
                                .child(IsLayer(Rect{0, 0, 150, 10}, "...Text")
                                           .content(IsTransformNode().child(
                                               IsTextNode()
                                                   .text("This is my text")
                                                   .pathOp(IsFillOp(IsColorPaint(Color::RED))))))));
}

static const char * DYNAMIC_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Text",
          "id": "ID",
          "width": 200,
          "height": 200,
          "text": "TEST",
          "color": "red",
          "fontSize": 10
        }
      }
    }
)apl";

TEST_F(SGTextTest, Dynamic)
{
    loadDocument(DYNAMIC_TEST);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 200, 200})
                                .content(IsTransformNode().child(IsTextNode().text("TEST").pathOp(
                                    IsFillOp(IsColorPaint(Color::RED)))))));

    // Each character is 10x10, so centering the text shifts it by (200 - 10*4) / 2 = 80 dp
    executeCommand("SetValue",
                   {{"componentId", "ID"}, {"property", "textAlign"}, {"value", "center"}}, true);

    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .dirty(sg::Layer::kFlagRedrawContent)
                .content(IsTransformNode()
                             .translate(Point{80, 0})
                             .child(IsTextNode()
                                        .text("TEST")
                                        .pathOp(IsFillOp(IsColorPaint(Color::RED)))))));

    // Change vertical alignment.  Note that this is not usually dynamic so things don't change
    executeCommand("SetValue",
                   {{"componentId", "ID"}, {"property", "textAlignVertical"}, {"value", "bottom"}},
                   true);

    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .dirty(sg::Layer::kFlagRedrawContent)
                .content(IsTransformNode()
                             .translate(Point{80, 190})
                             .child(IsTextNode()
                                        .text("TEST")
                                        .pathOp(IsFillOp(IsColorPaint(Color::RED)))))));

    // Change the text itself without changing the size of the text
    executeCommand("SetValue", {{"componentId", "ID"}, {"property", "text"}, {"value", "LEFT"}},
                   true);

    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .dirty(sg::Layer::kFlagRedrawContent)
                .content(IsTransformNode()
                             .translate(Point{80, 190})
                             .child(IsTextNode()
                                        .text("LEFT")
                                        .pathOp(IsFillOp(IsColorPaint(Color::RED)))))));

    // Update the text color
    executeCommand("SetValue", {{"componentId", "ID"}, {"property", "color"}, {"value", "blue"}},
                   true);

    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .dirty(sg::Layer::kFlagRedrawContent)
                .content(IsTransformNode()
                             .translate(Point{80, 190})
                             .child(IsTextNode()
                                        .text("LEFT")
                                        .pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))));
}


static const char *SIMPLE_VALUE = R"apl(
  {
    "type": "APL",
    "version": "1.8",
    "mainTemplate": {
      "bind": {
        "name": "Counter",
        "value": 10
      },
      "items": {
        "type": "Text",
        "id": "MyText",
        "text": "C=${Counter}",
        "width": 100,
        "height": 100,
        "color": "white"
      }
    }
  }
)apl";

TEST_F(SGTextTest, SimpleValue)
{
    loadDocument(SIMPLE_VALUE);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 100, 100})
                                .content(IsTransformNode().child(IsTextNode().text("C=10").pathOp(
                                    IsFillOp(IsColorPaint(Color::WHITE)))))));

    executeCommand("SetValue", {{"componentId", "MyText"}, {"property", "Counter"}, {"value", 99}},
                   true);
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(sg, IsLayer(Rect{0, 0, 100, 100})
                                        .dirty(sg::Layer::kFlagRedrawContent)
                                        .content(IsTransformNode().child(
                                            IsTextNode()
                                                .text("C=99")
                                                .pathOp(IsFillOp(IsColorPaint(Color::WHITE)))))));
}


static const char *PACKING = R"apl(
{
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": 500,
      "height": 500,
      "items": [
        {
          "type": "ScrollView",
          "width": "100%",
          "height": 1,
          "grow": 1,
          "items": {
            "type": "Text",
            "width": "100%",
            "color": "red",
            "fontSize": 40,
            "text": "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
          }
        },
        {
          "type": "Text",
          "width": "100%",
          "color": "blue",
          "fontSize": 40,
          "text": "Footnote"
        }
      ]
    }
  }
}
)apl";

TEST_F(SGTextTest, Packing)
{
    loadDocument(PACKING);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 500, 500})
            .child(
                IsLayer(Rect{0, 0, 500, 460})
                    .vertical()
                    .child(
                        IsLayer(Rect{0, 0, 500, 1520})
                            .content(IsTransformNode().child(
                                IsTextNode()
                                    .text(
                                        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.")
                                    .pathOp(IsFillOp(IsColorPaint(Color::RED)))))))
            .child(IsLayer(Rect{0, 460, 500, 40})
                       .content(IsTransformNode().child(
                           IsTextNode()
                               .text("Footnote")
                               .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));
}


static const char *UNKNOWN_WIDTH = R"apl(
{
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": 500,
      "height": 500,
      "alignItems": "end",
      "item": {
        "type": "Text",
        "id": "TEST",
        "text": "Hello",
        "color": "blue"
      }
    }
  }
}
)apl";

TEST_F(SGTextTest, UnknownWidth) {
    loadDocument(UNKNOWN_WIDTH);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 500, 500})
                .child(IsLayer(Rect{300, 0, 200, 40})
                           .content(IsTransformNode().child(IsTextNode().text("Hello").pathOp(
                               IsFillOp(IsColorPaint(Color::BLUE))))))));

    // Changing the text should trigger a new layout
    executeCommand("SetValue", {{"componentId", "TEST"}, {"property", "text"}, {"value", "A"}}, false);
    root->clearPending();
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 500, 500})
                .child(IsLayer(Rect{460, 0, 40, 40})
                           .dirty(sg::Layer::kFlagPositionChanged |
                                  sg::Layer::kFlagSizeChanged |
                                  sg::Layer::kFlagRedrawContent)
                           .content(IsTransformNode().child(IsTextNode().text("A").pathOp(
                               IsFillOp(IsColorPaint(Color::BLUE))))))));
}


static const char *CHANGING_SIZE = R"apl(
{
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "BOX",
      "width": 200,
      "height": 200,
      "item": {
        "type": "Text",
        "text": "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "fontSize": 40,
        "color": "blue",
        "width": "100%"
      }
    }
  }
}
)apl";

TEST_F(SGTextTest, ChangingSize) {
    loadDocument(CHANGING_SIZE);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .child(IsLayer(Rect{0, 0, 200, 240})   // 5 characters per line
                           .content(IsTransformNode().child(
                               IsTextNode()
                                   .measuredSize({200,240})
                                   .text("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
                                   .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));

    // Shrink the width.  This forces a new text layout
    executeCommand("SetValue", {{"componentId", "BOX"}, {"property", "width"}, {"value",100}}, false);
    root->clearPending();
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 200})
                .dirty(sg::Layer::kFlagSizeChanged)
                .child(IsLayer(Rect{0, 0, 100, 40 * 13})   // 2 characters per line
                           .dirty(sg::Layer::kFlagSizeChanged |
                                  sg::Layer::kFlagRedrawContent)
                           .content(IsTransformNode().child(
                               IsTextNode()
                                   .measuredSize({100,520})
                                   .text("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
                                   .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));

    // Return to the previous size.  This triggers a new text layout - but Yoga has cached the dimension
    executeCommand("SetValue", {{"componentId", "BOX"}, {"property", "width"}, {"value",200}}, false);
    root->clearPending();
    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .dirty(sg::Layer::kFlagSizeChanged)
                .child(IsLayer(Rect{0, 0, 200, 240})   // 5 characters per line
                           .dirty(sg::Layer::kFlagSizeChanged |
                                  sg::Layer::kFlagRedrawContent)
                           .content(IsTransformNode().child(
                               IsTextNode()
                                   .measuredSize({200,240})
                                   .text("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
                                   .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));
}


static const char * RESIZE = R"apl(
    {
      "type": "APL",
      "version": "1.9",
      "mainTemplate": {
        "item": {
          "type": "Text",
          "text": "Hello",
          "color": "red"
        }
      }
    }
)apl";

TEST_F(SGTextTest, Resize) {
    metrics.size(300, 300);
    loadDocument(RESIZE);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 300, 300})
                                .content(IsTransformNode().child(IsTextNode().text("Hello").pathOp(
                                    IsFillOp(IsColorPaint(Color::RED)))))));

    // Resize the screen
    configChange(ConfigurationChange(200, 200));
    root->clearPending();
    sg = root->getSceneGraph();
    DumpSceneGraph(sg);
    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 200, 200})
                                .dirty(sg::Layer::kFlagSizeChanged | sg::Layer::kFlagRedrawContent)
                                .content(IsTransformNode().child(IsTextNode().text("Hello").pathOp(
                                    IsFillOp(IsColorPaint(Color::RED)))))));
}
