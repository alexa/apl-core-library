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

#include "../test_sg_textmeasure.h"

using namespace apl;

class TextLayoutTest : public DocumentWrapper {};

static const char* TEXT_MEASURE_LAYOUT = R"({
  "type": "APL",
  "version": "2024.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": 500,
      "height": 500,
      "items": [
        {
          "type": "Text",
          "id": "AutoHeight",
          "width": "100%",
          "height": "auto",
          "text": "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa."
        }
      ]
    }
  }
})";

TEST_F(TextLayoutTest, OldMeasure) {
    config->measure(std::make_shared<SimpleTextMeasurement>(40, 40));

    loadDocument(TEXT_MEASURE_LAYOUT);

    auto t = root->findComponentById("AutoHeight");
    auto s = t->getCalculated(apl::kPropertyBounds).get<Rect>().getSize();
    ASSERT_EQ(500, s.getWidth());
    ASSERT_EQ(360, s.getHeight());
}

TEST_F(TextLayoutTest, LayoutMeasure) {
    config->measure(std::make_shared<MyTestMeasurement>());

    loadDocument(TEXT_MEASURE_LAYOUT);

    auto t = root->findComponentById("AutoHeight");
    auto s = t->getCalculated(apl::kPropertyBounds).get<Rect>().getSize();
    ASSERT_EQ(500, s.getWidth());
    ASSERT_EQ(360, s.getHeight());
}

static const char* TEXT_LAYOUT_TEST_START = R"({
  "type": "APL",
  "version": "2023.2",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": 400,
      "height": 400,
      "bind": [
        {
          "name": "LongText",
          "value": "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa."
        }
      ],
      "items": [
)";

static const char* TEXT_LAYOUT_TEST_END = R"(
      ]
    }
  }
})";

TEST_F(TextLayoutTest, TextLayoutNoEventWhenNoLayout) {
    const char* TEST = R"({
        "type": "ScrollView",
        "width": "100%",
        "height": "50%",
        "item": {
          "type": "Text",
          "text": "${LongText}",
          "width": "100%",
          "height": "auto",
          "onTextLayout": {
            "type": "SendEvent",
            "sequencer": "EVENTER",
            "arguments": [
              "${event.laidOutText}",
              "${event.isTruncated}",
              "${event.textWidth}",
              "${event.textHeight}"
            ]
          }
        }
      })";

    std::string doc(TEXT_LAYOUT_TEST_START);
    doc.append(TEST);
    doc.append(TEXT_LAYOUT_TEST_END);

    loadDocument(doc.c_str());

    ASSERT_TRUE(component);

    ASSERT_FALSE(CheckSendEvent(root));
}

TEST_F(TextLayoutTest, TextLayoutFixed) {
    config->measure(std::make_shared<MyTestMeasurement>());

    const char* TEST = R"({
        "type": "Text",
        "text": "${LongText}",
        "width": "100%",
        "height": "50%",
        "onTextLayout": {
          "type": "SendEvent",
          "sequencer": "EVENTER",
          "arguments": [
            "${event.laidOutText}",
            "${event.isTruncated}",
            "${event.textWidth}",
            "${event.textHeight}"
          ]
        }
      })";

    std::string doc(TEXT_LAYOUT_TEST_START);
    doc.append(TEST);
    doc.append(TEXT_LAYOUT_TEST_END);

    loadDocument(doc.c_str());

    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "Lorem ipsum dolor sit amet, consectetuer adipiscin", true, 400, 200));
}

TEST_F(TextLayoutTest, TextLayoutFixedNoLayoutWhenNoEvent) {
    auto measure = std::make_shared<MyTestMeasurement>();
    config->measure(measure);

    const char* TEST = R"({
        "type": "Text",
        "text": "${LongText}",
        "width": "100%",
        "height": "50%"
      })";

    std::string doc(TEXT_LAYOUT_TEST_START);
    doc.append(TEST);
    doc.append(TEXT_LAYOUT_TEST_END);

    loadDocument(doc.c_str());

    ASSERT_TRUE(component);

    ASSERT_EQ(0, measure->getLayoutCount());
}

TEST_F(TextLayoutTest, TextLayoutAtMax) {
    config->measure(std::make_shared<MyTestMeasurement>());

    const char* TEST = R"({
        "type": "Text",
        "text": "${LongText}",
        "width": "100%",
        "height": "auto",
        "maxHeight": "12.5%",
        "onTextLayout": {
          "type": "SendEvent",
          "sequencer": "EVENTER",
          "arguments": [
            "${event.laidOutText}",
            "${event.isTruncated}",
            "${event.textWidth}",
            "${event.textHeight}"
          ]
        }
      })";

    std::string doc(TEXT_LAYOUT_TEST_START);
    doc.append(TEST);
    doc.append(TEXT_LAYOUT_TEST_END);

    loadDocument(doc.c_str());

    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "Lorem ipsu", true, 400, 50));
}

TEST_F(TextLayoutTest, TextLayoutUndefined) {
    config->measure(std::make_shared<MyTestMeasurement>());

    const char* TEST = R"({
        "type": "ScrollView",
        "width": "100%",
        "height": "50%",
        "item": {
          "type": "Text",
          "text": "${LongText}",
          "width": "100%",
          "height": "auto",
          "onTextLayout": {
            "type": "SendEvent",
            "sequencer": "EVENTER",
            "arguments": [
              "${event.laidOutText}",
              "${event.isTruncated}",
              "${event.textWidth}",
              "${event.textHeight}"
            ]
          }
        }
      })";

    std::string doc(TEXT_LAYOUT_TEST_START);
    doc.append(TEST);
    doc.append(TEXT_LAYOUT_TEST_END);

    loadDocument(doc.c_str());

    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa.", false, 400, 440));
}

TEST_F(TextLayoutTest, TextLayoutAutosize) {
    config->measure(std::make_shared<MyTestMeasurement>());

    const char* TEST = R"({
        "bind": [
          {
            "name": "FontSize",
            "value": 40
          }
        ],
        "type": "Text",
        "text": "${LongText}",
        "width": "100%",
        "height": "auto",
        "maxHeight": "50%",
        "fontSize": "${FontSize}",
        "onTextLayout": [
          {
            "when": "${event.isTruncated}",
            "type": "SetValue",
            "property": "FontSize",
            "value": "${event.source.bind.FontSize - 10}"
          },
          {
            "type": "SendEvent",
            "sequencer": "EVENTER",
            "arguments": [
              "${event.laidOutText}",
              "${event.isTruncated}",
              "${event.textWidth}",
              "${event.textHeight}"
            ]
          }
        ]
      })";

    std::string doc(TEXT_LAYOUT_TEST_START);
    doc.append(TEST);
    doc.append(TEXT_LAYOUT_TEST_END);

    loadDocument(doc.c_str());

    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "Lorem ipsum dolor sit amet, consectetuer adipiscin", true, 400, 200));
    ASSERT_TRUE(CheckSendEvent(root, "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligul", true, 390, 200));
    ASSERT_TRUE(CheckSendEvent(root, "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa.", false, 400, 120));
}

TEST_F(TextLayoutTest, TextLayoutAutosizeFixed) {
    config->measure(std::make_shared<MyTestMeasurement>());

    const char* TEST = R"({
        "bind": [
          {
            "name": "FontSize",
            "value": 40
          }
        ],
        "type": "Text",
        "text": "${LongText}",
        "width": "100%",
        "height": "50%",
        "fontSize": "${FontSize}",
        "onTextLayout": [
          {
            "when": "${event.isTruncated && event.source.bind.FontSize > 10}",
            "type": "SetValue",
            "property": "FontSize",
            "value": "${event.source.bind.FontSize - 10}"
          },
          {
            "type": "SendEvent",
            "sequencer": "EVENTER",
            "arguments": [
              "${event.laidOutText}",
              "${event.isTruncated}",
              "${event.textWidth}",
              "${event.textHeight}"
            ]
          }
        ]
      })";

    std::string doc(TEXT_LAYOUT_TEST_START);
    doc.append(TEST);
    doc.append(TEXT_LAYOUT_TEST_END);

    loadDocument(doc.c_str());

    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "Lorem ipsum dolor sit amet, consectetuer adipiscin", true, 400, 200));
    ASSERT_TRUE(CheckSendEvent(root, "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligul", true, 390, 200));
    ASSERT_TRUE(CheckSendEvent(root, "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa.", false, 400, 120));
}

const static char *BASELINE_TEST = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "direction": "row",
      "alignItems": "baseline",
      "items": {
        "type": "Text",
        "fontSize": 10,
        "text": "${data}"
      },
      "data": [
        "Single line",
        "Double line<br>Double line",
        "Triple line<br>Triple line<br>Triple line"
      ]
    }
  }
})";

TEST_F(TextLayoutTest, BaselineTest)
{
    config->measure(std::make_shared<MyTestMeasurement>());

    loadDocument(BASELINE_TEST);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(3, component->getChildCount());

    // Test TextLayout does not handle multilines
    auto child = component->getChildAt(0);
    LOG(LogLevel::kError) << child->getCalculated(kPropertyBounds).get<Rect>();
    ASSERT_EQ(Rect(0, 0, 110, 10), child->getCalculated(kPropertyBounds).get<Rect>());

    child = component->getChildAt(1);
    LOG(LogLevel::kError) << child->getCalculated(kPropertyBounds).get<Rect>();
    ASSERT_EQ(Rect(110, 0, 260, 10), child->getCalculated(kPropertyBounds).get<Rect>());

    child = component->getChildAt(2);
    LOG(LogLevel::kError) << child->getCalculated(kPropertyBounds).get<Rect>();
    ASSERT_EQ(Rect(370, 0, 410, 10), child->getCalculated(kPropertyBounds).get<Rect>());
}

static const char* EDITTEXT_LAYOUT = R"({
  "type": "APL",
  "version": "2024.2",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "height": "100%",
      "width": "100%",
      "borderWidth": 2,
      "item": {
        "type": "EditText",
        "height": "auto",
        "width": "auto",
        "text": "Hello",
        "size": 3,
        "color": "#000000"
      }
    }
  }
})";

/**
 * Test text measurement for EditText component
 */
TEST_F(TextLayoutTest, EditTextMeasurement) {
    metrics.size(400, 400);
    config->measure(std::make_shared<MyTestMeasurement>());

    loadDocument(EDITTEXT_LAYOUT);
    ASSERT_TRUE(root);

    // Check the layout
    auto top = root->topComponent().get();
    ASSERT_EQ(Rect(0, 0, 400, 400), top->getCalculated(kPropertyBounds).get<Rect>());
    auto editText = top->getChildAt(0);
    ASSERT_EQ(Rect(2, 2, 120, 40), editText->getCalculated(kPropertyBounds).get<Rect>());
}

static const char* EDIT_TEXT_AUTOSIZED = R"({
  "type": "APL",
  "version": "2024.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "direction": "row",
      "items": [
        {
          "type": "EditText",
          "id": "EDITTEXT",
          "text": "hello",
          "size": 8,
          "shrink": 1.0
        }
      ]
    }
  }
})";

TEST_F(TextLayoutTest, EditTextAutosize) {
    metrics.size(600, 600);
    config->measure(std::make_shared<MyTestMeasurement>());

    loadDocument(EDIT_TEXT_AUTOSIZED);

    auto et = root->findComponentById("EDITTEXT");
    ASSERT_EQ(Size(320, 600), et->getCalculated(apl::kPropertyBounds).get<Rect>().getSize());

    // Change FontSize and ensure resize happened
    executeCommands(JsonData(R"([{ "type": "SetValue", "componentId": "EDITTEXT", "property": "fontSize", "value": 60 }])").moveToObject(), false);
    advanceTime(100);

    ASSERT_EQ(Size(480, 600), et->getCalculated(apl::kPropertyBounds).get<Rect>().getSize());
}

const static char *BASELINE_EDITTEXT_TEST = R"(
{
  "type":"APL",
  "version":"1.4",
  "mainTemplate":{
    "items":{
      "type":"Container",
      "width":"100%",
      "height":"100%",
      "direction":"row",
      "alignItems":"baseline",
      "items":{
        "type":"EditText",
        "fontSize": 10,
        "text":"${data}"
      },
      "data":[
        "Short",
        "Mid size text test.",
        "This is long text test for measure size.",
        "This is long text test for measure size. Last test text."
      ]
    }
  }
}
)";

TEST_F(TextLayoutTest, BaselineEditTextTest)
{
    config->measure(std::make_shared<MyTestMeasurement>());

    loadDocument(BASELINE_EDITTEXT_TEST);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(4, component->getChildCount());

    auto child = component->getChildAt(0);  // First child is one line
    ASSERT_EQ(Rect(0, 0, 80, 10), child->getCalculated(kPropertyBounds).get<Rect>());

    child = component->getChildAt(1);  // First child is one line
    ASSERT_EQ(Rect(80, 0, 80, 10), child->getCalculated(kPropertyBounds).get<Rect>());

    child = component->getChildAt(2);  // First child is one line
    ASSERT_EQ(Rect(160, 0, 80, 10), child->getCalculated(kPropertyBounds).get<Rect>());

    child = component->getChildAt(3);  // First child is one line
    ASSERT_EQ(Rect(240, 0, 80, 10), child->getCalculated(kPropertyBounds).get<Rect>());
}

TEST_F(TextLayoutTest, LayoutReusePossible) {
    config->measure(std::make_shared<LayoutReuseMeasurement>());

    loadDocument(TEXT_MEASURE_LAYOUT);

    auto t = root->findComponentById("AutoHeight");
    MyTestLayout* cachedLayout = t->getUserData<MyTestLayout>();
    ASSERT_TRUE(cachedLayout);
    auto layoutSize = cachedLayout->getSize();
    auto componentSize = t->getCalculated(apl::kPropertyBounds).get<Rect>().getSize();
    ASSERT_EQ(layoutSize.getWidth(), componentSize.getWidth());
    ASSERT_EQ(layoutSize.getHeight(), componentSize.getHeight());

}

TEST_F(TextLayoutTest, BoxReusePossible) {
    config->measure(std::make_shared<LayoutReuseMeasurement>());

    loadDocument(EDITTEXT_LAYOUT);
    auto top = root->topComponent().get();
    auto editText = top->getChildAt(0);
    MyTestBox* cachedLayout = editText->getUserData<MyTestBox>();
    ASSERT_TRUE(cachedLayout);
    auto layoutSize = cachedLayout->getSize();
    auto componentSize = editText->getCalculated(apl::kPropertyBounds).get<Rect>().getSize();
    ASSERT_EQ(layoutSize.getWidth(), componentSize.getWidth());
    ASSERT_EQ(layoutSize.getHeight(), componentSize.getHeight());
}
