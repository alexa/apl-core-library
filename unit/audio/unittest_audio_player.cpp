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

using namespace apl;

class AudioPlayerTest : public AudioTest {};

static const char *BASIC = R"apl(
{
  "type": "APL",
  "version": "1.8",
  "styles": {
    "TextStyle": {
      "values": [
        {
          "color": "blue"
        },
        {
          "when": "${state.karaoke}",
          "color": "red"
        }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "TEXT",
      "style": "TextStyle",
      "speech": "http://foo.com",
      "text": "Fuzzy duck"
    }
  }
}
)apl";

TEST_F(AudioPlayerTest, Basic)
{
    factory->addFakeContent({
        { "http://foo.com", 1000, 1000, -1, {} }   // 1000 ms long, 1000 ms buffer delay
    });

    loadDocument(BASIC);
    ASSERT_TRUE(component);

    // Execute SpeakItem
    executeCommand("SpeakItem", {{"componentId", "TEXT"}}, false);

    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(500);
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(500);
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(500);
    ASSERT_FALSE(factory->hasEvent());
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());
}

TEST_F(AudioPlayerTest, BasicWithMinimumTime)
{
    factory->addFakeContent({
        { "http://foo.com", 200, 100, -1, {} }   // 200 ms long, 100 ms buffer delay
    });

    loadDocument(BASIC);
    ASSERT_TRUE(component);

    // Execute speak item with a minimum dweel time
    auto ref = executeCommand("SpeakItem", {{"componentId", "TEXT"}, {"minimumDwellTime", 1000}}, false);

    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(200);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_TRUE(ref->isPending());
    advanceTime(1000);
    ASSERT_TRUE(ref->isResolved());
}

TEST_F(AudioPlayerTest, BasicWithFailure)
{
    factory->addFakeContent({
        { "http://foo.com", 2000, 100, 100, {} }   // 2 seconds long, 100 ms loading, fail after 100 ms
    });

    loadDocument(BASIC);
    ASSERT_TRUE(component);

    // Execute speak item with a minimum dweel time
    auto ref = executeCommand("SpeakItem", {{"componentId", "TEXT"}}, false);

    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kFail));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_TRUE(ref->isResolved());
}

TEST_F(AudioPlayerTest, BasicWithFailureMinimumTime)
{
    factory->addFakeContent({
        { "http://foo.com", 2000, 100, 100, {} }   // 2 seconds long, 100 ms loading, fail after 100 ms
    });

    loadDocument(BASIC);
    ASSERT_TRUE(component);

    // Execute speak item with a minimum dweel time
    auto ref = executeCommand("SpeakItem", {{"componentId", "TEXT"}, {"minimumDwellTime", 1000}}, false);

    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kFail));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_TRUE(ref->isPending());
    advanceTime(1000);
    ASSERT_TRUE(ref->isResolved());
}

