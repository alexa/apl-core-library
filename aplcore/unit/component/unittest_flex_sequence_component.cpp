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
#include <algorithm>

#include "../testeventloop.h"

using namespace apl;

class FlexSequenceComponentTest : public DocumentWrapper {
public:
    void executeScroll(float distance) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "Scroll", alloc);
        cmd.AddMember("componentId", rapidjson::Value(":root", alloc).Move(), alloc);
        cmd.AddMember("distance", distance, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        executeCommands(doc, false);
    }

    void completeScroll(float distance) {
        ASSERT_FALSE(root->hasEvent());
        executeScroll(distance);
        advanceTime(1000);
    }

    rapidjson::Document doc;
};

const char* BASIC = R"apl({
  "type": "APL",
  "version": "2024.3",
  "theme": "dark",
  "layouts": {
    "TB": {
      "parameters": [ { "name": "TXT", "default": "-1" } ],
      "items": {
        "type": "Text",
        "height": "100%",
        "width": "100%",
        "text": "${TXT}",
        "textAlignVertical": "center",
        "textAlign": "center"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "FlexSequence",
      "height": 240,
      "width": 1000,
      "scrollDirection": "horizontal",
      "data": [
        "big", "small", "small", "small", "small", "small", "small",
        "big", "big", "big", "small", "small", "small", "small", "big",
        "small", "big", "small", "small", "small", "small", "small",
        "small", "big", "big", "big", "small", "small", "small", "small",
        "big", "small"
      ],
      "items": [
        {
          "when": "${data == 'small'}",
          "height": 120,
          "width": 120,
          "bind": [ { "name": "ItemData", "value": "${index}" } ],
          "type": "Frame",
          "borderWidth": 2,
          "borderColor": "grey",
          "item": {
            "type": "Text",
            "height": "100%",
            "width": "100%",
            "text": "${ItemData}",
            "textAlignVertical": "center",
            "textAlign": "center"
          }
        },
        {
          "height": "100%",
          "width": 160,
          "bind": [ { "name": "ItemData", "value": "${index}" } ],
          "type": "Frame",
          "borderWidth": 2,
          "borderColor": "grey",
          "item": {
            "type": "TB",
            "TXT": "${ItemData}"
          }
        }
      ]
    }
  }
})apl";

TEST_F(FlexSequenceComponentTest, Basic) {
    config->set(kSequenceChildCache, 0);

    loadDocument(BASIC);
    ASSERT_EQ(Size(160, 240), component->getCoreChildAt(0)->getCalculated(kPropertyBounds).get<Rect>().getSize());
    ASSERT_EQ(Size(120, 120), component->getCoreChildAt(1)->getCalculated(kPropertyBounds).get<Rect>().getSize());
    ASSERT_EQ(Size(120, 120), component->getCoreChildAt(2)->getCalculated(kPropertyBounds).get<Rect>().getSize());
    ASSERT_EQ(Size(120, 120), component->getCoreChildAt(3)->getCalculated(kPropertyBounds).get<Rect>().getSize());
    ASSERT_EQ(Size(120, 120), component->getCoreChildAt(4)->getCalculated(kPropertyBounds).get<Rect>().getSize());
    ASSERT_EQ(Size(120, 120), component->getCoreChildAt(5)->getCalculated(kPropertyBounds).get<Rect>().getSize());
    ASSERT_EQ(Size(120, 120), component->getCoreChildAt(6)->getCalculated(kPropertyBounds).get<Rect>().getSize());
    ASSERT_EQ(Size(160, 240), component->getCoreChildAt(7)->getCalculated(kPropertyBounds).get<Rect>().getSize());
    ASSERT_EQ(Size(160, 240), component->getCoreChildAt(8)->getCalculated(kPropertyBounds).get<Rect>().getSize());

    ASSERT_EQ(32, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 12}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {13, 31}, false));

    completeScroll(1);

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 21}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {22, 31}, false));
}

const char* CROSS_AXIS_ALIGN = R"({
  "type": "APL",
  "version": "2024.3",
  "theme": "dark",
  "layouts": {
    "TB": {
      "parameters": [ { "name": "TXT", "default": "-1" } ],
      "items": {
        "type": "Text",
        "height": "100%",
        "width": "100%",
        "text": "${TXT}",
        "textAlignVertical": "center",
        "textAlign": "center"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "FlexSequence",
      "alignItems": "center",
      "height": 260,
      "width": 1000,
      "scrollDirection": "horizontal",
      "data": [
        "big", "small", "small", "small", "small", "small", "small",
        "big", "big", "big", "small", "small", "small", "small", "big",
        "small", "big", "small", "small", "small", "small", "small",
        "small", "big", "big", "big", "small", "small", "small", "small",
        "big", "small"
      ],
      "items": [
        {
          "when": "${data == 'small'}",
          "height": 120,
          "width": 120,
          "bind": [ { "name": "ItemData", "value": "${index}" } ],
          "type": "Frame",
          "borderWidth": 2,
          "borderColor": "grey",
          "item": {
            "type": "Text",
            "height": "100%",
            "width": "100%",
            "text": "${ItemData}",
            "textAlignVertical": "center",
            "textAlign": "center"
          }
        },
        {
          "height": 200,
          "width": 160,
          "bind": [ { "name": "ItemData", "value": "${index}" } ],
          "type": "Frame",
          "borderWidth": 2,
          "borderColor": "grey",
          "item": {
            "type": "TB",
            "TXT": "${ItemData}"
          }
        }
      ]
    }
  }
})";

TEST_F(FlexSequenceComponentTest, Align) {
    config->set(kSequenceChildCache, 0);

    loadDocument(CROSS_AXIS_ALIGN);

    ASSERT_EQ(Rect(0, 30, 160, 200), component->getCoreChildAt(0)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(160, 10, 120, 120), component->getCoreChildAt(1)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(160, 130, 120, 120), component->getCoreChildAt(2)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(280, 10, 120, 120), component->getCoreChildAt(3)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(280, 130, 120, 120), component->getCoreChildAt(4)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(400, 10, 120, 120), component->getCoreChildAt(5)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(400, 130, 120, 120), component->getCoreChildAt(6)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(520, 30, 160, 200), component->getCoreChildAt(7)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(680, 30, 160, 200), component->getCoreChildAt(8)->getCalculated(kPropertyBounds).get<Rect>());

    ASSERT_EQ(32, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 12}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {13, 31}, false));

    completeScroll(1);

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 21}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {22, 31}, false));
}

const char* FLEX_SEQUENCE_AUTOSIZE = R"({
  "type": "APL",
  "version": "2024.3",
  "theme": "dark",
  "mainTemplate": {
    "items":
    {
      "type": "Container",
      "height": 800,
      "width": 800,
      "items": [
        {
          "type": "FlexSequence",
          "height": "auto",
          "width": 100,
          "scrollDirection": "horizontal"
        },
        {
          "type": "FlexSequence",
          "height": 100,
          "width": "auto",
          "scrollDirection": "vertical"
        }
      ]
    }
  }
})";

TEST_F(FlexSequenceComponentTest, AutoFix) {
    loadDocument(FLEX_SEQUENCE_AUTOSIZE);

    ASSERT_EQ(100, component->getCoreChildAt(0)->getCalculated(kPropertyBounds).get<Rect>().getHeight());
    ASSERT_EQ(100, component->getCoreChildAt(1)->getCalculated(kPropertyBounds).get<Rect>().getWidth());
}