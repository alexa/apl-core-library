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

using namespace apl;

class AccessibilityPropertiesTest : public DocumentWrapper {};

static const char *BASIC_TEST =  R"apl(
    {
      "type": "APL",
      "version": "2024.1",
      "mainTemplate": {
        "item": {
          "type": "TouchWrapper",
          "id": "touch",
          "width": 100,
          "height": 100,
          "role": "adjustable",
          "accessibilityAdjustableValue": "50",
          "accessibilityAdjustableRange": {
            "minValue": 0,
            "maxValue": 100,
            "currentValue": 50
          }
        }
      }
    }
)apl";

TEST_F(AccessibilityPropertiesTest, Basic)
{
    loadDocument(BASIC_TEST);
    auto component = root->findComponentById("touch");
    auto jsonData = JsonData(R"({"minValue": 0, "maxValue": 100, "currentValue": 50})");
    ASSERT_EQ("50", component->getCalculated(kPropertyAccessibilityAdjustableValue).asString());
    ASSERT_EQ(Object(jsonData.get()), component->getCalculated(kPropertyAccessibilityAdjustableRange));
}

static const char *ACCESSIBILITY_ADJUSTABLE_RANGE_PROPERTY_MISSING = R"apl(
    {
      "type": "APL",
      "version": "2024.1",
      "mainTemplate": {
        "item": {
          "type": "TouchWrapper",
          "id": "touch",
          "width": 100,
          "height": 100,
          "role": "adjustable",
          "accessibilityAdjustableValue": "50"
        }
      }
    }
)apl";

TEST_F(AccessibilityPropertiesTest, AccessibilityAdjustableRangePropertyMissing)
{
    loadDocument(ACCESSIBILITY_ADJUSTABLE_RANGE_PROPERTY_MISSING);
    auto component = root->findComponentById("touch");
    ASSERT_EQ("50", component->getCalculated(kPropertyAccessibilityAdjustableValue).asString());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyAccessibilityAdjustableRange));
}

static const char *ACCESSIBILITY_ADJUSTABLE_RANGE_PROPERTY_INCOMPLETE = R"apl(
    {
      "type": "APL",
      "version": "2024.1",
      "mainTemplate": {
        "item": {
          "type": "TouchWrapper",
          "id": "touch",
          "width": 100,
          "height": 100,
          "role": "adjustable",
          "accessibilityAdjustableValue": "50",
          "accessibilityAdjustableRange": {
            "minValue": 0,
            "currentValue": 50
          }
        }
      }
    }
)apl";

TEST_F(AccessibilityPropertiesTest, AccessibilityAdjustableRangePropertyIncomplete)
{
    loadDocument(ACCESSIBILITY_ADJUSTABLE_RANGE_PROPERTY_INCOMPLETE);
    auto component = root->findComponentById("touch");
    ASSERT_EQ("50", component->getCalculated(kPropertyAccessibilityAdjustableValue).asString());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyAccessibilityAdjustableRange));
}

static const char *ACCESSIBILITY_ADJUSTABLE_RANGE_PROPERTY_NOT_MAP = R"apl(
    {
      "type": "APL",
      "version": "2024.1",
      "mainTemplate": {
        "item": {
          "type": "TouchWrapper",
          "id": "touch",
          "width": 100,
          "height": 100,
          "role": "adjustable",
          "accessibilityAdjustableValue": "50",
          "accessibilityAdjustableRange": [0, 100]
        }
      }
    }
)apl";

TEST_F(AccessibilityPropertiesTest, AccessibilityAdjustableRangePropertyNotMap)
{
    loadDocument(ACCESSIBILITY_ADJUSTABLE_RANGE_PROPERTY_NOT_MAP);
    auto component = root->findComponentById("touch");
    ASSERT_EQ("50", component->getCalculated(kPropertyAccessibilityAdjustableValue).asString());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyAccessibilityAdjustableRange));
}

static const char *ACCESSIBILITY_ADJUSTABLE_VALUE_PROPERTY_MISSING = R"apl(
    {
      "type": "APL",
      "version": "2024.1",
      "mainTemplate": {
        "item": {
          "type": "TouchWrapper",
          "id": "touch",
          "width": 100,
          "height": 100,
          "role": "adjustable",
          "accessibilityAdjustableRange": {
            "minValue": 0,
            "maxValue": 100,
            "currentValue": 50
          }
        }
      }
    }
)apl";

TEST_F(AccessibilityPropertiesTest, AccessibilityAdjustableValuePropertyMissing)
{
    loadDocument(ACCESSIBILITY_ADJUSTABLE_VALUE_PROPERTY_MISSING);
    auto component = root->findComponentById("touch");
    auto jsonData = JsonData(R"({"minValue": 0, "maxValue": 100, "currentValue": 50})");
    ASSERT_EQ("", component->getCalculated(kPropertyAccessibilityAdjustableValue).asString());
    ASSERT_EQ(Object(jsonData.get()), component->getCalculated(kPropertyAccessibilityAdjustableRange));
}