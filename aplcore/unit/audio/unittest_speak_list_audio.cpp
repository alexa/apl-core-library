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

#include "audiotest.h"
#include "apl/animation/coreeasing.h"

using namespace apl;

class SpeakListAudioTest : public AudioTest {
public:
    void executeSpeakList(const std::string& item,
                          CommandScrollAlign align,
                          CommandHighlightMode highlightMode,
                          int start,
                          int count,
                          int minimumDwell,
                          int delay,
                          std::string sequencer = "") {
        executeCommand("SpeakList",
                       {{"componentId", item},
                        {"align", sCommandAlignMap.at(align)},
                        {"highlightMode", sHighlightModeMap.at(highlightMode)},
                        {"start", start},
                        {"count", count},
                        {"minimumDwellTime", minimumDwell},
                        {"delay", delay},
                        {"sequencer", sequencer}
                       }, false);
    }

    void executeSpeakList(const ComponentPtr& component,
                          CommandScrollAlign align,
                          CommandHighlightMode highlightMode,
                          int start,
                          int count,
                          int minimumDwell,
                          int delay,
                          std::string sequencer = "") {

        executeSpeakList(component->getUniqueId(), align, highlightMode, start, count, minimumDwell, delay, sequencer);
    }

    void CheckWithEndScroll(const ComponentPtr& scroller,
                            const ComponentPtr& target,
                            const std::string& url,
                            int prerollDuration,
                            int scrollDuration,
                            int targetPosition,
                            int playDuration,
                            const std::string& msg) {
        assert(scrollDuration > prerollDuration);

        // Pre-roll has been queued
        ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kPreroll)) << msg;
        ASSERT_FALSE(factory->hasEvent()) << msg;

        // Advance time through the pre-roll
        advanceTime(prerollDuration);
        ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kReady)) << msg;
        ASSERT_FALSE(factory->hasEvent()) << msg;

        // Finish the scrolling
        advanceTime(scrollDuration - prerollDuration);
        ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kPlay)) << msg;
        ASSERT_FALSE(factory->hasEvent());
        ASSERT_TRUE(IsEqual(Dimension(targetPosition),
                            component->getCalculated(kPropertyScrollPosition)))
            << msg;
        ASSERT_TRUE(IsEqual(Color(Color::BLUE), target->getCalculated(kPropertyColor))) << msg;

        // Playback
        advanceTime(playDuration);
        ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kDone)) << msg;
        ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kRelease)) << msg;
        ASSERT_TRUE(IsEqual(Color(Color::GREEN), target->getCalculated(kPropertyColor))) << msg;
    }

    void CheckScrollAndPreroll(const ComponentPtr& scroller,
                              const ComponentPtr& target,
                              const std::string& url,
                              int prerollDuration,
                              int scrollDuration,
                              int targetPosition,
                              const std::string& msg) {
        assert(scrollDuration > prerollDuration);
        int lastPosition = scroller->getCalculated(apl::kPropertyScrollPosition).asInt();

        // Pre-roll has been queued
        ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kPreroll)) << msg;
        ASSERT_FALSE(factory->hasEvent()) << msg;

        // Advance time through the pre-roll
        advanceTime(prerollDuration);
        ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kReady)) << msg;

        if (lastPosition == targetPosition) {
            // If there was no scrolling, the play has already been queued
            ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kPlay)) << msg;
            ASSERT_FALSE(factory->hasEvent()) << msg;
            ASSERT_TRUE(IsEqual(Color(Color::BLUE), target->getCalculated(kPropertyColor))) << msg;
        }
        else {
            // There is some scrolling.  Playing has not started, scrolling is part-way through
            ASSERT_FALSE(factory->hasEvent()) << msg;
            auto position =
                lastPosition + (targetPosition - lastPosition) * prerollDuration / scrollDuration;
            ASSERT_TRUE(
                IsEqual(Dimension(position), component->getCalculated(kPropertyScrollPosition)))
                << msg;
            ASSERT_TRUE(IsEqual(Color(Color::GREEN), target->getCalculated(kPropertyColor))) << msg;

            // Finish the scrolling
            advanceTime(scrollDuration - prerollDuration);
            ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kPlay)) << msg;
            ASSERT_FALSE(factory->hasEvent());
            ASSERT_TRUE(IsEqual(Dimension(targetPosition),
                                component->getCalculated(kPropertyScrollPosition)))
                << msg;
            ASSERT_TRUE(IsEqual(Color(Color::BLUE), target->getCalculated(kPropertyColor))) << msg;
        }
    }

    void CheckScrollAndPlay(const ComponentPtr& scroller,
                            const ComponentPtr& target,
                            const std::string& url,
                            int prerollDuration,
                            int scrollDuration,
                            int targetPosition,
                            int playDuration,
                            const std::string& msg) {
        CheckScrollAndPreroll(scroller, target, url, prerollDuration, scrollDuration, targetPosition, msg);

        // Playback
        advanceTime(playDuration);
        ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kDone)) << msg;
        ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kRelease)) << msg;
        ASSERT_TRUE(IsEqual(Color(Color::GREEN), target->getCalculated(kPropertyColor))) << msg;
    }
};


static const char *TEST_STAGES = R"apl(
{
  "type": "APL",
  "version": "1.1",
  "styles": {
    "base": {
      "values": [
        {
          "color": "green"
        },
        {
          "when": "${state.karaoke}",
          "color": "blue"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "ScrollView",
      "width": 500,
      "height": 300,
      "item": {
        "type": "Container",
        "items": {
          "type": "Text",
          "style": "base",
          "text": "${data}",
          "speech": "http://${data}",
          "height": 200
        },
        "data": [
          "URL1",
          "URL2",
          "URL3",
          "URL4"
        ]
      }
    }
  }
}
)apl";

/**
 * Run a single SpeakList command and verify each stage.
 *
 * Assume that the speech takes longer than the minimum dwell time of 1000 milliseconds
 * Pick an item that needs to be scrolled and kCommandScrollAlignFirst
 */
TEST_F(SpeakListAudioTest, TestStages)
{
    factory->addFakeContent({
        {"http://URL1", 2000, 100, -1, {}}, // 2000 ms duration, 100 ms initial delay
        {"http://URL2", 2000, 100, -1, {}},
        {"http://URL3", 2000, 100, -1, {}},
        {"http://URL4", 2000, 100, -1, {}},
    });
    // Set how long it takes to scroll and make sure that scrolling is linear
    config->set(RootProperty::kScrollCommandDuration, 200);
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(TEST_STAGES);

    auto container = component->getChildAt(0);
    const int CHILD_COUNT = 4;
    ASSERT_EQ(CHILD_COUNT, container->getChildCount());

    // Check the starting colors
    for (int i = 0 ; i < CHILD_COUNT ; i++)
        ASSERT_EQ(Object(Color(Color::GREEN)), container->getChildAt(i)->getCalculated(kPropertyColor));

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(container,                   // The scrollview
                     kCommandScrollAlignFirst,    // Scroll to align at the top
                     kCommandHighlightModeBlock,  // Block highlighting
                     0,        // Start
                     100000,   // Count
                     1000,     // Minimum dwell time
                     500);     // Delay

    // Nothing happens because of the delay (including no preroll)
    ASSERT_FALSE(root->hasEvent());

    // After the delay has passed, we should get a preroll and the scroll should start
    advanceTime(500);

    for (int i = 0 ; i < CHILD_COUNT ; i++)
        CheckScrollAndPlay(component,
                           container->getChildAt(i),
                           "http://URL"+std::to_string(i+1),
                           100,  // preroll duration
                           200,  // scroll duration
                           std::min(200 * i, 500), // scroll position
                           2000,  // play duration
                           "child["+std::to_string(i+1)+"]");

    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(SpeakListAudioTest, DisallowedCommandPreventsEffects)
{
    config->set(RootProperty::kDisallowDialog, true);

    loadDocument(TEST_STAGES);

    auto container = component->getChildAt(0);
    executeSpeakList(container, kCommandScrollAlignFirst, kCommandHighlightModeBlock, 0, 100000, 1000, 500);

    loop->advanceToEnd();
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());
    
    // complaint about ignored command logged
    ASSERT_TRUE(ConsoleMessage());

    // time elapsed still reflects the base command delay 
    ASSERT_EQ(500, loop->currentTime());
}

/**
 * Start at item #2, last-align
 */
TEST_F(SpeakListAudioTest, TestStagesStartOffset)
{
    factory->addFakeContent({
        {"http://URL1", 2000, 100, -1, {}}, // 2000 ms duration, 100 ms initial delay
        {"http://URL2", 2000, 100, -1, {}},
        {"http://URL3", 2000, 100, -1, {}},
        {"http://URL4", 2000, 100, -1, {}},
    });
    // Set how long it takes to scroll and make sure that scrolling is linear
    config->set(RootProperty::kScrollCommandDuration, 200);
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);
    const int CHILD_COUNT = 4;

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(container,
                     kCommandScrollAlignLast,
                     kCommandHighlightModeBlock,
                     2,       // Start
                     100000,  // Count
                     1000,    // Minimum dwell time
                     500);    // Delay

    // Nothing happens initially (the delay must pass)
    ASSERT_FALSE(root->hasEvent());

    // After the delay has passed, we should get a preroll and the scroll should start
    advanceTime(500);

    for (int i = 2 ; i < CHILD_COUNT ; i++)
        CheckScrollAndPlay(component,
                           container->getChildAt(i),
                           "http://URL"+std::to_string(i+1),
                           100,  // preroll duration
                           200,  // scroll duration
                           std::max(0, std::min(200 * i - 100, 500)),  // scroll position
                           2000,  // play duration
                           "child["+std::to_string(i+1)+"]");

    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());
}


/**
 * Start at item -3 (=> item #1), do only 2
 */
TEST_F(SpeakListAudioTest, TestStagesStartNegativeOffset)
{
    factory->addFakeContent({
        {"http://URL1", 2000, 100, -1, {}}, // 2000 ms duration, 100 ms initial delay
        {"http://URL2", 2000, 100, -1, {}},
        {"http://URL3", 2000, 100, -1, {}},
        {"http://URL4", 2000, 100, -1, {}},
    });
    // Set how long it takes to scroll and make sure that scrolling is linear
    config->set(RootProperty::kScrollCommandDuration, 200);
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);

    // No delay to start, 1000 ms dwell time, align to end
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, -3, 2, 1000, 0);

    for (int i = 1 ; i < 3 ;i++)
        CheckScrollAndPlay(component,
                           container->getChildAt(i),
                           "http://URL"+std::to_string(i+1),
                           100,  // preroll duration
                           200,  // scroll duration
                           std::max(0, std::min(200 * i - 100, 500)),  // scroll position
                           2000,  // play duration
                           "child["+std::to_string(i+1)+"]");

    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());
}

/**
 * Start at item #-27, do only 2
 * This should trim to start at 0.
 */
TEST_F(SpeakListAudioTest, TestStagesStartWayNegativeOffset)
{
    factory->addFakeContent({
        {"http://URL1", 2000, 100, -1, {}}, // 2000 ms duration, 100 ms initial delay
        {"http://URL2", 2000, 100, -1, {}},
        {"http://URL3", 2000, 100, -1, {}},
        {"http://URL4", 2000, 100, -1, {}},
    });
    // Set how long it takes to scroll and make sure that scrolling is linear
    config->set(RootProperty::kScrollCommandDuration, 200);
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, -27, 2, 1000, 0);

    for (int i = 0 ; i < 2 ;i++)
        CheckScrollAndPlay(component,
                           container->getChildAt(i),
                           "http://URL"+std::to_string(i+1),
                           100,  // preroll duration
                           200,  // scroll duration
                           std::max(0, std::min(200 * i - 100, 500)),  // scroll position
                           2000,  // play duration
                           "child["+std::to_string(i+1)+"]");

    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());
}

/**
 * Test some cases where we shouldn't get any action
 */
TEST_F(SpeakListAudioTest, TestZeroLengthList)
{
    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);

    // Pass a zero count
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0, 0, 1000, 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_EQ(0, loop->size());  // Nothing pending

    // Pass a negative count
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0, -3, 1000, 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_EQ(0, loop->size());  // Nothing pending

    // Start index == len
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, 4, 2, 1000, 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_EQ(0, loop->size());  // Nothing pending

    // Start index > len
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, 10, 10, 1000, 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_EQ(0, loop->size());  // Nothing pending
}

/**
 * Terminate in the middle.
 */
TEST_F(SpeakListAudioTest, TestTerminate)
{
    factory->addFakeContent({
        {"http://URL1", 2000, 100, -1, {}}, // 2000 ms duration, 100 ms initial delay
        {"http://URL2", 2000, 100, -1, {}},
        {"http://URL3", 2000, 100, -1, {}},
        {"http://URL4", 2000, 100, -1, {}},
    });
    // Set how long it takes to scroll and make sure that scrolling is linear
    config->set(RootProperty::kScrollCommandDuration, 200);
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(container, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0, 4, 1000, 0);

    // Play the first two
    for (int i = 0 ; i < 2 ;i++)
        CheckScrollAndPlay(component,
                           container->getChildAt(i),
                           "http://URL"+std::to_string(i+1),
                           100,  // preroll duration
                           200,  // scroll duration
                           std::max(0, std::min(200 * i - 100, 500)),  // scroll position
                           2000,  // play duration
                           "child["+std::to_string(i+1)+"]");

    // Pre-roll the third
    CheckScrollAndPreroll(component,
                       container->getChildAt(2),
                       "http://URL3",
                       100,  // preroll duration
                       200,  // scroll duration
                       300,  // scroll position
                       "child[2]");

    // Terminate abruptly
    root->cancelExecution();

    // Audio playback had started, so it gets paused
    ASSERT_TRUE(CheckPlayer("http://URL3", TestAudioPlayer::kPause));
    ASSERT_TRUE(CheckPlayer("http://URL3", TestAudioPlayer::kRelease));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_FALSE(factory->hasEvent());
    ASSERT_EQ(0, loop->size());

    // Check all of the colors
    for (int i = 0 ; i < 4 ; i++) {
        auto child = container->getChildAt(0);
        ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));
    }
}

static const char *SPEAK_ITEM_BOSS = R"({
  "type": "APL",
  "version": "1.9",
  "theme": "auto",
  "styles": {
    "karaoke": {
      "values": [
        {
          "color": "green"
        },
        {
          "when": "${state.karaoke}",
          "color": "blue"
        }
      ]
    }
  },
  "onConfigChange": {
    "type": "Reinflate",
    "preservedSequencers": ["MAGIC"]
  },
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "when": "${viewport.pixelWidth > 350}",
      "id": "list",
      "width": "100%",
      "height": 300,
      "scrollDirection": "vertical",
      "data": [0,1,2,3,4,5,6,7,8,9],
      "item": {
        "type": "Text",
        "when": "${index < 7 || viewport.pixelWidth > 500}",
        "height": 200,
        "width": "100%",
        "style": "karaoke",
        "text": "Since <i>you</i> are not going <u>on a holiday this year Boss</u> I thought I should give your office a holiday look. Since you are not going on a holiday this year Boss I thought I should give your office a holiday look",
        "speech": "URL1"
      }
    }
  }
})";

TEST_F(SpeakListAudioTest, PreserveInBetween)
{
    // Limited subset of marks to avoid too much verification
    factory->addFakeContent({
        {"URL1", 2000, 100, -1, {}}
    });

    // Set how long it takes to scroll and make sure that scrolling is linear
    config->set(RootProperty::kScrollCommandDuration, 200);
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(SPEAK_ITEM_BOSS);

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(component, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0, 10, 1000, 0, "MAGIC");

    for (int i = 0 ; i < 5; i++) {
        CheckScrollAndPlay(component, component->getChildAt(i), "URL1",
                           100,                                       // preroll duration
                           200,                                       // scroll duration
                           std::max(0, std::min(200 * i - 100, 1700)), // scroll position
                           2000,                                      // play duration
                           "child[" + std::to_string(i) + "]");
    }

    ////////////////////////////////////////

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    // Old preroll
    ASSERT_EQ(factory->popEvent().eventType, TestAudioPlayer::kPreroll);
    // Release
    ASSERT_EQ(factory->popEvent().eventType, TestAudioPlayer::kRelease);

    // We are re-scrolling from the start
    CheckWithEndScroll(component, component->getChildAt(5), "URL1",
                       100,                                       // preroll duration
                       200,                                       // scroll duration
                       std::max(0, std::min(200 * 5 - 100, 1700)), // scroll position
                       2000,                                      // play duration
                       "child[" + std::to_string(5) + "]");

    for (int i = 6 ; i < 10; i++) {
        CheckScrollAndPlay(component, component->getChildAt(i), "URL1",
                           100,                                       // preroll duration
                           200,                                       // scroll duration
                           std::max(0, std::min(200 * i - 100, 1700)), // scroll position
                           2000,                                      // play duration
                           "child[" + std::to_string(i) + "]");
    }

    ASSERT_FALSE(factory->hasEvent());
}

TEST_F(SpeakListAudioTest, PreserveDuringPlayback)
{
    auto url = "URL1";

    // Limited subset of marks to avoid too much verification
    factory->addFakeContent({
        {url, 2000, 100, -1, {}}
    });

    // Set how long it takes to scroll and make sure that scrolling is linear
    config->set(RootProperty::kScrollCommandDuration, 200);
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(SPEAK_ITEM_BOSS);

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(component, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0, 10, 1000, 0, "MAGIC");

    for (int i = 0 ; i < 5; i++) {
        CheckScrollAndPlay(component, component->getChildAt(i), url,
                           100,                                       // preroll duration
                           200,                                       // scroll duration
                           std::max(0, std::min(200 * i - 100, 1700)), // scroll position
                           2000,                                      // play duration
                           "child[" + std::to_string(i) + "]");
    }

    ////////////////////////////////////////

    CheckScrollAndPreroll(component, component->getChildAt(5), url,
                          100,                                       // preroll duration
                          200,                                       // scroll duration
                          900, // scroll position
                          "child[5]");

    // Advance half
    advanceTime(1000);

    auto sp = component->scrollPosition();

    auto playerTimer = factory->getPlayers().at(0).lock()->getTimeoutId();
    loop->freeze(playerTimer);

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    loop->rehydrate(playerTimer);

    ASSERT_EQ(sp, component->scrollPosition());

    // Advance remainder
    advanceTime(1000);
    ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer(url, TestAudioPlayer::kRelease));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getChildAt(5)->getCalculated(kPropertyColor)));

    for (int i = 6 ; i < 10; i++) {
        CheckScrollAndPlay(component, component->getChildAt(i), "URL1",
                           100,                                       // preroll duration
                           200,                                       // scroll duration
                           std::max(0, std::min(200 * i - 100, 1700)), // scroll position
                           2000,                                      // play duration
                           "child[" + std::to_string(i) + "]");
    }

    ASSERT_FALSE(factory->hasEvent());
}

TEST_F(SpeakListAudioTest, PreserveNoTarget)
{
    // Limited subset of marks to avoid too much verification
    factory->addFakeContent({
        {"URL1", 2000, 100, -1, {}}
    });

    // Set how long it takes to scroll and make sure that scrolling is linear
    config->set(RootProperty::kScrollCommandDuration, 200);
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(SPEAK_ITEM_BOSS);

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(component, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0, 10, 1000, 0, "MAGIC");

    for (int i = 0 ; i < 5; i++) {
        CheckScrollAndPlay(component, component->getChildAt(i), "URL1",
                           100,                                       // preroll duration
                           200,                                       // scroll duration
                           std::max(0, std::min(200 * i - 100, 1700)), // scroll position
                           2000,                                      // play duration
                           "child[" + std::to_string(i) + "]");
    }

    ////////////////////////////////////////

    configChange(ConfigurationChange(300,1000));
    processReinflate();

    ASSERT_EQ(factory->popEvent().eventType, TestAudioPlayer::kPreroll);
    ASSERT_EQ(factory->popEvent().eventType, TestAudioPlayer::kRelease);
    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(SpeakListAudioTest, PreserveShortenedList)
{
    // Limited subset of marks to avoid too much verification
    factory->addFakeContent({
        {"URL1", 2000, 100, -1, {}}
    });

    // Set how long it takes to scroll and make sure that scrolling is linear
    config->set(RootProperty::kScrollCommandDuration, 200);
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(SPEAK_ITEM_BOSS);

    // Run speak list and pass a big number so we get everyone
    executeSpeakList(component, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0, 10, 1000, 0, "MAGIC");

    for (int i = 0 ; i < 5; i++) {
        CheckScrollAndPlay(component, component->getChildAt(i), "URL1",
                           100,                                       // preroll duration
                           200,                                       // scroll duration
                           std::max(0, std::min(200 * i - 100, 1700)), // scroll position
                           2000,                                      // play duration
                           "child[" + std::to_string(i) + "]");
    }

    ////////////////////////////////////////

    configChange(ConfigurationChange(400,1000));
    processReinflate();

    // Old preroll
    ASSERT_EQ(factory->popEvent().eventType, TestAudioPlayer::kPreroll);
    // Release
    ASSERT_EQ(factory->popEvent().eventType, TestAudioPlayer::kRelease);

    // We are re-scrolling from the start
    CheckWithEndScroll(component, component->getChildAt(5), "URL1",
                       100,                                       // preroll duration
                       200,                                       // scroll duration
                       std::max(0, std::min(200 * 5 - 100, 1700)), // scroll position
                       2000,                                      // play duration
                       "child[" + std::to_string(5) + "]");

    for (int i = 6 ; i < 7; i++) {
        CheckScrollAndPlay(component, component->getChildAt(i), "URL1",
                           100,                                       // preroll duration
                           200,                                       // scroll duration
                           std::max(0, std::min(200 * i - 100, 1700)), // scroll position
                           2000,                                      // play duration
                           "child[" + std::to_string(i) + "]");
    }

    ASSERT_EQ(7, component->getChildCount());

    ASSERT_FALSE(factory->hasEvent());
}
