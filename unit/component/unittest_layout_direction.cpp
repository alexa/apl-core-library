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

class LayoutDirectionText : public DocumentWrapper {};

// LayoutDirection component level override
static const char *COMPONENT_LEVEL_OVERRIDE = R"(
    {
      "type": "APL",
      "version": "1.7",
      "layouts": {
        "Box":{
          "parameters": [ "label" ],
          "items": {
            "type": "Frame",
            "layoutDirection": "${label}",
            "id": "Frame_${label}",
            "width": 100,
            "height": 100
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "id": "c1",
          "layoutDirection": "RTL",
          "items": [
            { "type": "Box", "label": "inherit" },
            { "type": "Box", "label": "LTR" },
            { "type": "Box", "label": "RTL" }
          ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, ComponentLevelOverride)
{
    loadDocument(COMPONENT_LEVEL_OVERRIDE);
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(3, component->getChildCount());

    auto child = component->getChildAt(0); // First child is "inherit"
    ASSERT_EQ(Object(kLayoutDirectionRTL), child->getCalculated(kPropertyLayoutDirection));

    child = component->getChildAt(1); // Second child is "LTR"
    ASSERT_EQ(Object(kLayoutDirectionLTR), child->getCalculated(kPropertyLayoutDirection));

    child = component->getChildAt(2); // Third child is "RTL"
    ASSERT_EQ(Object(kLayoutDirectionRTL), child->getCalculated(kPropertyLayoutDirection));

    // parent's layoutDirection changed
    component->setProperty(kPropertyLayoutDirectionAssigned, "LTR");
    ASSERT_TRUE(root->isDirty());

    child = component->getChildAt(0); // First child is "inherit"
    ASSERT_TRUE(CheckDirty(child, kPropertyBounds, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(Object(kLayoutDirectionLTR), child->getCalculated(kPropertyLayoutDirection));

    child = component->getChildAt(1); // Second child is "LTR"
    ASSERT_FALSE(CheckDirty(child, kPropertyBounds, kPropertyLayoutDirection));
    ASSERT_EQ(Object(kLayoutDirectionLTR), child->getCalculated(kPropertyLayoutDirection));

    child = component->getChildAt(2); // Third child is "RTL"
    ASSERT_FALSE(CheckDirty(child, kPropertyBounds, kPropertyLayoutDirection));
    ASSERT_EQ(Object(kLayoutDirectionRTL), child->getCalculated(kPropertyLayoutDirection));
}

// inserted child also inherits the layoutDirection from parent
static const char *INSERT_ELEMENT_INHERIT = R"(
    {
      "type": "Frame",
      "layoutDirection": "inherit",
      "id": "Frame_inserted",
      "width": 100,
      "height": 100
    }
)";

TEST_F(LayoutDirectionText, DynamicComponent)
{
    loadDocument(COMPONENT_LEVEL_OVERRIDE);
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));

    // Insert the child
    JsonData data(INSERT_ELEMENT_INHERIT);
    auto child = component->getContext()->inflate(data.get());
    ASSERT_TRUE(child);
    ASSERT_TRUE(component->insertChild(child, 0));
    root->clearPending();
    ASSERT_EQ(4, component->getChildCount());
    // The child has inherited LayoutDirection as RTL
    ASSERT_EQ(Object(kLayoutDirectionRTL), child->getCalculated(kPropertyLayoutDirection));

    // Remove the child and change the parent's layoutDirection
    child->remove();
    root->clearPending();
    ASSERT_EQ(3, component->getChildCount());
    component->setProperty(kPropertyLayoutDirectionAssigned, "LTR");
    // insert child again, and layoutDirection should be set to LTR
    component->insertChild(child, 0);
    root->clearPending();
    ASSERT_EQ(4, component->getChildCount());
    ASSERT_EQ(Object(kLayoutDirectionLTR), child->getCalculated(kPropertyLayoutDirection));
}

// Container component's flexbox respond to RTL
/*
 * |x x 3 2 1|
 */
static const char *RTL_THREE_CHILDREN_WIDE = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "layoutDirection": "RTL",
          "paddingLeft": 10,
          "paddingRight": 20,
          "paddingTop": 30,
          "paddingBottom": 40,
          "direction": "row",
          "items": {
            "type": "Frame",
            "width": 100,
            "height": 200,
            "paddingLeft": 1,
            "paddingRight": 2,
            "paddingTop": 3,
            "paddingBottom": 4
          },
          "data": [
            1,
            2,
            3
          ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLThreeChildrenWide)
{
    loadDocument(RTL_THREE_CHILDREN_WIDE);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(Rect(10, 30, 994, 730), component->getCalculated(kPropertyInnerBounds).getRect());
    ASSERT_EQ(3, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(904, 30, 100, 200), child->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(1, 3, 97, 193), child->getCalculated(kPropertyInnerBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(804, 30, 100, 200), child->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(1, 3, 97, 193), child->getCalculated(kPropertyInnerBounds).getRect());

    child = component->getChildAt(2);
    ASSERT_EQ(Rect(704, 30, 100, 200), child->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(1, 3, 97, 193), child->getCalculated(kPropertyInnerBounds).getRect());
}

/*
 * |  1|
 * |  2|
 * |  3|
 * |  x|
 */
static const char *RTL_OVERLY_TALL_CHILDREN = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "layoutDirection": "RTL",
          "items": {
            "type": "Frame",
            "width": 100,
            "height": 400
          },
          "data": [
            1,
            2,
            3
          ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLOverlyTallChildren)
{
    loadDocument(RTL_OVERLY_TALL_CHILDREN);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(3, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(924, 0, 100, 400), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(924, 400, 100, 400), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);
    ASSERT_EQ(Rect(924, 800, 100, 400), child->getCalculated(kPropertyBounds).getRect());
}

/*
 *   1 0
 *   3 2
 *   5 4
 *   x x
 *   x x
 */
static const char *RTL_WRAP_TEST_ROW = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "layoutDirection": "RTL",
          "wrap": "wrap",
          "height": 500,
          "width": 200,
          "direction": "row",
          "items": {
            "type": "Frame",
            "id": "Frame_${data}",
            "width": 100,
            "height": 100
          },
          "data": [ 0, 1, 2, 3, 4, 5 ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLWrapTestRow)
{
    loadDocument(RTL_WRAP_TEST_ROW);
    ASSERT_TRUE(IsEqual(Rect(0,0,200,500), component->getCalculated(kPropertyBounds)));
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(6, component->getChildCount());

    for (int i = 0 ; i < 6 ; i++) {
        auto child = component->getChildAt(i);
        auto expected = "Frame_" + std::to_string(i);
        ASSERT_EQ(expected, child->getId());
        auto x = i % 2 == 0 ? 100 : 0;
        auto y = 100 * (i / 2);
        ASSERT_TRUE(IsEqual(Rect(x, y, 100, 100), child->getCalculated(kPropertyBounds))) << expected;
    }
}

/*
 *   5 0
 *   x 1
 *   x 2
 *   x 3
 *   x 4
 */
static const char *RTL_WRAP_TEST_COLUMN = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "layoutDirection": "RTL",
          "wrap": "wrap",
          "height": 500,
          "width": 200,
          "direction": "column",
          "items": {
            "type": "Frame",
            "id": "Frame_${data}",
            "width": 100,
            "height": 100
          },
          "data": [ 0, 1, 2, 3, 4, 5 ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLWrapTestColumn)
{
    loadDocument(RTL_WRAP_TEST_COLUMN);
    ASSERT_TRUE(IsEqual(Rect(0,0,200,500), component->getCalculated(kPropertyBounds)));
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(6, component->getChildCount());

    for (int i = 0 ; i < 6 ; i++) {
        auto child = component->getChildAt(i);
        auto expected = "Frame_" + std::to_string(i);
        ASSERT_EQ(expected, child->getId());
        auto x = i < 5 ? 100 : 0;
        auto y = 100 * (i % 5);
        ASSERT_TRUE(IsEqual(Rect(x, y, 100, 100), child->getCalculated(kPropertyBounds))) << expected;
    }
}

/*
 *   x x
 *   x x
 *   5 4
 *   3 2
 *   1 0
 */
static const char *RTL_WRAP_TEST_ROW_REVERSE = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "layoutDirection": "RTL",
          "wrap": "wrap-reverse",
          "height": 500,
          "width": 200,
          "direction": "row",
          "items": {
            "type": "Frame",
            "id": "Frame_${data}",
            "width": 100,
            "height": 100
          },
          "data": [ 0, 1, 2, 3, 4, 5 ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLWrapTestRowReverse)
{
    loadDocument(RTL_WRAP_TEST_ROW_REVERSE);
    ASSERT_TRUE(IsEqual(Rect(0,0,200,500), component->getCalculated(kPropertyBounds)));
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(6, component->getChildCount());

    for (int i = 0 ; i < 6 ; i++) {
        auto child = component->getChildAt(i);
        auto expected = "Frame_" + std::to_string(i);
        ASSERT_EQ(expected, child->getId());
        auto x = i % 2 == 0 ? 100 : 0;
        auto y = 400 - 100 * (i / 2);
        ASSERT_TRUE(IsEqual(Rect(x, y, 100, 100), child->getCalculated(kPropertyBounds))) << expected;
    }
}

/*
 *   0 5
 *   1 x
 *   2 x
 *   3 x
 *   4 x
 */
static const char *RTL_WRAP_TEST_COLUMN_REVERSE = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "layoutDirection": "RTL",
          "wrap": "wrapReverse",
          "height": 500,
          "width": 200,
          "direction": "column",
          "items": {
            "type": "Frame",
            "id": "Frame_${data}",
            "width": 100,
            "height": 100
          },
          "data": [ 0, 1, 2, 3, 4, 5 ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLWrapTestColumnReverse)
{
    loadDocument(RTL_WRAP_TEST_COLUMN_REVERSE);
    ASSERT_TRUE(IsEqual(Rect(0,0,200,500), component->getCalculated(kPropertyBounds)));
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(6, component->getChildCount());

    for (int i = 0 ; i < 6 ; i++) {
        auto child = component->getChildAt(i);
        auto expected = "Frame_" + std::to_string(i);
        ASSERT_EQ(expected, child->getId());
        auto x = i < 5 ? 0 : 100;
        auto y = 100 * (i % 5);
        ASSERT_TRUE(IsEqual(Rect(x, y, 100, 100), child->getCalculated(kPropertyBounds))) << expected;
    }
}

/*
 *   0 1
 *   2 3
 *   4 5
 *   x x
 *   x x
 */
static const char *RTL_WRAP_TEST_REVERSE_ROW = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "layoutDirection": "RTL",
          "wrap": "wrap",
          "height": 500,
          "width": 200,
          "direction": "row-reverse",
          "items": {
            "type": "Frame",
            "id": "Frame_${data}",
            "width": 100,
            "height": 100
          },
          "data": [ 0, 1, 2, 3, 4, 5 ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLWrapTestReverseRow)
{
    loadDocument(RTL_WRAP_TEST_REVERSE_ROW);
    ASSERT_TRUE(IsEqual(Rect(0,0,200,500), component->getCalculated(kPropertyBounds)));
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(6, component->getChildCount());

    for (int i = 0 ; i < 6 ; i++) {
        auto child = component->getChildAt(i);
        auto expected = "Frame_" + std::to_string(i);
        ASSERT_EQ(expected, child->getId());
        auto x = i % 2 == 0 ? 0 : 100;
        auto y = 100 * (i / 2);
        ASSERT_TRUE(IsEqual(Rect(x, y, 100, 100), child->getCalculated(kPropertyBounds))) << expected;
    }
}

/*
 *   x 4
 *   x 3
 *   x 2
 *   x 1
 *   5 0
 */
static const char *RTL_WRAP_TEST_REVERSE_COLUMN = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "layoutDirection": "RTL",
          "wrap": "wrap",
          "height": 500,
          "width": 200,
          "direction": "column-reverse",
          "items": {
            "type": "Frame",
            "id": "Frame_${data}",
            "width": 100,
            "height": 100
          },
          "data": [ 0, 1, 2, 3, 4, 5 ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLWrapTestReverseColumn)
{
    loadDocument(RTL_WRAP_TEST_REVERSE_COLUMN);
    ASSERT_TRUE(IsEqual(Rect(0,0,200,500), component->getCalculated(kPropertyBounds)));
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(6, component->getChildCount());

    for (int i = 0 ; i < 6 ; i++) {
        auto child = component->getChildAt(i);
        auto expected = "Frame_" + std::to_string(i);
        ASSERT_EQ(expected, child->getId());
        auto x = i < 5 ? 100 : 0;
        auto y = 400 - 100 * (i % 5);
        ASSERT_TRUE(IsEqual(Rect(x, y, 100, 100), child->getCalculated(kPropertyBounds))) << expected;
    }
}

/*
 *   x x
 *   x x
 *   4 5
 *   2 3
 *   0 1
 */
static const char *RTL_WRAP_TEST_REVERSE_ROW_REVERSE = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "layoutDirection": "RTL",
          "wrap": "wrap-reverse",
          "height": 500,
          "width": 200,
          "direction": "rowReverse",
          "items": {
            "type": "Frame",
            "id": "Frame_${data}",
            "width": 100,
            "height": 100
          },
          "data": [ 0, 1, 2, 3, 4, 5 ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, WrapTestReverseRowReverse)
{
    loadDocument(RTL_WRAP_TEST_REVERSE_ROW_REVERSE);
    ASSERT_TRUE(IsEqual(Rect(0,0,200,500), component->getCalculated(kPropertyBounds)));
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(6, component->getChildCount());

    for (int i = 0 ; i < 6 ; i++) {
        auto child = component->getChildAt(i);
        auto expected = "Frame_" + std::to_string(i);
        ASSERT_EQ(expected, child->getId());
        auto x = i % 2 == 0 ? 0 : 100;
        auto y = 400 - 100 * (i / 2);
        ASSERT_TRUE(IsEqual(Rect(x, y, 100, 100), child->getCalculated(kPropertyBounds))) << expected;
    }
}

/*
 *   4 x
 *   3 x
 *   2 x
 *   1 x
 *   0 5
 */
static const char *WRAP_TEST_REVERSE_COLUMN_REVERSE = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "layoutDirection": "RTL",
          "wrap": "wrapReverse",
          "height": 500,
          "width": 200,
          "direction": "columnReverse",
          "items": {
            "type": "Frame",
            "id": "Frame_${data}",
            "width": 100,
            "height": 100
          },
          "data": [ 0, 1, 2, 3, 4, 5 ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, WrapTestReverseColumnReverse)
{
    loadDocument(WRAP_TEST_REVERSE_COLUMN_REVERSE);
    ASSERT_TRUE(IsEqual(Rect(0,0,200,500), component->getCalculated(kPropertyBounds)));
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(6, component->getChildCount());

    for (int i = 0 ; i < 6 ; i++) {
        auto child = component->getChildAt(i);
        auto expected = "Frame_" + std::to_string(i);
        ASSERT_EQ(expected, child->getId());
        auto x = i < 5 ? 0 : 100;
        auto y = 400 - 100 * (i % 5);
        ASSERT_TRUE(IsEqual(Rect(x, y, 100, 100), child->getCalculated(kPropertyBounds))) << expected;
    }
}

// Container property responds to RTL
static const char *RTL_ABSOLUTE_POSITION = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "layoutDirection": "RTL",
          "items": {
            "type": "Frame",
            "position": "absolute",
            "width": 100,
            "height": 100,
            "left": 5,
            "top": 10,
            "bottom": 15,
            "right": 20
          }
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLAbsolutePosition)
{
    // If top is set, bottom is ignored.
    // Also in RTL, if right is set, left is ignored.
    loadDocument(RTL_ABSOLUTE_POSITION);
    ASSERT_EQ(Rect(0, 0, 1024, 800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(1, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(904, 10, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

static const char *RTL_RELATIVE_POSITION = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "layoutDirection": "RTL",
          "items": {
            "type": "Frame",
            "position": "relative",
            "width": 100,
            "height": 100,
            "left": 5,
            "top": 10,
            "bottom": 15,
            "right": 20
          }
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLRelativePosition)
{
    // If top is set, bottom is ignored.
    // Also in RTL, if right is set, left is ignored.
    loadDocument(RTL_RELATIVE_POSITION);
    ASSERT_EQ(Rect(0, 0, 1024, 800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(1, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(904, 10, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

static const char *RTL_ALIGN_ITEMS_START = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "layoutDirection": "RTL",
          "alignItems": "start",
          "items": {
            "type": "Frame",
            "height": 100,
            "width": 100,
            "alignSelf": "${data}"
          },
          "data": [
            "auto",
            "start",
            "end",
            "center"
          ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLAlignItemsStart)
{
    loadDocument(RTL_ALIGN_ITEMS_START);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(4, component->getChildCount());

    auto child = component->getChildAt(0);  // First child is "auto", which will be right-aligned
    ASSERT_EQ(Rect(924, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);  // Second child is "start", which will be right-aligned
    ASSERT_EQ(Rect(924, 100, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);  // Third child is "end", which will be left-aligned
    ASSERT_EQ(Rect(0, 200, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(3);  // Fourth child is "center", which will be centered
    ASSERT_EQ(Rect(462, 300, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

static const char *RTL_ALIGN_ITEMS_END = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "layoutDirection": "RTL",
          "alignItems": "end",
          "items": {
            "type": "Frame",
            "height": 100,
            "width": 100,
            "alignSelf": "${data}"
          },
          "data": [
            "auto",
            "start",
            "end",
            "center"
          ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLAlignItemsEnd)
{
    loadDocument(RTL_ALIGN_ITEMS_END);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Object(kLayoutDirectionRTL), component->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(4, component->getChildCount());

    auto child = component->getChildAt(0);  // First child is "auto", which will be left-aligned
    ASSERT_EQ(Rect(0, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);  // Second child is "start", which will be right-aligned
    ASSERT_EQ(Rect(924, 100, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(2);  // Third child is "end", which will be left-aligned
    ASSERT_EQ(Rect(0, 200, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(3);  // Fourth child is "center", which will be centered
    ASSERT_EQ(Rect(462, 300, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

/*
 * |2 1  |
 */
static const char *RTL_JUSTIFY_END = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "layoutDirection": "RTL",
          "direction": "row",
          "justifyContent": "end",
          "items": {
            "type": "Frame",
            "width": 100,
            "height": 100
          },
          "data": [
            1,
            2
          ]
        }
      }
    }
)";

TEST_F(LayoutDirectionText, RTLJustifyEnd)
{
    loadDocument(RTL_JUSTIFY_END);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(2, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(Rect(100, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());

    child = component->getChildAt(1);
    ASSERT_EQ(Rect(0, 0, 100, 100), child->getCalculated(kPropertyBounds).getRect());
}

static const char *DOC_LAYOUTDIRECTION_PROPERTY_DEFAULT = R"(
{
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      }
    }
  }
}
)";

/**
 * Test the default layout direction
 */
TEST_F(LayoutDirectionText, LayoutDirectionDefaultValues)
{
    loadDocument(DOC_LAYOUTDIRECTION_PROPERTY_DEFAULT);
    auto ld1 = static_cast<LayoutDirection>(component->getCalculated(kPropertyLayoutDirection).asInt());
    ASSERT_EQ(ld1, kLayoutDirectionLTR);

    auto ld2 = static_cast<LayoutDirection>(component->getChildAt(0)->getCalculated(kPropertyLayoutDirection).asInt());
    ASSERT_EQ(ld2, kLayoutDirectionLTR);
}

static const char *DOC_LAYOUTDIRECTION_PROPERTY_SHADOW_DOC = R"(
{
  "type": "APL",
  "version": "1.7",
  "layoutDirection": "RTL",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      }
    }
  }
}
)";

/**
 * Check to make sure we shadow the document value as expected
 */
TEST_F(LayoutDirectionText, LayoutDirectionShadowDocument)
{
    loadDocument(DOC_LAYOUTDIRECTION_PROPERTY_SHADOW_DOC);
    auto ld1 = static_cast<LayoutDirection>(component->getCalculated(kPropertyLayoutDirection).asInt());
    ASSERT_EQ(ld1, kLayoutDirectionRTL);

    auto ld2 = static_cast<LayoutDirection>(component->getChildAt(0)->getCalculated(kPropertyLayoutDirection).asInt());
    ASSERT_EQ(ld2, kLayoutDirectionRTL);
}

static const char *DOC_LAYOUTDIRECTION_PROPERTY_NO_INHERIT = R"(
{
  "type": "APL",
  "version": "1.7",
  "layoutDirection": "RTL",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "layoutDirection": "LTR",
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      }
    }
  }
}
)";

/**
 * Check to make sure we don't shadow the document value if the value is set in a component
 */
TEST_F(LayoutDirectionText, LayoutDirectionNoInherit)
{
    loadDocument(DOC_LAYOUTDIRECTION_PROPERTY_NO_INHERIT);
    auto ld1 = static_cast<LayoutDirection>(component->getCalculated(kPropertyLayoutDirection).asInt());
    ASSERT_EQ(ld1, kLayoutDirectionLTR);

    auto ld2 = static_cast<LayoutDirection>(component->getChildAt(0)->getCalculated(kPropertyLayoutDirection).asInt());
    ASSERT_EQ(ld2, kLayoutDirectionLTR);
}


static const char *DOC_LAYOUTDIRECTION_PROPERTY_BAD_VALUE = R"(
{
  "type": "APL",
  "version": "1.7",
  "layoutDirection": "badvalue",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      }
    }
  }
}
)";

/**
 * Check we get an warning in the logs when we use a bad value
 */
TEST_F(LayoutDirectionText, LayoutDirectionBadValue) {
    loadDocument(DOC_LAYOUTDIRECTION_PROPERTY_BAD_VALUE);
    auto ld1 =
        static_cast<LayoutDirection>(component->getCalculated(kPropertyLayoutDirection).asInt());
    ASSERT_EQ(ld1, kLayoutDirectionLTR);

    auto ld2 = static_cast<LayoutDirection>(
        component->getChildAt(0)->getCalculated(kPropertyLayoutDirection).asInt());
    ASSERT_EQ(ld2, kLayoutDirectionLTR);

    ASSERT_TRUE(LogMessage()); // There should be a warning "Document 'layoutDirection' property is invalid : badvalue"
}

static const char *DOC_LAYOUTDIRECTION_PROPERTY_BAD_INHERIT_VALUE = R"(
{
  "type": "APL",
  "version": "1.7",
  "layoutDirection": "inherit",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      }
    }
  }
}
)";

/**
 * Check we get an warning in the logs when we use 'inherit' at the document level
 */
TEST_F(LayoutDirectionText, LayoutDirectionBadInheritValue)
{
    loadDocument(DOC_LAYOUTDIRECTION_PROPERTY_BAD_INHERIT_VALUE);
    auto ld1 = static_cast<LayoutDirection>(component->getCalculated(kPropertyLayoutDirection).asInt());
    ASSERT_EQ(ld1, kLayoutDirectionLTR);

    auto ld2 = static_cast<LayoutDirection>(component->getChildAt(0)->getCalculated(kPropertyLayoutDirection).asInt());
    ASSERT_EQ(ld2, kLayoutDirectionLTR);

    ASSERT_TRUE(LogMessage());  // There should be a warning "Document 'layoutDirection' can not be 'Inherit'"
}