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
#include "apl/content/jsondata.h"
#include "apl/time/sequencer.h"

using namespace apl;

class SequencerAudioTest : public AudioTest {
public:
    ActionPtr execute(const std::string& cmds, bool fastMode) {
        command.Parse(cmds.c_str());
        return root->executeCommands(command, fastMode);
    }
};


static const char *SPEAK_ITEM_AND_VIDEO = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": [
        {
          "type": "Text",
          "id": "text",
          "text": "Some text to say, really.",
          "speech": "URL3"
        },
        {
          "type": "Video",
          "id": "video",
          "source": ["URL1", "URL2"]
        }
      ]
    }
  }
})";

static const char *SPEAK_ITEM = R"([
{
  "type": "SpeakItem",
  "componentId": "text",
  "highlightMode": "block",
  "align": "center",
  "sequencer": "secondary"
}
])";

static const char *PLAY_MEDIA_FOREGROUND = R"([
  {
    "type": "PlayMedia",
    "componentId": "video",
    "source": "http://music.amazon.com/s3/MAGIC_TRACK_HERE",
    "audioTrack": "foreground",
    "sequencer": "tertiary"
  }
])";

TEST_F(SequencerAudioTest, SpeakItemAndPlayMediaForeground)
{
    factory->addFakeContent({
        {"URL3", 2000, 100, -1, {}}, // 2000 ms duration, 100 ms initial delay
    });

    loadDocument(SPEAK_ITEM_AND_VIDEO);

    execute(SPEAK_ITEM, false);

    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());

    // Finish pre-roll and start playback
    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());

    // Now introduce the PlayMedia command
    execute(PLAY_MEDIA_FOREGROUND, false);

    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kPause));
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypePlayMedia, event.getType());
    auto playMedia = event.getActionRef();

    loop->advanceToEnd();

    ASSERT_TRUE(playMedia.isPending());
}


static const char *SCROLLABLE_SPEAK_ITEM = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "ScrollView",
      "id": "scroll",
      "height": "10dp",
      "item": [
        {
          "type": "Text",
          "id": "text",
          "height": "100dp",
          "text": "Some text to say, really.",
          "speech": "URL3"
        }
      ]
    }
  }
})";

static const char *SCROLL_TO_POSITION = R"([
  {
    "type": "Scroll",
    "componentId": "scroll",
    "distance": 1,
    "sequencer": "tertiary"
  }
])";

TEST_F(SequencerAudioTest, SpeakItemAndScroll)
{
    factory->addFakeContent({
        {"URL3", 2000, 100, -1, {}}, // 2000 ms duration, 100 ms initial delay
    });

    config->set(RootProperty::kScrollCommandDuration, 1000);
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(SCROLLABLE_SPEAK_ITEM);

    execute(SPEAK_ITEM, false);

    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Move forward a bit.  The scroll should be half-done; the audio hasn't started yet
    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kReady));
    ASSERT_FALSE(factory->hasEvent());

    auto bounds = component->getCalculated(kPropertyBounds).getRect();
    auto childBounds = component->getChildAt(0)->getCalculated(kPropertyBounds).getRect();
    auto position = Point(0, (childBounds.getCenterY() - bounds.getCenterY()) / 2);
    ASSERT_EQ(position, component->scrollPosition());

    // Starting a scroll now will kill the SpeakItemAction because it is still scrolling
    execute(SCROLL_TO_POSITION, false);
    ASSERT_TRUE(CheckPlayer("URL3", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());  // The audio was not playing

    advanceTime(1000);
    position += Point(0, bounds.getHeight());
    ASSERT_EQ(position, component->scrollPosition());
    ASSERT_FALSE(factory->hasEvent());  // Still no audio playing - the SpeakItemAction was killed
    ASSERT_FALSE(root->hasEvent());
}


static const char * PAGER_1_3 = R"({
  "type": "APL",
  "version": "1.3",
  "mainTemplate": {
    "items": [
      {
        "type": "Pager",
        "id": "aPager",
        "items": [
          {
            "type": "Text",
            "id": "text1",
            "text": "Page 1",
            "speech": "URL1"
          },
          {
            "type": "Text",
            "id": "text2",
            "text": "Page2",
            "speech": "URL2"
          }
        ]
      }
    ]
  }
})";


static const char * PAGER_1_3_CMD = R"([{
  "type": "Parallel",
  "commands": [
    {
      "type": "SpeakItem",
      "componentId": "text2"
    },
    {
      "type": "SetPage",
      "componentId": "aPager",
      "position": "absolute",
      "value": 2
    }
  ]
}])";


TEST_F(SequencerAudioTest, Pager_1_3)
{
    factory->addFakeContent({
        {"URL1", 1000, 50, -1, {}}, // 50 ms initial delay, 1000 ms playback
        {"URL2", 1000, 50, -1, {}}, // 50 ms initial delay, 1000 ms playback
    });
    config->set(RootProperty::kDefaultPagerAnimationDuration, 500);

    loadDocument(PAGER_1_3);

    auto ref = execute(PAGER_1_3_CMD, false);

    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // The SetPage runs at the same time as the Speakitem
    // Move forward the page.  The speach has not finished yet
    advanceTime(500);
    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    // Speech finishes a little later
    advanceTime(550);
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL2", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_TRUE(ref->isResolved());
}
