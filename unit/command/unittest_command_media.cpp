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
#include "apl/primitives/mediastate.h"

using namespace apl;

class CommandMediaTest : public CommandTest {
public:
    ActionPtr executeControlMedia(const std::string& component, const std::string& command, int value, bool fastMode) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "ControlMedia", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("command", rapidjson::Value(command.c_str(), alloc).Move(), alloc);
        cmd.AddMember("value", value, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        return root->executeCommands(doc, fastMode);
    }

    ActionPtr executePlayMedia(const std::string& component, const std::string& audioTrack, const Object& source, bool fastMode) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "PlayMedia", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("audioTrack", rapidjson::Value(audioTrack.c_str(), alloc).Move(), alloc);
        cmd.AddMember("source", source.serialize(alloc).Move(), alloc);
        doc.SetArray().PushBack(cmd, alloc);
        return root->executeCommands(doc, fastMode);
    }

    rapidjson::Document doc;
};

static const char *VIDEO =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Video\","
    "          \"id\": \"myVideo\","
    "          \"width\": 100,"
    "          \"height\": 100,"
    "          \"source\": [\"URL1\", \"URL2\"]"
    "        },"
    "        {"
    "          \"type\": \"Video\","
    "          \"id\": \"myVideo3\","
    "          \"width\": 100,"
    "          \"height\": 100,"
    "          \"source\": \"URL1\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(CommandMediaTest, Control)
{
    loadDocument(VIDEO);

    // Play in normal mode
    executeControlMedia("myVideo", "play", 0, false);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeControlMedia, event.getType());
    ASSERT_EQ(kEventControlMediaPlay, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_EQ(component->getCoreChildAt(0), event.getComponent());
    ASSERT_FALSE(event.getActionRef().isEmpty());
    ASSERT_FALSE(root->hasEvent());

    // Play in fast mode ignored
    ASSERT_FALSE(ConsoleMessage());
    executeControlMedia("myVideo", "play", 0, true);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    // Pause in normal mode
    executeControlMedia("myVideo", "pause", 0, false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaPause, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_FALSE(root->hasEvent());

    // Pause in fast mode
    executeControlMedia("myVideo", "pause", 0, true);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaPause, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_FALSE(root->hasEvent());

    // Next in normal mode
    executeControlMedia("myVideo", "next", 0, false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaNext, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_FALSE(root->hasEvent());

    // Next in fast mode
    executeControlMedia("myVideo", "next", 0, true);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaNext, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_FALSE(root->hasEvent());

    // Previous in normal mode
    executeControlMedia("myVideo", "previous", 0, false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaPrevious, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_FALSE(root->hasEvent());

    // Previous in fast mode
    executeControlMedia("myVideo", "previous", 0, true);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaPrevious, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_FALSE(root->hasEvent());

    // Rewind in normal mode
    executeControlMedia("myVideo", "rewind", 0, false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaRewind, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_FALSE(root->hasEvent());

    // Rewind in fast mode
    executeControlMedia("myVideo", "rewind", 0, true);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaRewind, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_FALSE(root->hasEvent());

    // Seek in normal mode
    executeControlMedia("myVideo", "seek", 70, false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaSeek, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_EQ(70, event.getValue(kEventPropertyValue).asInt());
    ASSERT_FALSE(root->hasEvent());

    // Seek in fast mode
    executeControlMedia("myVideo", "seek", 70, true);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaSeek, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_EQ(70, event.getValue(kEventPropertyValue).asInt());
    ASSERT_FALSE(root->hasEvent());

    // SetTrack in normal mode
    executeControlMedia("myVideo", "setTrack", 1, false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaSetTrack, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_EQ(1, event.getValue(kEventPropertyValue).asInt());
    ASSERT_FALSE(root->hasEvent());

    // SetTrack in fast mode
    executeControlMedia("myVideo", "setTrack", 1, true);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventControlMediaSetTrack, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_EQ(1, event.getValue(kEventPropertyValue).asInt());
    ASSERT_FALSE(root->hasEvent());
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

    executePlayMedia("myVideo", "foreground", Object::EMPTY_ARRAY(), false);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypePlayMedia, event.getType());
    ASSERT_EQ(kEventAudioTrackForeground, event.getValue(kEventPropertyAudioTrack).getInteger());
    ASSERT_EQ(component->getCoreChildAt(0), event.getComponent());
    ASSERT_FALSE(event.getActionRef().isEmpty());
    event.getActionRef().resolve();

    // Play background audio
    executePlayMedia("myVideo", "background", Object::EMPTY_ARRAY(), false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypePlayMedia, event.getType());
    ASSERT_EQ(kEventAudioTrackBackground, event.getValue(kEventPropertyAudioTrack).getInteger());
    ASSERT_EQ(component->getCoreChildAt(0), event.getComponent());
    ASSERT_FALSE(event.getActionRef().isEmpty());

    // Play without audio
    executePlayMedia("myVideo", "none", Object::EMPTY_ARRAY(), false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypePlayMedia, event.getType());
    ASSERT_EQ(kEventAudioTrackNone, event.getValue(kEventPropertyAudioTrack).getInteger());
    ASSERT_EQ(component->getCoreChildAt(0), event.getComponent());
    ASSERT_FALSE(event.getActionRef().isEmpty());

    // Test the "mute" alias
    executePlayMedia("myVideo", "mute", Object::EMPTY_ARRAY(), false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypePlayMedia, event.getType());
    ASSERT_EQ(kEventAudioTrackNone, event.getValue(kEventPropertyAudioTrack).getInteger());
    ASSERT_EQ(component->getCoreChildAt(0), event.getComponent());
    ASSERT_FALSE(event.getActionRef().isEmpty());

    // Play in fast mode
    ASSERT_FALSE(ConsoleMessage());

    executePlayMedia("myVideo", "foreground", Object::EMPTY_ARRAY(), true);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executePlayMedia("myVideo", "background", Object::EMPTY_ARRAY(), true);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());

    executePlayMedia("myVideo", "none", Object::EMPTY_ARRAY(), true);
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

const static char *COMMAND_SERIES =
        "["
        "  {"
        "    \"type\": \"PlayMedia\","
        "    \"componentId\": \"myVideo\","
        "    \"source\": \"URLX\""
        "  },"
        "  {"
        "    \"type\": \"ControlMedia\","
        "    \"componentId\": \"myVideo\","
        "    \"command\": \"next\""
        "  },"
        "  {"
        "    \"type\": \"ControlMedia\","
        "    \"componentId\": \"myVideo\","
        "    \"command\": \"previous\""
        "  }"
        "]";



TEST_F(CommandMediaTest, ControlSeries)
{
    loadDocument(VIDEO);
    auto video = component->getChildAt(0);
    ASSERT_TRUE(video);

    auto json = JsonData(COMMAND_SERIES);
    auto action = root->executeCommands(json.get(), false);
    ASSERT_TRUE(action);
    ASSERT_TRUE(action->isPending());

    // There should be exactly one event on the stack
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_FALSE(root->hasEvent());

    // The first event should be a play with an action reference
    ASSERT_EQ(kEventTypePlayMedia, event.getType());
    ASSERT_FALSE(event.getActionRef().isEmpty());
    ASSERT_EQ(video, event.getComponent());

    // Update the video state
    MediaState state(0, 3, 0, 1000, false, false);
    video->updateMediaState(state, true);
    CheckMediaState(state, video->getCalculated());
    event.getActionRef().resolve();

    // The next command should run
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(action->isPending());

    // The second event should be a control media with an action reference
    ASSERT_EQ(kEventTypeControlMedia, event.getType());
    ASSERT_EQ(kEventControlMediaNext, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_FALSE(event.getActionRef().isEmpty());
    ASSERT_EQ(video, event.getComponent());

    // Update the video state.  It's paused because the control media commands pause it
    state = MediaState(1, 3, 0, 1000, true, false);
    video->updateMediaState(state, true);
    CheckMediaState(state, video->getCalculated());
    event.getActionRef().resolve();

    // The third command should run
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(action->isPending());

    // The third event should be a control media with an action reference
    ASSERT_EQ(kEventTypeControlMedia, event.getType());
    ASSERT_EQ(kEventControlMediaPrevious, event.getValue(kEventPropertyCommand).asInt());
    ASSERT_FALSE(event.getActionRef().isEmpty());
    ASSERT_EQ(video, event.getComponent());

    // Update the video state.  It's paused because the control media commands pause it
    state = MediaState(0, 3, 0, 1000, true, false);
    video->updateMediaState(state, true);
    CheckMediaState(state, video->getCalculated());
    event.getActionRef().resolve();

    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(action->isPending());
}
