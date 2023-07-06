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

#include "../embed/testdocumentmanager.h"

using namespace apl;

class SpeakItemAudioTest : public AudioTest {};

static const char *SPEAK_ITEM_TEST = R"apl(
{
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "xyzzy",
      "speech": "URL"
    }
  }
}
)apl";


// In this simple case, we don't expect to get a pre-roll or a scroll event
// The minimum dwell time guarantees that it will take 230 milliseconds to finish
TEST_F(SpeakItemAudioTest, SpeakItemTest)
{
    factory->addFakeContent({
        {"URL", 100, 100, -1, {}} // 100 ms duration, 100 ms initial delay
    });

    loadDocument(SPEAK_ITEM_TEST);

    executeSpeakItem("xyzzy", apl::kCommandScrollAlignCenter, apl::kCommandHighlightModeLine, 230);
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Advance until the preroll has finished
    advanceTime(100); // This should take us to the start of speech
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    // The audio should have finished here
    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    // There should still be a dwell time we are waiting on
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(loop->size() > 0);

    // Finish off the dwell
    loop->advanceToEnd();
    ASSERT_EQ(230, loop->currentTime()); // The minimum dwell time of 230 is longer than the speak
}

TEST_F(SpeakItemAudioTest, DisallowedCommandStillRespectsDelay)
{
    factory->addFakeContent({
        {"URL", 100, 100, -1, {}} // 100 ms duration, 100 ms initial delay
    });

    // Turn off speech here
    config->set(RootProperty::kDisallowDialog, true);
    loadDocument(SPEAK_ITEM_TEST);

    executeCommand("SpeakItem", {{"componentId", "xyzzy"}, {"delay", 100}}, false);
    ASSERT_FALSE(factory->hasEvent());  // No events should be posted

    ASSERT_FALSE(root->hasEvent());
    loop->advanceToEnd();

    ASSERT_EQ(100, loop->currentTime());

    // complaint about ignored command logged
    ASSERT_TRUE(ConsoleMessage());
}

static const char *SPEAK_ITEM_INVALID = R"apl(
{
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "nope",
      "speech": "URL"
    }
  }
}
)apl";

TEST_F(SpeakItemAudioTest, SpeakItemInvalid)
{
    factory->addFakeContent({
        {"URL", 100, 100, -1, {}} // 100 ms duration, 100 ms initial delay
    });

    loadDocument(SPEAK_ITEM_INVALID);

    executeCommand("SpeakItem", {{"componentId", "xyzzy"}, {"delay", 100}}, false);
    ASSERT_FALSE(factory->hasEvent());  // No events should be posted

    // Should fail because there is no component with id "xyzzy"
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

static const char *SPEAK_ITEM_THEN_SEND = R"apl(
{
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "onPress": [
        {
          "type": "SpeakItem",
          "componentId": "xyzzy"
        },
        {
          "type": "SendEvent",
          "arguments": "Done"
        }
      ],
      "items": {
        "type": "Text",
        "id": "xyzzy",
        "speech": "URL"
      }
    }
  }
}
)apl";

TEST_F(SpeakItemAudioTest, SpeakItemThenSend)
{
    factory->addFakeContent({
        {"URL", 100, 100, -1, {}} // 100 ms duration, 100 ms initial delay
    });

    loadDocument(SPEAK_ITEM_THEN_SEND);

    performTap(1,1);
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());

    // Step forward to the end of the pre-roll
    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());

    // Finish the speech
    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_TRUE(CheckSendEvent(root, "Done"));
}


static const char *TEST_STAGES = R"apl(
{
  "type": "APL",
  "version": "2023.2",
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
}
)apl";

/**
 * Run a single SpeakItem command and verify each stage.
 *
 * Assume that the speech takes longer than the minimum dwell time of 1000 milliseconds
 * Pick an item that needs to be scrolled and kCommandScrollAlignFirst
 * Run in block mode
 */
TEST_F(SpeakItemAudioTest, TestStages)
{
    factory->addFakeContent({
        {"URL1", 1000, 100, -1, {}}, // 1000 ms duration, 100 ms initial delay
        {"URL2", 1000, 100, -1, {}}, // 1000 ms duration, 100 ms initial delay
        {"URL3", 1000, 100, -1, {}}, // 1000 ms duration, 100 ms initial delay
        {"URL4", 1000, 100, -1, {}}, // 1000 ms duration, 100 ms initial delay
    });
    // Set how long it takes to scroll
    config->set(RootProperty::kScrollCommandDuration, 200);

    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);
    auto child = container->getChildAt(1);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem(child, kCommandScrollAlignFirst, kCommandHighlightModeBlock, 1000);

    // The first thing we should get is a pre-roll event
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());

    // Step forward 100 ms.  This takes us past the loading delay, and into the middle of our scrolling
    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kReady));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(Point(0, 100), component->scrollPosition()); // Halfway through scrolling

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(root, component));

    // Step forward 100 ms.  This finishes the scrolling and kicks off the speech command
    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(Point(0, 200), component->scrollPosition()); // Finished scrolling
    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));

    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(root, component, child));

    // Step forward another 500 ms.  We should still be speaking - nothing visually has changed
    advanceTime(500);
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_TRUE(CheckDirty(root));

    // Another 500 ms takes us to the end of speech.  Everything changes back
    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));
}

TEST_F(SpeakItemAudioTest, DisallowedCommandPreventsEffects)
{
    config->set(RootProperty::kDisallowDialog, true);
    loadDocument(TEST_STAGES);
    auto container = component->getChildAt(0);
    auto child = container->getChildAt(1);

    executeSpeakItem(child, kCommandScrollAlignFirst, kCommandHighlightModeBlock, 1000);
    loop->advanceToEnd();

    // no pre-roll or speak event
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(factory->hasEvent());
    
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
TEST_F(SpeakItemAudioTest, TestStagesFastSpeech)
{
    factory->addFakeContent({
        {"URL1", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL2", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL3", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL4", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
    });
    // Set how long it takes to scroll
    config->set(RootProperty::kScrollCommandDuration, 200);

    loadDocument(TEST_STAGES);

    auto container = component->getChildAt(0);
    auto child = container->getChildAt(2);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem(child, kCommandScrollAlignCenter, kCommandHighlightModeBlock, 1000);

    // The first thing we should get is a pre-roll event
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());

    // Step forward 100 ms.  This takes us past the loading delay, and into the middle of our scrolling
    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kReady));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(Point(0, 125), component->scrollPosition()); // Halfway through scrolling

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(root, component));

    // Step forward 100 ms.  This finishes the scrolling and kicks off the speech command
    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(Point(0, 250), component->scrollPosition()); // Finished scrolling
    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));

    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(root, component, child));

    // Step forward another 200 ms.  This finishes speaking, but the delay leaves us with no visual changes
    advanceTime(200);
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_TRUE(CheckDirty(root));

    // Another 800 ms finishes the dwell time and puts the colors back to normal
    advanceTime(800);
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));
}

/**
 * Same test as above, but:
 *
 * Skip the minimum dwell time
 * Pick an item that doesn't need to be scrolled.  Note that this will STILL result in a scrollTo event -
 *     that's because we want to cancel any fling scrolling that may be running on the device.
 * Run in block mode
 */
TEST_F(SpeakItemAudioTest, TestStagesNoScrollingRequired)
{
    factory->addFakeContent({
        {"URL1", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL2", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL3", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL4", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
    });
    // Set how long it takes to scroll
    config->set(RootProperty::kScrollCommandDuration, 200);

    loadDocument(TEST_STAGES);

    auto container = component->getChildAt(0);
    auto child = container->getChildAt(1);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem(child, kCommandScrollAlignVisible, kCommandHighlightModeBlock, 0);

    // The first thing we should get is a pre-roll event
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());

    // Step forward 100 ms.  This takes us past the loading delay.  There is no scrolling, so playback starts
    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));

    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));

    // Finish the playback (200 ms)
    advanceTime(200);
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));
}


/**
 * Same test as above, but:
 *
 * Test early termination during the Scroll command
 */
TEST_F(SpeakItemAudioTest, TestTerminationDuringScroll)
{
    factory->addFakeContent({
        {"URL1", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL2", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL3", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL4", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
    });
    // Set how long it takes to scroll
    config->set(RootProperty::kScrollCommandDuration, 1000);

    loadDocument(TEST_STAGES);

    auto container = component->getChildAt(0);
    auto child = container->getChildAt(3);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem(child, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0);

    // The first thing we should get is a pre-roll event
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());

    // Advance 500 ms.  This takes us halfway through the scroll command
    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kReady));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(Point(0, 150), component->scrollPosition()); // Halfway through scrolling

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(root, component));

    // Terminate the command
    root->cancelExecution();
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());  // No events pending because nothing was playing
    ASSERT_FALSE(root->hasEvent());  // No events pending
    ASSERT_TRUE(CheckDirty(root));   // No dirty properties
}


/**
 * Same test as above, but:
 *
 * Test termination during the Speak command
 */
TEST_F(SpeakItemAudioTest, TestTerminationDuringSpeech)
{
    factory->addFakeContent({
        {"URL1", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL2", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL3", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL4", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
    });
    // Set how long it takes to scroll
    config->set(RootProperty::kScrollCommandDuration, 200);

    loadDocument(TEST_STAGES);

    auto container = component->getChildAt(0);
    auto child = container->getChildAt(3);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem(child, kCommandScrollAlignLast, kCommandHighlightModeBlock, 0);

    // The first thing we should get is a pre-roll event
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());

    // Advance 200 ms.  This finishes scrolling and starts playback
    advanceTime(200);
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(Point(0, 300), component->scrollPosition()); // Halfway through scrolling
    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child, component));

    // Terminate the command
    root->cancelExecution();
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kPause));
    ASSERT_TRUE(CheckPlayer("URL4", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_FALSE(factory->hasEvent());  // No events pending because nothing was playing
    ASSERT_FALSE(root->hasEvent());  // No events pending

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));

    ASSERT_TRUE(CheckDirty(root));   // No dirty properties
}

static const char * MISSING_COMPONENT = R"apl(
{
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
}
)apl";

/**
 * Try to speak something that simply doesn't exist
 */
TEST_F(SpeakItemAudioTest, MissingComponent)
{
    loadDocument(MISSING_COMPONENT);

    executeSpeakItem("myOtherText", kCommandScrollAlignCenter, kCommandHighlightModeBlock, 1000);
    // No events should be fired - there is nothing to speak
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

static const char *MISSING_SPEECH = R"apl(
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
}
)apl";

/**
 * Speak something without the speech property, but still available for scrolling.
 */
TEST_F(SpeakItemAudioTest, MissingSpeech)
{
    loadDocument(MISSING_SPEECH);
    auto container = component->getChildAt(0);
    auto child = container->getChildAt(1);

    executeSpeakItem("text2", kCommandScrollAlignFirst, kCommandHighlightModeBlock, 1000);
    ASSERT_FALSE(factory->hasEvent());

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
TEST_F(SpeakItemAudioTest, MissingSpeechNoDwell)
{
    loadDocument(MISSING_SPEECH);
    auto container = component->getChildAt(0);
    auto child = container->getChildAt(1);

    executeSpeakItem("text2", kCommandScrollAlignFirst, kCommandHighlightModeBlock, 0);
    ASSERT_FALSE(factory->hasEvent());

    // Now we scroll the world.
    advanceTime(1000);
    ASSERT_EQ(Point(0,200), component->scrollPosition());
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(root, component));

    // At this point nothing should be left - without a dwell time or speech, we don't get a change
    ASSERT_FALSE(root->hasEvent());   // No events pending
    ASSERT_TRUE(CheckDirty(root));
}

static const char * MISSING_SPEECH_AND_SCROLL = R"apl(
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
}
)apl";

/**
 * In this test the spoken item can't scroll and has no speech.  It can still be highlighted due to dwell time.
 */
TEST_F(SpeakItemAudioTest, MissingSpeechAndScroll)
{
    loadDocument(MISSING_SPEECH_AND_SCROLL);
    auto child = component->getChildAt(1);

    executeSpeakItem("text2", kCommandScrollAlignFirst, kCommandHighlightModeBlock, 1000);
    ASSERT_FALSE(factory->hasEvent());

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
TEST_F(SpeakItemAudioTest, MissingSpeechAndScrollNoDwell)
{
    loadDocument(MISSING_SPEECH_AND_SCROLL);
    auto child = component->getChildAt(1);

    executeSpeakItem("text2", kCommandScrollAlignFirst, kCommandHighlightModeBlock, 0);
    ASSERT_FALSE(factory->hasEvent());

    // Nothing should happen
    ASSERT_FALSE(root->hasEvent());   // No events pending
    ASSERT_TRUE(CheckDirty(root));
}

static const char *MISSING_SCROLL = R"apl(
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
}
)apl";

/**
 * In this example there is nothing to scroll, but we can still speak
 */
TEST_F(SpeakItemAudioTest, MissingScroll)
{
    factory->addFakeContent({
        {"URL1", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
        {"URL2", 200, 100, -1, {}}, // 200 ms duration, 100 ms initial delay
    });

    loadDocument(MISSING_SCROLL);
    auto child = component->getChildAt(1);

    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    executeSpeakItem("text2", kCommandScrollAlignFirst, kCommandHighlightModeBlock, 1000);
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // The color update happens immediately because there is no scrolling
    ASSERT_EQ(Object(Color(Color::BLUE)), child->getCalculated(kPropertyColor));
    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));

    // Move forward a bit in time and finish speaking
    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPlay));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    // We haven't passed the minimum dwell time, so no color change
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(CheckDirty(root));

    // Move forward past the minimum dwell time
    advanceTime(500);
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_TRUE(CheckDirty(child, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, child));
    ASSERT_EQ(Object(Color(Color::GREEN)), child->getCalculated(kPropertyColor));

    ASSERT_FALSE(root->hasEvent());
}



//   NEW ADDITIONS

static const char *BOSS_KARAOKE = R"apl({
  "type": "APL",
  "version": "2022.1",
  "theme": "dark",
  "styles": {
    "flip": {
      "values": [
        { "when": "${state.karaoke}", "color": "blue" },
        { "when": "${!state.karaoke}", "color": "white" },
        { "when": "${state.karaokeTarget}", "color": "yellow" }
      ]
    }
  },
  "onConfigChange": {
    "type": "Reinflate",
    "preservedSequencers": ["MAGIC"]
  },
  "mainTemplate": {
    "items": [
      {
        "type": "ScrollView",
        "width": 800,
        "height": 500,
        "id": "scroll",
        "item": {
          "type": "Container",
          "width": "100%",
          "direction": "column",
          "alignItems": "center",
          "items": [
            {
              "type": "Frame",
              "width": "100%",
              "height": 300,
              "opacity": 0.3,
              "alignSelf": "center",
              "backgroundColor": "purple"
            },
            {
              "type": "Text",
              "when": "${viewport.pixelWidth > 350}",
              "id": "text1",
              "style": "flip",
              "text": "Since <i>you</i> are not going <u>on a holiday this year Boss</u> I thought I should give your office a holiday look",
              "speech": "URL1",
              "textAlign": "center",
              "fontSize": "56dp",
              "width": "80%"
            },
            {
              "type": "Frame",
              "width": "80%",
              "height": 300,
              "opacity": 0.3,
              "alignSelf": "center",
              "backgroundColor": "purple"
            }
          ]
        }
      }
    ]
  }
})apl";

class FixedSizeMeasurement : public TextMeasurement {
public:
    // We'll assign a 10x10 block for each text square.
    virtual LayoutSize measure( Component *component,
                               float width,
                               MeasureMode widthMode,
                               float height,
                               MeasureMode heightMode ) override {
        return {640,351 };
    }

    virtual float baseline( Component *component, float width, float height ) override
    {
        return height;
    }
};

::testing::AssertionResult
verifyLineUpdate(RootContextPtr root, const ComponentPtr& target, float offset, int rangeStart, int rangeEnd) {
    auto event = root->popEvent();
    if (kEventTypeRequestLineBounds != event.getType())
        return ::testing::AssertionFailure()
               << "Wrong event type " << event.getType();
    auto rs = event.getValue(apl::kEventPropertyRangeStart).getInteger();
    if (rangeStart != rs)
        return ::testing::AssertionFailure()
               << "Wrong rangeStart " << rs;
    auto re = event.getValue(apl::kEventPropertyRangeEnd).getInteger();
    if (rangeEnd != re)
        return ::testing::AssertionFailure()
               << "Wrong rangeEnd " << re;

    if (target != event.getComponent()) {
        return ::testing::AssertionFailure()
               << "Wrong target.";
    }

    event.getActionRef().resolve({0, offset, 500, 70});

    // Results in highlight
    event = root->popEvent();
    if (kEventTypeLineHighlight != event.getType())
        return ::testing::AssertionFailure()
               << "Wrong event type " << event.getType();
    rs = event.getValue(apl::kEventPropertyRangeStart).getInteger();
    if (rangeStart != rs)
        return ::testing::AssertionFailure()
               << "Wrong rangeStart " << rs;
    re = event.getValue(apl::kEventPropertyRangeEnd).getInteger();
    if (rangeEnd != re)
        return ::testing::AssertionFailure()
               << "Wrong rangeEnd " << re;
    return ::testing::AssertionSuccess();
}

TEST_F(SpeakItemAudioTest, TransitionalRequests)
{
    // Limited subset of marks to avoid too much verification
    factory->addFakeContent({
        {"URL1", 3000, 100, -1,
         {
             {SpeechMarkType::kSpeechMarkWord, 0, 5, 0, "Since"},
             {SpeechMarkType::kSpeechMarkWord, 42, 46, 1300, "year"},
             {SpeechMarkType::kSpeechMarkWord, 64, 70, 1900, "should"},
             {SpeechMarkType::kSpeechMarkWord, 90, 97, 2600, "holiday"},
             {SpeechMarkType::kSpeechMarkWord, 98, 102, 2800, "look"}
         }
        }
    });

    config->measure(std::make_shared<FixedSizeMeasurement>());

    loadDocument(BOSS_KARAOKE);

    executeSpeakItem("text1", kCommandScrollAlignFirst, apl::kCommandHighlightModeLine, 1000);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Preroll scroll should be here. With rect request
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeRequestLineBounds, event.getType());
    auto textFieldBoundary = root->findComponentById("text1")->getCalculated(apl::kPropertyBounds).get<Rect>();
    event.getActionRef().resolve({0, 0, textFieldBoundary.getWidth(), 10});

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kReady));

    // Move forward a bit in time to finished scrolling.
    advanceTime(900);
    // Should start playing "Since you are not going"
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPlay));
    ASSERT_EQ(component->scrollPosition().getY(), textFieldBoundary.getY());

    auto text = root->findComponentById("text1");

    // Will also ask to scroll to the first line for play
    ASSERT_TRUE(verifyLineUpdate(root, text, 0, 0, 4));

    // "on a holiday this year"
    advanceTime(1300);
    ASSERT_TRUE(verifyLineUpdate(root, text, 70, 42, 45));

    // "Boss I thought I should"
    advanceTime(600);
    ASSERT_TRUE(verifyLineUpdate(root, text, 140, 64, 69));

    // "give your office a holiday"
    advanceTime(700);
    ASSERT_TRUE(verifyLineUpdate(root, text, 210, 90, 96));

    // "look"
    advanceTime(200);
    ASSERT_TRUE(verifyLineUpdate(root, text, 280, 98, 101));

    advanceTime(500);

    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    // Highlight clean
    event = root->popEvent();
    ASSERT_EQ(kEventTypeLineHighlight, event.getType());
    ASSERT_EQ(-1, event.getValue(apl::kEventPropertyRangeStart).getInteger());
    ASSERT_EQ(-1, event.getValue(apl::kEventPropertyRangeEnd).getInteger());
}

static const char* HOST_DOC = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "top",
      "item": {
        "type": "Host",
        "width": "100%",
        "height": "100%",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "SEND_EVENTER",
            "arguments": ["LOADED"]
          }
        ],
        "onFail": [
          {
            "type": "InsertItem",
            "sequencer": "SEND_EVENTER",
            "arguments": ["FAILED"]
          }
        ]
      }
    }
  }
})";

TEST_F(SpeakItemAudioTest, EmbeddedTestStages)
{
    factory->addFakeContent({
        {"URL1", 3000, 100, -1,
         {
             {SpeechMarkType::kSpeechMarkWord, 0, 5, 0, "Since"},
             {SpeechMarkType::kSpeechMarkWord, 42, 46, 1300, "year"},
             {SpeechMarkType::kSpeechMarkWord, 64, 70, 1900, "should"},
             {SpeechMarkType::kSpeechMarkWord, 90, 97, 2600, "holiday"},
             {SpeechMarkType::kSpeechMarkWord, 98, 102, 2800, "look"}
         }
        }
    });

    config->measure(std::make_shared<FixedSizeMeasurement>());

    std::shared_ptr<TestDocumentManager> documentManager = std::make_shared<TestDocumentManager>();

    config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));

    //////////////////////////////////////////////////////////////
    loadDocument(HOST_DOC);

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto content = Content::create(BOSS_KARAOKE, session);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    root->clearPending();
    root->clearDirty();

    //////////////////////////////////////////////////////////////

    rapidjson::Document commandDocument;
    commandDocument.Parse(R"([{
      "type": "SpeakItem",
      "componentId": "text1",
      "align": "first",
      "highlightMode": "line",
      "minimumDwellTime": 1000
    }])");

    embeddedDocumentContext->executeCommands(std::move(commandDocument), false);

    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Preroll scroll should be here. With rect request
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeRequestLineBounds, event.getType());
    ASSERT_EQ(embeddedDocumentContext, event.getDocument());
    auto textFieldBoundary = root->findComponentById("text1")->getCalculated(apl::kPropertyBounds).get<Rect>();
    event.getActionRef().resolve({0, 0, textFieldBoundary.getWidth(), 10});

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kReady));

    // Move forward a bit in time to finished scrolling.
    advanceTime(900);
    // Should start playing "Since you are not going"
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPlay));
    ASSERT_EQ(root->findComponentById("scroll")->scrollPosition().getY(), textFieldBoundary.getY());

    auto text = root->findComponentById("text1");

    // Will also ask to scroll to the first line for play
    ASSERT_TRUE(verifyLineUpdate(root, text, 0, 0, 4));

    // "on a holiday this year"
    advanceTime(1300);
    ASSERT_TRUE(verifyLineUpdate(root, text, 70, 42, 45));

    // "Boss I thought I should"
    advanceTime(600);
    ASSERT_TRUE(verifyLineUpdate(root, text, 140, 64, 69));

    // "give your office a holiday"
    advanceTime(700);
    ASSERT_TRUE(verifyLineUpdate(root, text, 210, 90, 96));

    // "look"
    advanceTime(200);
    ASSERT_TRUE(verifyLineUpdate(root, text, 280, 98, 101));

    advanceTime(500);

    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    // Highlight clean
    event = root->popEvent();
    ASSERT_EQ(kEventTypeLineHighlight, event.getType());
    ASSERT_EQ(embeddedDocumentContext, event.getDocument());
    ASSERT_EQ(-1, event.getValue(apl::kEventPropertyRangeStart).getInteger());
    ASSERT_EQ(-1, event.getValue(apl::kEventPropertyRangeEnd).getInteger());
}

TEST_F(SpeakItemAudioTest, LineRequestTerminated)
{
    // Limited subset of marks to avoid too much verification
    factory->addFakeContent({
        {"URL1", 3000, 100, -1,
         {
             {SpeechMarkType::kSpeechMarkWord, 0, 5, 0, "Since"},
             {SpeechMarkType::kSpeechMarkWord, 42, 46, 1300, "year"},
             {SpeechMarkType::kSpeechMarkWord, 64, 70, 1900, "should"},
             {SpeechMarkType::kSpeechMarkWord, 90, 97, 2600, "holiday"},
             {SpeechMarkType::kSpeechMarkWord, 98, 102, 2800, "look"}
         }
        }
    });

    config->measure(std::make_shared<FixedSizeMeasurement>());

    loadDocument(BOSS_KARAOKE);

    auto commands = executeSpeakItem("text1", kCommandScrollAlignFirst, apl::kCommandHighlightModeLine, 1000);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Preroll scroll should be here. With rect request
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeRequestLineBounds, event.getType());
    root->cancelExecution();

    ASSERT_TRUE(commands->isTerminated());
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());  // No events pending because nothing was playing

    // Highlight clean
    event = root->popEvent();
    ASSERT_EQ(kEventTypeLineHighlight, event.getType());
    ASSERT_EQ(-1, event.getValue(apl::kEventPropertyRangeStart).getInteger());
    ASSERT_EQ(-1, event.getValue(apl::kEventPropertyRangeEnd).getInteger());
}


TEST_F(SpeakItemAudioTest, PreserveTesting)
{
    // Limited subset of marks to avoid too much verification
    factory->addFakeContent({
        {"URL1", 3000, 100, -1,
         {
             {SpeechMarkType::kSpeechMarkWord, 0, 5, 0, "Since"},
             {SpeechMarkType::kSpeechMarkWord, 42, 46, 1300, "year"},
             {SpeechMarkType::kSpeechMarkWord, 64, 70, 1900, "should"},
             {SpeechMarkType::kSpeechMarkWord, 90, 97, 2600, "holiday"},
             {SpeechMarkType::kSpeechMarkWord, 98, 102, 2800, "look"}
         }
        }
    });

    config->measure(std::make_shared<FixedSizeMeasurement>());

    loadDocument(BOSS_KARAOKE);

    auto action = executeSpeakItem("text1", kCommandScrollAlignFirst, apl::kCommandHighlightModeLine, 1000, "MAGIC");
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Preroll scroll should be here. With rect request
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeRequestLineBounds, event.getType());
    auto textFieldBoundary = root->findComponentById("text1")->getCalculated(apl::kPropertyBounds).get<Rect>();
    event.getActionRef().resolve({0, 0, textFieldBoundary.getWidth(), 10});

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kReady));

    // Move forward a bit in time to finished scrolling.
    advanceTime(900);
    // Should start playing "Since you are not going"
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPlay));
    ASSERT_EQ(component->scrollPosition().getY(), textFieldBoundary.getY());

    auto text = CoreComponent::cast(root->findComponentById("text1"));

    // Will also ask to scroll to the first line for play
    ASSERT_TRUE(verifyLineUpdate(root, text, 0, 0, 4));

    // "on a holiday this year"
    advanceTime(1300);
    ASSERT_TRUE(verifyLineUpdate(root, text, 70, 42, 45));

    // "Boss I thought I should"
    advanceTime(600);
    ASSERT_TRUE(verifyLineUpdate(root, text, 140, 64, 69));

    auto playerTimer = factory->getPlayers().at(0).lock()->getTimeoutId();
    loop->freeze(playerTimer);

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    loop->rehydrate(playerTimer);

    text = CoreComponent::cast(root->findComponentById("text1"));
    ASSERT_TRUE(text->getState().get(apl::kStateKaraoke));

    // "give your office a holiday"
    advanceTime(700);
    ASSERT_TRUE(verifyLineUpdate(root, text, 210, 90, 96));

    // "look"
    advanceTime(200);
    ASSERT_TRUE(verifyLineUpdate(root, text, 280, 98, 101));

    advanceTime(500);

    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    // Highlight clean
    event = root->popEvent();
    ASSERT_EQ(kEventTypeLineHighlight, event.getType());
    ASSERT_EQ(-1, event.getValue(apl::kEventPropertyRangeStart).getInteger());
    ASSERT_EQ(-1, event.getValue(apl::kEventPropertyRangeEnd).getInteger());
}

TEST_F(SpeakItemAudioTest, PreserveTestingNoTarget)
{
    // Limited subset of marks to avoid too much verification
    factory->addFakeContent({
        {"URL1", 3000, 100, -1,
         {
             {SpeechMarkType::kSpeechMarkWord, 0, 5, 0, "Since"},
             {SpeechMarkType::kSpeechMarkWord, 42, 46, 1300, "year"},
             {SpeechMarkType::kSpeechMarkWord, 64, 70, 1900, "should"},
             {SpeechMarkType::kSpeechMarkWord, 90, 97, 2600, "holiday"},
             {SpeechMarkType::kSpeechMarkWord, 98, 102, 2800, "look"}
         }
        }
    });

    config->measure(std::make_shared<FixedSizeMeasurement>());

    loadDocument(BOSS_KARAOKE);

    auto action = executeSpeakItem("text1", kCommandScrollAlignFirst, apl::kCommandHighlightModeLine, 1000, "MAGIC");
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Preroll scroll should be here. With rect request
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeRequestLineBounds, event.getType());
    auto textFieldBoundary = root->findComponentById("text1")->getCalculated(apl::kPropertyBounds).get<Rect>();
    event.getActionRef().resolve({0, 0, textFieldBoundary.getWidth(), 10});

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kReady));

    // Move forward a bit in time to finished scrolling.
    advanceTime(900);
    // Should start playing "Since you are not going"
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPlay));
    ASSERT_EQ(component->scrollPosition().getY(), textFieldBoundary.getY());

    auto text = CoreComponent::cast(root->findComponentById("text1"));

    // Will also ask to scroll to the first line for play
    ASSERT_TRUE(verifyLineUpdate(root, text, 0, 0, 4));

    // "on a holiday this year"
    advanceTime(1300);
    ASSERT_TRUE(verifyLineUpdate(root, text, 70, 42, 45));

    // "Boss I thought I should"
    advanceTime(600);
    ASSERT_TRUE(verifyLineUpdate(root, text, 140, 64, 69));

    auto playerTimer = factory->getPlayers().at(0).lock()->getTimeoutId();
    loop->freeze(playerTimer);

    configChange(ConfigurationChange(300,1000));
    processReinflate();

    loop->rehydrate(playerTimer);

    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

static const char *SPEECH_MARK_HANDLER = R"apl({
  "type": "APL",
  "version": "2022.2",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "width": 400,
        "height": 400,
        "id": "root",
        "speech": "URL1",
        "onSpeechMark": {
          "type": "SendEvent",
          "sequencer": "SPEAK",
          "arguments": [
            "TEST",
            "${event.source.source}",
            "${event.source.handler}",
            "${event.source.id}",
            "${event.source.value}",
            "${event.markType}",
            "${event.markTime}",
            "${event.markValue}"
          ]
        }
      }
    ]
  }
}
)apl";

TEST_F(SpeakItemAudioTest, SpeechMarkHandler)
{
    // Limited subset of marks to avoid too much verification
    factory->addFakeContent({
        {"URL1", 2500, 100, -1,
         {
             {SpeechMarkType::kSpeechMarkWord, 0, 5, 500, "uno"},
             {SpeechMarkType::kSpeechMarkSSML, 42, 46, 1000, "dos"},
             {SpeechMarkType::kSpeechMarkWord, 42, 46, 1250, "tres"},
             {SpeechMarkType::kSpeechMarkSentence, 64, 70, 1500, "I am a sentence"},
             {SpeechMarkType::kSpeechMarkViseme, 90, 97, 2000, "V"}
         }
        }
    });

    loadDocument(SPEECH_MARK_HANDLER);

    executeSpeakItem("root", kCommandScrollAlignFirst, apl::kCommandHighlightModeLine, 1000);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPlay));

    advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Container", "SpeechMark", "root", Object::NULL_OBJECT(), "word", 500, "uno"));

    advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Container", "SpeechMark", "root", Object::NULL_OBJECT(), "ssml", 1000, "dos"));

    advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Container", "SpeechMark", "root", Object::NULL_OBJECT(), "word", 1250, "tres"));
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Container", "SpeechMark", "root", Object::NULL_OBJECT(), "sentence", 1500, "I am a sentence"));

    advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Container", "SpeechMark", "root", Object::NULL_OBJECT(), "viseme", 2000, "V"));

    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());
}

static const char *BOSS_KARAOKE_WITH_HANDLER = R"apl({
  "type": "APL",
  "version": "2022.1",
  "theme": "dark",
  "styles": {
    "flip": {
      "values": [
        { "when": "${state.karaoke}", "color": "blue" },
        { "when": "${!state.karaoke}", "color": "white" },
        { "when": "${state.karaokeTarget}", "color": "yellow" }
      ]
    }
  },
  "onConfigChange": {
    "type": "Reinflate",
    "preservedSequencers": ["MAGIC"]
  },
  "mainTemplate": {
    "items": [
      {
        "type": "ScrollView",
        "width": 800,
        "height": 500,
        "id": "scroll",
        "item": {
          "type": "Container",
          "width": "100%",
          "direction": "column",
          "alignItems": "center",
          "items": [
            {
              "type": "Frame",
              "width": "100%",
              "height": 300,
              "opacity": 0.3,
              "alignSelf": "center",
              "backgroundColor": "purple"
            },
            {
              "type": "Text",
              "when": "${viewport.pixelWidth > 350}",
              "id": "text1",
              "style": "flip",
              "text": "Since <i>you</i> are not going <u>on a holiday this year Boss</u> I thought I should give your office a holiday look",
              "speech": "URL1",
              "textAlign": "center",
              "fontSize": "56dp",
              "width": "80%",
              "onSpeechMark": {
                "type": "SendEvent",
                  "sequencer": "SPEAK",
                  "arguments": [
                    "TEST",
                    "${event.source.source}",
                    "${event.source.handler}",
                    "${event.source.id}",
                    "${event.markType}",
                    "${event.markTime}",
                    "${event.markValue}"
                  ]
                }
            },
            {
              "type": "Frame",
              "width": "80%",
              "height": 300,
              "opacity": 0.3,
              "alignSelf": "center",
              "backgroundColor": "purple"
            }
          ]
        }
      }
    ]
  }
})apl";

TEST_F(SpeakItemAudioTest, MarksAfterText)
{
    // Limited subset of marks to avoid too much verification
    factory->addFakeContent({
        {"URL1", 3600, 100, -1,
         {
             {SpeechMarkType::kSpeechMarkWord, 0, 5, 0, "Since"},
             {SpeechMarkType::kSpeechMarkWord, 42, 46, 1300, "year"},
             {SpeechMarkType::kSpeechMarkWord, 64, 70, 1900, "should"},
             {SpeechMarkType::kSpeechMarkWord, 90, 97, 2600, "holiday"},
             {SpeechMarkType::kSpeechMarkWord, 98, 102, 2800, "look"},
             {SpeechMarkType::kSpeechMarkSSML, 0, 0, 3500, "potato"}
         }
        }
    });

    config->measure(std::make_shared<FixedSizeMeasurement>());

    loadDocument(BOSS_KARAOKE_WITH_HANDLER);

    executeSpeakItem("text1", kCommandScrollAlignFirst, apl::kCommandHighlightModeLine, 1000);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Preroll scroll should be here. With rect request
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeRequestLineBounds, event.getType());
    auto textFieldBoundary = root->findComponentById("text1")->getCalculated(apl::kPropertyBounds).get<Rect>();
    event.getActionRef().resolve({0, 0, textFieldBoundary.getWidth(), 10});

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kReady));

    // Move forward a bit in time to finished scrolling.
    advanceTime(900);
    // Should start playing "Since you are not going"
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPlay));
    ASSERT_EQ(component->scrollPosition().getY(), textFieldBoundary.getY());

    auto text = root->findComponentById("text1");

    // Will also ask to scroll to the first line for play
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Text", "SpeechMark", "text1", "word", 0, "Since"));
    ASSERT_TRUE(verifyLineUpdate(root, text, 0, 0, 4));

    // "on a holiday this year"
    advanceTime(1300);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Text", "SpeechMark", "text1", "word", 1300, "year"));
    ASSERT_TRUE(verifyLineUpdate(root, text, 70, 42, 45));

    // "Boss I thought I should"
    advanceTime(600);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Text", "SpeechMark", "text1", "word", 1900, "should"));
    ASSERT_TRUE(verifyLineUpdate(root, text, 140, 64, 69));

    // "give your office a holiday"
    advanceTime(700);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Text", "SpeechMark", "text1", "word", 2600, "holiday"));
    ASSERT_TRUE(verifyLineUpdate(root, text, 210, 90, 96));

    // "look"
    advanceTime(200);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Text", "SpeechMark", "text1", "word", 2800, "look"));
    ASSERT_TRUE(verifyLineUpdate(root, text, 280, 98, 101));

    advanceTime(700);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Text", "SpeechMark", "text1", "ssml", 3500, "potato"));

    advanceTime(100);

    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    // Highlight clean
    event = root->popEvent();
    ASSERT_EQ(kEventTypeLineHighlight, event.getType());
    ASSERT_EQ(-1, event.getValue(apl::kEventPropertyRangeStart).getInteger());
    ASSERT_EQ(-1, event.getValue(apl::kEventPropertyRangeEnd).getInteger());
}
