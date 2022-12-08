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
#include "audiotest.h"

using namespace apl;

class CommandPageAudioTest : public AudioTest {
public:
    ActionPtr executeSetPage(const std::string& component, const std::string& position, int value) {
        return executeCommand(
            "SetPage", {{"componentId", component}, {"position", position}, {"value", value}},
            false);
    }

    ActionPtr executeAutoPage(const std::string& component, int count, int duration) {
        return executeCommand(
            "AutoPage", {{"componentId", component}, {"count", count}, {"duration", duration}},
            false);
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

        auto actualBounds = child->getCalculated(kPropertyBounds).get<Rect>();
        if (bounds != actualBounds) {
            return ::testing::AssertionFailure()
                    << "child " << idx
                    << " bounds is wrong. Expected: " << bounds.toString()
                    << ", actual: " << actualBounds.toString();
        }

        return ::testing::AssertionSuccess();
    }
};


static const char *COMBINATION = R"({
  "type": "APL",
  "version": "1.6",
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
      "type": "SpeakItem",
      "componentId": "shooshSpeechId"
    },
    {
      "type": "SpeakItem",
      "componentId": "showingBoxValueSpeechId"
    }
  ]
}])";

TEST_F(CommandPageAudioTest, SpeakItemCombination)
{
    factory->addFakeContent({
        {"https://iamspeech.com/1.mp3", 500, 50, -1, {}}, // 1000 ms duration, 100 ms initial delay
        {"https://iamspeech.com/2.mp3", 1000, 100, -1, {}}, // 1000 ms duration, 100 ms initial delay
    });
    config->set(RootProperty::kDefaultPagerAnimationDuration, 500);

    loadDocument(COMBINATION, COMBINATION_DATA);
    clearDirty();
    ASSERT_TRUE(CheckDirty(root));

    auto container1 = component->getChildAt(0);
    auto container2 = component->getChildAt(1);

    ASSERT_EQ(0, component->pagePosition());
    rapidjson::Document  doc;
    doc.Parse(COMBINATION_COMMANDS);
    auto action = root->executeCommands(apl::Object(doc), false);

    // Should have preroll for first speech
    ASSERT_TRUE(CheckPlayer("https://iamspeech.com/2.mp3", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_TRUE(action->isPending());

    // The page starts animating here....but it is still on page 0
    ASSERT_EQ(0, component->pagePosition());
    clearDirty();   // Just about everything is dirty here because we bring up a new page

    // After 100 ms the audio should start playing, but the pager is still animating
    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("https://iamspeech.com/2.mp3", TestAudioPlayer::kReady));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(container1, kPropertyTransform));
    ASSERT_TRUE(CheckDirty(container2, kPropertyTransform));
    ASSERT_TRUE(CheckDirty(root, component, container1, container2));

    // After 400 ms the pager should be done and the audio starts
    advanceTime(400);
    ASSERT_TRUE(CheckPlayer("https://iamspeech.com/2.mp3", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(1, component->pagePosition());

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(container1, kPropertyTransform));
    ASSERT_TRUE(CheckDirty(container2, kPropertyTransform));
    ASSERT_TRUE(CheckDirty(root, component, container1, container2));

    // Nothing happens in the next 900 ms
    advanceTime(900);
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_TRUE(CheckDirty(root));

    // After another 1000 ms the audio has finished and the new speak item should start
    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("https://iamspeech.com/2.mp3", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("https://iamspeech.com/2.mp3", TestAudioPlayer::kRelease));
    ASSERT_TRUE(CheckPlayer("https://iamspeech.com/1.mp3", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_TRUE(CheckDirty(root));    // Nothing moves

    // 50 ms for preroll
    advanceTime(50);
    ASSERT_TRUE(CheckPlayer("https://iamspeech.com/1.mp3", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("https://iamspeech.com/1.mp3", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_TRUE(CheckDirty(root));    // Nothing moves
    ASSERT_FALSE(action->isResolved());

    // 500 ms for playback
    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("https://iamspeech.com/1.mp3", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("https://iamspeech.com/1.mp3", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_TRUE(CheckDirty(root));    // Nothing moves

    // The entire action has finished
    ASSERT_TRUE(action->isResolved());
}
