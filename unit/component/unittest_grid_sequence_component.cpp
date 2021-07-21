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

#include "apl/component/gridsequencecomponent.h"

using namespace apl;

class GridSequenceComponentTest : public DocumentWrapper {
public:
    void executeScroll(const ComponentPtr& component, float distance) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "Scroll", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component->getId().c_str(), alloc).Move(), alloc);
        cmd.AddMember("distance", distance, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        root->executeCommands(doc, false);
    }

    void completeScroll(const ComponentPtr& component, float distance) {
        ASSERT_FALSE(root->hasEvent());
        executeScroll(component, distance);
        advanceTime(1000);
    }

    rapidjson::Document doc;
};

::testing::AssertionResult
validateBounds(const ComponentPtr& component, const Rect& rect) {
    auto componentBounds = component->getCalculated(kPropertyBounds).getRect();

    if (componentBounds != rect) {
        return ::testing::AssertionFailure()
                << "component " << component->getId() << " bounds is whong, expected: " << rect.toDebugString() << ","
                << " actual: " <<componentBounds.toDebugString();
    }
    return ::testing::AssertionSuccess();
}

// helper function to calculate starting location based on previous sizes of columns or rows
float startingLocation(const std::vector<float>& dims, int index) {
    if (dims.size() == 1) {
        return index * dims[0];
    } else {
        float loc = 0;
        for (int i = 0; i < index; i++)
            loc += dims[i % dims.size()];
        return loc;
    }
}

::testing::AssertionResult
validateCellBounds(const CoreComponentPtr& grid,
                        int numRows,
                        int numColumns,
                        const std::vector<float>& childHeights,
                        const std::vector<float>& childWidths,
                        int firstIndex,
                        int firstLabel) {
    int numComponents = numRows * numColumns;
    auto scrollOffset = grid->getCalculated(kPropertyScrollPosition).asNumber();
    auto scrollDirection = grid->getCalculated(kPropertyScrollDirection);
    bool isLTR = grid->getCalculated(kPropertyLayoutDirection) == kLayoutDirectionLTR;
    auto startPoint = isLTR
                      ? grid->getCalculated(kPropertyInnerBounds).getRect().getTopLeft()
                      : grid->getCalculated(kPropertyInnerBounds).getRect().getTopRight();

    std::vector<std::string> labels;

    for (int i = firstLabel; i < firstLabel + numRows * numColumns; i++) {
        labels.emplace_back(std::to_string(i));
    }

    for (int i = 0; i < numComponents; i++) {
        auto curColumn = scrollDirection == kScrollDirectionHorizontal ? (i / numRows) : (i % numColumns);
        auto curRow = scrollDirection == kScrollDirectionHorizontal ? (i % numRows) : i / numColumns;
        auto childWidth = childWidths[curColumn % childWidths.size()];
        auto childHeight = childHeights[curRow % childHeights.size()];

        // if we do not have more children, breaks
        if (firstIndex + i >= grid->getChildCount()) break;

        auto child = grid->getChildAt(firstIndex + i);

        if (labels[i] != child->getId()) {
            return ::testing::AssertionFailure()
                    << "component " << i << " id is whong, expected: " << labels[i] << ","
                    << " actual: " << child->getId();
        }

        auto x = startingLocation(childWidths, curColumn);
        auto y = startingLocation(childHeights, curRow);
        if (scrollDirection == kScrollDirectionHorizontal) {
            if (isLTR) x += scrollOffset;
            else x -= scrollOffset;
        } else {
            y += scrollOffset;
        }

        if (childWidth < 1 || childHeight < 1) {
            // components with no height or width
            // do not have valid bounds
            if (scrollDirection == kScrollDirectionHorizontal) {
                if (0 != childHeight) {
                    return ::testing::AssertionFailure()
                            << "component " << i << " height, expected: " << 0 << ","
                            << " actual: " << childHeight;
                }
            } else {
                if (0 != childWidth) {
                    return ::testing::AssertionFailure()
                            << "component " << i << " width, expected: " << 0 << ","
                            << " actual: " << childWidth;
                }
            }
        } else {
            auto result = isLTR
                         ? validateBounds(child, Rect(x, y, childWidth, childHeight))
                         : validateBounds(child, Rect(startPoint.getX() - childWidth - x, y, childWidth, childHeight));
            if (result != ::testing::AssertionSuccess()) {
                return result;
            }
        }
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult
validateCellBounds(const CoreComponentPtr& grid,
                   int numRows,
                   int numColumns,
                   const std::vector<float>& childHeights,
                   const std::vector<float>& childWidths) {
    return validateCellBounds(grid, numRows, numColumns, childHeights, childWidths, 0, 1);
}

static const char* SIMPLE_GRID_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "width": "300dp",
      "snap": "center",
      "numbered": true,
      "childWidth": [ "200dp", "100dp" ],
      "childHeight": "50%"
    }
  }
})";

/**
 * Test that the defaults are as expected.
 */
TEST_F(GridSequenceComponentTest, ComponentSimple) {
    loadDocument(SIMPLE_GRID_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());

    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ(300, component->getCalculated(kPropertyWidth).asInt());
    ASSERT_EQ(100, component->getCalculated(kPropertyHeight).asInt());
    ASSERT_EQ(kSnapCenter, component->getCalculated(kPropertySnap).asInt());
    ASSERT_TRUE(component->getCalculated(kPropertyNumbered).getBoolean());
    ASSERT_EQ("50%", component->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ(kFlexboxWrapWrap, component->getCalculated(kPropertyWrap).asInt());
    ASSERT_EQ(200, component->getCalculated(kPropertyChildWidth).at(0).asInt());
    ASSERT_EQ(100, component->getCalculated(kPropertyChildWidth).at(1).asInt());
}


static std::vector<const char *> BAD_GRID_DOCS = {
    R"({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "item": {
          "type": "GridSequence",
          "width": "300dp",
          "snap": "center",
          "numbered": true,
          "childHeight": "50%"
        }
      }
    })",
    R"({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "item": {
          "type": "GridSequence",
          "width": "300dp",
          "snap": "center",
          "numbered": true,
          "childWidth": "50%"
        }
      }
    })"
};

/**
 * Certain grid sequence properties are required.
 * If they are not present, the grid sequence will not inflate.
 */
TEST_F(GridSequenceComponentTest, BadGridDoc)
{
    for (const auto& m : BAD_GRID_DOCS) {
        loadDocumentExpectFailure(m);
        ASSERT_FALSE(component);
        ASSERT_TRUE(ConsoleMessage());
    }
}


static const char* PLURAL_PROPS_GRID_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "width": "300dp",
      "childWidths": [ "200dp", "100dp" ],
      "childHeights": "50%"
    }
  }
})";

/**
 * Test that childWidths/childHeights work as expected.
 */
TEST_F(GridSequenceComponentTest, ComponentPluralProps) {
    loadDocument(PLURAL_PROPS_GRID_DOC);
    ASSERT_TRUE(component);

    auto et = root->topComponent();
    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(200, component->getCalculated(kPropertyChildWidth).at(0).asInt());
    ASSERT_EQ(100, component->getCalculated(kPropertyChildWidth).at(1).asInt());
    ASSERT_EQ("50%", component->getCalculated(kPropertyChildHeight).at(0).asString());
}

static const char* HORIZONTAL_UNIFORM_GRID_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "width": "300dp",
      "height": "110dp",
      "scrollDirection": "horizontal",
      "childWidth": "90dp",
      "childHeight": "50dp",
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6
      ]
    }
  }
})";

/**
 * Test that the defaults are as expected.
 */
TEST_F(GridSequenceComponentTest, HorizontalUniformGrid) {
    loadDocument(HORIZONTAL_UNIFORM_GRID_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionHorizontal, component->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ(90, component->getCalculated(kPropertyChildWidth).at(0).asInt());
    ASSERT_EQ(50, component->getCalculated(kPropertyChildHeight).at(0).asInt());
    ASSERT_EQ(2, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            2,  // num rows
            3,  // num columns
            {50, 50}, // child heights
            {90})); // child widths
}

static const char* HORIZONTAL_MULTI_HEIGHT_GRID_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "width": "190dp",
      "height": "110dp",
      "scrollDirection": "horizontal",
      "childWidth": "90dp",
      "childHeight": [ "50dp", "20dp", "10dp", "30dp" ],
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8
      ]
    }
  }
})";

TEST_F(GridSequenceComponentTest, HorizontalMultiHeightGrid) {
    loadDocument(HORIZONTAL_MULTI_HEIGHT_GRID_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());

    ASSERT_EQ(kScrollDirectionHorizontal, component->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ(90, component->getCalculated(kPropertyChildWidth).at(0).asInt());
    ASSERT_EQ(50, component->getCalculated(kPropertyChildHeight).at(0).asInt());
    ASSERT_EQ(4, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            4,  // num rows
            2,  // num columns
            {50, 20, 10, 30}, // child heights
            {90})); // child widths
}

static const char* VERTICAL_UNIFORM_GRID_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "width": "190dp",
      "height": "160dp",
      "scrollDirection": "vertical",
      "childWidth": "90dp",
      "childHeight": "25%",
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6
      ]
    }
  }
})";

/**
 * Test that the defaults are as expected.
 */
TEST_F(GridSequenceComponentTest, VerticalUniformGrid) {
    loadDocument(VERTICAL_UNIFORM_GRID_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());

    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ("90dp", component->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ("25%", component->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ(2, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            3,  // num rows
            2,  // num columns
            {40}, // child heights
            {90, 90})); // child widths
}

static const char* VERTICAL_AUTO_SIZE_GRID_WIDTH = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "width": "auto",
      "height": "100dp",
      "scrollDirection": "vertical",
      "childHeight": "50dp",
      "childWidth": ["90dp", "20%", "auto"],
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6
      ]
    }
  }
})";

/**
 * Test that the defaults are as expected.
 */
TEST_F(GridSequenceComponentTest, VerticalAutoSizeGridWidth) {
    loadDocument(VERTICAL_AUTO_SIZE_GRID_WIDTH);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ("90dp", component->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ("20%", component->getCalculated(kPropertyChildWidth).at(1).asString());
    ASSERT_TRUE(component->getCalculated(kPropertyChildWidth).at(2).asDimension(*component->getContext()).isAuto());
    ASSERT_EQ("50dp", component->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ(3, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            2,  // num rows
            3,  // num columns
            {50}, // child heights
            {90, 0, 0})); // child widths
}

static const char* HORIZONTAL_AUTO_SIZE_GRID_HEIGHT = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "width": "90dp",
      "height": "auto",
      "scrollDirection": "horizontal",
      "childWidth": "90dp",
      "childHeight": ["auto", "20%", "50dp"],
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6
      ]
    }
  }
})";

/**
 * Test that the defaults are as expected.
 */
TEST_F(GridSequenceComponentTest, HorizontalAutoSizeGridHeight) {
    loadDocument(HORIZONTAL_AUTO_SIZE_GRID_HEIGHT);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionHorizontal, component->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_TRUE(component->getCalculated(kPropertyChildHeight).at(0).asDimension(*component->getContext()).isAuto());
    ASSERT_EQ("20%", component->getCalculated(kPropertyChildHeight).at(1).asString());
    ASSERT_EQ("50dp", component->getCalculated(kPropertyChildHeight).at(2).asString());
    ASSERT_EQ("90dp", component->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ(3, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            3,  // num rows
            2,  // num columns
            {0, 0, 50}, // child heights
            {90})); // child widths
}

static const char* VERTICAL_MULTI_WIDTH_GRID_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "width": "190dp",
      "height": "160dp",
      "scrollDirection": "vertical",
      "childWidth": ["100dp", "60dp"],
      "childHeight": "50dp",
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6
      ]
    }
  }
})";

TEST_F(GridSequenceComponentTest, VerticalMultiWidthGrid) {
    loadDocument(VERTICAL_MULTI_WIDTH_GRID_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ("100dp", component->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ("60dp", component->getCalculated(kPropertyChildWidth).at(1).asString());
    ASSERT_EQ("50dp", component->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ(2, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            3,  // num rows
            2,  // num columns
            {50}, // child heights
            {100,60})); // child widths
}

static const char* MULTI_WIDTH_RELATIVE_GRID_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "width": "300dp",
      "height": "110dp",
      "scrollDirection": "vertical",
      "childWidth": ["100dp", "25%", "24%"],
      "childHeight": "55dp",
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4
      ]
    }
  }
})";

TEST_F(GridSequenceComponentTest, MultiWidthRelativeChildrenGrid) {
    loadDocument(MULTI_WIDTH_RELATIVE_GRID_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).asInt());
    // first width is an absolute
    ASSERT_EQ(100, component->getCalculated(kPropertyChildWidth).at(0).asInt());
    // next two are percentages
    ASSERT_EQ(25, component->getCalculated(kPropertyChildWidth).at(1).asInt());
    ASSERT_EQ(24, component->getCalculated(kPropertyChildWidth).at(2).asInt());
    ASSERT_EQ(55, component->getCalculated(kPropertyChildHeight).at(0).asInt());
    ASSERT_EQ(3, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            2,  // num rows
            3,  // num columns
            {55}, // child heights
            {100,75,72})); // child widths
}

static const char* SCROLLING_EVENT_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "parameters": [],
    "item": {
      "type": "GridSequence",
      "scrollDirection": "vertical",
      "onScroll": [
            {
              "type": "SetValue",
              "componentId": "textId",
              "property": "text",
              "value": "${event.source.itemsPerCourse}"
            }
       ],
      "width": 60,
      "height": 40,
      "childWidth": ["15dp", "15dp"],
      "childHeight": "20dp",
      "items": {
        "type": "Text",
        "id": "textId"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12
      ]
    }
  }
})";

TEST_F(GridSequenceComponentTest, ScrollEvent) {
    loadDocument(SCROLLING_EVENT_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).asInt());

    // scroll
    component->update(kUpdateScrollPosition, 40);
    loop->runPending();

    // our onScroll puts the itemsPerCourse in the Text property of the textId component
    ASSERT_EQ("2", component->findComponentById("textId")->getCalculated(kPropertyText).asString());
}

static const char* AUTO_SIZE_ALL_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "scrollDirection": "vertical",
      "width": "400dp",
      "height": "40dp",
      "childWidth": [ "auto", "auto", "auto" ],
      "childHeight": "20dp",
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6
      ]
    }
  }
})";

/**
 * Test of cross axis dimension sizing where all child dimensions are 'auto'
 */
TEST_F(GridSequenceComponentTest, AutoSizeAllChildren) {
    loadDocument(AUTO_SIZE_ALL_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ(400, component->getCalculated(kPropertyWidth).asInt());
    ASSERT_EQ(3, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            2,  // num rows
            3,  // num columns
            {20}, // child heights
            {133,134,133})); // child widths
}

static const char* AUTO_SIZE_SOME_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "scrollDirection": "horizontal",
      "width": "40dp",
      "height": "400dp",
      "childWidth": "20dp",
      "childHeight": [ "auto", "100dp", "auto", "60dp"],
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8
      ]
    }
  }
})";

/**
 * Test of cross axis dimension sizing where some child dimensions are 'auto'
 */
TEST_F(GridSequenceComponentTest, AutoSizeSomeChildren) {
    loadDocument(AUTO_SIZE_SOME_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionHorizontal, component->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ(400, component->getCalculated(kPropertyHeight).asInt());
    ASSERT_EQ(4, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            4,  // num rows
            2,  // num columns
            {120, 100, 120, 60}, // child heights
            {20})); // child widths
}

static const char* AUTO_SIZE_SOME_ZEROS_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "scrollDirection": "horizontal",
      "width": "40dp",
      "height": "160dp",
      "childWidth": "20dp",
      "childHeight": [ "auto", "100dp", "auto", "60dp" ],
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8
      ]
    }
  }
})";

/**
 * Test of cross axis dimension sizing where some child dimensions are 'auto' and resolve to 0
 */
TEST_F(GridSequenceComponentTest, AutoSizeChildrenZeros) {
    loadDocument(AUTO_SIZE_SOME_ZEROS_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(4, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            4,  // num rows
            2,  // num columns
            {0, 100, 0, 60}, // child heights
            {20})); // child widths
}

static const char *LIVE_GRID_SEQUENCE = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "id": "grid",
      "data": "${TestArray}",
      "scrollDirection": "horizontal",
      "height": 200,
      "width": 200,
      "childHeight": [75, 125],
      "childWidth": 100,
      "item": {
        "type": "Text",
        "id": "${data}",
        "text": "${data}"
      }
    }
  }
})";

TEST_F(GridSequenceComponentTest, GridSequenceScrollingContext)
{
    auto myArray = LiveArray::create({8, 9, 10, 11, 12, 13, 14});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_GRID_SEQUENCE);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(7, component->getChildCount());

    auto scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    // Just 1 page in view + 1 forward. Should be all laid-out.
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 6}, true));

    ASSERT_TRUE(validateCellBounds(
            component,
            2,  // num rows
            3,  // num columns
            {75, 125}, // child heights
            {100}, // child widths
            0,
            8));

    // Verify initial context
    rapidjson::Document document(rapidjson::kObjectType);
    auto context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    auto& tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    auto& list = tags["list"];
    ASSERT_EQ(7, list["itemCount"].GetInt());
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(3, list["highestIndexSeen"].GetInt());

    // Prepend whole columns (1 page backwards, so should be pre-loaded)
    myArray->insert(0, 7);
    myArray->insert(0, 6);
    myArray->insert(0, 5);
    myArray->insert(0, 4);
    root->clearPending();

    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlags(component, {0, 3}));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 10}, true));

    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(200, scrollPosition);

    // Check that original bunch just moved
    ASSERT_TRUE(validateCellBounds(
            component,
            2,  // num rows
            3,  // num columns
            {75, 125}, // child heights
            {100}, // child widths
            4,
            8));

    ASSERT_TRUE(component->isVisualContextDirty());
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(11, list["itemCount"].GetInt());
    ASSERT_EQ(4, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(7, list["highestIndexSeen"].GetInt());

    // scroll back and verify that it's still fine.
    completeScroll(component, -1);
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    // Check that we see recently added stuff now
    ASSERT_TRUE(validateCellBounds(
            component,
            2,  // num rows
            3 + 2,  // num columns (with prepended ones)
            {75, 125}, // child heights
            {100}, // child widths
            0,
            4));

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(11, list["itemCount"].GetInt());
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(7, list["highestIndexSeen"].GetInt());


    completeScroll(component, 1);
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(200, scrollPosition);

    myArray->insert(0, 3);
    myArray->insert(0, 2);
    myArray->insert(0, 1);
    myArray->insert(0, 0);

    myArray->push_back(15);
    myArray->push_back(16);
    myArray->push_back(17);
    myArray->push_back(18);
    myArray->push_back(19);
    root->clearPending();

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(20, list["itemCount"].GetInt());
    ASSERT_EQ(4, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(11, list["highestIndexSeen"].GetInt());

    // 1 back should not be laid out. Same for 1 forward.
    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlags(component, {2, 3}));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 16}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {17, 19}, false));

    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(300, scrollPosition);

    completeScroll(component, -2);
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    // Check that we see recently added stuff now
    ASSERT_TRUE(validateCellBounds(
            component,
            2,  // num rows
            3 + 2,  // num columns (with prepended ones)
            {75, 125}, // child heights
            {100}, // child widths
            0,
            0));

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(20, list["itemCount"].GetInt());
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(11, list["highestIndexSeen"].GetInt());

    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlags(component, {0, 1}));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 16}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {17, 19}, false));

    completeScroll(component, 3);
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(600, scrollPosition);

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(20, list["itemCount"].GetInt());
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(15, list["highestIndexSeen"].GetInt());

    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlags(component, {17, 19}));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 19}, true));
}

TEST_F(GridSequenceComponentTest, GridSequenceScrollingContextRTL)
{
    auto myArray = LiveArray::create({8, 9, 10, 11, 12, 13, 14});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_GRID_SEQUENCE);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(7, component->getChildCount());

    auto scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    // Just 1 page in view + 1 forward. Should be all laid-out.
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 6}, true));

    ASSERT_TRUE(validateCellBounds(
        component,
        2,  // num rows
        3,  // num columns
        {75, 125}, // child heights
        {100}, // child widths
        0,
        8));

    // Verify initial context
    rapidjson::Document document(rapidjson::kObjectType);
    auto context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    auto& tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    auto& list = tags["list"];
    ASSERT_EQ(7, list["itemCount"].GetInt());
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(3, list["highestIndexSeen"].GetInt());

    // Prepend whole columns (1 page backwards, so should be pre-loaded)
    myArray->insert(0, 7);
    myArray->insert(0, 6);
    myArray->insert(0, 5);
    myArray->insert(0, 4);
    root->clearPending();

    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 10}, true));

    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(-200, scrollPosition);

    // Check that original bunch just moved
    ASSERT_TRUE(validateCellBounds(
        component,
        2,  // num rows
        3,  // num columns
        {75, 125}, // child heights
        {100}, // child widths
        4,
        8));

    ASSERT_TRUE(component->isVisualContextDirty());
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(11, list["itemCount"].GetInt());
    ASSERT_EQ(4, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(7, list["highestIndexSeen"].GetInt());

    // scroll back and verify that it's still fine.
    completeScroll(component, -1);
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    // Check that we see recently added stuff now
    ASSERT_TRUE(validateCellBounds(
        component,
        2,  // num rows
        3 + 2,  // num columns (with prepended ones)
        {75, 125}, // child heights
        {100}, // child widths
        0,
        4));

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(11, list["itemCount"].GetInt());
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(7, list["highestIndexSeen"].GetInt());


    completeScroll(component, 1);
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(-200, scrollPosition);

    myArray->insert(0, 3);
    myArray->insert(0, 2);
    myArray->insert(0, 1);
    myArray->insert(0, 0);

    myArray->push_back(15);
    myArray->push_back(16);
    myArray->push_back(17);
    myArray->push_back(18);
    myArray->push_back(19);
    root->clearPending();

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(20, list["itemCount"].GetInt());
    ASSERT_EQ(4, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(11, list["highestIndexSeen"].GetInt());

    // 1 back should not be laid out. Same for 1 forward.
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 16}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {17, 19}, false));

    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(-300, scrollPosition);

    completeScroll(component, -2);
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    // Check that we see recently added stuff now
    ASSERT_TRUE(validateCellBounds(
        component,
        2,  // num rows
        3 + 2,  // num columns (with prepended ones)
        {75, 125}, // child heights
        {100}, // child widths
        0,
        0));

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(20, list["itemCount"].GetInt());
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(11, list["highestIndexSeen"].GetInt());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 16}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {17, 19}, false));

    completeScroll(component, 3);
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(-600, scrollPosition);

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_TRUE(CheckDirtyVisualContext(root));

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("grid", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(20, list["itemCount"].GetInt());
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(15, list["highestIndexSeen"].GetInt());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 19}, true));
}

static const char *LIVE_GRID_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "id": "grid",
      "data": "${TestArray}",
      "scrollDirection": "horizontal",
      "height": 500,
      "width": 300,
      "childHeight": [100, 150, 250],
      "childWidth": 100,
      "item": {
        "type": "Text",
        "id": "${data}",
        "text": "${data}"
      }
    }
  }
})";

TEST_F(GridSequenceComponentTest, GridSequenceLiveChanges)
{
    auto myArray = LiveArray::create({9, 10, 11, 12, 13, 14, 15, 16, 17});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_GRID_DOC);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(9, component->getChildCount());

    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Start point
    // +----------------------+
    // | +----+ +----+ +----+ |
    // | |  9 | | 12 | | 15 | |
    // | +----+ +----+ +----+ |
    // | +----+ +----+ +----+ |
    // | | 10 | | 13 | | 16 | |
    // | +----+ +----+ +----+ |
    // | +----+ +----+ +----+ |
    // | | 11 | | 14 | | 17 | |
    // | +----+ +----+ +----+ |
    // +----------------------+

    ASSERT_TRUE(validateCellBounds(
            component,
            3,  // num rows
            3,  // num columns
            {100,150,250}, // child heights
            {100}, // child widths
            0,
            9));

    // Prepend an item
    myArray->insert(0, "8");
    root->clearPending();
    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).asNumber());

    ASSERT_TRUE(validateCellBounds(
            component,
            3,  // num rows
            3,  // num columns
            {100,150,250}, // child heights
            {100}, // child widths
            0,
            8));

    // Add more
    // +-----------------------------+
    // | +----+ +----+ +----+ +----+ |
    // | |  8 | | 11 | | 14 | | 17 | |
    // | +----+ +----+ +----+ +----+ |
    // | +----+ +----+ +----+        |
    // | |  9 | | 12 | | 15 |        |
    // | +----+ +----+ +----+        |
    // | +----+ +----+ +----+        |
    // | | 10 | | 13 | | 16 |        |
    // | +----+ +----+ +----+        |
    // +-----------------------------+

    // Now insert few more
    myArray->insert(0, "6");
    root->clearPending();
    myArray->insert(1, "7");
    root->clearPending();
    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).asNumber());

    ASSERT_TRUE(validateCellBounds(
            component,
            3,  // num rows
            3,  // num columns
            {100,150,250}, // child heights
            {100}, // child widths
            0,
            6));

    // And up to full column
    // +-----------------------------+
    // | +----+ +----+ +----+ +----+ |
    // | |  6 | |  9 | | 12 | | 15 | |
    // | +----+ +----+ +----+ +----+ |
    // | +----+ +----+ +----+ +----+ |
    // | |  7 | | 10 | | 13 | | 16 | |
    // | +----+ +----+ +----+ +----+ |
    // | +----+ +----+ +----+ +----+ |
    // | |  8 | | 11 | | 14 | | 17 | |
    // | +----+ +----+ +----+ +----+ |
    // +-----------------------------+

    // Remove few now
    myArray->remove(0);
    myArray->remove(11);
    root->clearPending();
    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Still not moved
    ASSERT_TRUE(validateCellBounds(
            component,
            3,  // num rows
            3,  // num columns
            {100,150,250}, // child heights
            {100}, // child widths
            0,
            7));

    // Here we go again
    // +-----------------------------+
    // | +----+ +----+ +----+ +----+ |
    // | |  7 | | 10 | | 13 | | 16 | |
    // | +----+ +----+ +----+ +----+ |
    // | +----+ +----+ +----+        |
    // | |  8 | | 11 | | 14 |        |
    // | +----+ +----+ +----+        |
    // | +----+ +----+ +----+        |
    // | |  9 | | 12 | | 15 |        |
    // | +----+ +----+ +----+        |
    // +-----------------------------+
}

/**
 * This test recreates a std::out_of_range exception which occurred in
 * MultiChildScrollableComponent::processLayoutChange when mEnsuredChildren was empty but the
 * mChildren was NOT empty. We called: mChildren.at(mEnsuredChildren.lowerBound()); without
 * checking if it was a valid index
 */
TEST_F(GridSequenceComponentTest, CheckEmptyEnsuredChildren)
{
    auto myArray = LiveArray::create({1, 2, 3, 4});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_GRID_DOC);

    // Insert a bunch of elements at the start to push the lowerBound of mEnsuredChildren up
    for (int i = 0; i < 25; i++)
        myArray->insert(0, "x");

    root->clearPending();

    // The lower bounds is now 13 so we will remove all elements from 13 onwards to force mEnsuredChildren
    // to be empty
    for (int i = 0; i < 16; i++)
        myArray->remove(13);

    // Now that mEnsuredChildren is empty we call MultiChildScrollableComponent::processLayoutChanges
    root->clearPending();

    auto grid = std::dynamic_pointer_cast<GridSequenceComponent>(root->findComponentById("grid"));
    ASSERT_EQ(grid->getDisplayedChildCount(), 9);
}

static const char* CHILD_PADDING = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "scrollDirection": "horizontal",
      "width": 200,
      "height": 300,
      "childWidth": 200,
      "childHeight": "50%",
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4
      ]
    }
  }
})";

TEST_F(GridSequenceComponentTest, ChildPadding) {
    loadDocument(CHILD_PADDING);
    advanceTime(10);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(2, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            2,  // num rows
            2,  // num columns
            {150, 150}, // child heights
            {200})); // child widths
}

static const char* CHILD_PADDING_FIT = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "scrollDirection": "horizontal",
      "width": 200,
      "height": 100,
      "childWidth": 200,
      "childHeight": "23%",
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8
      ]
    }
  }
})";

TEST_F(GridSequenceComponentTest, ChildPaddingFit) {
    loadDocument(CHILD_PADDING_FIT);
    advanceTime(10);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(4, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            4,  // num rows
            2,  // num columns
            {23, 23, 23, 23}, // child heights
            {200})); // child widths
}

static const char* CHILD_CLIPPING = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "scrollDirection": "horizontal",
      "width": 200,
      "height": 1000,
      "childWidth": 200,
      "childHeights": ["auto", "30%", 500, "30%", "auto"],
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10
      ]
    }
  }
})";

TEST_F(GridSequenceComponentTest, ChildClipping) {
    loadDocument(CHILD_CLIPPING);
    advanceTime(10);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(5, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            5,  // num rows
            2,  // num columns
            {0, 300, 500, 200, 0}, // child heights
            {200})); // child widths
}

static const char* UNIFORM_RELATIVE = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "scrollDirection": "horizontal",
      "width": 200,
      "height": 1000,
      "childWidth": 200,
      "childHeights": "23%",
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8
      ]
    }
  }
})";

TEST_F(GridSequenceComponentTest, UniformRelative) {
    loadDocument(UNIFORM_RELATIVE);
    advanceTime(10);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(4, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            4,  // num rows
            2,  // num columns
            {230, 230, 230, 230}, // child heights
            {200})); // child widths
}

static const char* SINGLE_AUTO = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "GridSequence",
      "scrollDirection": "horizontal",
      "width": 200,
      "height": 100,
      "childWidth": 200,
      "childHeights": "auto",
      "items": {
        "type": "Text",
        "id": "${data}"
      },
      "data": [
        1,
        2
      ]
    }
  }
})";

TEST_F(GridSequenceComponentTest, SingleAuto) {
    loadDocument(SINGLE_AUTO);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(1, component->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            component,
            1,  // num rows
            2,  // num columns
            {100}, // child heights
            {200})); // child widths
}

static const char* AUTO_SEQUENCE = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "layouts": {
    "square": {
      "parameters": [
        "color",
        "index"
      ],
      "item": {
        "type": "Frame",
        "width": "100%",
        "height": "100%",
        "id": "${index + 1}",
        "backgroundColor": "${color}",
        "borderWidth": 2,
        "borderColor": "white",
        "item": {
          "type": "Text",
          "text": "Item ${index + 1}",
          "id": "text1",
          "color": "black",
          "width": "100%",
          "height": "100%",
          "textAlign": "center",
          "textAlignVertical": "center"
        }
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "direction": "row",
      "width": "100%",
      "height": "100%",
      "items": [
        {
          "type": "GridSequence",
          "width": "auto",
          "scrollDirection": "vertical",
          "childWidths": [ "300dp", "40%", "100dp", "auto"],
          "childHeight": "100dp",
          "data": [
            "yellow", "red", "blue", "green",
            "yellow", "red", "blue", "green",
            "yellow", "red", "blue", "green"
          ],
          "items": [
            {
              "type": "square",
              "color": "${data}",
              "index": "${index}"
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(GridSequenceComponentTest, AutoSequence) {
    loadDocument(AUTO_SEQUENCE);
    advanceTime(10);
    ASSERT_TRUE(component);

    auto grid = component->getCoreChildAt(0);

    ASSERT_EQ(kComponentTypeGridSequence, grid->getType());
    ASSERT_EQ(4, grid->getCalculated(kPropertyItemsPerCourse).asInt());

    auto bounds = grid->getCalculated(kPropertyBounds).getRect();
    ASSERT_EQ(Rect(0, 0, 400, 100), bounds);
    ASSERT_TRUE(validateCellBounds(
            grid,
            2,  // num rows - we just check what is loaded.
            4,  // num columns
            {100}, // child heights
            {300, 0, 100, 0})); // child widths
}

static const char* VERTICAL_GRID_SETVALUE = R"(
{
    "type": "APL",
    "version": "1.6",
    "mainTemplate": {
        "item": {
            "type": "GridSequence",
            "scrollDirection": "vertical",
            "height": "160dp",
            "width": "190dp",
            "childWidth": ["100dp", "auto"],
            "childHeight": "25%",
            "items": {
                "type": "Text",
                "id": "${data}"
            },
            "data": [1, 2, 3, 4, 5, 6]
        }
    }
}
)";

// Test for vertical grid seq child height/width properties for dynamic
TEST_F(GridSequenceComponentTest, ChildHeightWidthVertical) {
    loadDocument(VERTICAL_GRID_SETVALUE);
    ASSERT_TRUE(component);

    auto gridSeq = std::dynamic_pointer_cast<CoreComponent>(component);
    ASSERT_EQ(kComponentTypeGridSequence, gridSeq->getType());

    ASSERT_EQ(kScrollDirectionVertical, gridSeq->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ("100dp", gridSeq->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildWidth).at(1).asString());
    ASSERT_EQ("25%", gridSeq->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            3,  // num rows
            2,  // num columns
            {40}, // child heights
            {100, 90})); // child widths

    // Set childWidth property of grid sequence, it will impact all children of grid sequence
    gridSeq->setProperty(kPropertyChildWidth, Object(ObjectArray{"90dp", "100dp"}));

    root->clearPending();
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root,
            gridSeq,
            gridSeq->getChildAt(0),
            gridSeq->getChildAt(1),
            gridSeq->getChildAt(2),
            gridSeq->getChildAt(3),
            gridSeq->getChildAt(4),
            gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("90dp", gridSeq->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ("100dp", gridSeq->getCalculated(kPropertyChildWidth).at(1).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            3,  // num rows
            2,  // num columns
            {40}, // child heights
            {90, 100})); // child widths

    // Set childWidth property of grid sequence, it will impact 3 of its children
    gridSeq->setProperty(kPropertyChildWidth, "90dp");

    root->clearPending();
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root,
            gridSeq,
            gridSeq->getChildAt(1),
            gridSeq->getChildAt(3),
            gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("90dp", gridSeq->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            3,  // num rows
            2,  // num columns
            {40}, // child heights
            {90, 90})); // child widths

    // Set childWidth property of grid sequence, it will impact 5 children of grid sequence
    gridSeq->setProperty(kPropertyChildWidth, Object(ObjectArray{"90dp", "80dp", "auto"}));

    root->clearPending();
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root,
            gridSeq,
            gridSeq->getChildAt(1),
            gridSeq->getChildAt(2),
            gridSeq->getChildAt(3),
            gridSeq->getChildAt(4),
            gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(3, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("90dp", gridSeq->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ("80dp", gridSeq->getCalculated(kPropertyChildWidth).at(1).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildWidth).at(2).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            2,  // num rows
            3,  // num columns
            {40}, // child heights
            {90, 80, 20})); // child widths

    // Set childHeight property of grid sequence, it will impact all children of grid sequence
    gridSeq->setProperty(kPropertyChildHeight, "20%");

    root->clearPending();
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root,
            gridSeq,
            gridSeq->getChildAt(0),
            gridSeq->getChildAt(1),
            gridSeq->getChildAt(2),
            gridSeq->getChildAt(3),
            gridSeq->getChildAt(4),
            gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(3, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("90dp", gridSeq->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ("80dp", gridSeq->getCalculated(kPropertyChildWidth).at(1).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildWidth).at(2).asString());
    ASSERT_EQ("20%", gridSeq->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            2,  // num rows
            3,  // num columns
            {32}, // child heights
            {90, 80, 20})); // child widths
}

// Test for vertical grid seq height/width properties for dynamic
TEST_F(GridSequenceComponentTest, HeightWidthVertical) {
    loadDocument(VERTICAL_GRID_SETVALUE);
    ASSERT_TRUE(component);

    auto gridSeq = std::dynamic_pointer_cast<CoreComponent>(component);
    ASSERT_EQ(kComponentTypeGridSequence, gridSeq->getType());

    ASSERT_EQ(kScrollDirectionVertical, gridSeq->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ("100dp", gridSeq->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildWidth).at(1).asString());
    ASSERT_EQ("25%", gridSeq->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            3,  // num rows
            2,  // num columns
            {40}, // child heights
            {100, 90})); // child widths

    // Set height property of grid sequence, it will impact all components
    gridSeq->setProperty(kPropertyHeight, "200dp");

    root->clearPending();
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root, gridSeq,
            gridSeq->getChildAt(0),
            gridSeq->getChildAt(1),
            gridSeq->getChildAt(2),
            gridSeq->getChildAt(3),
            gridSeq->getChildAt(4),
            gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("100dp", gridSeq->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildWidth).at(1).asString());
    ASSERT_EQ("200dp", gridSeq->getCalculated(kPropertyHeight).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            3,  // num rows
            2,  // num columns
            {50}, // child heights
            {100, 90})); // child widths

    // Set width property of grid sequence, it will impact gridSeq and 3 children with width auto
    gridSeq->setProperty(kPropertyWidth, "200dp");

    root->clearPending();
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root, gridSeq,
            gridSeq->getChildAt(1),
            gridSeq->getChildAt(3),
            gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("100dp", gridSeq->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildWidth).at(1).asString());
    ASSERT_EQ("200dp", gridSeq->getCalculated(kPropertyWidth).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            3,  // num rows
            2,  // num columns
            {50}, // child heights
            {100, 100})); // child widths
}

static const char* HORIZONTAL_GRID_SETVALUE = R"(
{
    "type": "APL",
    "version": "1.6",
    "mainTemplate": {
        "item": {
            "type": "GridSequence",
            "scrollDirection": "horizontal",
            "height": "160dp",
            "width": "200dp",
            "childWidth": "25%",
            "childHeight": ["80dp", "auto"],
            "items": {
                "type": "Text",
                "id": "${data}"
            },
            "data": [1, 2, 3, 4, 5, 6]
        }
    }
}
)";

// Test for horizontal grid seq child height/width properties for dynamic
TEST_F(GridSequenceComponentTest, ChildHeightWidthHorizontal) {
    loadDocument(HORIZONTAL_GRID_SETVALUE);
    ASSERT_TRUE(component);

    auto gridSeq = std::dynamic_pointer_cast<CoreComponent>(component);
    ASSERT_EQ(kComponentTypeGridSequence, gridSeq->getType());

    ASSERT_EQ(kScrollDirectionHorizontal, gridSeq->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ("80dp", gridSeq->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildHeight).at(1).asString());
    ASSERT_EQ("25%", gridSeq->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            2,  // num rows
            3,  // num columns
            {80, 80}, // child heights
            {50})); // child widths

    // Set childHeight property of grid sequence, it will impact all children of grid sequence
    gridSeq->setProperty(kPropertyChildHeight, Object(ObjectArray{"60dp", "80dp"}));

    root->clearPending();
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds));
    ASSERT_TRUE(CheckDirty(root,
            gridSeq,
            gridSeq->getChildAt(0),
            gridSeq->getChildAt(1),
            gridSeq->getChildAt(2),
            gridSeq->getChildAt(3),
            gridSeq->getChildAt(4),
            gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("60dp", gridSeq->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ("80dp", gridSeq->getCalculated(kPropertyChildHeight).at(1).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            2,  // num rows
            3,  // num columns
            {60, 80}, // child heights
            {50})); // child widths

    // Set childHeight property of grid sequence, it will impact 3 of its children
    gridSeq->setProperty(kPropertyChildHeight, "60dp");

    root->clearPending();
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root,
            gridSeq,
            gridSeq->getChildAt(1),
            gridSeq->getChildAt(3),
            gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("60dp", gridSeq->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            2,  // num rows
            3,  // num columns
            {60, 60}, // child heights
            {50})); // child widths

    // Set childHeight property of grid sequence, it will impact 6 children of grid sequence
    gridSeq->setProperty(kPropertyChildHeight, Object(ObjectArray{"80dp", "60dp", "auto"}));

    root->clearPending();
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4), kPropertyBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root,
            gridSeq,
            gridSeq->getChildAt(0),
            gridSeq->getChildAt(1),
            gridSeq->getChildAt(2),
            gridSeq->getChildAt(3),
            gridSeq->getChildAt(4),
            gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(3, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("80dp", gridSeq->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ("60dp", gridSeq->getCalculated(kPropertyChildHeight).at(1).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildHeight).at(2).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            3,  // num rows
            2,  // num columns
            {80, 60, 20}, // child heights
            {50})); // child widths

    // Set childWidth property of grid sequence, it will impact all children of grid sequence
    gridSeq->setProperty(kPropertyChildWidth, "50%");

    root->clearPending();
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root,
                           gridSeq,
                           gridSeq->getChildAt(0),
                           gridSeq->getChildAt(1),
                           gridSeq->getChildAt(2),
                           gridSeq->getChildAt(3),
                           gridSeq->getChildAt(4),
                           gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(3, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("80dp", gridSeq->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ("60dp", gridSeq->getCalculated(kPropertyChildHeight).at(1).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildHeight).at(2).asString());
    ASSERT_EQ("50%", gridSeq->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            3,  // num rows
            2,  // num columns
            {80, 60, 20}, // child heights
            {100})); // child widths
}

// Test for horizontal grid seq height/width properties for dynamic
TEST_F(GridSequenceComponentTest, HeightWidthHorizontal) {
    loadDocument(HORIZONTAL_GRID_SETVALUE);
    ASSERT_TRUE(component);

    auto gridSeq = std::dynamic_pointer_cast<CoreComponent>(component);
    ASSERT_EQ(kComponentTypeGridSequence, gridSeq->getType());

    ASSERT_EQ(kScrollDirectionHorizontal, gridSeq->getCalculated(kPropertyScrollDirection).asInt());
    ASSERT_EQ("80dp", gridSeq->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildHeight).at(1).asString());
    ASSERT_EQ("25%", gridSeq->getCalculated(kPropertyChildWidth).at(0).asString());
    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());

    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            2,  // num rows
            3,  // num columns
            {80, 80}, // child heights
            {50})); // child widths

    // Set width property of grid sequence, it will impact all components
    gridSeq->setProperty(kPropertyWidth, "160dp");

    root->clearPending();
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root, gridSeq,
                           gridSeq->getChildAt(0),
                           gridSeq->getChildAt(1),
                           gridSeq->getChildAt(2),
                           gridSeq->getChildAt(3),
                           gridSeq->getChildAt(4),
                           gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("80dp", gridSeq->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildHeight).at(1).asString());
    ASSERT_EQ("160dp", gridSeq->getCalculated(kPropertyWidth).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            2,  // num rows
            3,  // num columns
            {80, 80}, // child heights
            {40})); // child widths

    // Set height property of grid sequence, it will impact gridSeq and 3 children with width auto
    gridSeq->setProperty(kPropertyHeight, "200dp");

    ASSERT_EQ(4, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(gridSeq, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(0)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(1), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(2)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(3), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(4)));
    ASSERT_TRUE(CheckDirty(gridSeq->getChildAt(5), kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root, gridSeq,
                           gridSeq->getChildAt(1),
                           gridSeq->getChildAt(3),
                           gridSeq->getChildAt(5)));
    root->clearDirty();

    ASSERT_EQ(2, gridSeq->getCalculated(kPropertyItemsPerCourse).asInt());
    ASSERT_EQ("80dp", gridSeq->getCalculated(kPropertyChildHeight).at(0).asString());
    ASSERT_EQ("auto", gridSeq->getCalculated(kPropertyChildHeight).at(1).asString());
    ASSERT_EQ("200dp", gridSeq->getCalculated(kPropertyHeight).asString());
    ASSERT_TRUE(validateCellBounds(
            gridSeq,
            2,  // num rows
            3,  // num columns
            {80, 120}, // child heights
            {40})); // child widths
}

const char * SNAP_MULTI_COMPS = R"apl(
{
  "type": "APL",
  "version": "1.7",
  "layoutDirection":"RTL",
  "mainTemplate": {
    "parameters": [],
    "item": {
      "type": "Container",
      "width": 600,
      "items": [{
        "type": "GridSequence",
        "id": "gridSequence",
        "scrollDirection": "vertical",
        "width": 650,
        "height": 300,
        "snap": "forceStart",
        "childWidth": 300,
        "childHeight": 150,
        "items": {
          "id":  "${data}",
          "type": "Frame",
          "borderColor": "green",
          "borderWidth": 4,
          "items": {
            "type": "Text",
            "text": "${data}"
          }
        },
        "data": [
          0,
          1,
          2,
          3,
          4,
          5,
          6,
          7,
          8
        ]
      }]
    }
  }
}
)apl";

/**
 * Test fixes a bug with MultiChildScrollableComponent::findChildCloseToPosition not returning the
 * correct closest child when we have more than one child per row.
 */
TEST_F(GridSequenceComponentTest, TestSnappingWithMultipleComponentsPerLine) {
    loadDocument(SNAP_MULTI_COMPS);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(300,20)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(300,100)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(300,100)));
    advanceTime(100);

    // Give time for the component to snap
    advanceTime(1000);
    auto grid = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("gridSequence"));

    // Verify we snap to the top of the component
    ASSERT_EQ(0, grid->getCalculated(kPropertyScrollPosition).asNumber());
}