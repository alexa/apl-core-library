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

#include "testmediaplayerfactory.h"

using namespace apl;

class MediaCommandPreservationTest : public DocumentWrapper {
public:
    MediaCommandPreservationTest() : DocumentWrapper() {
        config->enableExperimentalFeature(RootConfig::kExperimentalFeatureManageMediaRequests);
        mediaPlayerFactory = std::make_shared<TestMediaPlayerFactory>();
        config->mediaPlayerFactory(mediaPlayerFactory);
    }

    std::shared_ptr<TestMediaPlayerFactory> mediaPlayerFactory;
};

static const char *TEST_ENGINE = R"apl({
  "type": "APL",
  "version": "2022.1",
  "onConfigChange": {
    "type": "Reinflate",
    "preservedSequencers": ["MAGIC"]
  },
  "commands": {
    "DUMP": {
      "command": {
        "type": "SendEvent",
        "sequencer": "FOO",
        "arguments": [
          "${event.source.handler} ${event.source.url} ${event.currentTime}/${event.ended ? 'E' : ''}${event.paused ? 'P' : ''}"
        ]
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Video",
      "preserve": ["source", "playingState"],
      "when": "${viewport.pixelWidth > 350}",
      "id": "MyVideo",
      "onEnd":         { "type": "DUMP" },
      "onPause":       { "type": "DUMP" },
      "onPlay":        { "type": "DUMP" },
      "onTimeUpdate":  { "type": "DUMP" },
      "onTrackUpdate": { "type": "DUMP" },
      "onTrackReady":  { "type": "DUMP" },
      "onTrackFail":   { "type": "DUMP" }
    }
  }
})apl";

TEST_F(MediaCommandPreservationTest, PlaybackPreserve)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 0, -1},
    });

    loadDocument(TEST_ENGINE);
    ASSERT_TRUE(component);

    executeCommand("PlayMedia", {{"sequencer", "MAGIC"}, {"componentId", "MyVideo"}, {"source", "track1"}}, false);

    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));

    mediaPlayerFactory->advanceTime(500);
    advanceTime(500);

    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track1 500/"));

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    mediaPlayerFactory->advanceTime(400);
    advanceTime(400);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track1 900/"));

    mediaPlayerFactory->advanceTime(100);
    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "End track1 1000/EP"));
}

TEST_F(MediaCommandPreservationTest, PlaybackPreserveNoTargetComponent)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 0, -1},
    });

    loadDocument(TEST_ENGINE);
    ASSERT_TRUE(component);

    executeCommand("PlayMedia", {{"sequencer", "MAGIC"}, {"componentId", "MyVideo"}, {"source", "track1"}}, false);

    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));

    mediaPlayerFactory->advanceTime(500);
    advanceTime(500);

    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track1 500/"));

    configChange(ConfigurationChange(300,1000));
    processReinflate();

    mediaPlayerFactory->advanceTime(400);
    advanceTime(400);
    ASSERT_FALSE(root->hasEvent());

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

static const char *TEST_ENGINE_NO_PROPS = R"apl({
  "type": "APL",
  "version": "2022.1",
  "onConfigChange": {
    "type": "Reinflate",
    "preservedSequencers": ["MAGIC"]
  },
  "commands": {
    "DUMP": {
      "command": {
        "type": "SendEvent",
        "sequencer": "FOO",
        "arguments": [
          "${event.source.handler} ${event.source.url} ${event.currentTime}/${event.ended ? 'E' : ''}${event.paused ? 'P' : ''}"
        ]
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Video",
      "when": "${viewport.pixelWidth > 350}",
      "id": "MyVideo",
      "onEnd":         { "type": "DUMP" },
      "onPause":       { "type": "DUMP" },
      "onPlay":        { "type": "DUMP" },
      "onTimeUpdate":  { "type": "DUMP" },
      "onTrackUpdate": { "type": "DUMP" },
      "onTrackReady":  { "type": "DUMP" },
      "onTrackFail":   { "type": "DUMP" }
    }
  }
})apl";

TEST_F(MediaCommandPreservationTest, PlaybackPreserveNoProps)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 0, -1},
    });

    loadDocument(TEST_ENGINE_NO_PROPS);
    ASSERT_TRUE(component);

    executeCommand("PlayMedia", {{"sequencer", "MAGIC"}, {"componentId", "MyVideo"}, {"source", "track1"}}, false);

    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));

    mediaPlayerFactory->advanceTime(500);
    advanceTime(500);

    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track1 500/"));

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    mediaPlayerFactory->advanceTime(400);
    advanceTime(400);
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(session->checkAndClear());
}