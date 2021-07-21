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

class AccessibilityApiTest : public DocumentWrapper {};

static const char *BASIC_TEST = R"apl({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "id": "root",
        "items": [
          {
            "type": "Frame",
            "id": "notAccessibleFrame",
            "height": 100,
            "width": 100
          },
          {
            "type": "Container",
            "id": "transparentParent",
            "opacity": 0.5,
            "height": 200,
            "width": 200,
            "items": [
              {
                "type": "TouchWrapper",
                "id": "slightlyTransparent",
                "height": 100,
                "width": 100
              }
            ]
          },
          {
            "type": "Frame",
            "id": "accessibleFrame",
            "accessibilityLabel": "label",
            "height": 100,
            "width": 100
          },
          {
            "type": "Sequence",
            "id": "sequence",
            "height": 100,
            "width": 100,
            "data": [0,1,2],
            "item": {
              "type": "Frame",
              "height": 100,
              "width": 100
            }
          },
          {
            "type": "Pager",
            "id": "pager",
            "height": 100,
            "width": 100,
            "data": [0,1,2],
            "navigation": "wrap",
            "item": {
              "type": "Frame",
              "height": 100,
              "width": 100
            }
          },
          {
            "type": "VectorGraphic",
            "id": "nonAccessibleVG",
            "height": 100,
            "width": 100
          },
          {
            "type": "VectorGraphic",
            "id": "accessibleVG",
            "height": 100,
            "width": 100,
            "onPress": {
              "type": "SendEvent"
            }
          }
        ]
      }
    ]
  }
})apl";

TEST_F(AccessibilityApiTest, Basic)
{
    loadDocument(BASIC_TEST);

    auto notAccessibleFrame = component->findComponentById("notAccessibleFrame");
    ASSERT_FALSE(notAccessibleFrame->isFocusable());
    ASSERT_FALSE(notAccessibleFrame->isAccessible());

    auto transparentParent = component->findComponentById("transparentParent");
    ASSERT_FALSE(transparentParent->isFocusable());
    ASSERT_FALSE(transparentParent->isAccessible());
    ASSERT_EQ(Object(0.5), transparentParent->getCalculated(kPropertyOpacity));

    auto slightlyTransparent = component->findComponentById("slightlyTransparent");
    ASSERT_TRUE(slightlyTransparent->isFocusable());
    ASSERT_TRUE(slightlyTransparent->isAccessible());

    Rect rect;
    slightlyTransparent->getBoundsInParent(transparentParent, rect);
    ASSERT_EQ(Rect(0, 0, 100, 100), rect);
    slightlyTransparent->getBoundsInParent(component, rect);
    ASSERT_EQ(Rect(0, 100, 100, 100), rect);

    auto accessibleFrame = component->findComponentById("accessibleFrame");
    ASSERT_FALSE(accessibleFrame->isFocusable());
    ASSERT_TRUE(accessibleFrame->isAccessible());

    auto sequence = component->findComponentById("sequence");
    ASSERT_TRUE(sequence->isFocusable());
    ASSERT_TRUE(sequence->isAccessible());
    ASSERT_TRUE(sequence->allowForward());
    ASSERT_FALSE(sequence->allowBackwards());

    auto pager = component->findComponentById("pager");
    ASSERT_TRUE(pager->isFocusable());
    ASSERT_TRUE(pager->isAccessible());
    ASSERT_TRUE(pager->allowForward());
    ASSERT_TRUE(pager->allowBackwards());

    auto nonAccessibleVG = component->findComponentById("nonAccessibleVG");
    ASSERT_FALSE(nonAccessibleVG->isFocusable());
    ASSERT_FALSE(nonAccessibleVG->isAccessible());

    auto accessibleVG = component->findComponentById("accessibleVG");
    ASSERT_TRUE(accessibleVG->isFocusable());
    ASSERT_TRUE(accessibleVG->isAccessible());
}
