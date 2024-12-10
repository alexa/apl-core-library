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

class CommandMediaTest : public CommandTest {
public:
    CommandMediaTest() : CommandTest() {
        mediaPlayerFactory->addFakeContent({
            { "URL1", 1000, 0, -1 },
            { "URL2", 1000, 0, -1 },
            { "URL3", 1000, 0, -1 }
        });
    }

    ActionPtr executeControlMedia(const std::string& component, const std::string& command, int value, bool fastMode) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "ControlMedia", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("command", rapidjson::Value(command.c_str(), alloc).Move(), alloc);
        cmd.AddMember("value", value, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        return executeCommands(doc, fastMode);
    }

    ActionPtr executePlayMedia(const std::string& component, const std::string& audioTrack, const Object& source, bool fastMode) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "PlayMedia", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("audioTrack", rapidjson::Value(audioTrack.c_str(), alloc).Move(), alloc);
        cmd.AddMember("source", source.serialize(alloc).Move(), alloc);
        doc.SetArray().PushBack(cmd, alloc);
        return executeCommands(doc, fastMode);
    }

    rapidjson::Document doc;
};

static const char *VIDEO = R"apl({
  "type": "APL",
  "version": "2024.3",
  "commands": {
    "DUMP": {
      "command": {
        "type": "SendEvent",
        "sequencer": "FOO",
        "arguments": [
          "Handler: ${event.source.handler}",
          "URL: ${event.source.url}",
          "Position: ${event.source.currentTime} (${event.currentTime})",
          "Duration: ${event.source.duration} (${event.duration})",
          "Ended: ${event.source.ended ? 'YES' : 'NO'} (${event.ended ? 'YES' : 'NO'})",
          "Paused: ${event.source.paused ? 'YES' : 'NO'} (${event.paused ? 'YES' : 'NO'})",
          "Muted: ${event.source.muted ? 'YES' : 'NO'} (${event.muted ? 'YES' : 'NO'})",
          "TrackCount: ${event.source.trackCount} (${event.trackCount})",
          "TrackIndex: ${event.source.trackIndex} (${event.trackIndex})",
          "TrackState: ${event.source.trackState} (${event.trackState})"
        ]
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": [
        {
          "type": "Video",
          "id": "myVideo",
          "width": 100,
          "height": 100,
          "source": ["URL1", "URL2", "URL3"],
          "onEnd":         { "type": "DUMP" },
          "onPause":       { "type": "DUMP" },
          "onPlay":        { "type": "DUMP" },
          "onTimeUpdate":  { "type": "DUMP" },
          "onTrackUpdate": { "type": "DUMP" },
          "onTrackReady":  { "type": "DUMP" },
          "onTrackFail":   { "type": "DUMP" }
        },
        {
          "type": "Video",
          "id": "myVideo3",
          "width": 100,
          "height": 100,
          "source": "URL1",
          "onEnd":         { "type": "DUMP" },
          "onPause":       { "type": "DUMP" },
          "onPlay":        { "type": "DUMP" },
          "onTimeUpdate":  { "type": "DUMP" },
          "onTrackUpdate": { "type": "DUMP" },
          "onTrackReady":  { "type": "DUMP" },
          "onTrackFail":   { "type": "DUMP" }
        }
      ]
    }
  }
})apl";

TEST_F(CommandMediaTest, Control)
{
    loadDocument(VIDEO);

    // Preloads
    mediaPlayerFactory->advanceTime(100);
    clearEvents();

    // Play in normal mode
    executeControlMedia("myVideo", "play", 0, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: URL1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    // Advance playback a bit
    mediaPlayerFactory->advanceTime(100);
    clearEvents();

    // Play in fast mode ignored
    ASSERT_FALSE(ConsoleMessage());
    executeControlMedia("myVideo", "play", 0, true);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    // Pause in normal mode
    executeControlMedia("myVideo", "pause", 0, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Pause",
                               "URL: URL1",
                               "Position: 100 (100)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    clearEvents();

    // Pause in fast mode
    executeControlMedia("myVideo", "play", 0, false);
    clearEvents();
    executeControlMedia("myVideo", "pause", 0, true);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Pause",
                               "URL: URL1",
                               "Position: 100 (100)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    clearEvents();

    // Next in normal mode
    executeControlMedia("myVideo", "next", 0, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TrackUpdate",
                               "URL: URL2",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 1 (1)",
                               "TrackState: ready (ready)"));
    clearEvents();

    // Next in fast mode
    executeControlMedia("myVideo", "next", 0, true);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TrackUpdate",
                               "URL: URL3",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 2 (2)",
                               "TrackState: ready (ready)"));
    clearEvents();

    // Previous in normal mode
    executeControlMedia("myVideo", "previous", 0, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TrackUpdate",
                               "URL: URL2",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 1 (1)",
                               "TrackState: ready (ready)"));
    clearEvents();

    // Previous in fast mode
    executeControlMedia("myVideo", "previous", 0, true);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TrackUpdate",
                               "URL: URL1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    clearEvents();

    // Rewind in normal mode
    executeControlMedia("myVideo", "rewind", 0, false);
    executeControlMedia("myVideo", "play", 0, false);
    mediaPlayerFactory->advanceTime(150);
    clearEvents();

    executeControlMedia("myVideo", "pause", 0, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Pause",
                               "URL: URL1",
                               "Position: 150 (150)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    clearEvents();

    // Rewind in fast mode
    executeControlMedia("myVideo", "rewind", 0, true);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: URL1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    clearEvents();

    // Seek in normal mode
    executeControlMedia("myVideo", "seek", 70, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: URL1",
                               "Position: 70 (70)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    clearEvents();

    // Seek in fast mode
    executeControlMedia("myVideo", "seek", 140, true);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: URL1",
                               "Position: 140 (140)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    clearEvents();

    // SetTrack in normal mode
    executeControlMedia("myVideo", "setTrack", 1, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TrackUpdate",
                               "URL: URL2",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 1 (1)",
                               "TrackState: ready (ready)"));
    clearEvents();

    // SetTrack in fast mode
    executeControlMedia("myVideo", "setTrack", 2, true);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TrackUpdate",
                               "URL: URL3",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 2 (2)",
                               "TrackState: ready (ready)"));
    clearEvents();
}

TEST_F(CommandMediaTest, ControlMalformed)
{
    loadDocument(VIDEO);
    ASSERT_FALSE(ConsoleMessage());

    executeControlMedia("myVideo2", "play", 0, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executeControlMedia("myVideo", "playfuzz", 0, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executeControlMedia("myVideo", "setTrack", 10, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executeControlMedia("myVideo3", "setTrack", 10, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(CommandMediaTest, Play)
{
    loadDocument(VIDEO);
    mediaPlayerFactory->advanceTime(10);
    clearEvents();

    executePlayMedia("myVideo", "foreground", "URL1", false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: URL1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    // Advance playback a bit
    mediaPlayerFactory->advanceTime(1500);
    clearEvents();

    // Play background audio
    executePlayMedia("myVideo", "background", "URL1", false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: URL1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    // Advance playback a bit
    mediaPlayerFactory->advanceTime(1500);
    clearEvents();

    // Play without audio
    executePlayMedia("myVideo", "none", "URL1", false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: URL1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    // Advance playback a bit
    mediaPlayerFactory->advanceTime(1500);
    clearEvents();

    // Test the "mute" alias
    executePlayMedia("myVideo", "mute", "URL1", false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: URL1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    // Advance playback a bit
    mediaPlayerFactory->advanceTime(1500);
    clearEvents();

    // Play in fast mode
    ASSERT_FALSE(ConsoleMessage());

    executePlayMedia("myVideo", "foreground", "URL1", true);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executePlayMedia("myVideo", "background", "URL1", true);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executePlayMedia("myVideo", "none", "URL1", true);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(CommandMediaTest, PlayMalformed)
{
    loadDocument(VIDEO);
    ASSERT_FALSE(ConsoleMessage());

    executePlayMedia("myVideo2", "none", Object::EMPTY_ARRAY(), false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executePlayMedia("myVideo", "fun", Object::EMPTY_ARRAY(), false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

const static char *COMMAND_SERIES = R"([
  {
    "type": "ControlMedia",
    "componentId": "myVideo",
    "command": "play"
  },
  {
    "type": "ControlMedia",
    "componentId": "myVideo",
    "command": "next",
    "delay": 100
  },
  {
    "type": "ControlMedia",
    "componentId": "myVideo",
    "command": "previous",
    "delay": 100
  }
])";



TEST_F(CommandMediaTest, ControlSeries)
{
    loadDocument(VIDEO);
    auto video = component->getChildAt(0);
    ASSERT_TRUE(video);

    auto json = JsonData(COMMAND_SERIES);
    auto action = executeCommands(json.get(), false);
    ASSERT_TRUE(action);
    ASSERT_TRUE(action->isPending());

    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: URL1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    clearEvents();

    advanceTime(1000);
    mediaPlayerFactory->advanceTime(1000);
    advanceTime(50);

    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TrackUpdate",
                               "URL: URL2",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 1 (1)",
                               "TrackState: ready (ready)"));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TrackUpdate",
                               "URL: URL1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TrackReady",
                               "URL: URL1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 3 (3)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    clearEvents();
}
