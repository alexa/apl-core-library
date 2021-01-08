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
#include "apl/graphic/graphicdependant.h"

using namespace apl;

class GraphicDataTest : public DocumentWrapper {};

static const char *MULTIPLE_TOP_LEVEL_CHILDREN = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "graphics": {
        "box": {
          "type": "AVG",
          "version": "1.2",
          "height": 500,
          "width": 500,
          "items": [
            {
              "type": "text",
              "text": "Alpha ${index} of ${length}"
            },
            {
              "type": "text",
              "text": "Bravo ${index} of ${length}"
            },
            {
              "type": "text",
              "when": false,
              "text": "Charlie ${index} of ${length}"
            },
            {
              "type": "text",
              "text": "Delta ${index} of ${length}"
            }
          ]
        }
      },
      "mainTemplate": {
        "items": [
          {
            "type": "VectorGraphic",
            "source": "box"
          }
        ]
      }
    }
)apl";

// Inflate an array of children with "when" clauses
TEST_F(GraphicDataTest, MultipleTopLevelChildren)
{
    loadDocument(MULTIPLE_TOP_LEVEL_CHILDREN);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    auto box = graphic->getRoot();
    ASSERT_TRUE(box);

    ASSERT_EQ(3, box->getChildCount());
    ASSERT_TRUE(IsEqual("Alpha 0 of 4", box->getChildAt(0)->getValue(kGraphicPropertyText)));
    ASSERT_TRUE(IsEqual("Bravo 1 of 4", box->getChildAt(1)->getValue(kGraphicPropertyText)));
    ASSERT_TRUE(IsEqual("Delta 2 of 4", box->getChildAt(2)->getValue(kGraphicPropertyText)));
}

static const char *DATA_BINDING = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "graphics": {
        "box": {
          "type": "AVG",
          "version": "1.2",
          "height": 500,
          "width": 500,
          "items": {
            "type": "text",
            "text": "${data} [${data == 'Sunday' || data == 'Saturday' ? 'weekend' : 'weekday'}] ${index} ${length}"
          },
          "data": [
            "Sunday",
            "Monday",
            "Tuesday",
            "Wednesday",
            "Thursday",
            "Friday",
            "Saturday"
          ]
        }
      },
      "mainTemplate": {
        "items": [
          {
            "type": "VectorGraphic",
            "source": "box"
          }
        ]
      }
    }
)apl";

// Inflate a single item with an array of data items
TEST_F(GraphicDataTest, DataBinding)
{
    std::vector<std::string> EXPECTED = {
        "Sunday [weekend] 0 7",
        "Monday [weekday] 1 7",
        "Tuesday [weekday] 2 7",
        "Wednesday [weekday] 3 7",
        "Thursday [weekday] 4 7",
        "Friday [weekday] 5 7",
        "Saturday [weekend] 6 7"
    };

    loadDocument(DATA_BINDING);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    auto box = graphic->getRoot();
    ASSERT_TRUE(box);

    ASSERT_EQ(EXPECTED.size(), box->getChildCount());
    for (int i = 0 ; i < EXPECTED.size() ; i++)
        ASSERT_TRUE(IsEqual(EXPECTED.at(i), box->getChildAt(i)->getValue(kGraphicPropertyText))) << i;
}


static const char *DATA_BINDING_TO_ITEMS = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "graphics": {
        "box": {
          "type": "AVG",
          "version": "1.2",
          "height": 500,
          "width": 500,
          "items": [
            {
              "type": "text",
              "when": "${data % 3 != 0}",
              "text": "Alpha ${index} of ${length} [${data}]"
            },
            {
              "type": "text",
              "when": "${data != 3}",
              "text": "Bravo ${index} of ${length} [${data}]"
            }
          ],
          "data": "${Array.range(-1,7)}"
        }
      },
      "mainTemplate": {
        "items": [
          {
            "type": "VectorGraphic",
            "source": "box"
          }
        ]
      }
    }
)apl";

// Merge an array of children with an array of data items including "when" clauses
TEST_F(GraphicDataTest, DataBindingToItems)
{
    std::vector<std::string> EXPECTED = {
        "Alpha 0 of 8 [-1]",
        "Bravo 1 of 8 [0]",
        "Alpha 2 of 8 [1]",
        "Alpha 3 of 8 [2]",
        // "Bravo 4 of 8 [3]",
        "Alpha 4 of 8 [4]",
        "Alpha 5 of 8 [5]",
        "Bravo 6 of 8 [6]"
    };

    loadDocument(DATA_BINDING_TO_ITEMS);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    auto box = graphic->getRoot();
    ASSERT_TRUE(box);

    ASSERT_EQ(EXPECTED.size(), box->getChildCount());
    for (int i = 0 ; i < EXPECTED.size() ; i++)
        ASSERT_TRUE(IsEqual(EXPECTED.at(i), box->getChildAt(i)->getValue(kGraphicPropertyText))) << i;
}


static const char *GRID = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "graphics": {
        "Nested": {
          "type": "AVG",
          "version": "1.2",
          "height": 500,
          "width": 500,
          "parameters": [
            {
              "name": "ROWS",
              "type": "number"
            },
            {
              "name": "COLS",
              "type": "number"
            },
            "LABEL"
          ],
          "data": "${Array.range(ROWS)}",
          "items": {
            "type": "Group",
            "when": "${data % 2 == 0}",
            "items": {
              "type": "Text",
              "text": "${LABEL} ${index} of ${length}",
              "when": "${data < 4}"
            },
            "data": "${Array.range(COLS)}"
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "source": "Nested",
          "ROWS": 6,
          "COLS": 6,
          "LABEL": "Woof"
        }
      }
    }
)apl";

// Verify that groups also inflate data-bound children
TEST_F(GraphicDataTest, Grid)
{
    loadDocument(GRID);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    auto box = graphic->getRoot();

    ASSERT_EQ(3, box->getChildCount());
    for (size_t i = 0 ; i < box->getChildCount() ; i++) {
        auto row = box->getChildAt(i);
        ASSERT_EQ(4, row->getChildCount());
        for (size_t j = 0 ; j < row->getChildCount() ; j++) {
            auto cell = row->getChildAt(j);
            ASSERT_TRUE(IsEqual("Woof " + std::to_string(j) + " of 6", cell->getValue(kGraphicPropertyText)));
        }
    }
}


static const char *TEST_VERSION = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "graphics": {
        "TestVersion": {
          "type": "AVG",
          "version": "1.0",
          "height": 500,
          "width": 500,
          "data": "${Array.range(3)}",
          "items": {
            "type": "Text",
            "text": "Item ${index}"
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "source": "TestVersion"
        }
      }
    }
)apl";

// AVG versions less than 1.2 should not allow multi-child expansion
TEST_F(GraphicDataTest, TestVersion)
{
    loadDocument(TEST_VERSION);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    auto box = graphic->getRoot();

    // Data is ignored, so a single item should be inflated
    ASSERT_EQ(1, box->getChildCount());
    // The ${index} value is not bound.  Note that the StyledText code will strip the trailing space
    ASSERT_TRUE(IsEqual("Item", box->getChildAt(0)->getValue(kGraphicPropertyText)));
}
