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

class SpeakListTest : public CommandTest {
public:
    void executeSpeakList(const std::string& item,
                          CommandScrollAlign align,
                          CommandHighlightMode highlightMode,
                          int start,
                          int count,
                          int minimumDwell,
                          int delay) {

        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "SpeakList", alloc);
        cmd.AddMember("componentId", rapidjson::Value(item.c_str(), alloc).Move(), alloc);
        cmd.AddMember("align", rapidjson::StringRef(sCommandAlignMap.at(align).c_str()), alloc);
        // Note: Technically, highlight mode is not a part of the command.  We're testing future additions
        cmd.AddMember("highlightMode", rapidjson::StringRef(sHighlightModeMap.at(highlightMode).c_str()), alloc);
        cmd.AddMember("start", start, alloc);
        cmd.AddMember("count", count, alloc);
        cmd.AddMember("minimumDwellTime", minimumDwell, alloc);
        cmd.AddMember("delay", delay, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        root->executeCommands(doc, false);
    }

    void executeSpeakList(const ComponentPtr& component,
                          CommandScrollAlign align,
                          CommandHighlightMode highlightMode,
                          int start,
                          int count,
                          int minimumDwell,
                          int delay) {

        executeSpeakList(component->getUniqueId(), align, highlightMode, start, count, minimumDwell, delay);
    }

    rapidjson::Document doc;
};


static const char *TEST_STAGES =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"styles\": {"
    "    \"base\": {"
    "      \"values\": ["
    "        {"
    "          \"color\": \"green\""
    "        },"
    "        {"
    "          \"when\": \"${state.karaoke}\","
    "          \"color\": \"blue\""
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"ScrollView\","
    "      \"width\": 500,"
    "      \"height\": 500,"
    "      \"item\": {"
    "        \"type\": \"Container\","
    "        \"items\": {"
    "          \"type\": \"Text\","
    "          \"style\": \"base\","
    "          \"text\": \"${data}\","
    "          \"speech\": \"http-${data}\","
    "          \"height\": 200"
    "        },"
    "        \"data\": ["
    "          \"URL1\","
    "          \"URL2\","
    "          \"URL3\","
    "          \"URL4\""
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

/**
 * Run a single SpeakList command and verify each stage.
 *
 * Assume that the speech takes longer than the minimum dwell time of 1000 milliseconds
 * Pick an item that needs to be scrolled and kCommandScrollAlignFirst
 */
TEST_F(SpeakListTest, TestStages)
{
    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);

    const int CHILD_COUNT = 4;

    ASSERT_EQ(CHILD_COUNT, container->getChildCount());
    std::vector<ComponentPtr> child;
    for (int i = 0 ; i < CHILD_COUNT ; i++)
        child.emplace_back(container->getChildAt(i));

    // Check the starting colors
    for (int i = 0 ; i < CHILD_COUNT ; i++)
        ASSERT_EQ(Object(Color(Color::GREEN)), child[i]->getCalculated(kPropertyColor));

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(container, kCommandScrollAlignFirst, kCommandHighlightModeBlock, 0, 100000, 1000, 500);

    // Nothing happens initially (the delay must pass)
    ASSERT_FALSE(root->hasEvent());
    advanceTime(500);

    for (int i = 0 ; i < CHILD_COUNT ; i++) {
        auto msg = "child[" + std::to_string(i) + "]";
        auto target = child[i];
        auto url = "http-URL" + std::to_string(i+1);
        auto scrollPosition = 200 * i;
        if (scrollPosition > 300) scrollPosition = 300;   // Max scroll position

        // The first thing we should get is a pre-roll event
        ASSERT_TRUE(root->hasEvent()) << msg;
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypePreroll, event.getType()) << msg;
        ASSERT_EQ(Object(url), event.getValue(kEventPropertySource)) << msg;

        // Scroll
        advanceTime(1000);
        ASSERT_EQ(Point(0, scrollPosition), component->scrollPosition()) << msg;

        // We should have an event for speaking.
        ASSERT_TRUE(root->hasEvent()) << msg;
        event = root->popEvent();
        ASSERT_EQ(kEventTypeSpeak, event.getType()) << msg;
        ASSERT_EQ(Object(url), event.getValue(kEventPropertySource)) << msg;
        ASSERT_EQ(Object(kCommandHighlightModeBlock), event.getValue(kEventPropertyHighlightMode)) << msg;

        // The item should have updated colors
        ASSERT_TRUE(CheckDirty(target, kPropertyColor, kPropertyColorKaraokeTarget)) << msg;
        ASSERT_EQ(Object(Color(Color::BLUE)), target->getCalculated(kPropertyColor)) << msg;

        // We'll assume that speech is SLOWER than the timeout (takes longer than 1000 milliseconds)
        advanceTime(2000);

        // Mark speech as finished
        event.getActionRef().resolve();
        root->clearPending();

        // Karaoke clears
        ASSERT_TRUE(CheckDirty(target, kPropertyColor, kPropertyColorKaraokeTarget)) << msg;
        ASSERT_EQ(Object(Color(Color::GREEN)), target->getCalculated(kPropertyColor)) << msg;
    }

    ASSERT_FALSE(root->hasEvent());
}

/**
 * Start at item #2, last-align
 */
TEST_F(SpeakListTest, TestStagesStartOffset)
{
    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);

    const int CHILD_COUNT = 4;

    ASSERT_EQ(CHILD_COUNT, container->getChildCount());
    std::vector<ComponentPtr> child;
    for (int i = 0 ; i < CHILD_COUNT ; i++)
        child.emplace_back(container->getChildAt(i));

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, 2, 100000, 1000, 500);

    // Nothing happens initially (the delay must pass)
    ASSERT_FALSE(root->hasEvent());
    advanceTime(500);

    for (int i = 2 ; i < CHILD_COUNT ; i++) {
        auto msg = "child[" + std::to_string(i) + "]";
        auto target = child[i];
        auto url = "http-URL" + std::to_string(i+1);
        auto scrollPosition = 200 * i - 300;
        if (scrollPosition > 300) scrollPosition = 300;   // Max scroll position
        if (scrollPosition < 0) scrollPosition = 0;

        // The first thing we should get is a pre-roll event
        ASSERT_TRUE(root->hasEvent()) << msg;
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypePreroll, event.getType()) << msg;
        ASSERT_EQ(Object(url), event.getValue(kEventPropertySource)) << msg;

        // Now we scroll the world.  To keep it real, let's advance the time a bit too.
        advanceTime(1000);
        ASSERT_EQ(Point(0, scrollPosition), component->scrollPosition()) << msg;

        // We should have an event for speaking.
        ASSERT_TRUE(root->hasEvent()) << msg;
        event = root->popEvent();
        ASSERT_EQ(kEventTypeSpeak, event.getType()) << msg;
        ASSERT_EQ(Object(url), event.getValue(kEventPropertySource)) << msg;
        ASSERT_EQ(Object(kCommandHighlightModeBlock), event.getValue(kEventPropertyHighlightMode)) << msg;

        // The item should have updated colors
        ASSERT_TRUE(CheckDirty(target, kPropertyColor, kPropertyColorKaraokeTarget)) << msg;
        ASSERT_EQ(Object(Color(Color::BLUE)), target->getCalculated(kPropertyColor)) << msg;

        // We'll assume that speech is SLOWER than the timeout (takes longer than 1000 milliseconds)
        advanceTime(2000);

        // Mark speech as finished
        event.getActionRef().resolve();
        root->clearPending();

        // Karaoke clears
        ASSERT_TRUE(CheckDirty(target, kPropertyColor, kPropertyColorKaraokeTarget)) << msg;
        ASSERT_EQ(Object(Color(Color::GREEN)), target->getCalculated(kPropertyColor)) << msg;
    }

    ASSERT_FALSE(root->hasEvent());
}


/**
 * Start at item #-3, do only 2
 */
TEST_F(SpeakListTest, TestStagesStartNegativeOffset)
{
    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, -3, 2, 1000, 0);

    for (int i = 1 ; i < 3 ; i++) {
        auto msg = "child[" + std::to_string(i) + "]";
        auto target = container->getChildAt(i);
        auto url = "http-URL" + std::to_string(i+1);

        // The first thing we should get is a pre-roll event
        ASSERT_TRUE(root->hasEvent()) << msg;
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypePreroll, event.getType()) << msg;
        ASSERT_EQ(Object(url), event.getValue(kEventPropertySource)) << msg;

        advanceTime(1000);

        // We should have an event for speaking.
        ASSERT_TRUE(root->hasEvent()) << msg;
        event = root->popEvent();
        ASSERT_EQ(kEventTypeSpeak, event.getType()) << msg;
        ASSERT_EQ(Object(url), event.getValue(kEventPropertySource)) << msg;

        // We'll assume that speech is SLOWER than the timeout (takes longer than 1000 milliseconds)
        advanceTime(2000);
        event.getActionRef().resolve();
    }

    ASSERT_FALSE(root->hasEvent());
}

/**
 * Start at item #-27, do only 2
 * This should trim to start at 0.
 */
TEST_F(SpeakListTest, TestStagesStartWayNegativeOffset)
{
    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, -27, 2, 1000, 0);

    for (int i = 0 ; i < 2 ; i++) {
        auto msg = "child[" + std::to_string(i) + "]";
        auto target = container->getChildAt(i);
        auto url = "http-URL" + std::to_string(i+1);

        // The first thing we should get is a pre-roll event
        ASSERT_TRUE(root->hasEvent()) << msg;
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypePreroll, event.getType()) << msg;
        ASSERT_EQ(Object(url), event.getValue(kEventPropertySource)) << msg;

        advanceTime(1000);

        // We should have an event for speaking.
        ASSERT_TRUE(root->hasEvent()) << msg;
        event = root->popEvent();
        ASSERT_EQ(kEventTypeSpeak, event.getType()) << msg;
        ASSERT_EQ(Object(url), event.getValue(kEventPropertySource)) << msg;

        // We'll assume that speech is SLOWER than the timeout (takes longer than 1000 milliseconds)
        advanceTime(2000);
        event.getActionRef().resolve();
    }

    ASSERT_FALSE(root->hasEvent());
}

/**
 * Test some cases where we shouldn't get any action
 */
TEST_F(SpeakListTest, TestZeroLengthList)
{
    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);

    // Pass a zero count
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0, 0, 1000, 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());  // Nothing pending

    // Pass a negative count
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0, -3, 1000, 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());  // Nothing pending

    // Start index == len
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, 4, 2, 1000, 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());  // Nothing pending

    // Start index > len
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, 10, 10, 1000, 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());  // Nothing pending
}

/**
 * Terminate in the middle.
 */
TEST_F(SpeakListTest, TestTerminate)
{
    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0, 4, 1000, 0);

    for (int i = 0 ; i < 4 ; i++) {
        auto msg = "child[" + std::to_string(i) + "]";
        auto target = container->getChildAt(i);
        auto url = "http-URL" + std::to_string(i+1);

        // The first thing we should get is a pre-roll event
        ASSERT_TRUE(root->hasEvent()) << msg;
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypePreroll, event.getType()) << msg;
        ASSERT_EQ(Object(url), event.getValue(kEventPropertySource)) << msg;

        advanceTime(1000);

        // We should have an event for speaking.
        ASSERT_TRUE(root->hasEvent()) << msg;
        event = root->popEvent();
        ASSERT_EQ(kEventTypeSpeak, event.getType()) << msg;
        ASSERT_EQ(Object(url), event.getValue(kEventPropertySource)) << msg;

        // This is where we'll terminate everything
        if (i == 2) {
            root->cancelExecution();
            break;
        }

        // We'll assume that speech is SLOWER than the timeout (takes longer than 1000 milliseconds)
        advanceTime(2000);
        event.getActionRef().resolve();
    }

    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());

    // Check all of the colors
    for (int i = 0 ; i < 4 ; i++) {
        auto child = container->getChildAt(0);
        ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));
    }
}
