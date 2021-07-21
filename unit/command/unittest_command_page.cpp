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

#include "apl/engine/event.h"

using namespace apl;

class CommandPageTest : public CommandTest {
public:
    ActionPtr executeSetPage(const std::string& component, const std::string& position, int value) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "SetPage", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("position", rapidjson::Value(position.c_str(), alloc).Move(), alloc);
        cmd.AddMember("value", value, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        return root->executeCommands(doc, false);
    }

    ActionPtr executeAutoPage(const std::string& component, int count, int duration) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "AutoPage", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("count", count, alloc);
        cmd.AddMember("duration", duration, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        return root->executeCommands(doc, false);
    }

    ::testing::AssertionResult
    CheckChild(size_t idx, const std::string& id, const Rect& bounds) {
        auto child = component->getChildAt(idx);

        auto actualId = child->getId();
        if (id != actualId) {
            return ::testing::AssertionFailure()
                    << "child " << idx
                    << " id is wrong. Expected: " << id
                    << ", actual: " << actualId;
        }

        auto actualBounds = child->getCalculated(kPropertyBounds).getRect();
        if (bounds != actualBounds) {
            return ::testing::AssertionFailure()
                    << "child " << idx
                    << " bounds is wrong. Expected: " << bounds.toString()
                    << ", actual: " << actualBounds.toString();
        }

        return ::testing::AssertionSuccess();
    }

    rapidjson::Document doc;
};


static const char *PAGER_TEST = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "id": "myPager",
      "width": 100,
      "height": 100,
      "navigation": "normal",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "text": "TEXT${data}",
        "speech": "URL${data}"
      },
      "data": [ 1, 2, 3, 4, 5, 6 ],
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

TEST_F(CommandPageTest, Pager)
{
    loadDocument(PAGER_TEST);
    advanceTime(10);

    ASSERT_EQ(6, component->getChildCount());
    // Only initial pages ensured
    ASSERT_TRUE(CheckChild(0, "id1", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(1, "id2", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(2, "id3", Rect(0, 0, 0, 0)));
    ASSERT_TRUE(CheckChild(3, "id4", Rect(0, 0, 0, 0)));
    ASSERT_TRUE(CheckChild(4, "id5", Rect(0, 0, 0, 0)));
    ASSERT_TRUE(CheckChild(5, "id6", Rect(0, 0, 0, 0)));

    executeSetPage("myPager", "relative", 2);  // Page forward twice

    advanceTime(600);
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->pagePosition());
    // Target one becomes ensured
    ASSERT_TRUE(CheckChild(2, "id3", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(3, "id4", Rect(0, 0, 100, 100)));

    // Ones around visible page ensured too AFTER a layout pass
    root->clearPending();
    ASSERT_TRUE(CheckChild(3, "id4", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(4, "id5", Rect(0, 0, 0, 0)));

    // We should have a SendEvent
    ASSERT_TRUE(CheckSendEvent(root, 2));

    ASSERT_TRUE(CheckNoActions());
}

static const char *SIMPLE_PAGER = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "id": "myPager",
      "width": 100,
      "height": 100,
      "initialPage": 2,
      "navigation": "normal",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "text": "TEXT${data}",
        "speech": "URL${data}"
      },
      "data": [ 1, 2, 3, 4, 5 ]
    }
  }
})";

TEST_F(CommandPageTest, SimplePageRelative)
{
    loadDocument(SIMPLE_PAGER);
    advanceTime(10);
    root->clearDirty();
    ASSERT_EQ(2, component->pagePosition());

    for (int i = -3 ; i <= 3 ; i++) {
        executeSetPage("myPager", "relative", i);
        advanceTime(500);
        std::string msg = "Relative(" + std::to_string(i) + ")";

        int target = i + 2;
        if (i == 0 || target < 0 || target > 4) {
            ASSERT_TRUE(CheckDirty(component)) << msg;
        }
        else {
            ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged)) << msg;
            ASSERT_EQ(target, component->pagePosition()) << msg;
        }

        // Reset position
        component->update(kUpdatePagerPosition, 2);
        root->clearDirty();
    }
}

TEST_F(CommandPageTest, SimplePageAbsolute)
{
    loadDocument(SIMPLE_PAGER);
    ASSERT_EQ(2, component->pagePosition());

    for (int i = -8 ; i <= 8 ; i++) {
        executeSetPage("myPager", "absolute", i);
        advanceTime(500);
        std::string msg = "Absolute(" + std::to_string(i) + ")";

        int target = i;
        if (target < 0) target += 5;   // Negative values measure from the end
        if (target < 0) target = 0;
        if (target > 4) target = 4;

        if (target == 2) {
            ASSERT_TRUE(CheckDirty(component)) << msg;
        }
        else {
            ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged)) << msg;
            ASSERT_EQ(target, component->pagePosition()) << msg;
        }

        // Reset position
        component->update(kUpdatePagerPosition, 2);
        root->clearDirty();
    }
}



static const char *SIMPLE_PAGER_WRAP = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "id": "myPager",
      "width": 100,
      "height": 100,
      "initialPage": 2,
      "navigation": "wrap",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "text": "TEXT${data}",
        "speech": "URL${data}"
      },
      "data": [ 1, 2, 3, 4, 5 ]
    }
  }
})";

TEST_F(CommandPageTest, SimplePageRelativeWrap)
{
    loadDocument(SIMPLE_PAGER_WRAP);
    advanceTime(10);
    ASSERT_EQ(2, component->pagePosition());

    // Pages around #2 are laid out
    ASSERT_TRUE(CheckChild(0, "id1", Rect(0, 0, 0, 0)));
    ASSERT_TRUE(CheckChild(1, "id2", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(2, "id3", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(3, "id4", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(4, "id5", Rect(0, 0, 0, 0)));

    for (int i = -8 ; i <= 8 ; i++) {
        executeSetPage("myPager", "relative", i);
        advanceTime(500);
        std::string msg = "Relative(" + std::to_string(i) + ")";

        int target = i + 2;
        while (target < 0) target += 5;
        while (target >= 5) target -= 5;
        if (target == 2) {
            ASSERT_TRUE(CheckDirty(component)) << msg;
        }
        else {
            ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged)) << msg;
            ASSERT_EQ(target, component->pagePosition()) << msg;
        }

        // Reset position
        component->update(kUpdatePagerPosition, 2);
        root->clearDirty();
    }
}

TEST_F(CommandPageTest, SimplePageAbsoluteWrap)
{
    loadDocument(SIMPLE_PAGER_WRAP);
    ASSERT_EQ(2, component->pagePosition());

    for (int i = -8 ; i <= 8 ; i++) {
        executeSetPage("myPager", "absolute", i);
        advanceTime(500);
        std::string msg = "Absolute(" + std::to_string(i) + ")";

        int target = i;
        if (target < 0) target += 5;   // Negative values measure from the end
        if (target < 0) target = 0;
        if (target > 4) target = 4;

        if (target == 2) {
            ASSERT_TRUE(CheckDirty(component)) << msg;
        }
        else {
            ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged)) << msg;
            ASSERT_EQ(target, component->pagePosition()) << msg;
        }

        // Reset position
        component->update(kUpdatePagerPosition, 2);
        root->clearDirty();
    }
}


static const char *AUTO_PAGE_BASIC = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "id": "myPager",
      "width": 100,
      "height": 100,
      "initialPage": 1,
      "navigation": "wrap",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "text": "TEXT${data}",
        "speech": "URL${data}"
      },
      "data": [ 1, 2, 3, 4, 5 ]
    }
  }
})";


TEST_F(CommandPageTest, AutoPage)
{
    loadDocument(AUTO_PAGE_BASIC);

    executeAutoPage("myPager", 100000, 1000);  // Play all, pausing for 1000 milliseconds between them
    advanceTime(600);

    for (int index = 2 ; index < 5 ; index++) {
        std::string msg = "Auto(" + std::to_string(index) + ")";
        ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged)) << msg;
        ASSERT_EQ(index, component->pagePosition()) << msg;

        advanceTime(1600);
    }

    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, AutoPageNoDelay)
{
    loadDocument(AUTO_PAGE_BASIC);

    executeAutoPage("myPager", 100000, 0);  // Play all, no delay
    advanceTime(600);

    for (int index = 2 ; index < 5 ; index++) {
        std::string msg = "Auto(" + std::to_string(index) + ")";
        ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged)) << msg;
        ASSERT_EQ(index, component->pagePosition()) << msg;
        advanceTime(600);
    }

    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, AutoPageShort)
{
    loadDocument(AUTO_PAGE_BASIC);

    executeAutoPage("myPager", 2, 1000);  // Just show two pages
    advanceTime(600);

    for (int index = 2 ; index < 4 ; index++) {
        std::string msg = "Auto(" + std::to_string(index) + ")";
        ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged)) << msg;
        ASSERT_EQ(index, component->pagePosition()) << msg;

        advanceTime(1600);
    }

    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, AutoPageTerminateInDelay)
{
    loadDocument(AUTO_PAGE_BASIC);

    auto action = executeAutoPage("myPager", 2, 1000);  // Just show two pages
    advanceTime(600);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->pagePosition());

    advanceTime(600);
    action->terminate();

    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, AutoPageAbortSetPage)
{
    loadDocument(AUTO_PAGE_BASIC);

    auto action = executeAutoPage("myPager", 2, 1000);  // Just show two pages
    bool terminated = false;
    action->addTerminateCallback([&terminated](const TimersPtr&){
        terminated = true;
    });

    advanceTime(600);

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(2, component->pagePosition());

    advanceTime(600);
    root->cancelExecution();

    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(terminated);
    ASSERT_TRUE(CheckNoActions());
    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, AutoPageNone)
{
    loadDocument(AUTO_PAGE_BASIC);
    advanceTime(10);

    executeAutoPage("myPager", 0, 0);  // Ask for zero
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());

    executeAutoPage("myPager", -2, 0);  // Ask for negative
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());
}


static const char *EMPTY_PAGER = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "id": "myPager",
      "width": 100,
      "height": 100,
      "initialPage": 2,
      "navigation": "wrap",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "text": "TEXT${data}",
        "speech": "URL${data}"
      },
      "data": []
    }
  }
})";

TEST_F(CommandPageTest, EmptyPagerSetPage)
{
    loadDocument(EMPTY_PAGER);

    executeSetPage("myPager", "absolute", 0);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component));

    executeSetPage("myPager", "relative", 1);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component));
}

TEST_F(CommandPageTest, EmptyPagerAutoPage)
{
    loadDocument(EMPTY_PAGER);

    executeAutoPage("myPager", 2, 0);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component));
}


static const char *SINGLE_PAGE = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "id": "myPager",
      "width": 100,
      "height": 100,
      "initialPage": 2,
      "navigation": "wrap",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "text": "TEXT${data}",
        "speech": "URL${data}"
      },
      "data": [ 1 ]
    }
  }
})";

TEST_F(CommandPageTest, SinglePageSetPage)
{
    loadDocument(SINGLE_PAGE);

    executeSetPage("myPager", "absolute", 0);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component));

    executeSetPage("myPager", "relative", 1);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component));
}

TEST_F(CommandPageTest, SinglePageAutoPage)
{
    loadDocument(SINGLE_PAGE);

    executeAutoPage("myPager", 1, 0);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component));
}

static const char *COMBINATION = R"({
  "type": "APL",
  "version": "1.0",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [ "payload" ],
    "items": [
      {
        "type": "Pager",
        "id": "aPager",
        "navigation": "none",
        "width": "100%",
        "height": "100%",
        "items": [
          {
            "type": "Container",
            "items": [
              {
                "type": "Text",
                "text": "Page 0"
              }
            ]
          },
          {
            "type": "Container",
            "items": [
              {
                "type": "Text",
                "text": "Page 1"
              },
              {
                "type": "Text",
                "id": "shooshSpeechId",
                "text": "",
                "speech": "${payload.data.properties.shooshSpeech}"
              },
              {
                "type": "Text",
                "id": "showingBoxValueSpeechId",
                "text": "",
                "speech": "${payload.data.properties.showingBoxValueSpeech}"
              }
            ]
          }
        ]
      }
    ]
  }
})";

static const char *COMBINATION_DATA = R"({
  "data": {
    "type": "object",
    "properties": {
      "showingBoxValueSpeech": "https://iamspeech.com/1.mp3",
      "shooshSpeech": "https://iamspeech.com/2.mp3"
    }
  }
})";

static const char *COMBINATION_COMMANDS = R"([{
  "type": "Sequential",
  "commands": [
    {
      "type": "Parallel",
      "commands": [
        {
          "type": "SpeakItem",
          "componentId": "shooshSpeechId"
        },
        {
          "type": "SetPage",
          "componentId": "aPager",
          "position": "absolute",
          "value": 1
        }
      ]
    },
    {
      "type": "SpeakItem",
      "componentId": "showingBoxValueSpeechId"
    }
  ]
}])";

TEST_F(CommandPageTest, SpeakItemCombination)
{
    loadDocument(COMBINATION, COMBINATION_DATA);
    advanceTime(10);
    clearDirty();

    ASSERT_EQ(0, component->pagePosition());
    doc.Parse(COMBINATION_COMMANDS);
    auto action = root->executeCommands(apl::Object(doc), false);

    // Should have preroll for first speech
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypePreroll, event.getType());
    if (!event.getActionRef().isEmpty() && event.getActionRef().isPending()) event.getActionRef().resolve();

    // And page should have switched - command is in parallel
    ASSERT_EQ(1, component->pagePosition());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSpeak, event.getType());
    if (!event.getActionRef().isEmpty() && event.getActionRef().isPending()) event.getActionRef().resolve();

    root->clearPending();

    // Should start next karaoke here
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypePreroll, event.getType());
    if (!event.getActionRef().isEmpty() && event.getActionRef().isPending()) event.getActionRef().resolve();

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSpeak, event.getType());
    if (!event.getActionRef().isEmpty() && event.getActionRef().isPending()) event.getActionRef().resolve();
}

static const char *AUTO_PAGER_ON_MOUNT_WITH_DELAY = R"apl(
{
  "type": "APL",
  "version": "1.6",
  "commands": {
    "NextCard": {
      "command": {
        "type": "SetPage",
        "sequencer": "dummySequencer",
        "delay": 5000,
        "position": "relative",
        "value": 1
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": "100vw",
      "height": "100vh",
      "onMount": [
        {
          "type": "NextCard"
        }
      ],
      "onPageChanged": [
        {
          "type": "NextCard"
        }
      ],
      "items": [
        {
          "type": "Frame",
          "backgroundColor": "red"
        },
        {
          "type": "Frame",
          "backgroundColor": "green"
        },
        {
          "type": "Frame",
          "backgroundColor": "blue"
        },
        {
          "type": "Frame",
          "backgroundColor": "orange"
        }
      ]
    }
  }
}
)apl";

TEST_F(CommandPageTest, AutoPagerOnMountWithDelay)
{
    loadDocument(AUTO_PAGER_ON_MOUNT_WITH_DELAY);
    ASSERT_EQ(0, component->pagePosition());

    advanceTime(5000);
    ASSERT_EQ(0, component->pagePosition());

    advanceTime(600);
    ASSERT_EQ(1, component->pagePosition());

    advanceTime(5000);
    advanceTime(600);
    ASSERT_EQ(2, component->pagePosition());

    advanceTime(5000);
    advanceTime(600);
    ASSERT_EQ(3, component->pagePosition());

    advanceTime(5000);
    advanceTime(600);
    ASSERT_EQ(0, component->pagePosition());
}
