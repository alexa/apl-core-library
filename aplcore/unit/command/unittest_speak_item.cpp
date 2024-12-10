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

class SpeakItemTest : public CommandTest {
public:
    SpeakItemTest() {
        audioPlayerFactory->addFakeContent({
            { "URL", 1000, 100, -1, {} },
            { "URL1", 1000, 100, -1, {} },
            { "URL2", 1000, 100, -1, {} },
            { "URL3", 1000, 100, -1, {} },
            { "URL4", 1000, 100, -1, {} },
        });
    }

    void executeSpeakItem(const std::string& item, CommandScrollAlign align, CommandHighlightMode highlightMode, int minimumDwell) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "SpeakItem", alloc);
        cmd.AddMember("componentId", rapidjson::Value(item.c_str(), alloc).Move(), alloc);
        cmd.AddMember("align", rapidjson::StringRef(sCommandAlignMap.at(align).c_str()), alloc);
        cmd.AddMember("highlightMode", rapidjson::StringRef(sHighlightModeMap.at(highlightMode).c_str()), alloc);
        cmd.AddMember("minimumDwellTime", minimumDwell, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        executeCommands(doc, false);
    }

    void executeSpeakItem(const ComponentPtr& component, CommandScrollAlign align, CommandHighlightMode highlightMode, int minimumDwell) {
        executeSpeakItem(component->getUniqueId(), align, highlightMode, minimumDwell);
    }

    rapidjson::Document doc;
};

static const char *DATA = R"apl({ "title": "Pecan Pie V" })apl";

static const char *SPEAK_ITEM_TEST = R"apl({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SpeakItem",
        "delay": 100,
        "componentId": "xyzzy",
        "align": "center",
        "highlightMode": "line",
        "minimumDwellTime": 230
      },
     "items": {
       "type": "Text",
       "id": "xyzzy"
     }
    }
  }
})apl";

// In this simple case, we don't expect to get a pre-roll or a scroll event
// The minimum dwell time guarantees that it will take 230 milliseconds to finish
TEST_F(SpeakItemTest, SpeakItemTest)
{
    loadDocument(SPEAK_ITEM_TEST, DATA);

    performTap(1,1);

    ASSERT_EQ(1, mIssuedCommands.size());
    auto command = std::static_pointer_cast<CoreCommand>(mIssuedCommands.at(0));
    ASSERT_EQ(100, command->getValue(kCommandPropertyDelay).asInt());

    // there should be no RequestFirstLineBounds here because the component is not in a scrollable container.
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(loop->size() > 0);
    loop->advanceToEnd();

    ASSERT_EQ(330, loop->currentTime());  // The command delayed by 100 first and then had a minimum dwell time of 230

    session->checkAndClear();
}

TEST_F(SpeakItemTest, DisallowedCommandStillRespectsDelay)
{
    config->set(RootProperty::kDisallowDialog, true);
    loadDocument(SPEAK_ITEM_TEST, DATA);

    performTap(1,1);

    ASSERT_EQ(1, mIssuedCommands.size());

    ASSERT_FALSE(root->hasEvent());
    loop->advanceToEnd();

    ASSERT_EQ(100, loop->currentTime());

    // complaint about ignored command logged
    ASSERT_TRUE(ConsoleMessage());
}

static const char *SPEAK_ITEM_INVALID = R"apl({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SpeakItem",
        "delay": 100,
        "componentId": "xyzzy",
        "align": "center",
        "highlightMode": "line",
        "minimumDwellTime": 230
      }
    }
  }
})apl";

TEST_F(SpeakItemTest, SpeakItemInvalid)
{
    loadDocument(SPEAK_ITEM_INVALID, DATA);

    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    ASSERT_FALSE(ConsoleMessage());
    performTap(1,1);

    // Should fail because there is no component with id "xyzzy
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

static const char *SPEAK_ITEM_THEN_SEND = R"apl({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "TouchWrapper",
      "onPress": [
        {
          "type": "SpeakItem",
          "componentId": "xyzzy"
        },
        {
          "type": "SendEvent"
        }
      ],
     "items": {
       "type": "Text",
       "id": "xyzzy",
       "speech": "URL"
     }
    }
  }
})apl";

// The speak item should run directly without a pre-roll or a scroll
TEST_F(SpeakItemTest, SpeakItemThenSend)
{
    loadDocument(SPEAK_ITEM_THEN_SEND, DATA);

    auto map = component->getCalculated();
    auto onPress = map["onPress"];

    performTap(1,1);

    ASSERT_EQ(1, mIssuedCommands.size());
    auto ptr = mIssuedCommands.at(0);
    auto p = std::static_pointer_cast<CoreCommand>(ptr);

    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPreroll));
    advanceTime(100);

    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPlay));

    // The send event will execute when we resolve the speak item
    ASSERT_FALSE(root->hasEvent());

    advanceTime(1000);

    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kRelease));

    advanceTime(100);

    ASSERT_TRUE(root->hasEvent());

    ASSERT_TRUE(CheckSendEvent(root));

    ASSERT_FALSE(root->hasEvent());
}


static const char *TEST_STAGES = R"apl({
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
      "height": 500,
      "item": {
        "type": "Container",
        "items": {
          "type": "Text",
          "style": "base",
          "text": "${data}",
          "speech": "${data}",
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
})apl";

/**
 * Run a single SpeakItem command and verify each stage.
 *
 * Assume that the speech takes longer than the minimum dwell time of 1000 milliseconds
 * Pick an item that needs to be scrolled and kCommandScrollAlignFirst
 * Run in block mode
 */
TEST_F(SpeakItemTest, TestStages)
{
    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);
    auto child = container->getChildAt(1);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem(child, kCommandScrollAlignFirst, kCommandHighlightModeBlock, 1000);

    // The first thing we should get is a pre-roll event
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPreroll));

    // Now we scroll the world.
    advanceTime(1000);
    ASSERT_EQ(Point(0,200), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    // We should have an event for speaking.
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPlay));

    // The item should have updated colors
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child));
    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));

    // We'll assume that speech is SLOWER than the timeout (takes longer than 1000 milliseconds)
    advanceTime(500);
    ASSERT_TRUE(CheckDirty(root));   // No karaoke changes yet

    advanceTime(500);
    root->clearPending();

    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kRelease));

    // No more events
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));
    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));
}

TEST_F(SpeakItemTest, DisallowedCommandPreventsEffects)
{
    config->set(RootProperty::kDisallowDialog, true);
    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);
    auto child = container->getChildAt(1);

    executeSpeakItem(child, kCommandScrollAlignFirst, kCommandHighlightModeBlock, 1000);
    loop->advanceToEnd();

    // no pre-roll or speak event
    ASSERT_FALSE(root->hasEvent());
    
    // complaint about ignored command logged
    ASSERT_TRUE(ConsoleMessage());

    // actual time
    ASSERT_EQ(0, loop->currentTime());
}

/**
 * Same test as above, but:
 *
 * Assume that the speech is shorter than the minimum dwell time of 1000 milliseconds
 * Pick an item that needs to be scrolled and kCommandScrollAlignCenter
 * Run in block mode
 */
TEST_F(SpeakItemTest, TestStagesFastSpeech)
{
    loadDocument(TEST_STAGES);

    auto container = component->getChildAt(0);
    auto child = container->getChildAt(2);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem(child, kCommandScrollAlignCenter, kCommandHighlightModeBlock, 1000);

    // Check pre-roll event
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kPreroll));

    // Now we scroll the world.
    advanceTime(1000);
    ASSERT_EQ(Point(0,250), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    // We should have an event for speaking.
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kPlay));

    // The item should have updated colors
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child));
    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));

    // We'll assume that speech is faster than the timeout
    advanceTime(500);   // Move forward by 500 milliseconds

    // There should be no changes yet - we're still waiting for dwell time
    root->clearPending();
    ASSERT_TRUE(CheckDirty(root));  // No karaoke changes yet

    // Reach the dwell time
    advanceTime(500);

    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kRelease));

    // No further events, but the color should have changed back
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));
    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));
}

/**
 * Same test as above, but:
 *
 * Skip the minimum dwell time
 * Pick an item that doesn't need to be scrolled.  Note that this will STILL result in a scrollTo event -
 *     that's because we want to cancel any fling scrolling that may be running on the device.
 * Run in line mode
 */
TEST_F(SpeakItemTest, TestStagesNoScrollingRequired)
{
    loadDocument(TEST_STAGES);

    auto container = component->getChildAt(0);
    auto child = container->getChildAt(1);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem(child, kCommandScrollAlignVisible, kCommandHighlightModeLine, 0);

    // Check pre-roll event
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPreroll));

    // Advance time by 1000 and indicate we're finished scrolling
    advanceTime(1000);
    ASSERT_EQ(Point(0,0), component->scrollPosition());


    // We should have an event for speaking.
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPlay));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeLineHighlight, event.getType());

    // The item should have updated colors
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyRangeKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));
    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));

    // We'll assume that speech is faster than the timeout
    advanceTime(500);   // Move forward by 500 milliseconds

    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kRelease));

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeLineHighlight, event.getType());

    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyRangeKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));
    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));
}


/**
 * Same test as above, but:
 *
 * Test early termination during the Scroll command
 */
TEST_F(SpeakItemTest, TestTerminationDuringScroll)
{
    loadDocument(TEST_STAGES);

    auto container = component->getChildAt(0);
    auto child = container->getChildAt(3);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem(child, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0);

    // Check pre-roll event
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kPreroll));

    advanceTime(500);

    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kReady));

    ASSERT_EQ(Point(0,150), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(root, component));   // No dirty properties yet - except children visibility

    // Terminate the command
    root->cancelExecution();
    ASSERT_FALSE(root->hasEvent());  // No events pending
    ASSERT_TRUE(CheckDirty(root));   // No dirty properties

    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kRelease));
}


/**
 * Same test as above, but:
 *
 * Test termination during the Speak command
 */
TEST_F(SpeakItemTest, TestTerminationDuringSpeech)
{
    loadDocument(TEST_STAGES);

    auto container = component->getChildAt(0);
    auto child = container->getChildAt(3);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem(child, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0);

    // Check pre-roll event
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kPreroll));

    advanceTime(1000);
    ASSERT_EQ(Point(0,300), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    // We should have an event for speaking.
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kPlay));

    // The item should have updated colors
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child));
    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));

    // Move forward a bit in time and then terminate the command
    advanceTime(500);
    root->cancelExecution();

    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kPause));
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kRelease));

    // No events should be pending, but the color should change back to green
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));
    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));
}

static const char * MISSING_COMPONENT = R"apl({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "ScrollView",
      "width": 500,
      "height": 500,
      "item": {
        "type": "Text",
        "id": "myText",
        "text": "Hello!",
        "speech": "URL1"
      }
    }
  }
})apl";

/**
 * Try to speak something that simply doesn't exist
 */
TEST_F(SpeakItemTest, MissingComponent)
{
    loadDocument(MISSING_COMPONENT);

    executeSpeakItem("myOtherText", kCommandScrollAlignCenter, kCommandHighlightModeBlock, 1000);
    // No events should be fired - there is nothing to speak
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

static const char *MISSING_SPEECH = R"apl({
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
      "width": 300,
      "height": 300,
      "item": {
        "type": "Container",
        "items": [
          {
            "type": "Text",
            "id": "text1",
            "height": 200,
            "style": "base",
            "text": "Hello!"
          },
          {
            "type": "Text",
            "id": "text2",
            "height": 200,
            "style": "base",
            "text": "Good afternoon!"
          },
          {
            "type": "Text",
            "id": "text3",
            "height": 200,
            "style": "base",
            "text": "Good day!"
          },
          {
            "type": "Text",
            "id": "text4",
            "height": 200,
            "style": "base",
            "text": "Good bye!"
          }
        ]
      }
    }
  }
})apl";

/**
 * Speak something without the speech property, but still available for scrolling.
 */
TEST_F(SpeakItemTest, MissingSpeech)
{
    loadDocument(MISSING_SPEECH);
    auto container = component->getChildAt(0);
    auto child = container->getChildAt(1);

    executeSpeakItem("text2", kCommandScrollAlignFirst, kCommandHighlightModeBlock, 1000);

    session->checkAndClear();

    // Now we scroll the world.
    advanceTime(1000);
    ASSERT_EQ(Point(0,200), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    // We'll need to wait out the minimum dwell time because one was set
    ASSERT_FALSE(root->hasEvent());   // No events pending
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));  // Color change
    ASSERT_TRUE(CheckDirty(root, component, child));
    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));

    // Run through the minimum dwell time
    advanceTime(1000);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));  // Color change
    ASSERT_TRUE(CheckDirty(root, child));
    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));
}

/**
 * Same test as above, but this time set the minimum dwell time to zero
 */
TEST_F(SpeakItemTest, MissingSpeechNoDwell)
{
    loadDocument(MISSING_SPEECH);
    auto container = component->getChildAt(0);
    auto child = container->getChildAt(1);

    executeSpeakItem("text2", kCommandScrollAlignFirst, kCommandHighlightModeBlock, 0);

    session->checkAndClear();

    // Now we scroll the world.
    advanceTime(1000);
    ASSERT_EQ(Point(0,200), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(root, component));

    // At this point nothing should be left - without a dwell time or speech, we don't get a change
    ASSERT_FALSE(root->hasEvent());   // No events pending
    ASSERT_TRUE(CheckDirty(root));
}

static const char * MISSING_SPEECH_AND_SCROLL = R"apl({
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
      "type": "Container",
      "items": [
        {
          "type": "Text",
          "id": "text1",
          "height": 200,
          "style": "base",
          "text": "Hello!"
        },
        {
          "type": "Text",
          "id": "text2",
          "height": 200,
          "style": "base",
          "text": "Good afternoon!"
        }
      ]
    }
  }
})apl";

/**
 * In this test the spoken item can't scroll and has no speech.  It can still be highlighted due to dwell time.
 */
TEST_F(SpeakItemTest, MissingSpeechAndScroll)
{
    loadDocument(MISSING_SPEECH_AND_SCROLL);
    auto child = component->getChildAt(1);

    executeSpeakItem("text2", kCommandScrollAlignFirst, kCommandHighlightModeBlock, 1000);

    session->checkAndClear();

    // We'll need to wait out the minimum dwell time because one was set
    ASSERT_FALSE(root->hasEvent());   // No events pending
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));  // Color change
    ASSERT_TRUE(CheckDirty(root, child));
    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));

    // Run through the minimum dwell time
    advanceTime(1000);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));  // Color change
    ASSERT_TRUE(CheckDirty(root, child));
    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));
}

/**
 * Same as the last example, but this time we set the dwell time to zero.
 */
TEST_F(SpeakItemTest, MissingSpeechAndScrollNoDwell)
{
    loadDocument(MISSING_SPEECH_AND_SCROLL);
    auto child = component->getChildAt(1);

    executeSpeakItem("text2", kCommandScrollAlignFirst, kCommandHighlightModeBlock, 0);

    session->checkAndClear();

    // Nothing should happen
    ASSERT_FALSE(root->hasEvent());   // No events pending
    ASSERT_TRUE(CheckDirty(root));
}

static const char *MISSING_SCROLL = R"apl({
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
      "type": "Container",
      "items": [
        {
          "type": "Text",
          "id": "text1",
          "height": 200,
          "style": "base",
          "text": "Hello!",
          "speech": "URL1"
        },
        {
          "type": "Text",
          "id": "text2",
          "height": 200,
          "style": "base",
          "text": "Good afternoon!",
          "speech": "URL2"
        }
      ]
    }
  }
})apl";

/**
 * In this example there is nothing to scroll, but we can still speak
 */
TEST_F(SpeakItemTest, MissingScroll)
{
    loadDocument(MISSING_SCROLL);
    auto child = component->getChildAt(1);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem("text2", kCommandScrollAlignFirst, kCommandHighlightModeBlock, 1000);

    // Check pre-roll event
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPreroll));

    // Preroll
    advanceTime(100);

    // We should have an event for speaking.
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPlay));

    // The item should have updated colors
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));
    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));

    // Move forward a bit in time and finish speaking
    advanceTime(500);

    // We haven't passed the minimum dwell time
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CheckDirty(root));

    // Move forward past the minimum dwell time
    advanceTime(500);

    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kRelease));

    // No events should be pending, but the color should change back to green
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));
    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));
}

