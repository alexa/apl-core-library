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
        executeCommands(doc, false);
    }

    void executeScroll(const std::string& component, float distance, int duration) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "Scroll", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("distance", distance, alloc);
        cmd.AddMember("targetDuration", duration, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        executeCommands(doc, false);
    }

    void executeScroll(const std::string& component, const std::string& distance) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "Scroll", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("distance", rapidjson::Value(distance.c_str(), alloc).Move(), alloc);
        doc.SetArray().PushBack(cmd, alloc);
        executeCommands(doc, false);
    }

    void completeScroll(const ComponentPtr& component, float distance) {
        ASSERT_FALSE(root->hasEvent());
        executeScroll(component->getId(), distance);
        advanceTime(1000);
    }

    void completeScroll(const ComponentPtr& component, float distance, int duration) {
        ASSERT_FALSE(root->hasEvent());
        executeScroll(component->getId(), distance, duration);
        advanceTime(duration);
    }

    void completeScroll(const ComponentPtr& component, const std::string& distance) {
        ASSERT_FALSE(root->hasEvent());
        executeScroll(component->getId(), distance);
        advanceTime(1000);
    }

    void executeScrollToIndex(const std::string& component, int index, CommandScrollAlign align) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "ScrollToIndex", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("index", index, alloc);
        cmd.AddMember("align", rapidjson::StringRef(sCommandAlignMap.at(align).c_str()), alloc);
        doc.SetArray().PushBack(cmd, alloc);
        executeCommands(doc, false);
    }

    void executeScrollToIndex(const std::string& component, int index, CommandScrollAlign align, int duration) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "ScrollToIndex", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("index", index, alloc);
        cmd.AddMember("align", rapidjson::StringRef(sCommandAlignMap.at(align).c_str()), alloc);
        cmd.AddMember("targetDuration", duration, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        executeCommands(doc, false);
    }

    void scrollToIndex(const ComponentPtr& component, int index, CommandScrollAlign align) {
        ASSERT_FALSE(root->hasEvent());
        executeScrollToIndex(component->getId(), index, align);
        advanceTime(1000);
    }

    void scrollToIndex(const ComponentPtr& component, int index, CommandScrollAlign align, int duration) {
        ASSERT_FALSE(root->hasEvent());
        executeScrollToIndex(component->getId(), index, align, duration);
        advanceTime(duration);
    }

    void executeScrollToComponent(const std::string& component, CommandScrollAlign align) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "ScrollToComponent", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("align", rapidjson::StringRef(sCommandAlignMap.at(align).c_str()), alloc);
        doc.SetArray().PushBack(cmd, alloc);
        executeCommands(doc, false);
    }

    void executeScrollToComponent(const std::string& component, CommandScrollAlign align, int duration) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "ScrollToComponent", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("align", rapidjson::StringRef(sCommandAlignMap.at(align).c_str()), alloc);
        cmd.AddMember("targetDuration", duration, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        executeCommands(doc, false);
    }

    void scrollToComponent(const ComponentPtr& component, CommandScrollAlign align) {
        ASSERT_FALSE(root->hasEvent());
        executeScrollToComponent(component->getId(), align);
        advanceTime(1000);
    }

    void scrollToComponent(const ComponentPtr& component, CommandScrollAlign align, int duration) {
        ASSERT_FALSE(root->hasEvent());
        executeScrollToComponent(component->getId(), align, duration);
        advanceTime(duration);
    }

    rapidjson::Document doc;
};

static const char *SCROLL_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": 200,
      "height": 300,
      "items": [
        {
          "type": "ScrollView",
          "id": "myScrollView",
          "width": "200",
          "height": "200",
          "items": {
            "type": "Frame",
            "id": "myFrame",
            "width": 200,
            "height": 1000
          }
        },
        {
          "type": "TouchWrapper",
          "id": "myTouch",
          "height": 10,
          "onPress": {
            "type": "Scroll",
            "componentId": "myScrollView",
            "distance": 0.5
          }
        }
      ]
    }
  }
})";

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

    advanceTime(1000);
    ASSERT_TRUE(CheckDirty(frame));  // Scrolling doesn't cause any dirty events - the DOM hasn't changed.
    ASSERT_TRUE(CheckDirty(scroll, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(root, scroll));

    ASSERT_EQ(Point(0, 100), scroll->scrollPosition());
    ASSERT_EQ(Rect(0, -100, 200, 1000), frame->getGlobalBounds());

    ASSERT_TRUE(CheckNoActions());
}

TEST_F(ScrollTest, ScrollForwardMultiple)
{
    loadDocument(SCROLL_TEST);
    rapidjson::Document scrollDoc(rapidjson::kObjectType);
    auto touch = context->findComponentById("myTouch");
    auto scroll = context->findComponentById("myScrollView");
    auto frame = context->findComponentById("myFrame");

    ASSERT_EQ(0, root->serializeDocumentState(scrollDoc.GetAllocator()).Size());
    for (int i = 0 ; i < 20 ; i++) {
        performTap(0, 200);
        auto animationState = root->serializeDocumentState(scrollDoc.GetAllocator());
        ASSERT_EQ(1, animationState.Size());
        ASSERT_TRUE(animationState[0].HasMember("actions"));
        ASSERT_EQ(1, animationState[0]["actions"].Size());
        ASSERT_TRUE(animationState[0]["actions"][0].HasMember("component"));
        ASSERT_STREQ("_main/mainTemplate/items/items/0", animationState[0]["actions"][0]["component"]["provenance"].GetString());
        ASSERT_STREQ("myScrollView", animationState[0]["actions"][0]["component"]["targetId"].GetString());
        ASSERT_STREQ("ScrollView", animationState[0]["actions"][0]["component"]["targetComponentType"].GetString());
        ASSERT_TRUE(animationState[0]["actions"][0].HasMember("actionHint"));
        ASSERT_STREQ("Scrolling", animationState[0]["actions"][0]["actionHint"].GetString());

        advanceTime(1000);

        int target = 100 * (i+1);
        if (target > 800) target = 800;
        ASSERT_EQ(Point(0, target), scroll->scrollPosition()) << "case: " << i;
        ASSERT_EQ(Rect(0, -target, 200, 1000), frame->getGlobalBounds()) << "case: " << i;
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
    advanceTime(1000);
    ASSERT_EQ(Point(0,125), scroll->scrollPosition());
}

static const char *SCROLL_TO_RECT_IN_NO_SCROLLABLE_TEST = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": 200,
      "height": 300,
      "items": [
        {
          "type": "Frame",
          "id": "myFrame",
          "width": 200,
          "height": 1000
        },
        {
          "type": "TouchWrapper",
          "id": "myTouch",
          "height": 10,
          "onPress": {
            "type": "Scroll",
            "componentId": "myScrollView",
            "distance": 0.5
          }
        }
      ]
    }
  }
})";

TEST_F(ScrollTest, ScrollTextWithAlignmentNoScrolling)
{
    loadDocument(SCROLL_TO_RECT_IN_NO_SCROLLABLE_TEST);
    auto touch = context->findComponentById("myTouch");
    auto frame = context->findComponentById("myFrame");
    ASSERT_TRUE(touch);
    ASSERT_TRUE(frame);

    ASSERT_EQ(Rect(0, 0, 200, 1000), frame->getGlobalBounds());
    root->scrollToRectInComponent(frame, Rect(0, 200, 1000, 50), kCommandScrollAlignCenter);
    advanceTime(1000);
}

static const char *SCROLLVIEW_WITH_PADDING = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "ScrollView",
      "id": "myScrollView",
      "paddingTop": 50,
      "paddingBottom": 50,
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "id": "myFrame",
        "width": 100,
        "height": 1000
      }
    }
  }
})";

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

static const char *SCROLLVIEW_SMALL = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "ScrollView",
      "id": "myScrollView",
      "paddingTop": 50,
      "paddingBottom": 50,
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "id": "myFrame",
        "width": 100,
        "height": 50
      }
    }
  }
})";

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

static const char *SCROLLVIEW_NONE = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "ScrollView",
      "id": "myScrollView",
      "paddingTop": 50,
      "paddingBottom": 50,
      "width": 200,
      "height": 300
    }
  }
})";

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


static const char *SEQUENCE_TEST_HORIZONTAL = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "id": "foo",
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";

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

TEST_F(ScrollTest, SequenceRTL)
{
    loadDocument(SEQUENCE_TEST_HORIZONTAL);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);
    ASSERT_EQ(Point(-200,0), component->scrollPosition());

    completeScroll(component, 5);  // This maxes out
    ASSERT_EQ(Point(-800,0), component->scrollPosition());

    completeScroll(component, 5);
    ASSERT_EQ(Point(-800,0), component->scrollPosition());

    completeScroll(component, -0.5f);
    ASSERT_EQ(Point(-700,0), component->scrollPosition());

    completeScroll(component, -20);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    // animation logic
    executeScroll(component->getId(), 1);
    advanceTime(500);
    ASSERT_EQ(Point(-100,0), component->scrollPosition());
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
      "data": "${Array.range(1,13)}"
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

TEST_F(ScrollTest, GridSequenceRTL)
{
    loadDocument(GRID_SEQUENCE_TEST_HORIZONTAL);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);
    ASSERT_EQ(Point(-200,0), component->scrollPosition());

    completeScroll(component, 2);  // This maxes out
    ASSERT_EQ(Point(-400,0), component->scrollPosition());

    completeScroll(component, 5);
    ASSERT_EQ(Point(-400,0), component->scrollPosition());

    completeScroll(component, -0.5f);
    ASSERT_EQ(Point(-300,0), component->scrollPosition());

    completeScroll(component, -20);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

static const char *SEQUENCE_TEST_HORIZONTAL_SMALL = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "id": "foo",
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "width": 50,
        "height": 100
      },
      "data": [1, 2, 3]
    }
  }
})";

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

TEST_F(ScrollTest, SequenceSmallRTL)
{
    loadDocument(SEQUENCE_TEST_HORIZONTAL_SMALL);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);  // Can't scroll forward
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, -20);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}


static const char *SEQUENCE_TEST_HORIZONTAL_PADDING_SPACING = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "paddingLeft": 50,
      "paddingRight": 50,
      "id": "foo",
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "spacing": 10,
        "width": 100,
        "height": 100
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";


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

static const char *SEQUENCE_TEST_VERTICAL = R"(
{
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "vertical",
      "id": "foo",
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";

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
      "data": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
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

static const char *SEQUENCE_TEST_VERTICAL_PADDING_SPACING = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "vertical",
      "paddingTop": 50,
      "paddingBottom": 50,
      "id": "foo",
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "spacing": 10,
        "width": 100,
        "height": 100
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";

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

static const char *SEQUENCE_TEST_VERTICAL_PADDING_SPACING_SMALL = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "vertical",
      "paddingTop": 50,
      "paddingBottom": 50,
      "id": "foo",
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "spacing": 10,
        "width": 100,
        "height": 10
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";

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

static const char *SEQUENCE_DIFFERENT_UNITS = R"(
{
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "vertical",
      "id": "foo",
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 200
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";

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

static const char *SEQUENCE_EMPTY = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "id": "foo",
      "width": 200,
      "height": 300,
      "items": []
    }
  }
})";


TEST_F(ScrollTest, SequenceEmpty)
{
    loadDocument(SEQUENCE_EMPTY);

    completeScroll(component, -1);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1);  // Can't scroll forwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

static const char *SEQUENCE_WITH_INDEX = R"(
{
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "vertical",
      "id": "foo",
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";

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


static const char *SEQUENCE_WITH_INDEX_AND_PADDING = R"(
{
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "vertical",
      "id": "foo",
      "width": 200,
      "height": 300,
      "paddingTop": 50,
      "paddingBottom": 50,
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100,
        "spacing": 10
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";

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


static const char *HORIZONTAL_SEQUENCE_WITH_INDEX_AND_PADDING = R"(
{
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "id": "foo",
      "width": 400,
      "height": 300,
      "paddingLeft": 50,
      "paddingRight": 50,
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100,
        "spacing": 10
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";

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

static const char *HORIZONTAL_SEQUENCE_WITH_INDEX_RTL = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "layoutDirection": "RTL",
      "id": "foo",
      "width": 400,
      "height": 300,
      "paddingLeft": 50,
      "paddingRight": 50,
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";

TEST_F(ScrollTest, ScrollToIndexHorizontalRTL)
{
    loadDocument(HORIZONTAL_SEQUENCE_WITH_INDEX_RTL);

    // The second item is already in view
    scrollToIndex(component, 1, kCommandScrollAlignVisible);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    // Force it to the left
    scrollToIndex(component, 1, kCommandScrollAlignFirst);
    ASSERT_EQ(Point(-100,0), component->scrollPosition());

    // Center (center of child=550, center of view=150)
    scrollToIndex(component, 5, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(-400, 0), component->scrollPosition());

    // Make a previous item aligned right
    scrollToIndex(component, 5, kCommandScrollAlignLast);
    ASSERT_EQ(Point(-300, 0), component->scrollPosition());

    // Crash into the end
    scrollToIndex(component, 9, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(-700, 0), component->scrollPosition());

    // Back to the start
    scrollToIndex(component, 0, kCommandScrollAlignLast);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignCenter);
    ASSERT_EQ(Point(-400,0), component->scrollPosition());
}


static const char *MISSING_INDEX = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "id": "foo",
      "width": 400,
      "height": 300,
      "paddingLeft": 50,
      "paddingRight": 50,
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100,
        "spacing": 10
      },
      "data": "${Array.range(1,5)}"
    }
  }
})";

TEST_F(ScrollTest, ScrollToMissingIndex)
{
    loadDocument(MISSING_INDEX);

    // First, execute a valid scroll
    executeScrollToIndex("foo", 1, kCommandScrollAlignFirst);
    advanceTime(1000);

    // Now specify an invalid component
    ASSERT_FALSE(ConsoleMessage());
    executeScrollToIndex("foobar", 1, kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_TRUE(ConsoleMessage());

    // Try an invalid index
    executeScrollToIndex("foo", 15, kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_TRUE(ConsoleMessage());

    // Valid negative index scroll
    executeScrollToIndex("foo", -1, kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_FALSE(ConsoleMessage());

    // Try an invalid negative index
    executeScrollToIndex("foo", -15, kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_TRUE(ConsoleMessage());
}


static const char *VERTICAL_SCROLLVIEW = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "ScrollView",
      "paddingTop": 50,
      "paddingBottom": 50,
      "width": 200,
      "height": 300,
      "items": {
        "type": "Container",
        "direction": "vertical",
        "items": [
          {
            "type": "Frame",
            "id": "frame1",
            "width": 100,
            "height": 200
          },
          {
            "type": "Frame",
            "id": "frame2",
            "width": 100,
            "height": 300
          },
          {
            "type": "Frame",
            "id": "frame3",
            "width": 100,
            "height": 100
          },
          {
            "type": "Frame",
            "id": "frame4",
            "width": 100,
            "height": 400
          },
          {
            "type": "Frame",
            "id": "frame5",
            "width": 100,
            "height": 100
          },
          {
            "type": "Frame",
            "id": "frame6",
            "width": 100,
            "height": 300
          }
        ]
      }
    }
  }
})";

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
    advanceTime(1000);

    // Now try an invalid component
    ASSERT_FALSE(ConsoleMessage());
    executeScrollToComponent("frame26", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(ScrollTest, ScrollWithTermination)
{
    loadDocument(VERTICAL_SCROLLVIEW);

    // Start a valid scroll command
    executeScrollToComponent("frame2", kCommandScrollAlignFirst);
    advanceTime(500);
    auto currentPosition = component->scrollPosition();

    root->cancelExecution();
    advanceTime(500);
    ASSERT_EQ(currentPosition, component->scrollPosition());
}

static const char *VERTICAL_DEEP_SEQUENCE = R"(
{
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "id": "seq",
      "width": 600,
      "height": 600,
      "data": "${Array.range(0, 20)}",
      "scrollDirection": "vertical",
      "items": {
        "type": "TouchWrapper",
        "id": "item${data}",
        "item": {
          "type": "Container",
          "id": "container${data}",
          "items": [
            {
              "type": "Text",
              "id": "text${data}",
              "width": 150,
              "height": 150,
              "text": "${data}"
            }
          ]
        }
      }
    }
  }
})";

TEST_F(ScrollTest, SequenceToVerticalComponent)
{
    loadDocument(VERTICAL_DEEP_SEQUENCE);

    // Scroll to ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("item1", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(0, 150), component->scrollPosition());

    // Scroll to non-ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("item10", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(0, 1500), component->scrollPosition());

    // Scroll to non-ensured one by index (we don't forward-ensure)
    ASSERT_FALSE(root->hasEvent());
    executeScrollToIndex("seq", 12, kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(0, 1800), component->scrollPosition());
}

TEST_F(ScrollTest, SequenceToVerticalSubComponent)
{
    loadDocument(VERTICAL_DEEP_SEQUENCE);

    // Scroll to ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("text1", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(0, 150), component->scrollPosition());

    // Scroll to non-ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("text10", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(0, 1500), component->scrollPosition());
    session->checkAndClear();
}

static const char *HORIZONTAL_DEEP_SEQUENCE = R"(
{
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "id": "seq",
      "width": 600,
      "height": 500,
      "data": "${Array.range(0, 20)}",
      "scrollDirection": "horizontal",
      "items": {
        "type": "Container",
        "id": "item${index}",
        "items": [
          {
            "type": "Text",
            "id": "text${data}",
            "width": 150,
            "height": 150,
            "text": "${data}"
          }
        ]
      }
    }
  }
})";

TEST_F(ScrollTest, SequenceToHorizontalComponent)
{
    loadDocument(HORIZONTAL_DEEP_SEQUENCE);

    // Scroll to ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("item1", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(150, 0), component->scrollPosition());

    // Scroll to non-ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("item10", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(1500, 0), component->scrollPosition());

    // Scroll to non-ensured one by index (we don't forward-ensure)
    ASSERT_FALSE(root->hasEvent());
    executeScrollToIndex("seq", 12, kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(1800, 0), component->scrollPosition());
}

TEST_F(ScrollTest, SequenceToHorizontalSubComponent)
{
    loadDocument(HORIZONTAL_DEEP_SEQUENCE);

    // Scroll to ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("text1", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(150, 0), component->scrollPosition());

    // Scroll to non-ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("text10", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(1500, 0), component->scrollPosition());
    session->checkAndClear();
}

TEST_F(ScrollTest, SequenceToHorizontalComponentRTL)
{
    loadDocument(HORIZONTAL_DEEP_SEQUENCE);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    // Scroll to ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("item1", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(-150, 0), component->scrollPosition());

    // Scroll to non-ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("item10", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(-1500, 0), component->scrollPosition());

    // Scroll to non-ensured one by index (we don't forward-ensure)
    ASSERT_FALSE(root->hasEvent());
    executeScrollToIndex("seq", 12, kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(-1800, 0), component->scrollPosition());
}

TEST_F(ScrollTest, SequenceToHorizontalSubComponentRTL)
{
    loadDocument(HORIZONTAL_DEEP_SEQUENCE);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    // Scroll to ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("text1", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(-150, 0), component->scrollPosition());

    // Scroll to non-ensured one
    ASSERT_FALSE(root->hasEvent());
    executeScrollToComponent("text10", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(Point(-1500, 0), component->scrollPosition());
    session->checkAndClear();
}

static const char *PAGER_TEST = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "id": "myPager",
      "width": 100,
      "height": 100,
      "items": {
        "type": "Text",
        "id": "id${data}",
        "text": "TEXT${data}",
        "speech": "URL${data}"
      },
      "data": "${Array.range(1, 5)}",
      "onPageChanged": {
        "type": "SendEvent",
        "sequencer": "SET_PAGE",
        "arguments": [
          "${event.target.page}"
        ]
      }
    }
  }
})";

TEST_F(ScrollTest, Pager)
{
    loadDocument(PAGER_TEST);

    executeScrollToComponent("id2", kCommandScrollAlignFirst);
    advanceTime(1000);
    ASSERT_EQ(1, component->pagePosition());

    root->popEvent();
}


static const char *TEST_BASIC_TOP_BOTTOM_OFFSET_STICKY = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": [
      {
        "type": "Frame",
        "height": 600,
        "width": 500,
        "padding": 40,
        "backgroundColor": "black",
        "items": [
          {
            "id": "scrollone",
            "type": "ScrollView",
            "width": 400,
            "height": 500,
            "item": {
              "type": "Container",
              "height": 1000,
              "width": 400,
              "items": [
                {
                  "type": "Frame",
                  "height": 400,
                  "width": 200,
                  "backgroundColor": "white",
                  "items": []
                },
                {
                  "type": "Frame",
                  "height": 300,
                  "width": 400,
                  "backgroundColor": "#1a73e8",
                  "items": [
                    {
                      "type": "Container",
                      "height": 300,
                      "width": 400,
                      "items": [
                        {
                          "id": "topsticky",
                          "position": "sticky",
                          "top": 0,
                          "type": "Frame",
                          "height": 100,
                          "width": 300,
                          "backgroundColor": "#dc3912",
                          "items": []
                        },
                        {
                          "type": "Frame",
                          "height": 100,
                          "width": 200,
                          "backgroundColor": "#4caf50",
                          "items": []
                        },
                        {
                          "id": "bottomsticky",
                          "position": "sticky",
                          "bottom": 0,
                          "type": "Frame",
                          "height": 100,
                          "width": 150,
                          "backgroundColor": "blue",
                          "items": []
                        }
                      ]
                    }
                  ]
                }
              ]
            }
          }
        ]
      }
    ]
  }
})apl";

TEST_F(ScrollTest, BasicStickyTestTopOffset)
{
    loadDocument(TEST_BASIC_TOP_BOTTOM_OFFSET_STICKY);
    auto scroll = context->findComponentById("scrollone");
    auto stickyComp = context->findComponentById("topsticky");
    ASSERT_TRUE(scroll);
    ASSERT_TRUE(stickyComp);

    EXPECT_TRUE(expectBounds(stickyComp, 0, 0, 100, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 0), true));
    advanceTime(200);

    // Check the sticky component hasn't been moved
    EXPECT_TRUE(expectBounds(stickyComp, 0, 0, 100, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -350), true));
    advanceTime(200);

    // Check the sticky component has updated
    EXPECT_TRUE(expectBounds(stickyComp, 50, 0, 150, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -400), true));
    advanceTime(200);

    // Check the sticky component has updated
    EXPECT_TRUE(expectBounds(stickyComp, 100, 0, 200, 300));
}

TEST_F(ScrollTest, BasicStickyTestBottomOffset)
{
    loadDocument(TEST_BASIC_TOP_BOTTOM_OFFSET_STICKY);
    auto scroll = context->findComponentById("scrollone");
    auto stickyComp = context->findComponentById("bottomsticky");
    ASSERT_TRUE(scroll);
    ASSERT_TRUE(stickyComp);

    EXPECT_TRUE(expectBounds(stickyComp,   0,  0, 100, 150));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 0), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 100, 0, 200, 150));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -350), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 200, 0, 300, 150));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -400), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 200, 0, 300, 150));
}

static const char *TEST_SKIP_BOTTOM_OFFSET_STICKY = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": [
      {
        "type": "Frame",
        "height": 600,
        "width": 500,
        "padding": 40,
        "backgroundColor": "black",
        "items": [
          {
            "id": "scrollone",
            "type": "ScrollView",
            "width": 400,
            "height": 500,
            "item": {
              "type": "Container",
              "height": 1000,
              "width": 400,
              "items": [
                {
                  "type": "Frame",
                  "height": 400,
                  "width": 200,
                  "backgroundColor": "white",
                  "items": []
                },
                {
                  "type": "Frame",
                  "height": 800,
                  "width": 400,
                  "backgroundColor": "#1a73e8",
                  "items": [
                    {
                      "type": "Container",
                      "height": 800,
                      "width": 400,
                      "items": [
                        {
                          "type": "Frame",
                          "height": 100,
                          "width": 300,
                          "backgroundColor": "#dc3912",
                          "items": []
                        },
                        {
                          "type": "Frame",
                          "height": 100,
                          "width": 200,
                          "backgroundColor": "#4caf50",
                          "items": []
                        },
                        {
                          "id": "bottomsticky",
                          "position": "sticky",
                          "top": 300,
                          "bottom": 300,
                          "type": "Frame",
                          "height": 100,
                          "width": 150,
                          "backgroundColor": "blue",
                          "items": []
                        }
                      ]
                    }
                  ]
                }
              ]
            }
          }
        ]
      }
    ]
  }
})apl";

/**
 * Make sure we skip the bottom offset when top + bottom offset is bigger than the scrollable height
 */
TEST_F(ScrollTest, BasicStickyTestSkipBottomOffset)
{
    loadDocument(TEST_SKIP_BOTTOM_OFFSET_STICKY);
    auto scroll = context->findComponentById("scrollone");
    auto stickyComp = context->findComponentById("bottomsticky");
    ASSERT_TRUE(scroll);
    ASSERT_TRUE(stickyComp);

    EXPECT_TRUE(expectBounds(stickyComp,   200,  0, 300, 150));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 0), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 200, 0, 300, 150));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -350), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 350, 0, 450, 150));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -800), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 400, 0, 500, 150));
}


static const char *TEST_TOP_NESTED_STICKY = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": [
      {
        "type": "Frame",
        "height": 600,
        "width": 500,
        "padding": 40,
        "backgroundColor": "black",
        "items": [
          {
            "id": "scrollone",
            "type": "ScrollView",
            "width": 400,
            "height": 500,
            "item": {
              "type": "Container",
              "height": 1000,
              "width": 400,
              "items": [
                {
                  "type": "Frame",
                  "height": 300,
                  "width": 400,
                  "backgroundColor": "#1a73e8",
                  "items": [
                    {
                      "type": "Container",
                      "height": 300,
                      "width": 400,
                      "items": [
                        {
                          "position": "sticky",
                          "top": 0,
                          "bottom": 10,
                          "type": "Frame",
                          "height": 100,
                          "width": 300,
                          "backgroundColor": "#dc3912",
                          "items": []
                        },
                        {
                          "position": "sticky",
                          "top": 10,
                          "type": "Frame",
                          "height": 100,
                          "width": 200,
                          "backgroundColor": "#4caf50",
                          "items": []
                        },
                        {
                          "type": "Frame",
                          "height": 100,
                          "width": 150,
                          "backgroundColor": "blue",
                          "items": []
                        }
                      ]
                    }
                  ]
                },
                {
                  "type": "Frame",
                  "height": 100,
                  "width": 400,
                  "backgroundColor": "white"
                },
                {
                  "type": "Frame",
                  "height": 1000,
                  "width": 400,
                  "backgroundColor": "#1a73e8",
                  "items": [
                    {
                      "type": "Container",
                      "height": 1000,
                      "width": 400,
                      "items": [
                        {
                          "type": "Frame",
                          "height": 100,
                          "width": 400,
                          "backgroundColor": "#dc3912",
                          "items": []
                        },
                        {
                          "position": "sticky",
                          "id": "outerSticky",
                          "top": 100,
                          "type": "Frame",
                          "height": 300,
                          "width": 400,
                          "backgroundColor": "#de7700",
                          "items": [
                            {
                              "type": "Container",
                              "height": 300,
                              "width": 300,
                              "items": [
                                {
                                  "type": "Frame",
                                  "id": "innnerSticky",
                                  "position": "sticky",
                                  "top": 120,
                                  "height": 100,
                                  "width": 300,
                                  "backgroundColor": "white",
                                  "items": []
                                }
                              ]
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
              ]
            }
          }
        ]
      }
    ]
  }
})apl";

TEST_F(ScrollTest, NestedStickyComponents)
{
    loadDocument(TEST_TOP_NESTED_STICKY);
    auto scroll = context->findComponentById("scrollone");
    auto stickyComp = context->findComponentById("outerSticky");
    auto stickyCompInner = context->findComponentById("innnerSticky");
    ASSERT_TRUE(scroll);
    ASSERT_TRUE(stickyComp);

    EXPECT_TRUE(expectBounds(stickyComp, 100, 0, 400, 400));
    EXPECT_TRUE(expectBounds(stickyCompInner, 0, 0, 100, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 0), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 100, 0, 400, 400));
    EXPECT_TRUE(expectBounds(stickyCompInner, 0, 0, 100, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -350), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 150, 0, 450, 400));
    EXPECT_TRUE(expectBounds(stickyCompInner, 20, 0, 120, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -400), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 200, 0, 500, 400));
    EXPECT_TRUE(expectBounds(stickyCompInner, 20, 0, 120, 300));
}

static const char *DEEP_NESTED_COMPONENTS = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": [
      {
        "type": "Frame",
        "height": 600,
        "width": 500,
        "padding": 40,
        "backgroundColor": "black",
        "items": [
          {
            "id": "scrollone",
            "type": "ScrollView",
            "width": 400,
            "height": 500,
            "item": {
              "type": "Container",
              "height": 1000,
              "width": 400,
              "items": [
                {
                  "type": "Frame",
                  "height": 100,
                  "width": 400,
                  "backgroundColor": "white"
                },
                {
                  "type": "Frame",
                  "height": 1000,
                  "width": 400,
                  "backgroundColor": "#1a73e8",
                  "items": [
                    {
                      "type": "Container",
                      "height": 1000,
                      "width": 400,
                      "items": [
                        {
                          "type": "Frame",
                          "height": 100,
                          "width": 400,
                          "backgroundColor": "#dc3912",
                          "items": []
                        },
                        {
                          "position": "sticky",
                          "id": "outerSticky",
                          "top": 10,
                          "type": "Frame",
                          "height": 300,
                          "width": 400,
                          "backgroundColor": "#de7700",
                          "items": [
                            {
                              "type": "Container",
                              "height": 300,
                              "width": 300,
                              "items": [
                                {
                                  "type": "Frame",
                                  "id": "innerSticky1",
                                  "position": "sticky",
                                  "top": 20,
                                  "height": 100,
                                  "width": 300,
                                  "backgroundColor": "red",
                                  "item": {
                                    "type": "Container",
                                    "height": 300,
                                    "width": 300,
                                    "items": [
                                      {
                                        "type": "Frame",
                                        "id": "innerSticky2",
                                        "position": "sticky",
                                        "top": 30,
                                        "height": 90,
                                        "width": 290,
                                        "backgroundColor": "green",
                                        "item": {
                                          "type": "Container",
                                          "height": 300,
                                          "width": 300,
                                          "items": [
                                            {
                                              "type": "Frame",
                                              "id": "innerSticky3",
                                              "position": "sticky",
                                              "top": 40,
                                              "height": 80,
                                              "width": 280,
                                              "backgroundColor": "blue",
                                              "item": {
                                                "type": "Container",
                                                "height": 300,
                                                "width": 300,
                                                "items": [
                                                  {
                                                    "type": "Frame",
                                                    "id": "innerSticky4",
                                                    "position": "sticky",
                                                    "top": 50,
                                                    "height": 70,
                                                    "width": 270,
                                                    "backgroundColor": "pink"
                                                  }
                                                ]
                                              }
                                            }
                                          ]
                                        }
                                      }
                                    ]
                                  }
                                }
                              ]
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
              ]
            }
          }
        ]
      }
    ]
  }
})apl";

TEST_F(ScrollTest, DeepNestedStickyComponents)
{
    loadDocument(DEEP_NESTED_COMPONENTS);
    auto scroll = context->findComponentById("scrollone");
    auto stickyComp = context->findComponentById("outerSticky");
    auto stickyCompInner1 = context->findComponentById("innerSticky1");
    auto stickyCompInner2 = context->findComponentById("innerSticky2");
    auto stickyCompInner3 =
        CoreComponent::cast(context->findComponentById("innerSticky3"));
    auto stickyCompInner4 = context->findComponentById("innerSticky4");
    ASSERT_TRUE(scroll);
    ASSERT_TRUE(stickyComp);
    ASSERT_TRUE(stickyCompInner1);
    ASSERT_TRUE(stickyCompInner2);
    ASSERT_TRUE(stickyCompInner3);
    ASSERT_TRUE(stickyCompInner4);

    EXPECT_TRUE(expectBounds(stickyComp, 100, 0, 400, 400));
    EXPECT_TRUE(expectBounds(stickyCompInner1, 0, 0, 100, 300));
    EXPECT_TRUE(expectBounds(stickyCompInner2, 0, 0, 90, 290));
    EXPECT_TRUE(expectBounds(stickyCompInner3, 0, 0, 80, 280));
    EXPECT_TRUE(expectBounds(stickyCompInner4, 0, 0, 70, 270));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    root->updateTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 0), true));
    root->updateTime(400);

    EXPECT_TRUE(expectBounds(stickyComp, 100, 0, 400, 400));
    EXPECT_TRUE(expectBounds(stickyCompInner1, 0, 0, 100, 300));
    EXPECT_TRUE(expectBounds(stickyCompInner2, 0, 0, 90, 290));
    EXPECT_TRUE(expectBounds(stickyCompInner3, 0, 0, 80, 280));
    EXPECT_TRUE(expectBounds(stickyCompInner4, 0, 0, 70, 270));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -100), true));
    root->updateTime(600);

    EXPECT_TRUE(expectBounds(stickyComp, 110, 0, 410, 400));
    EXPECT_TRUE(expectBounds(stickyCompInner1, 10, 0, 110, 300));
    EXPECT_TRUE(expectBounds(stickyCompInner2, 10, 0, 100, 290));
    EXPECT_TRUE(expectBounds(stickyCompInner3, 10, 0, 90, 280));
    EXPECT_TRUE(expectBounds(stickyCompInner4, 10, 0, 80, 270));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -200), true));
    root->updateTime(800);

    EXPECT_TRUE(expectBounds(stickyComp, 210, 0, 510, 400));
    EXPECT_TRUE(expectBounds(stickyCompInner1, 10, 0, 110, 300));
    EXPECT_TRUE(expectBounds(stickyCompInner2, 10, 0, 100, 290));
    EXPECT_TRUE(expectBounds(stickyCompInner3, 10, 0, 90, 280));
    EXPECT_TRUE(expectBounds(stickyCompInner4, 10, 0, 80, 270));

    stickyCompInner3->setProperty(kPropertyPosition, "relative");

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -300), true));
    root->updateTime(900);

    EXPECT_TRUE(expectBounds(stickyComp, 310, 0, 610, 400));
    EXPECT_TRUE(expectBounds(stickyCompInner1, 10, 0, 110, 300));
    EXPECT_TRUE(expectBounds(stickyCompInner2, 10, 0, 100, 290));
    EXPECT_TRUE(expectBounds(stickyCompInner3, 10, 0, 90, 280));
    EXPECT_TRUE(expectBounds(stickyCompInner4, 10, 0, 80, 270));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 100), true));
    root->updateTime(1000);

    EXPECT_TRUE(expectBounds(stickyComp, 100, 0, 400, 400));
    EXPECT_TRUE(expectBounds(stickyCompInner1, 0, 0, 100, 300));
    EXPECT_TRUE(expectBounds(stickyCompInner2, 0, 0, 90, 290));
    EXPECT_TRUE(expectBounds(stickyCompInner3, 10, 0, 90, 280));
    EXPECT_TRUE(expectBounds(stickyCompInner4, 0, 0, 70, 270));
}

static const char *TEST_BASIC_LEFT_RIGHT_OFFSET_STICKY = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": [
      {
        "type": "Frame",
        "height": 600,
        "width": 500,
        "padding": 40,
        "backgroundColor": "black",
        "items": [
          {
            "id": "scrollone",
            "type": "Sequence",
            "scrollDirection": "horizontal",
            "width": 400,
            "height": 400,
            "item": {
              "type": "Container",
              "height": 400,
              "width": 1000,
              "direction": "row",
              "items": [
                {
                  "type": "Frame",
                  "height": 300,
                  "width": 300,
                  "backgroundColor": "white",
                  "items": []
                },
                {
                  "type": "Frame",
                  "height": 300,
                  "width": 400,
                  "backgroundColor": "#1a73e8",
                  "items": [
                    {
                      "type": "Container",
                      "height": 300,
                      "width": 400,
                      "direction": "row",
                      "items": [
                        {
                          "id": "leftsticky",
                          "position": "sticky",
                          "left": 0,
                          "type": "Frame",
                          "height": 300,
                          "width": 100,
                          "backgroundColor": "#dc3912",
                          "items": []
                        },
                        {
                          "type": "Frame",
                          "height": 200,
                          "width": 100,
                          "backgroundColor": "#4caf50",
                          "items": []
                        },
                        {
                          "id": "rightsticky",
                          "position": "sticky",
                          "right": 0,
                          "type": "Frame",
                          "height": 150,
                          "width": 100,
                          "backgroundColor": "blue",
                          "items": []
                        }
                      ]
                    }
                  ]
                }
              ]
            }
          }
        ]
      }
    ]
  }
})apl";

TEST_F(ScrollTest, BasicStickyTestLeftOffset)
{
    loadDocument(TEST_BASIC_LEFT_RIGHT_OFFSET_STICKY);
    auto scroll = context->findComponentById("scrollone");
    auto stickyComp = context->findComponentById("leftsticky");
    ASSERT_TRUE(scroll);
    ASSERT_TRUE(stickyComp);

    EXPECT_TRUE(expectBounds(stickyComp, 0, 0, 300, 100));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0, 100), true));
    advanceTime(200);

    // Check the sticky component hasn't been moved
    EXPECT_TRUE(expectBounds(stickyComp, 0, 0, 300, 100));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-350, 100), true));
    advanceTime(200);

    // Check the sticky component has updated
    EXPECT_TRUE(expectBounds(stickyComp, 0, 150, 300, 250));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-400, 100), true));
    advanceTime(200);

    // Check the sticky component has updated
    EXPECT_TRUE(expectBounds(stickyComp, 0, 200, 300, 300));
}

TEST_F(ScrollTest, BasicStickyTestRightOffset)
{
    loadDocument(TEST_BASIC_LEFT_RIGHT_OFFSET_STICKY);
    auto scroll = context->findComponentById("scrollone");
    auto stickyComp = context->findComponentById("rightsticky");
    ASSERT_TRUE(scroll);
    ASSERT_TRUE(stickyComp);

    EXPECT_TRUE(expectBounds(stickyComp,   0,  0, 150, 100));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0, 100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 0, 100, 150, 200));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-350, 100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 0, 200, 150, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-400, 100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 0, 200, 150, 300));
}

/**
 * Make sure we skip the right offset
 */
TEST_F(ScrollTest, StickyTestSkipRightOffset)
{
    loadDocument(TEST_BASIC_LEFT_RIGHT_OFFSET_STICKY);
    auto scroll = context->findComponentById("scrollone");
    auto stickyComp = CoreComponent::cast(context->findComponentById("rightsticky"));
    ASSERT_TRUE(scroll);
    ASSERT_TRUE(stickyComp);

    stickyComp->setProperty(kPropertyLeft, "300");
    stickyComp->setProperty(kPropertyRight, "300");
    root->clearPending(); // Forces the layout

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0, 100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 0, 200, 150, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-350, 100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 0, 300, 150, 400));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-400, 100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyComp, 0, 300, 150, 400));
}

static const char *TEST_LEFT_NESTED_STICKY = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": [{
      "type": "Frame",
      "height": 600,
      "width": 500,
      "padding": 40,
      "backgroundColor": "black",
      "items": [{
        "id": "scrollone",
        "type": "Sequence",
        "scrollDirection": "horizontal",
        "width": 400,
        "height": 500,
        "item": {
          "type": "Container",
          "height": 4000,
          "width": 1000,
          "direction": "row",
          "items": [{
            "type": "Frame",
            "backgroundColor": "white",
            "height": 300,
            "width": 100
          },
            {
              "type": "Frame",
              "height": 300,
              "width": 300,
              "backgroundColor": "#1a73e8",
              "items": [{
                "type": "Container",
                "height": 300,
                "width": 400,
                "items": []
              }]
            },
            {
              "type": "Frame",
              "height": 400,
              "width": 1000,
              "backgroundColor": "#1a73e8",
              "items": [{
                "type": "Container",
                "height": 400,
                "width": 1000,
                "direction": "row",
                "items": [{
                  "type": "Frame",
                  "height": 400,
                  "width": 100,
                  "backgroundColor": "#dc3912",
                  "items": []
                },
                  {
                    "position": "sticky",
                    "id": "outerSticky",
                    "left": 100,
                    "type": "Frame",
                    "height": 300,
                    "width": 400,
                    "backgroundColor": "#de7700",
                    "items": [{
                      "type": "Container",
                      "height": 300,
                      "width": 300,
                      "items": [{
                        "type": "Frame",
                        "id": "innerSticky",
                        "position": "sticky",
                        "left": 120,
                        "height": 300,
                        "width": 100,
                        "backgroundColor": "white",
                        "item":

                        {
                          "id": "leafContainer",
                          "type": "Container",
                          "height": 140,
                          "width": 140,
                          "items": [{
                            "type": "Frame",
                            "id": "leafSticky",
                            "position": "sticky",
                            "left": 130,
                            "height": 300,
                            "width": 100,
                            "backgroundColor": "green",
                            "items": []
                          }]
                        }
                      }]
                    }]
                  }
                ]
              }]
            }
          ]
        }
      }]
    }]
  }
})apl";

TEST_F(ScrollTest, NestedStickyComponentsLeft)
{
    loadDocument(TEST_LEFT_NESTED_STICKY);
    auto scroll = context->findComponentById("scrollone");
    auto outerSticky = context->findComponentById("outerSticky");
    auto innerSticky = context->findComponentById("innerSticky");
    ASSERT_TRUE(scroll);
    ASSERT_TRUE(outerSticky);

    EXPECT_TRUE(expectBounds(outerSticky, 0, 100, 300, 500));
    EXPECT_TRUE(expectBounds(innerSticky, 0, 0, 300, 100));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(0, 100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(outerSticky, 0, 100, 300, 500));
    EXPECT_TRUE(expectBounds(innerSticky, 0, 0, 300, 100));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-350, 100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(outerSticky, 0, 150, 300, 550));
    EXPECT_TRUE(expectBounds(innerSticky, 0, 20, 300, 120));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(outerSticky, 0, 100, 300, 500));
    EXPECT_TRUE(expectBounds(innerSticky, 0, 0, 300, 100));
}

/**
 * Test adding a sticky element deep in the component tree. During document inflation each inserted
 * child only has one parent. This code tests adding a child with many sticky parents to test the
 * StickyChildrenTree code.
 */
TEST_F(ScrollTest, DeepNestedStickyComponentsAddRemove)
{
    loadDocument(TEST_LEFT_NESTED_STICKY);
    auto leafContainer = context->findComponentById("leafContainer");
    auto leafSticky    = context->findComponentById("leafSticky");

    // Remove the leaf sticky component and make sure it isn't in the sticky tree by //FIXME:verifiying it
    // doesn't react to scrolling
    leafSticky->remove();

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-500, 100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(leafSticky, 0, 0, 300, 100));

    // Insert the leaf sticky component and make sure it is correctly added the sticky tree by
    // verifiying its sticky offsets are correct.
    leafContainer->insertChild(leafSticky, 0);
    root->clearPending();

    EXPECT_TRUE(expectBounds(leafSticky, 0, 10, 300, 110));
}

static const char *TEST_BASIC_TOP_BOTTOM_OFFSET_STICKY_WITHOUT_STICKIES = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": [
      {
        "type": "Frame",
        "height": 600,
        "width": 500,
        "padding": 40,
        "backgroundColor": "black",
        "items": [
          {
            "type": "Sequence",
            "width": 400,
            "height": 500,
            "scrollDirection": "horizontal",
            "items" : [
              {
                "id": "scrollone",
                "type": "ScrollView",
                "width": 400,
                "height": 500,
                "item" : {
                  "type": "Container",
                  "height": 1000,
                  "width": 400,
                  "items": [
                    {
                      "type": "Frame",
                      "height": 400,
                      "width": 200,
                      "backgroundColor": "white",
                      "items": []
                    },
                    {
                      "type": "Frame",
                      "height": 300,
                      "width": 400,
                      "backgroundColor": "#1a73e8",
                      "items": [
                        {
                          "type": "Container",
                          "id": "stickyContainer",
                          "height": 300,
                          "width": 400,
                          "items": [
                            {
                              "type": "Frame",
                              "height": 100,
                              "width": 200,
                              "backgroundColor": "#4caf50",
                              "items": []
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
              }
            ]
          }
        ]
      }
    ]
  }
})apl";

const static char * STICKY_CHILD_TOP = R"apl({
  "id": "topsticky",
  "position": "sticky",
  "top": 0,
  "type": "Frame",
  "height": 100,
  "width": 300,
  "backgroundColor": "#dc3912",
  "items": []
})apl";

const static char * STICKY_CHILD_BOTTOM = R"apl({
  "id": "bottomsticky",
  "position": "sticky",
  "bottom": 0,
  "type": "Frame",
  "height": 100,
  "width": 150,
  "backgroundColor": "blue",
  "items": []
})apl";

/**
 * Check if an inserted child registers it's scroll callback correctly
 */
TEST_F(ScrollTest, InsertStickyChildTest)
{
    loadDocument(TEST_BASIC_TOP_BOTTOM_OFFSET_STICKY_WITHOUT_STICKIES);

    JsonData dataTop(STICKY_CHILD_TOP);
    auto childTop = context->inflate(dataTop.get());
    ASSERT_TRUE(childTop);

    auto scroll = context->findComponentById("scrollone");
    auto stickyCont = context->findComponentById("stickyContainer");
    stickyCont->insertChild(childTop, 0);
    ASSERT_TRUE(component->needsLayout());
    root->clearPending();  // Forces the layout

    auto stickyTop = context->findComponentById("topsticky");
    assert(stickyTop);

    EXPECT_TRUE(expectBounds(stickyTop, 0, 0, 100, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -350), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyTop, 50, 0, 150, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 0), true));
    advanceTime(200);

    //Check it also works with a second child
    JsonData dataBottom(STICKY_CHILD_BOTTOM);
    auto childBottom = context->inflate(dataBottom.get());
    ASSERT_TRUE(childBottom);

    stickyCont->insertChild(childBottom, 2);
    ASSERT_TRUE(component->needsLayout());
    root->clearPending();  // Forces the layout

    auto stickyBottom = context->findComponentById("bottomsticky");
    assert(stickyBottom);

    EXPECT_TRUE(expectBounds(stickyBottom, 200, 0, 300, 150));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -50), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyBottom, 150, 0, 250, 150));
}

const static char * NON_STICKY_CHILD_TOP = R"apl({
  "type": "Frame",
  "height": 100,
  "width": 300,
  "backgroundColor": "#dc3912",
  "items": [
    {
      "id": "topsticky",
      "position": "sticky",
      "top": 0,
      "type": "Frame",
      "height": 100,
      "width": 300,
      "backgroundColor": "black",
      "items": []
    }
  ]
})apl";

/**
 * Check inserting child which isn't sticky but contains a sticky child
 */
TEST_F(ScrollTest, InsertStickyChildComplexTest)
{
    loadDocument(TEST_BASIC_TOP_BOTTOM_OFFSET_STICKY_WITHOUT_STICKIES);

    JsonData dataTop(NON_STICKY_CHILD_TOP);
    auto childTop = context->inflate(dataTop.get());
    ASSERT_TRUE(childTop);

    auto scroll = context->findComponentById("scrollone");
    auto stickyCont = context->findComponentById("stickyContainer");
    stickyCont->insertChild(childTop, 0);
    ASSERT_TRUE(component->needsLayout());
    root->clearPending();  // Forces the layout

    auto stickyTop = context->findComponentById("topsticky");
    assert(stickyTop);

    EXPECT_TRUE(expectBounds(stickyTop, 0, 0, 100, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -350), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyTop, 0, 0, 100, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 100), true));
    advanceTime(200);
}

const static char * SCROLLABLE_WITH_STICKY = R"apl({
  "id": "scrollableWithStickyChild",
  "type": "ScrollView",
  "height": 300,
  "width": 300,
  "backgroundColor": "#dc3912",
  "items": [
    {
      "type": "Container",
      "height": 1000,
      "width": 300,
      "backgroundColor": "black",
      "items": [
        {
          "id": "topsticky",
          "position": "sticky",
          "top": 0,
          "type": "Frame",
          "height": 100,
          "width": 300,
          "backgroundColor": "black",
          "items": []
        }
      ]
    }
  ]
})apl";

/**
 * Check inserting child which is scrollable and contains a sticky child
 */
TEST_F(ScrollTest, InsertScrollableWithStickyChildTest)
{
    loadDocument(TEST_BASIC_TOP_BOTTOM_OFFSET_STICKY_WITHOUT_STICKIES);

    JsonData dataTop(SCROLLABLE_WITH_STICKY);
    auto childTop = context->inflate(dataTop.get());
    ASSERT_TRUE(childTop);

    auto scroll = context->findComponentById("scrollone");
    auto stickyCont = context->findComponentById("stickyContainer");
    stickyCont->insertChild(childTop, 0);
    ASSERT_TRUE(component->needsLayout());
    root->clearPending();  // Forces the layout

    auto stickyTop = context->findComponentById("topsticky");
    assert(stickyTop);

    EXPECT_TRUE(expectBounds(stickyTop, 0, 0, 100, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -350), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyTop, 0, 0, 100, 300));
}

const static char * NON_STICKY_CHILD_TOP_WITH_OFFSET = R"apl({
  "id": "topsticky",
  "top": 100,
  "type": "Frame",
  "height": 100,
  "width": 300,
  "backgroundColor": "#dc3912",
  "items": []
})apl";

const static char * NON_STICKY_CHILD_BOTTOM_WITHOUT_OFFSET = R"apl({
  "id": "bottomsticky",
  "type": "Frame",
  "height": 100,
  "width": 150,
  "backgroundColor": "blue",
  "items": []
})apl";

TEST_F(ScrollTest, SetUnsetStickyChildTest)
{
    loadDocument(TEST_BASIC_TOP_BOTTOM_OFFSET_STICKY_WITHOUT_STICKIES);

    JsonData dataTop(NON_STICKY_CHILD_TOP_WITH_OFFSET);
    auto childTop = context->inflate(dataTop.get());
    auto coreChildTop = CoreComponent::cast(childTop);
    ASSERT_TRUE(coreChildTop);

    auto scroll = context->findComponentById("scrollone");
    auto stickyCont = context->findComponentById("stickyContainer");
    stickyCont->insertChild(childTop, 0);
    ASSERT_TRUE(component->needsLayout());
    root->clearPending();  // Forces the layout

    EXPECT_TRUE(expectBounds(coreChildTop, 100, 0, 200, 300));

    coreChildTop->setProperty(kPropertyPosition, "sticky");
    ASSERT_EQ(coreChildTop->getCalculated(kPropertyPosition), kPositionSticky);

    ASSERT_TRUE(component->needsLayout());
    root->clearPending();  // Forces the layout

    auto stickyTop = context->findComponentById("topsticky");
    assert(stickyTop);

    EXPECT_TRUE(expectBounds(stickyTop, 0, 0, 100, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(1000);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -350), true));
    advanceTime(1000);

    EXPECT_TRUE(expectBounds(stickyTop, 150, 0, 250, 300));

    coreChildTop->setProperty(kPropertyPosition, "relative");
    ASSERT_EQ(coreChildTop->getCalculated(kPropertyPosition), kPositionRelative);
    root->clearPending();  // Forces the layout

    EXPECT_TRUE(expectBounds(stickyTop, 100, 0, 200, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 100), true));
    advanceTime(500);

    EXPECT_TRUE(expectBounds(stickyTop, 100, 0, 200, 300));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -350), true));
    advanceTime(500);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 100), true));
    advanceTime(1000);

    //Check it also works with a second child
    JsonData dataBottom(NON_STICKY_CHILD_BOTTOM_WITHOUT_OFFSET);
    auto childBottom = context->inflate(dataBottom.get());
    auto coreChildBottom = CoreComponent::cast(childBottom);
    ASSERT_TRUE(childBottom);

    stickyCont->insertChild(childBottom, 2);

    coreChildBottom->setProperty(kPropertyPosition, "sticky");
    coreChildBottom->setProperty(kPropertyBottom, "0");

    ASSERT_TRUE(component->needsLayout());
    root->clearPending();  // Forces the layout

    auto stickyBottom = context->findComponentById("bottomsticky");
    assert(stickyBottom);

    EXPECT_TRUE(expectBounds(stickyBottom, 200, 0, 300, 150));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -50), true));
    advanceTime(1000);

    EXPECT_TRUE(expectBounds(stickyBottom, 150, 0, 250, 150));
}

const static char * NESTED_SCROLLABLES_WITH_STICKY = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": [
      {
        "type": "Frame",
        "height": 600,
        "width": 500,
        "padding": 40,
        "backgroundColor": "black",
        "items": [
          {
            "id": "scrollone",
            "type": "Sequence",
            "scrollDirection": "horizontal",
            "width": 400,
            "height": 400,
            "item" : {
              "type": "Container",
              "height": 400,
              "width": 2000,
              "direction": "row",
              "items": [
                {
                  "type": "Frame",
                  "height": 300,
                  "width": 300,
                  "backgroundColor": "white",
                  "items": []
                },
                {
                  "type": "Frame",
                  "height": 300,
                  "width": 400,
                  "backgroundColor": "#1a73e8",
                  "items": [
                    {
                      "type": "Container",
                      "height": 300,
                      "width": 400,
                      "direction": "row",
                      "items": [
                        {
                          "id": "leftsticky",
                          "position": "sticky",
                          "left": 0,
                          "type": "Frame",
                          "height": 300,
                          "width": 100,
                          "backgroundColor": "#dc3912",
                          "items": []
                        },
                        {
                          "type": "Frame",
                          "height": 200,
                          "width": 100,
                          "backgroundColor": "#4caf50",
                          "items": []
                        },
                        {
                          "id": "rightsticky",
                          "position": "sticky",
                          "right": 0,
                          "type": "Frame",
                          "height": 150,
                          "width": 100,
                          "backgroundColor": "blue",
                          "items": []
                        }
                      ]
                    }
                  ]
                },
                {
                  "id": "scrolltwo",
                  "type": "Sequence",
                  "scrollDirection": "horizontal",
                  "width": 400,
                  "height": 200,
                  "item" : {
                    "type": "Container",
                    "height": 400,
                    "width": 1000,
                    "direction": "row",
                    "items": [
                      {
                        "type": "Frame",
                        "height": 300,
                        "width": 400,
                        "backgroundColor": "#1a73e8",
                        "items": [
                          {
                            "type": "Container",
                            "height": 300,
                            "width": 400,
                            "direction": "row",
                            "items": [
                              {
                                "id": "leftstickyscrolltwo",
                                "position": "sticky",
                                "left": 0,
                                "type": "Frame",
                                "height": 300,
                                "width": 100,
                                "backgroundColor": "#dc3912",
                                "items": []
                              },
                              {
                                "type": "Frame",
                                "height": 200,
                                "width": 100,
                                "backgroundColor": "#4caf50",
                                "items": []
                              },
                              {
                                "id": "rightstickyscrolltwo",
                                "position": "sticky",
                                "right": 0,
                                "type": "Frame",
                                "height": 150,
                                "width": 100,
                                "backgroundColor": "blue",
                                "items": []
                              }
                            ]
                          }
                        ]
                      },
                      {
                        "type": "Frame",
                        "height": 300,
                        "width": 300,
                        "backgroundColor": "orange",
                        "items": []
                      }
                    ]
                  }
                }
              ]
            }
          }
        ]
      }
    ]
  }
})apl";

/**
 * Make sure a sticky components in a nested scrollable don't react to a scrollable ancestor
 * that isn't it's direct scrollable vertical/horizontal ancestor
 */
TEST_F(ScrollTest, NestedScrollablesSameTypeWithStickies)
{
    loadDocument(NESTED_SCROLLABLES_WITH_STICKY);

    auto scrollTop = context->findComponentById("scrollone");
    auto scrollBottom = context->findComponentById("scrolltwo");
    auto stickyCont = context->findComponentById("stickyContainer");

    auto stickyTop = context->findComponentById("leftstickyscrolltwo");
    assert(stickyTop);

    expectBounds(stickyTop, 0, 0, 300, 100);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(2000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-350, 100), true));

    expectBounds(stickyTop, 0, 0, 300, 100);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-850, 100), true));
    advanceTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(-850, 100), true));
    advanceTime(1000);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-300, 100), true));
    advanceTime(1000);

    expectBounds(stickyTop, 0, 300, 300, 400);
}

/**
 * Make sure a combination of horizontal and vertical scrollables works
 */
const static char * NESTED_SCROLLABLES_WITH_SAME_AND_DIFFERENT_TYPES = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": [
      {
        "type": "Frame",
        "height": 600,
        "width": 500,
        "padding": 10,
        "backgroundColor": "black",
        "items": [
          {
            "id": "horzscrollone",
            "type": "Sequence",
            "scrollDirection": "horizontal",
            "width": 400,
            "height": 400,
            "item" :
            {
              "type": "Frame",
              "height": 1000,
              "width": 1000,
              "backgroundColor": "pink",
              "padding": 10,
              "item":
                {
                "id": "vertscrollone",
                "type": "Sequence",
                "scrollDirection": "vertical",
                "width":  1000,
                "height": 400,
                "item" : {
                  "type": "Container",
                  "height": 1000,
                  "width": 1000,
                  "direction": "row",
                  "items": [
                    {
                      "id": "topSticky",
                      "type": "Frame",
                      "position": "sticky",
                      "top": 0,
                      "left": 40,
                      "height": 350,
                      "width": 150,
                      "backgroundColor": "blue"
                    },
                    {
                      "id": "vertscrolltwo",
                      "type": "Sequence",
                      "scrollDirection": "vertical",
                      "width":  1000,
                      "height": 300,
                      "items": [
                        {
                          "type": "Frame",
                          "height": 1000,
                          "width": 500,
                          "backgroundColor": "green",
                          "item" : {
                            "type": "Container",
                            "height": 1000,
                            "width": 1000,
                            "direction": "row",
                            "items": [
                              {
                                "id": "deepestSticky",
                                "type": "Frame",
                                "position": "sticky",
                                "top": 20,
                                "left": 20,
                                "height": 150,
                                "width": 150,
                                "backgroundColor": "red"
                              },
                              {
                                "type": "Frame",
                                "height": 100,
                                "width": 100,
                                "backgroundColor": "purple"
                              }
                            ]
                          }
                        }
                      ]
                    }
                  ]
                }
              }
            }
          }
        ]
      }
    ]
  }
})apl";

TEST_F(ScrollTest, NestedScrollablesSameAndDifferntTypeWithStickies)
{
    loadDocument(NESTED_SCROLLABLES_WITH_SAME_AND_DIFFERENT_TYPES);

    auto vertscrollone = context->findComponentById("vertscrollone");
    auto vertscrolltwo = context->findComponentById("vertscrolltwo");
    auto horzscrollone = context->findComponentById("horzscrollone");

    auto stickyTop = context->findComponentById("topSticky");
    auto deepestSticky =
        CoreComponent::cast(context->findComponentById("deepestSticky"));
    assert(stickyTop);
    assert(deepestSticky);

    EXPECT_TRUE(expectBounds(stickyTop, 0, 30, 350, 180));

    EXPECT_TRUE(expectBounds(deepestSticky, 20, 0, 170, 150));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(2000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -300), true));
    advanceTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100, -300), true));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(-300, 100), true));
    advanceTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(-300, 100), true));
    advanceTime(1000);

    EXPECT_TRUE(expectBounds(stickyTop, 400, 430, 750, 580));

    //Check to make sure this component has only reacted to the horizontal scrollable
    EXPECT_TRUE(expectBounds(deepestSticky, 20, 260, 170, 410));

    //Move the second vertical scrollable back into view
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 500), true));
    advanceTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100, 500), true));
    advanceTime(1000);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(1000);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -100), true));
    advanceTime(1000);

    //Check to make sure this component has not reacted
    EXPECT_TRUE(expectBounds(stickyTop, 0, 430, 350, 580));

    //Check to make sure this component has updated it's vertical position
    EXPECT_TRUE(expectBounds(deepestSticky, 220, 260, 370, 410));

    deepestSticky->setProperty(kPropertyPosition, "relative");
    root->clearPending(); // Forces the layout

    EXPECT_TRUE(expectBounds(deepestSticky, 20, 20, 170, 170));

    deepestSticky->setProperty(kPropertyPosition, "sticky");
    root->clearPending(); // Forces the layout

    EXPECT_TRUE(expectBounds(deepestSticky, 420, 500, 570, 650));
}

const static char * REMOVE_STICKY_COMPONENT_DOC = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "id": "scrollone",
      "type": "ScrollView",
      "height": 400,
      "item": {
        "id": "stickyContainer",
        "type": "Container",
        "height": 2000,
        "item": {
          "type": "Frame",
          "id": "1",
          "position": "sticky",
          "top": 100,
          "height": 100,
          "width": 100
        }
      }
    }
  }
})apl";

/**
 * Make sure a removed component doesn't react to scrolling
 */
TEST_F(ScrollTest, RemoveAndReplaceStickyComponet)
{
    loadDocument(REMOVE_STICKY_COMPONENT_DOC);

    auto scrollTop = context->findComponentById("scrollone");
    auto stickyTop = context->findComponentById("1");
    auto stickyContainer = context->findComponentById("stickyContainer");
    assert(stickyTop);

    EXPECT_TRUE(expectBounds(stickyTop, 100, 0, 200, 100));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyTop, 300, 0, 400, 100));

    stickyTop->remove();
    root->clearPending();
    ASSERT_EQ(stickyContainer->getChildCount(), 0);

    EXPECT_TRUE(expectBounds(stickyTop, 300, 0, 400, 100));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyTop, 300, 0, 400, 100));
}


const static char * REPLACE_STICKY_COMPONENT_DOC = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items" : [
        {
          "type": "ScrollView",
          "id": "scrollone",
          "height": 200,
          "item": {
            "id": "stickyContainer",
            "type": "Container",
            "height": 2000,
            "item": {
              "type": "Frame",
              "id": "1",
              "backgroundColor": "yellow",
              "position": "sticky",
              "top": 100,
              "height": 100,
              "width": 100
            }
          }
        },
        {
          "type": "ScrollView",
          "id": "scrolltwo",
          "height": 200,
          "item": {
            "id": "stickyContainertwo",
            "type": "Container",
            "height": 2000
          }
        }
      ]
    }
  }
})apl";

/**
 * Move a component from one scrollable to another and check offsets are correct
 */
TEST_F(ScrollTest, ReplaceAndCheckStickyComponent)
{
    loadDocument(REPLACE_STICKY_COMPONENT_DOC);

    auto scrollTop = context->findComponentById("scrollone");
    auto scrolltwo = context->findComponentById("scrolltwo");
    auto stickyTop = context->findComponentById("1");
    auto stickyContainer = context->findComponentById("stickyContainer");
    auto stickyContainertwo = context->findComponentById("stickyContainertwo");

    EXPECT_TRUE(expectBounds(stickyTop, 100, 0, 200, 100));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 100), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, -100), true));
    advanceTime(200);

    EXPECT_TRUE(expectBounds(stickyTop, 300, 0, 400, 100));

    stickyTop->remove();
    root->clearPending();
    ASSERT_EQ(stickyContainer->getChildCount(), 0);

    EXPECT_TRUE(expectBounds(stickyTop, 300, 0, 400, 100));

    // Check to make sure the component isn't reacting to scrolling
    advanceTime(200);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 100), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(100, 100), true));

    EXPECT_TRUE(expectBounds(stickyTop, 300, 0, 400, 100));

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 210), false));
    advanceTime(200);

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 110), true));
    advanceTime(200);

    stickyContainertwo->insertChild(stickyTop, 0);
    root->clearPending();

    EXPECT_TRUE(expectBounds(stickyTop, 200, 0, 300, 100));
}

static const char *SEQUENCE_TEST_VERTICAL_DURATION = R"(
{
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "vertical",
      "id": "foo",
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";

TEST_F(ScrollTest, SequenceVerticalDuration)
{
    loadDocument(SEQUENCE_TEST_VERTICAL_DURATION);

    completeScroll(component, -1, 200);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1, 200);
    ASSERT_EQ(Point(0, 300), component->scrollPosition());

    completeScroll(component, 5, 200);  // This maxes out
    ASSERT_EQ(Point(0, 700), component->scrollPosition());

    completeScroll(component, 5, 200);
    ASSERT_EQ(Point(0, 700), component->scrollPosition());

    completeScroll(component, -0.5f, 200);
    ASSERT_EQ(Point(0, 550), component->scrollPosition());

    completeScroll(component, -20, 200);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

static const char *SEQUENCE_WITH_INDEX_DURATION = R"(
{
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "scrollDirection": "vertical",
      "id": "foo",
      "width": 200,
      "height": 300,
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      },
      "data": "${Array.range(1,11)}"
    }
  }
})";

TEST_F(ScrollTest, ScrollToIndexFirstDuration)
{
    loadDocument(SEQUENCE_WITH_INDEX_DURATION);

    // Move the second item up to the top of the scroll view.
    scrollToIndex(component, 1, kCommandScrollAlignFirst, 200);
    ASSERT_EQ(Point(0,100), component->scrollPosition());

    // Repeat the command - it shouldn't move.
    scrollToIndex(component, 1, kCommandScrollAlignFirst, 200);
    ASSERT_EQ(Point(0,100), component->scrollPosition());

    scrollToIndex(component, 5, kCommandScrollAlignFirst, 200);
    ASSERT_EQ(Point(0,500), component->scrollPosition());

    scrollToIndex(component, 3, kCommandScrollAlignFirst, 200);
    ASSERT_EQ(Point(0,300), component->scrollPosition());

    // The last component can't scroll all the way to the top
    scrollToIndex(component, 9, kCommandScrollAlignFirst, 200);
    ASSERT_EQ(Point(0, 700), component->scrollPosition());

    scrollToIndex(component, 0, kCommandScrollAlignFirst, 200);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignFirst, 200);
    ASSERT_EQ(Point(0,500), component->scrollPosition());
}

TEST_F(ScrollTest, ScrollToComponentDuration)
{
    loadDocument(VERTICAL_SCROLLVIEW);

    std::map<std::string, ComponentPtr> map;
    for (int i = 1 ; i <= 6 ; i++) {
        std::string name = "frame" + std::to_string(i);
        map.emplace(name, root->context().findComponentById(name));
    }

    scrollToComponent(map["frame4"], kCommandScrollAlignFirst, 300);
    ASSERT_EQ(Point(0,600), component->scrollPosition());
}

TEST_F(ScrollTest, SequenceVerticalInstant)
{
    loadDocument(SEQUENCE_TEST_VERTICAL_DURATION);

    completeScroll(component, -1, 0);  // Can't scroll backwards
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    completeScroll(component, 1, 0);
    ASSERT_EQ(Point(0, 300), component->scrollPosition());

    completeScroll(component, 5, 0);  // This maxes out
    ASSERT_EQ(Point(0, 700), component->scrollPosition());

    completeScroll(component, 5, 0);
    ASSERT_EQ(Point(0, 700), component->scrollPosition());

    completeScroll(component, -0.5f, 0);
    ASSERT_EQ(Point(0, 550), component->scrollPosition());

    completeScroll(component, -20, 0);
    ASSERT_EQ(Point(0,0), component->scrollPosition());
}

TEST_F(ScrollTest, ScrollToIndexFirstInstant)
{
    loadDocument(SEQUENCE_WITH_INDEX_DURATION);

    // Move the second item up to the top of the scroll view.
    scrollToIndex(component, 1, kCommandScrollAlignFirst, 0);
    ASSERT_EQ(Point(0,100), component->scrollPosition());

    // Repeat the command - it shouldn't move.
    scrollToIndex(component, 1, kCommandScrollAlignFirst, 0);
    ASSERT_EQ(Point(0,100), component->scrollPosition());

    scrollToIndex(component, 5, kCommandScrollAlignFirst, 0);
    ASSERT_EQ(Point(0,500), component->scrollPosition());

    scrollToIndex(component, 3, kCommandScrollAlignFirst, 0);
    ASSERT_EQ(Point(0,300), component->scrollPosition());

    // The last component can't scroll all the way to the top
    scrollToIndex(component, 9, kCommandScrollAlignFirst, 0);
    ASSERT_EQ(Point(0, 700), component->scrollPosition());

    scrollToIndex(component, 0, kCommandScrollAlignFirst, 0);
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    scrollToIndex(component, -5, kCommandScrollAlignFirst, 0);
    ASSERT_EQ(Point(0,500), component->scrollPosition());
}

TEST_F(ScrollTest, ScrollToComponentInstant)
{
    loadDocument(VERTICAL_SCROLLVIEW);

    std::map<std::string, ComponentPtr> map;
    for (int i = 1 ; i <= 6 ; i++) {
        std::string name = "frame" + std::to_string(i);
        map.emplace(name, root->context().findComponentById(name));
    }

    scrollToComponent(map["frame4"], kCommandScrollAlignFirst, 0);
    ASSERT_EQ(Point(0,600), component->scrollPosition());
}

static const char *LIVE_SCROLL_RAINBOWS = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "data": "${rainbows}",
      "item": {
        "type": "Sequence",
        "id": "Rainbow${data}",
        "width": 200,
        "height": 200,
        "data": ["red", "orange", "yellow", "green", "blue", "indigo", "violet"],
        "items": [
          {
            "type": "Frame",
            "id": "${data}${index}",
            "backgroundColor": "${data}",
            "width": 200,
            "height": 50
          }
        ]
      }
    }
  }
}
)";

TEST_F(ScrollTest, ClearLiveDataDuringChildScrollCommand)
{
    auto liveArray = LiveArray::create({"One", "Two"});
    config->liveData("rainbows", liveArray);
    loadDocument(LIVE_SCROLL_RAINBOWS);
    advanceTime(100);

    ASSERT_EQ(2, component->getChildCount());

    auto rainbowOne = component->getCoreChildAt(0);
    ASSERT_EQ("RainbowOne", rainbowOne->getId());
    ASSERT_EQ(Point(), rainbowOne->scrollPosition());

    auto rainbowTwo = component->getCoreChildAt(1);
    ASSERT_EQ("RainbowTwo", rainbowTwo->getId());
    ASSERT_EQ(Point(), rainbowTwo->scrollPosition());

    // Initiate scroll down by 100 over a duration of 1 second
    executeScroll("RainbowOne", 100, 1000);

    // Move forward by 0.5 seconds, and we'll be partway there
    advanceTime(500);
    ASSERT_NE(Point(), rainbowOne->scrollPosition());
    ASSERT_EQ(Point(), rainbowTwo->scrollPosition());

    // Clear live data
    liveArray->clear();
    advanceTime(10);

    // Rainbows are gone
    ASSERT_EQ(0, component->getChildCount());
}

TEST_F(ScrollTest, ClearLiveDataDuringChildScrollGesture)
{
  auto liveArray = LiveArray::create({"One", "Two"});
  config->liveData("rainbows", liveArray);
  loadDocument(LIVE_SCROLL_RAINBOWS);
  advanceTime(100);

  ASSERT_EQ(2, component->getChildCount());

  auto rainbowOne = component->getCoreChildAt(0);
  ASSERT_EQ("RainbowOne", rainbowOne->getId());
  ASSERT_EQ(Point(), rainbowOne->scrollPosition());

  auto rainbowTwo = component->getCoreChildAt(1);
  ASSERT_EQ("RainbowTwo", rainbowTwo->getId());
  ASSERT_EQ(Point(), rainbowTwo->scrollPosition());

  // Scroll down (rainbow #2) by 100, but don't release finger
  ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(100, 300), false));
  advanceTime(100);
  ASSERT_EQ(Point(), rainbowOne->scrollPosition());
  ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(100, 200), true));
  ASSERT_EQ(Point(0, 100), rainbowTwo->scrollPosition());

  // Clear live data
  liveArray->clear();
  advanceTime(10);

  // Rainbows are gone
  ASSERT_EQ(0, component->getChildCount());
}

static const char *SIMPLE_PAGER_WITH_CHILD = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "id": "myPager",
      "width": 100,
      "height": 100,
      "initialPage": 1,
      "navigation": "normal",
      "items": {
        "type": "Sequence",
        "id": "id${data}",
        "items": [
          {
            "type": "Text",
            "id": "childId${data}",
            "text": "TEXT${data}"
          }
        ]
      },
      "data": [1,2,3,4,5]
    }
  }
})";

TEST_F(ScrollTest, ScrollToUnattachedChild)
{
  loadDocument(SIMPLE_PAGER_WITH_CHILD);
  advanceTime(10);
  ASSERT_EQ(1, component->pagePosition());

  executeScrollToComponent("childId5", kCommandScrollAlignFirst);
  ASSERT_EQ("scrolltoaction.cpp:start : Trying to scroll a component that was never laid out. Ignoring the command.", logBridge->getLast());
  ASSERT_TRUE(LogMessage());
}

static const char *SCROLL_TO_INDEX_ON_MOUNT = R"({
  "type": "APL",
  "version": "2024.1",
  "theme": "dark",
  "onMount": [
    {
      "type": "ScrollToIndex",
      "componentId": "sequenceID",
      "index": 4,
      "align": "center"
    }
  ],
  "mainTemplate": {
    "bind": [
      {
        "name": "handler",
        "value": "handler message"
      }
    ],
    "items": [
      {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "paddingTop": "16dp",
        "paddingLeft": "16dp",
        "paddingRight": "16dp",
        "paddingBottom": "16dp",
        "items": [
          {
            "type": "Sequence",
            "id": "sequenceID",
            "scrollDirection": "horizontal",
            "height": "200dp",
            "width": "200",
            "snap": "center",
            "data": [
              "TEXT 0",
              "TEXT 1",
              "TEXT 2",
              "TEXT 3",
              "TEXT 4",
              "TEXT 5",
              "TEXT 6"
            ],
            "items": {
              "type": "Text",
              "text": "Type ${data}",
              "fontSize": "24dp",
              "paddingTop": "12dp",
              "paddingBottom": "12dp",
              "height": "32dp",
              "width": "200",
              "onLayout": {
                "when": "${event.source.index <= 3}",
                "type": "Sequential",
                "commands": [
                  {
                    "type": "SetValue",
                    "property": "handler",
                    "value": "${event.source.handler}",
                    "componentId": "TextContainer"
                  }
                ]
              }
            }
          },
          {
            "type": "Text",
            "id": "TextContainer",
            "text": "handler: ${handler}",
            "fontSize": "20dp",
            "height": "32dp",
            "width": "100%"
          }
        ]
      }
    ]
  }
})";

TEST_F(ScrollTest, ScrollToIndexOnMount)
{
    loadDocument(SCROLL_TO_INDEX_ON_MOUNT);
    advanceTime(2000);
    auto sequence = root->findComponentById("sequenceID");
    ASSERT_NE(0, sequence->scrollPosition().getX());
}