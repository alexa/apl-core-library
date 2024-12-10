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

#include <map>

#include "testmediaplayerfactory.h"

using namespace apl;

class MediaPlayerTest : public DocumentWrapper {
public:
    MediaPlayerTest() : DocumentWrapper(), mediaDocument(rapidjson::kObjectType) {
        config->enableExperimentalFeature(RootConfig::kExperimentalFeatureManageMediaRequests);
        config->mediaPlayerFactory(mediaPlayerFactory);
    }

    ::testing::AssertionResult
    checkVisualContext(const std::string& id, int position) {
        if (!root->isVisualContextDirty()) return ::testing::AssertionFailure() << "Visual context not dirty.";
        visualContext = root->serializeVisualContext(mediaDocument.GetAllocator());

        if (!visualContext.HasMember("tags")) return ::testing::AssertionFailure() << "Visual context have no tags.";
        if (id != visualContext["id"].GetString()) return ::testing::AssertionFailure() << "ID is incorrect.";

        auto& tags = visualContext["tags"];
        if (!tags.HasMember("media")) return ::testing::AssertionFailure() << "Visual context has no media tag.";
        auto& media = tags["media"];
        if (position != media["positionInMilliseconds"].GetInt()) return ::testing::AssertionFailure() << "Track position is incorrect.";

        return ::testing::AssertionSuccess();
    }

    // Step forward time for both the system clock AND the media player in small increments
    void stepForward(apl_duration_t duration) {
        while (duration > 0) {
            auto delta = std::min(10.0, duration);
            mediaPlayerFactory->advanceTime(delta);
            advanceTime(delta);
            duration -= delta;
        }
    };

protected:
    rapidjson::Document mediaDocument;
    rapidjson::Value visualContext;
};

static const char *BASIC_PLAYBACK = R"apl(
    {
      "type": "APL",
      "version": "1.7",
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
        "item": {
          "type": "Video",
          "id": "MyVideo",
          "source": [
            "track1"
          ],
          "width": 100,
          "height": 100,
          "onEnd":         { "type": "DUMP" },
          "onPause":       { "type": "DUMP" },
          "onPlay":        { "type": "DUMP" },
          "onTimeUpdate":  { "type": "DUMP" },
          "onTrackUpdate": { "type": "DUMP" },
          "onTrackReady":  { "type": "DUMP" },
          "onTrackFail":   { "type": "DUMP" }
        }
      }
    }
)apl";

/**
 * The "source" field for a video component takes simple text strings and rich data objects.
 * These should be recursively evaluated in the event context when they are evaluated as media source objects.
 */
TEST_F(MediaPlayerTest, BasicPlayback)
{
    mediaPlayerFactory->addFakeContent({
        { "track1", 1000, 100, -1 },  // 1000 ms long, 100 ms buffer delay
    });

    loadDocument(BASIC_PLAYBACK);
    ASSERT_TRUE(component);

    // After 100 milliseconds the "onTrackReady" handler executes
    mediaPlayerFactory->advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TrackReady",
                               "URL: track1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",  // We asked for the whole video to play
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
            {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
            {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1}
    }));

    // The video is not playing yet
    mediaPlayerFactory->advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    // Start the video playing.  The "onPlay" handler executes
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: track1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    ASSERT_TRUE(root->isVisualContextDirty());

    // Move forward 500 milliseconds.  The "onTimeUpdate" handler executes
    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 500 (500)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));
    ASSERT_TRUE(checkVisualContext("MyVideo", 500));

    // Move forward another 500 milliseconds.  The "onEnd" handler executes
    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: End",
                               "URL: track1",
                               "Position: 1000 (1000)",
                               "Duration: 0 (0)",
                               "Ended: YES (YES)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Advance time just to prove that the video is no longer playing
    mediaPlayerFactory->advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    // Rewind the track to the start
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "rewind"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Start playing
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: track1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Seek in the video (this pauses the video as well)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "seek"}, {"value", 500}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 500 (500)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Start playing (again!)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: track1",
                               "Position: 500 (500)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Move forward 250 milliseconds
    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 750 (750)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Seek in the video (this pauses the video as well)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "seek"}, {"value", 100}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 100 (100)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Start playing (again!)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: track1",
                               "Position: 100 (100)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Move forward 650 milliseconds
    mediaPlayerFactory->advanceTime(650);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 750 (750)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // SeekTo in the video (this pauses the video as well)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "seekTo"}, {"value", 100}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 100 (100)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Start playing (again!)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: track1",
                               "Position: 100 (100)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Move forward 650 milliseconds
    mediaPlayerFactory->advanceTime(650);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 750 (750)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Pause the video
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "pause"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Pause",
                               "URL: track1",
                               "Position: 750 (750)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Try to go to the "next" video.  There isn't one, but we advance to the end of this one.
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "next"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 1000 (1000)",
                               "Duration: 0 (0)",
                               "Ended: YES (YES)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Try to go to the previous video.  There isn't one, but we go to the start of the video
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "previous"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Call rewind.  Nothing should happen because we are at the beginning.
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "rewind"}}, false);
    ASSERT_FALSE(root->hasEvent());
}

static const char *BASIC_PLAYBACK_NESTED = R"apl(
    {
      "type": "APL",
      "version": "1.7",
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
        "item": {
          "type": "Container",
          "item": {
            "type": "Video",
            "id": "MyVideo",
            "source": [
              "track1"
            ],
            "width": 100,
            "height": 100,
            "onEnd":         { "type": "DUMP" },
            "onPause":       { "type": "DUMP" },
            "onPlay":        { "type": "DUMP" },
            "onTimeUpdate":  { "type": "DUMP" },
            "onTrackUpdate": { "type": "DUMP" },
            "onTrackReady":  { "type": "DUMP" },
            "onTrackFail":   { "type": "DUMP" }
          }
        }
      }
    }
)apl";

/**
 * Nesting a video component in a multi-child parent can cause additional updates due to layout
 * properties. Make sure that we don't accidentally trigger spurious media player interactions in
 * such cases.
 */
TEST_F(MediaPlayerTest, BasicPlaybackNested)
{
    mediaPlayerFactory->addFakeContent({
       { "track1", 1000, 100, -1 },  // 1000 ms long, 100 ms buffer delay
   });

    loadDocument(BASIC_PLAYBACK_NESTED);
    ASSERT_TRUE(component);

    // After 100 milliseconds the "onTrackReady" handler executes
    mediaPlayerFactory->advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TrackReady",
                               "URL: track1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",  // We asked for the whole video to play
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1}
    }));

    // The video is not playing yet
    mediaPlayerFactory->advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    // Start the video playing.  The "onPlay" handler executes
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: track1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Move forward 500 milliseconds.  The "onTimeUpdate" handler executes
    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 500 (500)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Move forward another 500 milliseconds.  The "onEnd" handler executes
    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: End",
                               "URL: track1",
                               "Position: 1000 (1000)",
                               "Duration: 0 (0)",
                               "Ended: YES (YES)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Advance time just to prove that the video is no longer playing
    mediaPlayerFactory->advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    // Rewind the track to the start
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "rewind"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Start playing
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: track1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Seek in the video (this pauses the video as well)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "seek"}, {"value", 500}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 500 (500)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Start playing (again!)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: track1",
                               "Position: 500 (500)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Move forward 250 milliseconds
    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 750 (750)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // SeekTo in the video (this pauses the video as well)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "seekTo"}, {"value", 100}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 100 (100)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Start playing (again!)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Play",
                               "URL: track1",
                               "Position: 100 (100)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Move forward 650 milliseconds
    mediaPlayerFactory->advanceTime(650);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 750 (750)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: NO (NO)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Pause the video
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "pause"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: Pause",
                               "URL: track1",
                               "Position: 750 (750)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Try to go to the "next" video.  There isn't one, but we advance to the end of this one.
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "next"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 1000 (1000)",
                               "Duration: 0 (0)",
                               "Ended: YES (YES)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Try to go to the previous video.  There isn't one, but we go to the start of the video
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "previous"}}, false);
    ASSERT_TRUE(CheckSendEvent(root,
                               "Handler: TimeUpdate",
                               "URL: track1",
                               "Position: 0 (0)",
                               "Duration: 0 (0)",
                               "Ended: NO (NO)",
                               "Paused: YES (YES)",
                               "Muted: NO (NO)",
                               "TrackCount: 1 (1)",
                               "TrackIndex: 0 (0)",
                               "TrackState: ready (ready)"));

    // Call rewind.  Nothing should happen because we are at the beginning.
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "rewind"}}, false);
    ASSERT_FALSE(root->hasEvent());
}

static const char *MULTI_TRACK_PLAYBACK = R"apl(
    {
      "type": "APL",
      "version": "1.7",
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
          "id": "MyVideo",
          "source": [
            "track1",
            "track2",
            { "url": "track3", "repeatCount": 1 }
          ],
          "onEnd":         { "type": "DUMP" },
          "onPause":       { "type": "DUMP" },
          "onPlay":        { "type": "DUMP" },
          "onTimeUpdate":  { "type": "DUMP" },
          "onTrackUpdate": { "type": "DUMP" },
          "onTrackReady":  { "type": "DUMP" },
          "onTrackFail":   { "type": "DUMP" }
        }
      }
    }
)apl";

TEST_F(MediaPlayerTest, MultiTrackPlayback)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 100, -1},   // 1000 ms long, 100 ms buffer delay
        {"track2", 2000, 100, 1200}, // 2000 ms long, 100 ms buffer delay, fails at 1200 ms
        {"track3", 500, 0, -1}       // 500 ms long, no buffer delay
    });

    loadDocument(MULTI_TRACK_PLAYBACK);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->screenLock());  // Nothing is playing

    // After 100 milliseconds the "onTrackReady" handler executes
    mediaPlayerFactory->advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/P"));
    ASSERT_FALSE(root->screenLock());  // Nothing is playing

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1}
    }));
    eventCounts.clear();

    // Start playing.  We'll let the player go through track1 onto track2.  Track 2 fails after 1200 ms.
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));
    ASSERT_TRUE(root->screenLock());  // Playing causes a screen lock

    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track1 500/"));
    ASSERT_TRUE(root->screenLock());

    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TrackUpdate track2 0/"));
    ASSERT_TRUE(root->screenLock());

    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track2 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track2 400/"));
    ASSERT_TRUE(root->screenLock());

    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track2 900/"));
    ASSERT_TRUE(root->screenLock());

    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TrackFail track2 1200/P"));
    ASSERT_FALSE(root->screenLock());

    // The player pauses automatically on a fail
    mediaPlayerFactory->advanceTime(100);
    ASSERT_FALSE(root->screenLock());
    ASSERT_FALSE(root->hasEvent());

    // Skip to the next track
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "next"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "TrackUpdate track3 0/P"));
    ASSERT_FALSE(root->screenLock());

    // Start playback again
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track3 0/"));
    ASSERT_TRUE(root->screenLock());

    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track3 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 250/"));
    ASSERT_TRUE(root->screenLock());

    // Note that the third track repeats once
    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 0/"));
    ASSERT_TRUE(root->screenLock());

    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 250/"));
    ASSERT_TRUE(root->screenLock());

    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "End track3 500/EP"));
    ASSERT_FALSE(root->screenLock());

    // Jump back to the first track
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "setTrack"}, {"value", 0}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "TrackUpdate track1 0/P"));
    ASSERT_FALSE(root->screenLock());

    // Jump back to the first track AGAIN.  This should not generate an event (there's no new information)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "setTrack"}, {"value", 0}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(root->screenLock());

    // Even if we don't start playing, it buffers up to get ready
    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/P"));
    ASSERT_FALSE(root->screenLock());

    // Advance to the third track
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "next"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "TrackUpdate track2 0/P"));
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "next"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "TrackUpdate track3 0/P"));
    ASSERT_FALSE(root->screenLock());

    // Play through the entire track.  There is a repeat, so we need to run twice as long
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track3 0/"));
    mediaPlayerFactory->advanceTime(750);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track3 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 250/"));  // One repeat has occurred, so we've wrapped
    mediaPlayerFactory->advanceTime(1000);
    ASSERT_TRUE(CheckSendEvent(root, "End track3 500/EP"));  // One repeat has occurred, so we've wrapped
    ASSERT_FALSE(root->screenLock());

    // Calling setTrack on this track will reset the repeat counter and take it out of the End state
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "setTrack"}, {"value", 2}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 0/P"));
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track3 0/"));
    mediaPlayerFactory->advanceTime(300);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 300/"));
    mediaPlayerFactory->advanceTime(300);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 100/"));  // We've wrapped
    ASSERT_TRUE(root->screenLock());

    // Finally, stop the playback
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "pause"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Pause track3 100/P"));
    ASSERT_FALSE(root->screenLock());
}

static const char *PLAY_MEDIA = R"apl(
    {
      "type": "APL",
      "version": "1.7",
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
    }
)apl";

/**
 * Various ways of calling "PlayMedia".  You can call it directly and start a new set of tracks
 * playing.  You can also call "SetValue" on the source array of the video component; that will
 * replace the existing tracks with a new set.
 */
TEST_F(MediaPlayerTest, PlayMedia)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 100, -1},   // 1000 ms long, 100 ms buffer delay
        {"track2", 2000, 100, 1200}, // 2000 ms long, 100 ms buffer delay, fails at 1200 ms
        {"track3", 500, 0, -1}       // 500 ms long, no buffer delay
    });

    loadDocument(PLAY_MEDIA);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->screenLock());

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1}
    }));
    eventCounts.clear();

    // After 100 milliseconds nothing happens
    mediaPlayerFactory->advanceTime(100);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(root->screenLock());

    // Play an existing track
    executeCommand("PlayMedia", {{"componentId", "MyVideo"}, {"source", "track3" }}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track3 0/"));
    ASSERT_TRUE(root->screenLock());

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1},
        {TestMediaPlayer::EventType::kPlayerEventPlay, 1}
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track3 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 250/"));
    ASSERT_TRUE(root->screenLock());

    // Play a non-existent track.  This will fail immediately
    executeCommand("PlayMedia", {{"componentId", "MyVideo"}, {"source", "track9" }}, false);
    // A track fail terminates action which pauses the previously playing track
    ASSERT_TRUE(CheckSendEvent(root, "Pause track3 250/P"));
    ASSERT_TRUE(CheckSendEvent(root, "Play track9 0/"));
    ASSERT_TRUE(root->screenLock());  // We briefly think we have screen lock until told otherwise.

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1},
        {TestMediaPlayer::EventType::kPlayerEventPlay, 1}
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "TrackFail track9 0/EP"));
    ASSERT_FALSE(root->screenLock());

    // Use "SetValue" to change the tracks. This doesn't report a "PLAY" event because it wasn't a play command
    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "source"}, {"value", "track1" }}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(root->screenLock());

    // However, the track does start to buffer, so it sends a Ready
    mediaPlayerFactory->advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/P"));

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
    }));
    eventCounts.clear();

    // Start playing, then use another SetValue to stop the existing playback
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));
    ASSERT_TRUE(root->screenLock());
    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventPlay, 1},
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track1 100/"));

    // This should stop the playback, but it doesn't emit an event (should it?)
    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "source"}, {"value", "track3" }}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(root->screenLock());

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(10);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track3 0/P"));
}

/**
 * Check that the mediaplayer is paused when the screen is touched during a PlayMedia command execution
 * and the audioTrack is foreground
 */
TEST_F(MediaPlayerTest, PlayMediaTerminationByTap)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 100, -1},   // 1000 ms long, 100 ms buffer delay
        {"track2", 2000, 100, 1200}, // 2000 ms long, 100 ms buffer delay, fails at 1200 ms
        {"track3", 500, 0, -1}       // 500 ms long, no buffer delay
    });

    loadDocument(PLAY_MEDIA);
    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
            {TestMediaPlayer::EventType::kPlayerEventSetTrackList,  1},
            {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1}
        })
    );
    eventCounts.clear();

    // After 100 milliseconds nothing happens
    mediaPlayerFactory->advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    // Play an existing track with audioTrack background
    executeCommand("PlayMedia", {{"componentId", "MyVideo"}, {"source", "track3" }}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track3 0/"));

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
            {TestMediaPlayer::EventType::kPlayerEventSetTrackList,  1},
            {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1},
            {TestMediaPlayer::EventType::kPlayerEventPlay,          1}
        })
    );
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track3 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 250/"));

    performTap(1, 100);
    ASSERT_TRUE(CheckSendEvent(root, "Pause track3 250/P"));

    // After 100 milliseconds nothing happens
    mediaPlayerFactory->advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    // Play an existing track with audioTrack background
    executeCommand("PlayMedia", {{"componentId", "MyVideo"}, {"source", "track3" }, {"audioTrack", "background"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track3 0/"));
    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
            {TestMediaPlayer::EventType::kPlayerEventSetTrackList,  1},
            {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1},
            {TestMediaPlayer::EventType::kPlayerEventPlay,          1}
        })
    );
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track3 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 250/"));

    performTap(1, 100);
    // Player is not paused if audioTrack is anything other than foreground
    ASSERT_FALSE(CheckSendEvent(root, "Pause track3 250/P"));
    ASSERT_TRUE(root->screenLock());  // Screen lock is still held

    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "pause"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Pause track3 250/P"));
    ASSERT_FALSE(root->screenLock());  // Screen lock has been released
}

static const char *PLAY_MEDIA_IN_SEQUENCE = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "commands": {
        "DUMP": {
          "command": {
            "type": "SendEvent",
            "sequencer": "FOO",
            "arguments": [
              "${event.source.handler} ${event.source.url} ${event.currentTime}/${event.ended ? 'E' : ''}${event.paused ? 'P' : ''}"
            ]
          }
        },
        "PLAY_AND_SEND": {
          "parameters": {
            "name": "audioTrack",
            "default": "foreground"
          },
          "command": [
            {
              "type": "PlayMedia",
              "componentId": "MyVideo",
              "source": "track1",
              "audioTrack": "${audioTrack}"
            },
            {
              "type": "SendEvent",
              "arguments": [ "FINISHED" ]
            }
          ]
        }
      },
      "mainTemplate": {
        "item": {
          "type": "Video",
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
    }
)apl";

/**
 * Test chaining commands with PlayMedia.  This requires the MediaPlayer to correctly resolve
 * action references.  Note that have to put the PlayMeda and SendEvent[Finished] commands
 * on a different sequencer than the main sequencer.
 */
TEST_F(MediaPlayerTest, PlayMediaInSequence)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 100, -1},   // 1000 ms long, 100 ms buffer delay
    });

    loadDocument(PLAY_MEDIA_IN_SEQUENCE);
    ASSERT_TRUE(component);

    // Play an existing track
    executeCommand("PLAY_AND_SEND", {}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));

    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track1 150/"));

    mediaPlayerFactory->advanceTime(1000);
    ASSERT_TRUE(CheckSendEvent(root, "End track1 1000/EP"));

    // After playing we should receive a final send event
    ASSERT_TRUE(CheckSendEvent(root, "FINISHED"));

    // Now re-issue the command, but this time put it on the background audio track
    // The FINISHED message gets sent immediately
    executeCommand("PLAY_AND_SEND", {{"audioTrack", "background"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "FINISHED"));

    mediaPlayerFactory->advanceTime(2000);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "End track1 1000/EP"));
}


static const char *CONTROL_MEDIA_IN_SEQUENCE = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "commands": {
        "DUMP": {
          "command": {
            "type": "SendEvent",
            "sequencer": "FOO",
            "arguments": [
              "${event.source.handler} ${event.source.url} ${event.currentTime}/${event.ended ? 'E' : ''}${event.paused ? 'P' : ''}"
            ]
          }
        },
        "PLAY_AND_SEND": {
          "command": [
            {
              "type": "ControlMedia",
              "componentId": "MyVideo",
              "command": "play"
            },
            {
              "type": "SendEvent",
              "arguments": [ "STARTED" ]
            }
          ]
        }
      },
      "mainTemplate": {
        "item": {
          "type": "Video",
          "id": "MyVideo",
          "autoplay": false,
          "source": "track1",
          "onEnd":         { "type": "DUMP" },
          "onPause":       { "type": "DUMP" },
          "onPlay":        { "type": "DUMP" },
          "onTimeUpdate":  { "type": "DUMP" },
          "onTrackUpdate": { "type": "DUMP" },
          "onTrackReady":  { "type": "DUMP" },
          "onTrackFail":   { "type": "DUMP" }
        }
      }
    }
)apl";

/**
 * Test chaining commands with ControlMedia.  Action references is resolved immediately.
 * Note that have to put the ControlMedia.play and SendEvent[Finished] commands
 * on a different sequencer than the main sequencer.
 */
TEST_F(MediaPlayerTest, ControlMediaInSequence)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 100, -1},   // 1000 ms long, 100 ms buffer delay
    });

    loadDocument(CONTROL_MEDIA_IN_SEQUENCE);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->screenLock());

    // Play the track in foreground
    executeCommand("PLAY_AND_SEND", {}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));
    // After the command we should receive a send event immediately
    ASSERT_TRUE(CheckSendEvent(root, "STARTED"));
    ASSERT_TRUE(root->screenLock());

    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "pause"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Pause track1 0/P"));
    ASSERT_FALSE(root->screenLock());
}


static const char *AUTO_PLAY = R"apl(
    {
      "type": "APL",
      "version": "1.7",
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
          "id": "MyVideo",
          "autoplay": true,
          "source": "track1",
          "onEnd":         { "type": "DUMP" },
          "onPause":       { "type": "DUMP" },
          "onPlay":        { "type": "DUMP" },
          "onTimeUpdate":  { "type": "DUMP" },
          "onTrackUpdate": { "type": "DUMP" },
          "onTrackReady":  { "type": "DUMP" },
          "onTrackFail":   { "type": "DUMP" }
        }
      }
    }
)apl";

TEST_F(MediaPlayerTest, AutoPlay)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 100, -1}
    });

    loadDocument(AUTO_PLAY);
    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));
    ASSERT_TRUE(root->screenLock());

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1},
        {TestMediaPlayer::EventType::kPlayerEventPlay, 1}
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(2000);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "End track1 1000/EP"));
    ASSERT_FALSE(root->screenLock());
}

static const char *AUTO_PLAY_NESTED = R"apl(
    {
      "type": "APL",
      "version": "1.7",
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
          "type": "Container",
          "item": {
            "type": "Video",
            "id": "MyVideo",
            "autoplay": true,
            "source": "track1",
            "onEnd":         { "type": "DUMP" },
            "onPause":       { "type": "DUMP" },
            "onPlay":        { "type": "DUMP" },
            "onTimeUpdate":  { "type": "DUMP" },
            "onTrackUpdate": { "type": "DUMP" },
            "onTrackReady":  { "type": "DUMP" },
            "onTrackFail":   { "type": "DUMP" }
          }
        }
      }
    }
)apl";

TEST_F(MediaPlayerTest, AutoPlayNested)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 100, -1}
    });

    loadDocument(AUTO_PLAY_NESTED);
    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));
    ASSERT_TRUE(root->screenLock());

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1},
        {TestMediaPlayer::EventType::kPlayerEventPlay, 1}
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(2000);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "End track1 1000/EP"));
    ASSERT_FALSE(root->screenLock());
}


static const char *MULTIPLE_PLAYERS = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "commands": {
        "DUMP": {
          "command": {
            "type": "SendEvent",
            "sequencer": "123",
            "arguments": [
              "${event.source.handler} ${event.source.url} ${event.currentTime}/${event.ended ? 'E' : ''}${event.paused ? 'P' : ''}"
            ]
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": {
            "type": "Video",
            "id": "MyVideo${index+1}",
            "source": "${data}",
            "onEnd":         { "type": "DUMP" },
            "onPause":       { "type": "DUMP" },
            "onPlay":        { "type": "DUMP" },
            "onTrackUpdate": { "type": "DUMP" },
            "onTrackReady":  { "type": "DUMP" },
            "onTrackFail":   { "type": "DUMP" }
          },
          "data": [ "track1", "track2" ]
        }
      }
    }
)apl";

TEST_F(MediaPlayerTest, MultiplePlayers)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 50, -1},
        {"track2", 1000, 150, -1}
    });

    loadDocument(MULTIPLE_PLAYERS);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());

    // Both tracks load automatically
    mediaPlayerFactory->advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/P"));

    mediaPlayerFactory->advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track2 0/P"));

    // Start playing on the first track
    executeCommand("ControlMedia", {{"componentId", "MyVideo1"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));

    // Stagger the start times
    mediaPlayerFactory->advanceTime(100);
    executeCommand("ControlMedia", {{"componentId", "MyVideo2"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track2 0/"));

    // The first track should finish
    mediaPlayerFactory->advanceTime(900);  // This should just finish track 1
    ASSERT_TRUE(CheckSendEvent(root, "End track1 1000/EP"));

    // The second track finishes later
    mediaPlayerFactory->advanceTime(900);  // This should just finish track 1
    ASSERT_TRUE(CheckSendEvent(root, "End track2 1000/EP"));
}


static const char *OVERLAPPING = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "commands": {
        "DELAY": {
          "parameters": "amount",
          "command": [
            {
              "type": "SendEvent",
              "sequencer": "123",
              "arguments": [
                "${event.source.handler} ${event.source.url} ${event.currentTime}/${event.ended ? 'E' : ''}${event.paused ? 'P' : ''}"
              ]
            },
            {
              "type": "SendEvent",
              "description": "This runs on the regular sequencer after a delay",
              "delay": "${amount}",
              "arguments": [
                "DELAYED ${event.source.handler} ${event.source.url} ${event.currentTime}/${event.ended ? 'E' : ''}${event.paused ? 'P' : ''}"
              ]
            }
          ]
        }
      },
      "mainTemplate": {
        "item": {
          "type": "Container",
          "items": {
            "type": "Video",
            "id": "MyVideo${index+1}",
            "onEnd": { "type": "DELAY", "amount": "${data}" },
            "onPause": { "type": "DELAY", "amount": "${data}" },
            "onPlay": { "type": "DELAY", "amount": "${data}" }
          },
          "data": [ 500, 1000 ]
        }
      }
    }
)apl";

TEST_F(MediaPlayerTest, OverlappingResults)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 0, -1},
        {"track2", 1000, 0, -1}
    });

    loadDocument(OVERLAPPING);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());

    // Play the first video
    executeCommand("PlayMedia", {{"componentId", "MyVideo1"}, {"source", "track1"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));
    ASSERT_FALSE(root->hasEvent());

    // Jump forward so that the second SendEvent triggers
    stepForward(500);
    ASSERT_TRUE(CheckSendEvent(root, "DELAYED Play track1 0/"));
    ASSERT_FALSE(root->hasEvent());

    // Advance to the end of the track
    stepForward(500);
    ASSERT_TRUE(CheckSendEvent(root, "End track1 1000/EP"));
    ASSERT_FALSE(root->hasEvent());

    // Finally, the delayed send fires
    stepForward(500);
    ASSERT_TRUE(CheckSendEvent(root, "DELAYED End track1 1000/EP"));

    // TODO: Add test case so that the delayed SendEvent from the onPlay is clobbered by onEnd
    // TODO: Add test case showing that two separate Video components don't clobber each other
}


static const char *NO_TRACKS = R"apl(
    {
      "type": "APL",
      "version": "1.7",
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
    }
)apl";

TEST_F(MediaPlayerTest, NoTracks)
{
    loadDocument(NO_TRACKS);
    ASSERT_TRUE(component);

    // Start playing
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_FALSE(root->hasEvent());

    // Assign some tracks and play them
    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "source"}, {"value", "track1"}}, false);
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));

    // Remove those tracks and try playing them again
    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "source"}, {"value", ""}}, false);
    ASSERT_TRUE(ConsoleMessage());   // Warning about the empty string in SetValue

    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_FALSE(root->hasEvent());

    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "pause"}}, false);
    ASSERT_FALSE(root->hasEvent());

    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "pause"}}, false);
    ASSERT_FALSE(root->hasEvent());

    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "next"}}, false);
    ASSERT_FALSE(root->hasEvent());

    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "previous"}}, false);
    ASSERT_FALSE(root->hasEvent());

    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "seek"}, {"value", 1000}}, false);
    ASSERT_FALSE(root->hasEvent());

    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "setTrack"}, {"value", 0}}, false);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());  // Track index out of bounds
}


static const char *DESTROY_MEDIA_PLAYER = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "items": {
            "type": "Video",
            "id": "MyVideo"
          }
        }
      }
    }
)apl";

TEST_F(MediaPlayerTest, DestroyMediaPlayer)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 0, -1},
    });

    loadDocument(DESTROY_MEDIA_PLAYER);
    root->clearPending();

    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_FALSE(root->screenLock());

    executeCommand("PlayMedia", {{"componentId", "MyVideo"}, {"source", "track1"}}, false);
    stepForward(500);
    ASSERT_FALSE(root->hasEvent());
    root->clearPending();
    ASSERT_TRUE(root->screenLock());

    auto child = component->getChildAt(0);
    ASSERT_TRUE(child->getType() == kComponentTypeVideo);
    auto mp = child->getMediaPlayer();
    ASSERT_FALSE(std::static_pointer_cast<TestMediaPlayer>(mp)->isReleased());

    ASSERT_TRUE(child->remove());
    child = nullptr;  // This should release the media player
    ASSERT_EQ(0, component->getChildCount());
    ASSERT_FALSE(root->screenLock());

    // We need this to clear out the old OnPlay handler that is holding onto the video resource
    root->clearPending();
    root->clearVisualContextDirty();

    ASSERT_TRUE(std::static_pointer_cast<TestMediaPlayer>(mp)->isReleased());
}

static const char *MUTE_MEDIA_PLAYER = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "items": {
            "type": "Video",
            "id": "MyVideo",
            "muted": true,
             "source": [
                "track1"
             ]
          }
        }
      }
    }
)apl";

TEST_F(MediaPlayerTest, MuteVideo) {
    loadDocument(MUTE_MEDIA_PLAYER);
    ASSERT_TRUE(component);

    auto child = component->getChildAt(0);
    ASSERT_TRUE(child->getType() == kComponentTypeVideo);

    auto mp = child->getMediaPlayer();
    auto testMediaPlayer = std::static_pointer_cast<TestMediaPlayer>(mp);
    ASSERT_TRUE(testMediaPlayer->isMuted());

    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "muted"}, {"value", false}}, false);
    ASSERT_FALSE(testMediaPlayer->isMuted());

    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "muted"}, {"value", true}}, false);
    ASSERT_TRUE(testMediaPlayer->isMuted());

}

static const char* VIDEO_IN_CONTAINER_WITH_AUTOPLAY = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": 200,
      "height": 200,
      "items": {
        "type": "Video",
        "id": "VIDEO",
        "autoplay": true,
        "width": "100%",
        "height": "100%",
        "onPlay": {
          "type": "SendEvent",
          "sequencer": "FOO",
          "arguments": [
            "Handler: ${event.source.handler}"
          ]
        }
      }
    }
  }
})";

TEST_F(MediaPlayerTest, AutoplayDoesntPlayVideoWhenDisallowVideoTrue) {
    config->set(RootProperty::kDisallowVideo, true);
    loadDocument(VIDEO_IN_CONTAINER_WITH_AUTOPLAY);

    ASSERT_TRUE(component);
    auto v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    // No media player when disallow is true
    ASSERT_EQ(v->getMediaPlayer(), nullptr);
    // onPlay was not triggered
    ASSERT_FALSE(root->hasEvent());
}


static const char *SCREEN_LOCK_PROPERTY = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "items": {
            "type": "Video",
            "id": "MyVideo",
            "screenLock": false
          }
        }
      }
    }
)apl";

TEST_F(MediaPlayerTest, ScreenLockProperty)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 0, -1},
    });

    loadDocument(SCREEN_LOCK_PROPERTY);
    root->clearPending();

    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_FALSE(root->screenLock());

    // Playing media with screenLock=FALSE doesn't do anything
    executeCommand("PlayMedia", {{"componentId", "MyVideo"}, {"source", "track1"}}, false);
    stepForward(500);
    ASSERT_FALSE(root->hasEvent());
    root->clearPending();
    ASSERT_FALSE(root->screenLock());

    // Changing screenLock=TRUE should toggle the screen lock
    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "screenLock"}, {"value", true}}, true);
    ASSERT_TRUE(root->screenLock());

    // Change it back to false - the screen lock is released
    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "screenLock"}, {"value", false}}, true);
    ASSERT_FALSE(root->screenLock());

    // Pause the media playback
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "pause"}}, true);
    ASSERT_FALSE(root->screenLock());

    // Now turn screenLock=TRUE - but since there is no media, it doesn't change
    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "screenLock"}, {"value", true}}, true);
    ASSERT_FALSE(root->screenLock());
}


static const char *SCREEN_LOCK_AUTO_PLAY = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "items": {
            "type": "Video",
            "id": "MyVideo",
            "screenLock": true,
            "autoplay": true,
            "source": "track1"
          }
        }
      }
    }
)apl";

TEST_F(MediaPlayerTest, ScreenLockVideoRemoval)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 0, -1},
    });

    loadDocument(SCREEN_LOCK_AUTO_PLAY);
    root->clearPending();

    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(root->screenLock());

    // Now remove the component while the video is playing.
    executeCommand("RemoveItem", {{"componentId", "MyVideo"}}, true);
    ASSERT_FALSE(root->screenLock());
}


static const char *SCREEN_LOCK_MULTIPLE_VIDEOS = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "data": ["A", "B", "C"],
          "items": {
            "type": "Video",
            "id": "MyVideo${index}",
            "screenLock": true,
            "autoplay": true,
            "source": "track1"
          }
        }
      }
    }
)apl";

TEST_F(MediaPlayerTest, MultipleVideos)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 0, -1},
    });

    loadDocument(SCREEN_LOCK_MULTIPLE_VIDEOS);
    root->clearPending();

    ASSERT_TRUE(component);
    ASSERT_EQ(3, component->getChildCount());
    ASSERT_TRUE(root->screenLock());

    // Stop the players one by one. Stopping the last one should remove the screen lock.
    executeCommand("ControlMedia", {{"componentId", "MyVideo0"}, {"command", "pause"}}, true);
    ASSERT_TRUE(root->screenLock());

    executeCommand("ControlMedia", {{"componentId", "MyVideo1"}, {"command", "pause"}}, true);
    ASSERT_TRUE(root->screenLock());

    executeCommand("ControlMedia", {{"componentId", "MyVideo2"}, {"command", "pause"}}, true);
    ASSERT_FALSE(root->screenLock());

    // Restart a few videos and stop in random order
    executeCommand("ControlMedia", {{"componentId", "MyVideo0"}, {"command", "play"}}, false);
    ASSERT_TRUE(root->screenLock());

    executeCommand("ControlMedia", {{"componentId", "MyVideo1"}, {"command", "play"}}, false);
    ASSERT_TRUE(root->screenLock());

    executeCommand("ControlMedia", {{"componentId", "MyVideo1"}, {"command", "pause"}}, true);
    ASSERT_TRUE(root->screenLock());

    executeCommand("ControlMedia", {{"componentId", "MyVideo0"}, {"command", "pause"}}, true);
    ASSERT_FALSE(root->screenLock());
}


static const char * PLAY_MEDIA_WITH_SCREEN_LOCK = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Video",
          "id": "MyVideo"
        }
      }
    }
)apl";

TEST_F(MediaPlayerTest, VideoWithSequencer)
{
    mediaPlayerFactory->addFakeContent({
        {"track1", 1000, 0, -1},
    });

    loadDocument(PLAY_MEDIA_WITH_SCREEN_LOCK);
    root->clearPending();

    ASSERT_TRUE(component);
    ASSERT_FALSE(root->screenLock());

    // Play the video on the foreground audio track with a screen lock on the command
    executeCommand("PlayMedia", {{"componentId", "MyVideo"}, {"source", "track1"}, {"screenLock", true}}, false);
    ASSERT_TRUE(root->screenLock());

    // Change the component screenLock value to false. Because the PlayMedia command specified
    // a screen lock, we continue to hold the screen lock.
    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "screenLock"}, {"value", false}}, true);
    ASSERT_TRUE(root->screenLock());

    // Interrupt the video playback by issuing a new command.  This should stop the PlayMedia command
    // which will release the screen lock.
    executeCommand("Idle", {}, false);
    ASSERT_FALSE(root->screenLock());

    // Calling PlayMedia again without a screen lock does not result in a screen lock
    executeCommand("PlayMedia", {{"componentId", "MyVideo"}, {"source", "track1"}}, false);
    ASSERT_FALSE(root->screenLock());

    // Switch the component back to holding a screen lock
    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "screenLock"}, {"value", true}}, true);
    ASSERT_TRUE(root->screenLock());

    // And stop it again
    executeCommand("Idle", {}, false);
    ASSERT_FALSE(root->screenLock());
}