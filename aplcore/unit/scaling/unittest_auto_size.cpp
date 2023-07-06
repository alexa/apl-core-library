/*
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
 *
 */

#include "../testeventloop.h"

using namespace apl;

class AutoSizeTest : public apl::DocumentWrapper {};

static const char *BASIC_TEST = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": 123,
      "height": 345
    }
  }
}
)apl";

TEST_F(AutoSizeTest, Basic)
{
    metrics.size(300, 300).autoSizeHeight(true).autoSizeWidth(true);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0,0,123,345), component->getCalculated(apl::kPropertyBounds)));
}

static const char *EMBEDDED_TEST = R"apl(
{
    "type": "APL",
    "version": "2022.2",
    "mainTemplate": {
        "item": {
            "type": "Frame",
            "item": {
                "type": "Frame",
                "width": 100,
                "height": 200
            }
        }
    }
}
)apl";

TEST_F(AutoSizeTest, Embedded)
{
    metrics.size(300, 300).autoSizeWidth(true);
    loadDocument(EMBEDDED_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0,0,100,300), component->getCalculated(apl::kPropertyBounds)));

    metrics.size(500,500).autoSizeWidth(false).autoSizeHeight(true);
    loadDocument(EMBEDDED_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0,0,500,200), component->getCalculated(apl::kPropertyBounds)));

    metrics.size(400,400).autoSizeWidth(true).autoSizeHeight(true);
    loadDocument(EMBEDDED_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0,0,100,200), component->getCalculated(apl::kPropertyBounds)));
}

static const char *SCROLL_VIEW = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "ScrollView",
      "item": {
        "type": "Frame",
        "width": 300,
        "height": 1000
      }
    }
  }
}
)apl";

TEST_F(AutoSizeTest, ScrollView)
{
    // The ScrollView defaults to an auto-sized width and a height of 100.
    metrics.autoSizeWidth(true).autoSizeHeight(true);
    loadDocument(SCROLL_VIEW);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0,0,300,100), component->getCalculated(apl::kPropertyBounds)));
}

static const char *RESIZING = R"apl(
{
    "type": "APL",
    "version": "2022.2",
    "mainTemplate": {
        "item": {
            "type": "Frame",
            "borderWidth": 1,
            "item": {
                "type": "Frame",
                "id": "FOO",
                "width": 10,
                "height": 20
            }
        }
    }
}
)apl";

TEST_F(AutoSizeTest, Resizing)
{
    auto doInitialize = [&](const char *document, float width, float height) -> ::testing::AssertionResult {
        loadDocument(document);
        if (!component)
            return ::testing::AssertionFailure() << "Failed to load document";
        return IsEqual(Rect(0, 0, width, height), component->getCalculated(kPropertyBounds));
    };

    auto doTest = [&](const char *property, int value, float width, float height) -> ::testing::AssertionResult {
        executeCommand("SetValue", {{"componentId", "FOO"}, {"property", property}, {"value", value}}, true);
        root->clearPending();
        return IsEqual(Rect(0,0,width,height), component->getCalculated(kPropertyBounds));
    };

    // Allow resizing in both direction
    metrics.size(100,200).autoSizeWidth(true).autoSizeHeight(true);
    ASSERT_TRUE(doInitialize(RESIZING, 12, 22));
    ASSERT_TRUE(doTest("width", 40, 42, 22));
    ASSERT_TRUE(doTest("height", 70, 42, 72));

    // Auto-size width
    metrics.size(100,200).autoSizeWidth(true).autoSizeHeight(false);
    ASSERT_TRUE(doInitialize(RESIZING, 12, 200));
    ASSERT_TRUE(doTest("width", 40, 42, 200));
    ASSERT_TRUE(doTest("height", 70, 42, 200));

    // Auto-size height
    metrics.size(100,200).autoSizeWidth(false).autoSizeHeight(true);
    ASSERT_TRUE(doInitialize(RESIZING, 100, 22));
    ASSERT_TRUE(doTest("width", 40, 100, 22));
    ASSERT_TRUE(doTest("height", 70, 100, 72));

    // No auto-sizing
    metrics.size(100,200).autoSizeWidth(false).autoSizeHeight(false);
    ASSERT_TRUE(doInitialize(RESIZING, 100, 200));
    ASSERT_TRUE(doTest("width", 40, 100, 200));
    ASSERT_TRUE(doTest("height", 70, 100, 200));
}