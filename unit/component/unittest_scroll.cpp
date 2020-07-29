/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <iostream>

#include "gtest/gtest.h"

#include "../testeventloop.h"

using namespace apl;

class ScrollTest : public DocumentWrapper {
public:
    void executeScroll(const std::string& component, float distance) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "Scroll", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("distance", distance, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        root->executeCommands(doc, false);
    }

    void executeScroll(const std::string& component, const std::string& distance) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "Scroll", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("distance", rapidjson::Value(distance.c_str(), alloc).Move(), alloc);
        doc.SetArray().PushBack(cmd, alloc);
        root->executeCommands(doc, false);
    }

    void completeScroll(const ComponentPtr& component, float distance) {
        ASSERT_FALSE(root->hasEvent());
        executeScroll(component->getId(), distance);
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();

        auto position = event.getValue(kEventPropertyPosition).asDimension(*context);
        event.getComponent()->update(kUpdateScrollPosition, position.getValue());
        event.getActionRef().resolve();
    }

    void completeScroll(const ComponentPtr& component, const std::string& distance) {
        ASSERT_FALSE(root->hasEvent());
        executeScroll(component->getId(), distance);
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();

        auto position = event.getValue(kEventPropertyPosition).asDimension(*context);
        event.getComponent()->update(kUpdateScrollPosition, position.getValue());
        event.getActionRef().resolve();
        root->clearPending();
    }

    void executeScrollToIndex(const std::string& component, int index, CommandScrollAlign align) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "ScrollToIndex", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("index", index, alloc);
        cmd.AddMember("align", rapidjson::StringRef(sCommandAlignMap.at(align).c_str()), alloc);
        doc.SetArray().PushBack(cmd, alloc);
        root->executeCommands(doc, false);
    }

    void scrollToIndex(const ComponentPtr& component, int index, CommandScrollAlign align) {
        ASSERT_FALSE(root->hasEvent());
        executeScrollToIndex(component->getId(), index, align);
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();

        auto position = event.getValue(kEventPropertyPosition).asDimension(*context);
        event.getComponent()->update(kUpdateScrollPosition, position.getValue());
        event.getActionRef().resolve();
        root->clearPending();
    }

    void executeScrollToComponent(const std::string& component, CommandScrollAlign align) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "ScrollToComponent", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("align", rapidjson::StringRef(sCommandAlignMap.at(align).c_str()), alloc);
        doc.SetArray().PushBack(cmd, alloc);
        root->executeCommands(doc, false);
    }

    void scrollToComponent(const ComponentPtr& component, CommandScrollAlign align) {
        ASSERT_FALSE(root->hasEvent());
        executeScrollToComponent(component->getId(), align);
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();

        auto position = event.getValue(kEventPropertyPosition).asDimension(*context);
        event.getComponent()->update(kUpdateScrollPosition, position.getValue());
        event.getActionRef().resolve();
        root->clearPending();
    }

    rapidjson::Document doc;
};

static const char *SCROLL_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": ["
    "        {"
    "          \"type\": \"ScrollView\","
    "          \"id\": \"myScrollView\","
    "          \"width\": \"200\","
    "          \"height\": \"200\","
    "          \"items\": {"
    "            \"type\": \"Frame\","
    "            \"id\": \"myFrame\","
    "            \"width\": 200,"
    "            \"height\": 1000"
    "          }"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"myTouch\","
    "          \"height\": 10,"
    "          \"onPress\": {"
    "            \"type\": \"Scroll\","
    "            \"componentId\": \"myScrollView\","
    "            \"distance\": 0.5"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, ScrollForward)
{
    loadDocument(SCROLL_TEST);
    auto touch = context->findComponentById("myTouch");
    auto scroll = context->findComponentById("myScrollView");
    auto frame = context->findComponentById("myFrame");
    ASSERT_TRUE(touch);
    ASSERT_TRUE(scroll);
    ASSERT_TRUE(frame);

    ASSERT_EQ(Rect(0, 0, 200, 1000), frame->getGlobalBounds());
    ASSERT_EQ(Point(), scroll->scrollPosition());
    performTap(0, 200);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeScrollTo, event.getType());
    ASSERT_EQ(Object(Dimension(100)), event.getValue(kEventPropertyPosition));

    scroll->update(kUpdateScrollPosition, 100);
    ASSERT_TRUE(CheckDirty(frame));  // Scrolling doesn't cause any dirty events - the DOM hasn't changed.
    ASSERT_TRUE(CheckDirty(scroll));
    ASSERT_TRUE(CheckDirty(root));

    ASSERT_EQ(Point(0, 100), scroll->scrollPosition());
    ASSERT_EQ(Rect(0, -100, 200, 1000), frame->getGlobalBounds());

    ASSERT_FALSE(event.getActionRef().isEmpty());
    event.getActionRef().resolve();
    ASSERT_TRUE(CheckNoActions());
}

TEST_F(ScrollTest, ScrollForwardMultiple)
{
    loadDocument(SCROLL_TEST);
    auto touch = context->findComponentById("myTouch");
    auto scroll = context->findComponentById("myScrollView");
    auto frame = context->findComponentById("myFrame");

    for (int i = 0 ; i < 20 ; i++) {
        performTap(0, 200);

        // Handle the "ScrollTo" event
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypeScrollTo, event.getType()) << "case: " << i;
        auto position = event.getValue(kEventPropertyPosition).asDimension(*context);
        int target = 100 * (i+1);
        if (target > 800) target = 800;
        ASSERT_EQ(target, position.getValue()) << "case: " << i;

        // Update our scrolled position and resolve
        scroll->update(kUpdateScrollPosition, position.getValue());
        ASSERT_EQ(Point(0, position.getValue()), scroll->scrollPosition()) << "case: " << i;
        ASSERT_EQ(Rect(0, -position.getValue(), 200, 1000), frame->getGlobalBounds()) << "case: " << i;
        event.getActionRef().resolve();
    }

    ASSERT_TRUE(CheckNoActions());
}

TEST_F(ScrollTest, BothDirections)
{
    loadDocument(SCROLL_TEST);
    auto touch = context->findComponentById("myTouch");
    auto scroll = context->findComponentById("myScrollView");
    auto frame = context->findComponentById("myFrame");

    completeScroll(scroll, 2.0);
    ASSERT_EQ(Point(0, 400), scroll->scrollPosition());
    ASSERT_EQ(Rect(0, -400, 200, 1000), frame->getGlobalBounds());

    completeScroll(scroll, 2.0);
    ASSERT_EQ(Point(0, 800), scroll->scrollPosition());
    ASSERT_EQ(Rect(0, -800, 200, 1000), frame->getGlobalBounds());

    // Can't scroll past the end
    completeScroll(scroll, 0.4);
    ASSERT_EQ(Point(0, 800), scroll->scrollPosition());
    ASSERT_EQ(Rect(0, -800, 200, 1000), frame->getGlobalBounds());

    completeScroll(scroll, -1.0f);
    ASSERT_EQ(Point(0, 600), scroll->scrollPosition());
    ASSERT_EQ(Rect(0, -600, 200, 1000), frame->getGlobalBounds());

    // Can't scroll past the beginning
    completeScroll(scroll, -5.0f);
    ASSERT_EQ(Point(0,0), scroll->scrollPosition());
    ASSERT_EQ(Rect(0, 0, 200, 1000), frame->getGlobalBounds());
}

TEST_F(ScrollTest, ScrollTextWithAlignment)
{
    loadDocument(SCROLL_TEST);
    auto touch = context->findComponentById("myTouch");
    auto scroll = context->findComponentById("myScrollView");
    auto frame = context->findComponentById("myFrame");
    ASSERT_TRUE(touch);
    ASSERT_TRUE(scroll);
    ASSERT_TRUE(frame);

    ASSERT_EQ(Rect(0, 0, 200, 1000), frame->getGlobalBounds());
    ASSERT_EQ(Point(), scroll->scrollPosition());
    root->scrollToRectInComponent(frame, Rect(0, 200, 1000, 50), kCommandScrollAlignCenter);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeScrollTo, event.getType());
    auto position = event.getValue(kEventPropertyPosition).asDimension(*context);
    ASSERT_EQ(Dimension(125), position);
}

static const char *SCROLLVIEW_WITH_PADDING =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"ScrollView\","
    "      \"id\": \"myScrollView\","
    "      \"paddingTop\": 50,"
    "      \"paddingBottom\": 50,"
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"myFrame\","
    "        \"width\": 100,"
    "        \"height\": 1000"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, ScrollViewPadding)
{
    // Content height is 1000
    // ScrollView inner height is 200, with a 50 dp padding offset at the top
    // Max scroll distance is 800, with a distance of 4.

    loadDocument(SCROLLVIEW_WITH_PADDING);
    auto frame = context->findComponentById("myFrame");

    completeScroll(component, -2.0f);
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    ASSERT_EQ(Rect(0, 50, 100, 1000), frame->getGlobalBounds());

    completeScroll(component, 2.0);
    ASSERT_EQ(Point(0, 400), component->scrollPosition());
    ASSERT_EQ(Rect(0, -350, 100, 1000), frame->getGlobalBounds());

    completeScroll(component, 3.0);   // Maximum
    ASSERT_EQ(Point(0, 800), component->scrollPosition());
    ASSERT_EQ(Rect(0, -750, 100, 1000), frame->getGlobalBounds());
}

static const char *SCROLLVIEW_SMALL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"ScrollView\","
    "      \"id\": \"myScrollView\","
    "      \"paddingTop\": 50,"
    "      \"paddingBottom\": 50,"
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"myFrame\","
    "        \"width\": 100,"
    "        \"height\": 50"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, ScrollViewSmall)
{
    // Content height is 50
    // ScrollView inner height is 200, with a 50 dp padding offset at the top
    // Max scroll distance is 0

    loadDocument(SCROLLVIEW_SMALL);
    auto frame = context->findComponentById("myFrame");

    completeScroll(component, -2.0f);
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    ASSERT_EQ(Rect(0, 50, 100, 50), frame->getGlobalBounds());

    completeScroll(component, 2.0);
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    ASSERT_EQ(Rect(0, 50, 100, 50), frame->getGlobalBounds());
}

static const char *SCROLLVIEW_NONE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"ScrollView\","
    "      \"id\": \"myScrollView\","
    "      \"paddingTop\": 50,"
    "      \"paddingBottom\": 50,"
    "      \"width\": 200,"
    "      \"height\": 300"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, ScrollViewNone)
{
    // No inner content
    // Max scroll distance is 0

    loadDocument(SCROLLVIEW_NONE);

    completeScroll(component, -2.0f);
    ASSERT_EQ(Point(0, 0), component->scrollPosition());

    completeScroll(component, 2.0);
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
}


static const char *SEQUENCE_TEST_HORIZONTAL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"horizontal\","
    "      \"id\": \"foo\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, Sequence)
{
    loadDocument(SEQUENCE_TEST_HORIZONTAL);

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);
    ASSERT_EQ(Point(200,0), component->scrollPosition());

    completeScroll(component, 5);  // This maxes out
    ASSERT_EQ(Point(800,0), component->scrollPosition());

    completeScroll(component, 5);
    ASSERT_EQ(Point(800,0), component->scrollPosition());

    completeScroll(component, -0.5f);
    ASSERT_EQ(Point(700,0), component->scrollPosition());

    completeScroll(component, -20);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

static const char *GRID_SEQUENCE_TEST_HORIZONTAL = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "GridSequence",
      "scrollDirection": "horizontal",
      "id": "foo",
      "width": 200,
      "height": 300,
      "childWidth": 100,
      "childHeight": "50%",
      "item": {
        "type": "Frame"
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

TEST_F(ScrollTest, GridSequence)
{
    loadDocument(GRID_SEQUENCE_TEST_HORIZONTAL);

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);
    ASSERT_EQ(Point(200,0), component->scrollPosition());

    completeScroll(component, 2);  // This maxes out
    ASSERT_EQ(Point(400,0), component->scrollPosition());

    completeScroll(component, 5);
    ASSERT_EQ(Point(400,0), component->scrollPosition());

    completeScroll(component, -0.5f);
    ASSERT_EQ(Point(300,0), component->scrollPosition());

    completeScroll(component, -20);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

static const char *SEQUENCE_TEST_HORIZONTAL_SMALL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"horizontal\","
    "      \"id\": \"foo\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 50,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, SequenceSmall)
{
    loadDocument(SEQUENCE_TEST_HORIZONTAL_SMALL);

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, -20);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}


static const char *SEQUENCE_TEST_HORIZONTAL_PADDING_SPACING =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"horizontal\","
    "      \"paddingLeft\": 50,"
    "      \"paddingRight\": 50,"
    "      \"id\": \"foo\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"spacing\": 10,"
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";


TEST_F(ScrollTest, SequenceHorizontalPaddingSpacing)
{
    // The inner width of the sequence is 100.
    // There are 1090 dp of children.
    // The maximum scroll position is 990, which is 9.9 screens

    loadDocument(SEQUENCE_TEST_HORIZONTAL_PADDING_SPACING);

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);
    ASSERT_EQ(Point(100,0), component->scrollPosition());

    completeScroll(component, 5);  // This doesn't max out
    ASSERT_EQ(Point(600,0), component->scrollPosition());

    completeScroll(component, 5);  // This does max out
    ASSERT_EQ(Point(990,0), component->scrollPosition());

    completeScroll(component, -0.5f);
    ASSERT_EQ(Point(940,0), component->scrollPosition());

    completeScroll(component, -20);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

static const char *SEQUENCE_TEST_VERTICAL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"vertical\","
    "      \"id\": \"foo\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, SequenceVertical)
{
    loadDocument(SEQUENCE_TEST_VERTICAL);

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);
    ASSERT_EQ(Point(0, 300), component->scrollPosition());

    completeScroll(component, 5);  // This maxes out
    ASSERT_EQ(Point(0, 700), component->scrollPosition());

    completeScroll(component, 5);
    ASSERT_EQ(Point(0, 700), component->scrollPosition());

    completeScroll(component, -0.5f);
    ASSERT_EQ(Point(0, 550), component->scrollPosition());

    completeScroll(component, -20);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

static const char *GRID_SEQUENCE_TEST_VERTICAL = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "GridSequence",
      "scrollDirection": "vertical",
      "id": "foo",
      "width": 200,
      "height": 200,
      "childHeight": "100dp",
      "childWidth": "100dp",
      "items": {
        "type": "Frame"
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

TEST_F(ScrollTest, GridSequenceVertical)
{
    loadDocument(GRID_SEQUENCE_TEST_VERTICAL);

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);
    ASSERT_EQ(Point(0, 200), component->scrollPosition());

    completeScroll(component, 5);  // This maxes out
    ASSERT_EQ(Point(0, 400), component->scrollPosition());

    completeScroll(component, 5);
    ASSERT_EQ(Point(0, 400), component->scrollPosition());

    completeScroll(component, -0.5f);
    ASSERT_EQ(Point(0, 300), component->scrollPosition());

    completeScroll(component, -20);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

static const char *SEQUENCE_TEST_VERTICAL_PADDING_SPACING =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"vertical\","
    "      \"paddingTop\": 50,"
    "      \"paddingBottom\": 50,"
    "      \"id\": \"foo\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"spacing\": 10,"
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, SequenceVerticalPaddingSpacing)
{
    loadDocument(SEQUENCE_TEST_VERTICAL_PADDING_SPACING);

    // The inner height of the sequence is 200.
    // There are 1090 dp of children.
    // The maximum scroll position is 890, which is 4.45 screens

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);
    ASSERT_EQ(Point(0, 200), component->scrollPosition());

    completeScroll(component, 5);  // This maxes out - tries to scroll to 1200
    ASSERT_EQ(Point(0, 890), component->scrollPosition());

    completeScroll(component, 5);
    ASSERT_EQ(Point(0, 890), component->scrollPosition());

    completeScroll(component, -0.5f);
    ASSERT_EQ(Point(0, 790), component->scrollPosition());

    completeScroll(component, -20);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

static const char *SEQUENCE_TEST_VERTICAL_PADDING_SPACING_SMALL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"vertical\","
    "      \"paddingTop\": 50,"
    "      \"paddingBottom\": 50,"
    "      \"id\": \"foo\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"spacing\": 10,"
    "        \"width\": 100,"
    "        \"height\": 10"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, SequenceVerticalPaddingSpacingSmall)
{
    loadDocument(SEQUENCE_TEST_VERTICAL_PADDING_SPACING_SMALL);

    // The inner height of the sequence is 200.
    // There are 190 dp of children.
    // The maximum scroll position is 0

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);
    ASSERT_EQ(Point(0, 0), component->scrollPosition());

    completeScroll(component, -20);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

static const char *SEQUENCE_DIFFERENT_UNITS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"vertical\","
    "      \"id\": \"foo\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 200"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, DifferentUnits)
{
    loadDocument(SEQUENCE_DIFFERENT_UNITS);

    completeScroll(component, -1);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, "150%");
    ASSERT_EQ(Point(0,450), component->scrollPosition());

    completeScroll(component, "-50%");
    ASSERT_EQ(Point(0,300), component->scrollPosition());

    completeScroll(component, "10vh");   // Should be 80
    ASSERT_EQ(Point(0, 380), component->scrollPosition());

    completeScroll(component, "-0.5");   // Should be -150
    ASSERT_EQ(Point(0, 230), component->scrollPosition());

    completeScroll(component, -0.5f);  // Another -150
    ASSERT_EQ(Point(0, 80), component->scrollPosition());
}

static const char *SEQUENCE_EMPTY =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Sequence\","
        "      \"id\": \"foo\","
        "      \"width\": 200,"
        "      \"height\": 300,"
        "      \"items\": []"
        "    }"
        "  }"
        "}";


TEST_F(ScrollTest, SequenceEmpty)
{
    loadDocument(SEQUENCE_EMPTY);

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);  // Can't scroll forwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

static const char *SEQUENCE_WITH_INDEX =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"vertical\","
    "      \"id\": \"foo\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, ScrollToIndexFirst)
{
    loadDocument(SEQUENCE_WITH_INDEX);

    // Move the second item up to the top of the scroll view.
    scrollToIndex(component, 1, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,100), component->scrollPosition());

    // Repeat the command - it shouldn't move.
    scrollToIndex(component, 1, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,100), component->scrollPosition());

    scrollToIndex(component, 5, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,500), component->scrollPosition());

    scrollToIndex(component, 3, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,300), component->scrollPosition());

    // The last component can't scroll all the way to the top
    scrollToIndex(component, 9, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0, 700), component->scrollPosition());

    scrollToIndex(component, 0, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,500), component->scrollPosition());
}

TEST_F(ScrollTest, ScrollToIndexLast)
{
    loadDocument(SEQUENCE_WITH_INDEX);

    // Hits the top limit
    scrollToIndex(component, 1, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, 5, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,300), component->scrollPosition());

    // Repeat the command - nothing moves
    scrollToIndex(component, 5, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,300), component->scrollPosition());

    scrollToIndex(component, 3, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,100), component->scrollPosition());

    // Scroll to the last element
    scrollToIndex(component, 9, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0, 700), component->scrollPosition());

    scrollToIndex(component, 0, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,300), component->scrollPosition());
}

TEST_F(ScrollTest, ScrollToIndexCenter)
{
    loadDocument(SEQUENCE_WITH_INDEX);

    // This one should already be centered
    scrollToIndex(component, 1, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, 2, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,100), component->scrollPosition());

    scrollToIndex(component, 5, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,400), component->scrollPosition());

    // Repeat the command - nothing moves
    scrollToIndex(component, 5, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,400), component->scrollPosition());

    scrollToIndex(component, 3, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,200), component->scrollPosition());

    // Scroll to the last element
    scrollToIndex(component, 9, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0, 700), component->scrollPosition());

    scrollToIndex(component, 0, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,400), component->scrollPosition());
}

TEST_F(ScrollTest, ScrollToIndexVisible)
{
    loadDocument(SEQUENCE_WITH_INDEX);

    // This one is already visible
    scrollToIndex(component, 1, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    // So is this one
    scrollToIndex(component, 2, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    // This one will end up at the bottom
    scrollToIndex(component, 5, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,300), component->scrollPosition());

    // This one is already visible
    scrollToIndex(component, 3, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,300), component->scrollPosition());

    // Move to the second-to-last one
    scrollToIndex(component, 8, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,600), component->scrollPosition());

    // Showing the last one should just scroll it into view
    scrollToIndex(component, 9, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,700), component->scrollPosition());

    // Going back by three will scroll it down just a one notch
    scrollToIndex(component, 6, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,600), component->scrollPosition());

    // Go back to the first one
    scrollToIndex(component, 0, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,300), component->scrollPosition());
}


static const char *SEQUENCE_WITH_INDEX_AND_PADDING =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"vertical\","
    "      \"id\": \"foo\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"paddingTop\": 50,"
    "      \"paddingBottom\": 50,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 100,"
    "        \"spacing\": 10"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, ScrollToIndexFirstPadding)
{
    loadDocument(SEQUENCE_WITH_INDEX_AND_PADDING);

    // Move the second item up to the top of the scroll view.
    scrollToIndex(component, 1, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,110), component->scrollPosition());

    // Repeat the command - it shouldn't move.
    scrollToIndex(component, 1, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,110), component->scrollPosition());

    scrollToIndex(component, 5, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,550), component->scrollPosition());

    scrollToIndex(component, 3, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,330), component->scrollPosition());

    // The last component can't scroll all the way to the top
    scrollToIndex(component, 9, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,890), component->scrollPosition());

    scrollToIndex(component, 0, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,550), component->scrollPosition());
}


TEST_F(ScrollTest, ScrollToIndexLastPadding)
{
    loadDocument(SEQUENCE_WITH_INDEX_AND_PADDING);

    // The spacing means we scroll just a bit to bring it into view
    scrollToIndex(component, 1, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,10), component->scrollPosition());

    scrollToIndex(component, 5, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,450), component->scrollPosition());

    // Repeat the command - nothing moves
    scrollToIndex(component, 5, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,450), component->scrollPosition());

    scrollToIndex(component, 3, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,230), component->scrollPosition());

    // Scroll to the last element
    scrollToIndex(component, 9, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,890), component->scrollPosition());

    scrollToIndex(component, 0, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,450), component->scrollPosition());
}

TEST_F(ScrollTest, ScrollToIndexCenterPadding)
{
    loadDocument(SEQUENCE_WITH_INDEX_AND_PADDING);

    // This one should already be centered
    scrollToIndex(component, 1, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,60), component->scrollPosition());

    scrollToIndex(component, 2, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,170), component->scrollPosition());

    scrollToIndex(component, 5, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,500), component->scrollPosition());

    // Repeat the command - nothing moves
    scrollToIndex(component, 5, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,500), component->scrollPosition());

    scrollToIndex(component, 3, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,280), component->scrollPosition());

    // Scroll to the last element
    scrollToIndex(component, 9, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,890), component->scrollPosition());

    scrollToIndex(component, 0, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,500), component->scrollPosition());
}

TEST_F(ScrollTest, ScrollToIndexVisiblePadding)
{
    loadDocument(SEQUENCE_WITH_INDEX_AND_PADDING);

    scrollToIndex(component, 1, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,10), component->scrollPosition());

    // Aligns to bottom
    scrollToIndex(component, 2, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,120), component->scrollPosition());

    // Aligns to bottom
    scrollToIndex(component, 5, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,450), component->scrollPosition());

    // Aligns to top
    scrollToIndex(component, 3, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,330), component->scrollPosition());

    // Aligns to the bottom
    scrollToIndex(component, 8, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,780), component->scrollPosition());

    // Aligns to the bottom
    scrollToIndex(component, 9, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,890), component->scrollPosition());

    // Aligns to the top
    scrollToIndex(component, 6, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,660), component->scrollPosition());

    // Go back to the first one
    scrollToIndex(component, 0, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,450), component->scrollPosition());
}


static const char *HORIZONTAL_SEQUENCE_WITH_INDEX_AND_PADDING =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"horizontal\","
    "      \"id\": \"foo\","
    "      \"width\": 400,"
    "      \"height\": 300,"
    "      \"paddingLeft\": 50,"
    "      \"paddingRight\": 50,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 100,"
    "        \"spacing\": 10"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, ScrollToIndexHorizontal)
{
    loadDocument(HORIZONTAL_SEQUENCE_WITH_INDEX_AND_PADDING);

    // The second item is already in view
    scrollToIndex(component, 1, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    // Force it to the left
    scrollToIndex(component, 1, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(110,0), component->scrollPosition());

    // Center (center of child=600, center of view=150)
    scrollToIndex(component, 5, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(450, 0), component->scrollPosition());

    // Make the previous item visible (shifts just a little back)
    scrollToIndex(component, 4, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(440, 0), component->scrollPosition());

    // Make the next item visible (shifts slightly to the right)
    scrollToIndex(component, 6, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(460, 0), component->scrollPosition());

    // Make a previous item aligned right
    scrollToIndex(component, 5, kCommandScrollAlignLast);
    ASSERT_EQ(Point(350, 0), component->scrollPosition());

    // Crash into the end
    scrollToIndex(component, 9, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(790, 0), component->scrollPosition());

    // Back to the start
    scrollToIndex(component, 0, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(450,0), component->scrollPosition());
}


static const char *MISSING_INDEX =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"horizontal\","
    "      \"id\": \"foo\","
    "      \"width\": 400,"
    "      \"height\": 300,"
    "      \"paddingLeft\": 50,"
    "      \"paddingRight\": 50,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 100,"
    "        \"spacing\": 10"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, ScrollToMissingIndex)
{
    loadDocument(MISSING_INDEX);

    // First, execute a valid scroll
    executeScrollToIndex("foo", 1, kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    event.getActionRef().resolve();

    // Now specify an invalid component
    ASSERT_FALSE(ConsoleMessage());
    executeScrollToIndex("foobar", 1, kCommandScrollAlignFirst);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    // Try an invalid index
    executeScrollToIndex("foo", 15, kCommandScrollAlignFirst);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    // Valid negative index scroll
    executeScrollToIndex("foo", -1, kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    event.getActionRef().resolve();
    ASSERT_FALSE(ConsoleMessage());

    // Try an invalid negative index
    executeScrollToIndex("foo", -15, kCommandScrollAlignFirst);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}


static const char *VERTICAL_SCROLLVIEW =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"ScrollView\","
    "      \"paddingTop\": 50,"
    "      \"paddingBottom\": 50,"
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Container\","
    "        \"direction\": \"vertical\","
    "        \"items\": ["
    "          {"
    "            \"type\": \"Frame\","
    "            \"id\": \"frame1\","
    "            \"width\": 100,"
    "            \"height\": 200"
    "          },"
    "          {"
    "            \"type\": \"Frame\","
    "            \"id\": \"frame2\","
    "            \"width\": 100,"
    "            \"height\": 300"
    "          },"
    "          {"
    "            \"type\": \"Frame\","
    "            \"id\": \"frame3\","
    "            \"width\": 100,"
    "            \"height\": 100"
    "          },"
    "          {"
    "            \"type\": \"Frame\","
    "            \"id\": \"frame4\","
    "            \"width\": 100,"
    "            \"height\": 400"
    "          },"
    "          {"
    "            \"type\": \"Frame\","
    "            \"id\": \"frame5\","
    "            \"width\": 100,"
    "            \"height\": 100"
    "          },"
    "          {"
    "            \"type\": \"Frame\","
    "            \"id\": \"frame6\","
    "            \"width\": 100,"
    "            \"height\": 300"
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, ScrollToComponent)
{
    loadDocument(VERTICAL_SCROLLVIEW);

    std::map<std::string, ComponentPtr> map;
    for (int i = 1 ; i <= 6 ; i++) {
        std::string name = "frame" + std::to_string(i);
        map.emplace(name, root->context().findComponentById(name));
    }

    // First, test scrolling to show the top
    scrollToComponent(map["frame2"], kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,200), component->scrollPosition());

    scrollToComponent(map["frame4"], kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,600), component->scrollPosition());

    scrollToComponent(map["frame6"], kCommandScrollAlignFirst);
    ASSERT_EQ(Point(0,1100), component->scrollPosition());

    // Now align to the bottom (this pushes frame6 up just a bit)
    scrollToComponent(map["frame6"], kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,1200), component->scrollPosition());

    scrollToComponent(map["frame1"], kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToComponent(map["frame4"], kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,800), component->scrollPosition());

    // Test center alignment, particularly large items
    scrollToComponent(map["frame4"], kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,700), component->scrollPosition());

    scrollToComponent(map["frame6"], kCommandScrollAlignCenter);
    ASSERT_EQ(Point(0,1150), component->scrollPosition());

    // Check visible alignment
    scrollToComponent(map["frame6"], kCommandScrollAlignVisible);  // Already totally covering; should align top
    ASSERT_EQ(Point(0,1100), component->scrollPosition());

    scrollToComponent(map["frame5"], kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0, 1000), component->scrollPosition());

    scrollToComponent(map["frame2"], kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0, 200), component->scrollPosition());

    scrollToComponent(map["frame5"], kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
}

TEST_F(ScrollTest, ScrollToMissingComponent)
{
    loadDocument(VERTICAL_SCROLLVIEW);

    // Check a valid component first
    executeScrollToComponent("frame2", kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    event.getActionRef().resolve();

    // Now try an invalid component
    ASSERT_FALSE(ConsoleMessage());
    executeScrollToComponent("frame26", kCommandScrollAlignFirst);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(ScrollTest, ScrollWithTermination)
{
    loadDocument(VERTICAL_SCROLLVIEW);

    // Start a valid scroll command
    executeScrollToComponent("frame2", kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    // Add a termination callback
    bool wasTerminated = false;
    event.getActionRef().addTerminateCallback([&wasTerminated](const TimersPtr&) {
        wasTerminated = true;
    });

    root->cancelExecution();
    ASSERT_TRUE(wasTerminated);
}

static const char *VERTICAL_DEEP_SEQUENCE =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Sequence\","
        "      \"id\": \"seq\","
        "      \"width\": 600,"
        "      \"height\": 600,"
        "      \"data\": [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19],"
        "      \"scrollDirection\": \"vertical\","
        "      \"items\": {"
        "        \"type\": \"TouchWrapper\","
        "        \"id\": \"item${data}\","
        "        \"item\": {"
        "          \"type\": \"Container\","
        "          \"id\": \"container${data}\","
        "          \"items\": ["
        "            {"
        "              \"type\": \"Text\","
        "              \"id\": \"text${data}\","
        "              \"width\": 150,"
        "              \"height\": 150,"
        "              \"text\": \"${data}\""
        "            }"
        "          ]"
        "        }"
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(ScrollTest, SequenceToVerticalComponent)
{
    loadDocument(VERTICAL_DEEP_SEQUENCE);

    // Scroll to ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("item1", kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    auto position = event.getValue(kEventPropertyPosition).asDimension(*context);
    ASSERT_EQ(150, position.getValue());
    event.getComponent()->update(kUpdateScrollPosition, position.getValue());
    event.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Point(0, 150), event.getComponent()->scrollPosition());

    // Scroll to non-ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("item10", kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();

    position = event.getValue(kEventPropertyPosition).asDimension(*context);
    ASSERT_EQ(1500, position.getValue());
    event.getComponent()->update(kUpdateScrollPosition, position.getValue());
    event.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Point(0, 1500), event.getComponent()->scrollPosition());

    // Scroll to non-ensured one by index (we don't forward-ensure)
    ASSERT_FALSE(root->hasEvent());
    executeScrollToIndex("seq", 12, kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();

    position = event.getValue(kEventPropertyPosition).asDimension(*context);
    ASSERT_EQ(1800, position.getValue());
    event.getComponent()->update(kUpdateScrollPosition, position.getValue());
    event.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Point(0, 1800), event.getComponent()->scrollPosition());
}

TEST_F(ScrollTest, SequenceToVerticalSubComponent)
{
    loadDocument(VERTICAL_DEEP_SEQUENCE);

    // Scroll to ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("text1", kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    auto position = event.getValue(kEventPropertyPosition).asDimension(*context);
    ASSERT_EQ(150, position.getValue());
    event.getComponent()->update(kUpdateScrollPosition, position.getValue());
    event.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Point(0, 150), event.getComponent()->scrollPosition());

    // Scroll to non-ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("text10", kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();

    position = event.getValue(kEventPropertyPosition).asDimension(*context);
    ASSERT_EQ(1500, position.getValue());
    event.getComponent()->update(kUpdateScrollPosition, position.getValue());
    event.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Point(0, 1500), event.getComponent()->scrollPosition());
}

static const char *HORIZONTAL_DEEP_SEQUENCE =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Sequence\","
        "      \"id\": \"seq\","
        "      \"width\": 600,"
        "      \"height\": 500,"
        "      \"data\": [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19],"
        "      \"scrollDirection\": \"horizontal\","
        "      \"items\": {"
        "        \"type\": \"Container\","
        "        \"id\": \"item${index}\","
        "        \"items\": ["
        "          {"
        "            \"type\": \"Text\","
        "            \"id\": \"text${data}\","
        "            \"width\": 150,"
        "            \"height\": 150,"
        "            \"text\": \"${data}\""
        "          }"
        "        ]"
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(ScrollTest, SequenceToHorizontalComponent)
{
    loadDocument(HORIZONTAL_DEEP_SEQUENCE);

    // Scroll to ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("item1", kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    auto position = event.getValue(kEventPropertyPosition).asDimension(*context);
    ASSERT_EQ(150, position.getValue());
    event.getComponent()->update(kUpdateScrollPosition, position.getValue());
    event.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Point(150, 0), event.getComponent()->scrollPosition());

    // Scroll to non-ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("item10", kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();

    position = event.getValue(kEventPropertyPosition).asDimension(*context);
    ASSERT_EQ(1500, position.getValue());
    event.getComponent()->update(kUpdateScrollPosition, position.getValue());
    event.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Point(1500, 0), event.getComponent()->scrollPosition());

    // Scroll to non-ensured one by index (we don't forward-ensure)
    ASSERT_FALSE(root->hasEvent());
    executeScrollToIndex("seq", 12, kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();

    position = event.getValue(kEventPropertyPosition).asDimension(*context);
    ASSERT_EQ(1800, position.getValue());
    event.getComponent()->update(kUpdateScrollPosition, position.getValue());
    event.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Point(1800, 0), event.getComponent()->scrollPosition());
}

TEST_F(ScrollTest, SequenceToHorizontalSubComponent)
{
    loadDocument(HORIZONTAL_DEEP_SEQUENCE);

    // Scroll to ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("text1", kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    auto position = event.getValue(kEventPropertyPosition).asDimension(*context);
    ASSERT_EQ(150, position.getValue());
    event.getComponent()->update(kUpdateScrollPosition, position.getValue());
    event.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Point(150, 0), event.getComponent()->scrollPosition());

    // Scroll to non-ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("text10", kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();

    position = event.getValue(kEventPropertyPosition).asDimension(*context);
    ASSERT_EQ(1500, position.getValue());
    event.getComponent()->update(kUpdateScrollPosition, position.getValue());
    event.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Point(1500, 0), event.getComponent()->scrollPosition());
}

static const char *PAGER_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Pager\","
    "      \"id\": \"myPager\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"id${data}\","
    "        \"text\": \"TEXT${data}\","
    "        \"speech\": \"URL${data}\""
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4"
    "      ],"
    "      \"onPageChanged\": {"
    "        \"type\": \"SendEvent\","
    "        \"arguments\": ["
    "          \"${event.target.page}\""
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(ScrollTest, Pager)
{
    loadDocument(PAGER_TEST);

    executeScrollToComponent("id2", kCommandScrollAlignFirst);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(kEventTypeSetPage, event.getType());
    ASSERT_EQ(component, event.getComponent());
    ASSERT_EQ(Dimension(1), event.getValue(kEventPropertyPosition).asDimension(*context));
    ASSERT_EQ(kEventDirectionForward, event.getValue(kEventPropertyDirection).getInteger());
}
