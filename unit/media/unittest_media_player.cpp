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
#include "apl/component/videocomponent.h"

using namespace apl;

using EventExpectation = std::pair<TestMediaPlayer::EventType, int>;

::testing::AssertionResult
CheckPlayerEvents(const std::map<TestMediaPlayer::EventType, int> &events, std::vector<EventExpectation> &&expectations)
{
    if (expectations.size() != events.size()) {
        return ::testing::AssertionFailure() << "Expected " << events.size() << " events but found " << expectations.size();
    }

    for (const EventExpectation &expectation : expectations) {
        const auto &it = events.find(expectation.first);
        int actual;
        if (it == events.end()) {
            actual = 0;
        } else {
            actual = it->second;
        }

        if (actual != expectation.second) {
            return ::testing::AssertionFailure() << "Expected " << expectation.second
                                                 << " but found " << actual
                                                 << " for event type " << TestMediaPlayer::sEventTypeMap.get(expectation.first, "???");
        }
    }

    return ::testing::AssertionSuccess();
}

class MediaPlayerTest : public DocumentWrapper {
public:
    MediaPlayerTest() : DocumentWrapper() {
        config->enableExperimentalFeature(RootConfig::kExperimentalFeatureManageMediaRequests);
        mediaPlayerFactory = std::make_shared<TestMediaPlayerFactory>();
        mediaPlayerFactory->setEventCallback([&](TestMediaPlayer::EventType event) {
              if (eventCounts.count(event) == 0) {
                  eventCounts[event] = 1;
              } else {
                  eventCounts[event] = eventCounts.at(event) + 1;
              }
        });
        config->mediaPlayerFactory(mediaPlayerFactory);
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


    std::shared_ptr<TestMediaPlayerFactory> mediaPlayerFactory;
    std::map<TestMediaPlayer::EventType, int> eventCounts;
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

    // After 100 milliseconds the "onTrackReady" handler executes
    mediaPlayerFactory->advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/P"));

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1}
    }));
    eventCounts.clear();

    // Start playing.  We'll let the player go through track1 onto track2.  Track 2 fails after 1200 ms.
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track1 0/"));

    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track1 500/"));

    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TrackUpdate track2 0/"));

    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track2 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track2 400/"));

    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track2 900/"));

    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TrackFail track2 1200/P"));

    // The player pauses automatically on a fail
    mediaPlayerFactory->advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    // Skip to the next track
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "next"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "TrackUpdate track3 0/P"));

    // Start playback again
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track3 0/"));

    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track3 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 250/"));

    // Note that the third track repeats once
    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 0/"));

    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 250/"));

    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "End track3 500/EP"));

    // Jump back to the first track
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "setTrack"}, {"value", 0}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "TrackUpdate track1 0/P"));

    // Jump back to the first track AGAIN.  This should not generate an event (there's no new information)
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "setTrack"}, {"value", 0}}, false);
    ASSERT_FALSE(root->hasEvent());

    // Even if we don't start playing, it buffers up to get ready
    mediaPlayerFactory->advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/P"));

    // Advance to the third track
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "next"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "TrackUpdate track2 0/P"));
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "next"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "TrackUpdate track3 0/P"));

    // Play through the entire track.  There is a repeat, so we need to run twice as long
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track3 0/"));
    mediaPlayerFactory->advanceTime(750);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track3 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 250/"));  // One repeat has occurred, so we've wrapped
    mediaPlayerFactory->advanceTime(1000);
    ASSERT_TRUE(CheckSendEvent(root, "End track3 500/EP"));  // One repeat has occurred, so we've wrapped

    // Calling setTrack on this track will reset the repeat counter and take it out of the End state
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "setTrack"}, {"value", 2}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 0/P"));
    executeCommand("ControlMedia", {{"componentId", "MyVideo"}, {"command", "play"}}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track3 0/"));
    mediaPlayerFactory->advanceTime(300);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 300/"));
    mediaPlayerFactory->advanceTime(300);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 100/"));  // We've wrapped
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

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1}
    }));
    eventCounts.clear();

    // After 100 milliseconds nothing happens
    mediaPlayerFactory->advanceTime(100);
    ASSERT_FALSE(root->hasEvent());

    // Play an existing track
    executeCommand("PlayMedia", {{"componentId", "MyVideo"}, {"source", "track3" }}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track3 0/"));

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1},
        {TestMediaPlayer::EventType::kPlayerEventPlay, 1}
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(250);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track3 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track3 250/"));

    // Play a non-existent track.  This will fail immediately
    executeCommand("PlayMedia", {{"componentId", "MyVideo"}, {"source", "track9" }}, false);
    ASSERT_TRUE(CheckSendEvent(root, "Play track9 0/"));

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1},
        {TestMediaPlayer::EventType::kPlayerEventPlay, 1}
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "TrackFail track9 0/EP"));

    // Use "SetValue" to change the tracks. This doesn't report a "PLAY" event because it wasn't a play command
    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "source"}, {"value", "track1" }}, false);
    ASSERT_FALSE(root->hasEvent());

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
    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventPlay, 1},
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, "TimeUpdate track1 100/"));

    // This should stop the playback, but it doesn't emit an event (should it?)
    executeCommand("SetValue", {{"componentId", "MyVideo"}, {"property", "source"}, {"value", "track3" }}, false);
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(10);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track3 0/P"));
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

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1},
        {TestMediaPlayer::EventType::kPlayerEventPlay, 1}
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(2000);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "End track1 1000/EP"));
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

    ASSERT_TRUE(CheckPlayerEvents(eventCounts, {
        {TestMediaPlayer::EventType::kPlayerEventSetTrackList, 1},
        {TestMediaPlayer::EventType::kPlayerEventSetAudioTrack, 1},
        {TestMediaPlayer::EventType::kPlayerEventPlay, 1}
    }));
    eventCounts.clear();

    mediaPlayerFactory->advanceTime(2000);
    ASSERT_TRUE(CheckSendEvent(root, "TrackReady track1 0/"));
    ASSERT_TRUE(CheckSendEvent(root, "End track1 1000/EP"));
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

    executeCommand("PlayMedia", {{"componentId", "MyVideo"}, {"source", "track1"}}, false);
    stepForward(500);
    ASSERT_FALSE(root->hasEvent());
    root->clearPending();

    auto child = component->getChildAt(0);
    ASSERT_TRUE(child->getType() == kComponentTypeVideo);
    auto mp = std::dynamic_pointer_cast<VideoComponent>(child)->getMediaPlayer();
    ASSERT_FALSE(std::dynamic_pointer_cast<TestMediaPlayer>(mp)->isReleased());

    ASSERT_TRUE(child->remove());
    child = nullptr;  // This should release the media player
    ASSERT_EQ(0, component->getChildCount());

    // We need this to clear out the old OnPlay handler that is holding onto the video resource
    root->clearPending();

    ASSERT_TRUE(std::dynamic_pointer_cast<TestMediaPlayer>(mp)->isReleased());
}
