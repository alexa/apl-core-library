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
#include "apl/content/jsondata.h"
#include "apl/time/sequencer.h"

using namespace apl;

class SequencerTest : public CommandTest {
public:
    ActionPtr execute(const std::string& cmds, bool fastMode) {
        command.Parse(cmds.c_str());
        return root->executeCommands(command, fastMode);
    }
};

static const char *BASIC = R"({
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "item": {
          "type": "Container"
        }
      }
    }
  )";

static const char *SEND_EVENT_ON_MAIN = R"([
  {
    "type": "SendEvent",
    "arguments": [1]
  }
])";

static const char *SEND_EVENT_ON_SECONDARY = R"([
  {
    "type": "SendEvent",
    "delay": 100,
    "sequencer": "secondary",
    "arguments": [2]
  }
])";

static const char *SEND_EVENT_ON_TERTIARY = R"([
  {
    "type": "SendEvent",
    "delay": 200,
    "sequencer": "tertiary",
    "arguments": [3]
  }
])";

static const char *TERMINATE_MAIN = R"([
  {
    "type": "Idle"
  }
])";

static const char *TERMINATE_SECONDARY = R"([
  {
    "type": "Idle",
    "sequencer": "secondary"
  }
])";

static const char *TERMINATE_TERTIARY = R"([
  {
    "type": "Idle",
    "sequencer": "tertiary"
  }
])";

TEST_F(SequencerTest, OnSequencerTerminateMain)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(SEND_EVENT_ON_SECONDARY, false);
    // Submit idle on main sequencer will terminate it (it's empty anyway)
    execute(TERMINATE_MAIN, false);

    auto sequencer = context->sequencer();
    ASSERT_FALSE(sequencer.empty("secondary"));
    ASSERT_FALSE(sequencer.empty(MAIN_SEQUENCER_NAME));

    // Overcome timeout.
    loop->advanceToEnd();

    // Should still fire
    ASSERT_TRUE(CheckSendEvent(root,2));
}

TEST_F(SequencerTest, OnSequencerTerminateScheduled)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(SEND_EVENT_ON_SECONDARY, false);
    // Submit idle on secondary sequencer will terminate it
    execute(TERMINATE_SECONDARY, false);

    auto sequencer = context->sequencer();
    ASSERT_TRUE(sequencer.empty("secondary"));
    ASSERT_TRUE(sequencer.empty(MAIN_SEQUENCER_NAME));

    // Overcome timeout.
    loop->advanceToEnd();

    // Should not fire
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(SequencerTest, ParallelNormal)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(SEND_EVENT_ON_SECONDARY, false);
    // Submit same on main
    execute(SEND_EVENT_ON_MAIN, false);

    auto sequencer = context->sequencer();
    ASSERT_FALSE(sequencer.empty("secondary"));
    ASSERT_FALSE(sequencer.empty(MAIN_SEQUENCER_NAME));

    // Overcome timeout.
    loop->advanceToEnd();

    ASSERT_TRUE(CheckSendEvent(root, 1));
    ASSERT_TRUE(CheckSendEvent(root, 2));
}

TEST_F(SequencerTest, OnSequencerTerminateSecondary)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(SEND_EVENT_ON_SECONDARY, false);
    execute(SEND_EVENT_ON_TERTIARY, false);

    auto sequencer = context->sequencer();
    ASSERT_FALSE(sequencer.empty("secondary"));
    ASSERT_FALSE(sequencer.empty("tertiary"));
    ASSERT_TRUE(sequencer.empty(MAIN_SEQUENCER_NAME));

    // Submit idle on one of the sequencers will terminate it
    execute(TERMINATE_SECONDARY, false);

    // Overcome timeout.
    loop->advanceToTime(101);

    // Should still fire
    ASSERT_FALSE(root->hasEvent());

    // Overcome timeout.
    loop->advanceToTime(201);

    ASSERT_TRUE(CheckSendEvent(root, 3));
    ASSERT_FALSE(root->hasEvent());
}

static const char *SEQUENTIAL_ON_SECONDARY = R"([
  {
    "type": "Sequential",
    "sequencer": "magic",
    "commands": [
      {
        "type": "SendEvent",
        "delay": 100,
        "arguments": [1]
      },
      {
        "type": "SendEvent",
        "delay": 200,
        "arguments": [2]
      }
    ]
  }
])";

TEST_F(SequencerTest, SequentialOnSequencer)
{
    loadDocument(BASIC);

    auto btn = context->findComponentById("btn");

    // Should schedule send event on magic sequencer
    execute(SEQUENTIAL_ON_SECONDARY, false);
    auto sequencer = context->sequencer();
    ASSERT_FALSE(sequencer.empty("magic"));
    ASSERT_TRUE(sequencer.empty(MAIN_SEQUENCER_NAME));

    // Overcome timeout.
    loop->advanceToTime(101);

    ASSERT_TRUE(CheckSendEvent(root, 1));

    ASSERT_FALSE(root->hasEvent());

    loop->advanceToTime(201);
    ASSERT_FALSE(root->hasEvent());

    loop->advanceToTime(301);

    ASSERT_TRUE(CheckSendEvent(root, 2));

    ASSERT_FALSE(root->hasEvent());
}

static const char *PARALLEL_ON_SECONDARY = R"([
  {
    "type": "Parallel",
    "sequencer": "magic",
    "commands": [
      {
        "type": "SendEvent",
        "delay": 100,
        "arguments": [1]
      },
      {
        "type": "SendEvent",
        "delay": 200,
        "arguments": [2]
      }
    ]
  }
])";

TEST_F(SequencerTest, ParallelOnSequencer)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(PARALLEL_ON_SECONDARY, false);
    auto sequencer = context->sequencer();
    ASSERT_FALSE(sequencer.empty("magic"));
    ASSERT_TRUE(sequencer.empty(MAIN_SEQUENCER_NAME));

    // Overcome timeout.
    loop->advanceToTime(101);

    ASSERT_TRUE(CheckSendEvent(root, 1));

    ASSERT_FALSE(root->hasEvent());

    loop->advanceToTime(201);

    ASSERT_TRUE(CheckSendEvent(root, 2));

    ASSERT_FALSE(root->hasEvent());
}

static const char *SEQUENTIAL_ON_DIFFERENT_SEQUENCER = R"([
  {
    "type": "Sequential",
    "sequencer": "secondary",
    "commands": [
      {
        "type": "SendEvent",
        "delay": 100,
        "sequencer": "tertiary",
        "arguments": [3]
      },
      {
        "type": "SendEvent",
        "delay": 200,
        "arguments": [2]
      }
    ]
  }
])";

TEST_F(SequencerTest, SequentialOnDifferentSequencer)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(SEQUENTIAL_ON_DIFFERENT_SEQUENCER, false);
    // Terminate "parent" sequencer.
    execute(TERMINATE_SECONDARY, false);
    auto sequencer = context->sequencer();

    // Overcome timeout.
    loop->advanceToEnd();

    // One that was scheduled on separate sequencer should still fire.
    ASSERT_TRUE(CheckSendEvent(root, 3));

    ASSERT_FALSE(root->hasEvent());
}

TEST_F(SequencerTest, SequentialOnDifferentSequencerTerminate)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(SEQUENTIAL_ON_DIFFERENT_SEQUENCER, false);
    // Terminate child sequencer.
    execute(TERMINATE_TERTIARY, false);

    // Overcome timeout.
    loop->advanceToEnd();

    // One that was scheduled on separate sequencer should still fire.
    ASSERT_TRUE(CheckSendEvent(root, 2));

    ASSERT_FALSE(root->hasEvent());
}

static const char *SEQUENTIAL_WITH_FINALLY = R"([
  {
    "type": "Sequential",
    "sequencer": "secondary",
    "commands": [
      {
        "type": "SendEvent",
        "delay": 100,
        "sequencer": "tertiary",
        "arguments": [3]
      },
      {
        "type": "SendEvent",
        "delay": 200,
        "arguments": [2]
      }
    ],
    "finally": [
      {
        "delay": 200,
        "type": "SendEvent",
        "arguments": [0]
      }
    ]
  }
])";

TEST_F(SequencerTest, SequentialWithFinally)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(SEQUENTIAL_WITH_FINALLY, false);
    // Terminate "parent" sequencer.
    execute(TERMINATE_SECONDARY, false);

    // Overcome timeout.
    loop->advanceToEnd();

    // Finally happened on termination so will run in fast mode, not parent sequencer.
    ASSERT_TRUE(session->checkAndClear());

    ASSERT_TRUE(CheckSendEvent(root, 3));

    ASSERT_FALSE(root->hasEvent());
}

TEST_F(SequencerTest, SequentialWithFinallyTerminate)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(SEQUENTIAL_WITH_FINALLY, false);
    // Terminate "child" sequencer.
    execute(TERMINATE_TERTIARY, false);
    // Terminate on main will be ignored
    execute(TERMINATE_MAIN, false);

    // Overcome timeout.
    loop->advanceToEnd();

    ASSERT_TRUE(CheckSendEvent(root, 2));

    ASSERT_TRUE(CheckSendEvent(root, 0));

    ASSERT_FALSE(root->hasEvent());
}

static const char *PARALLEL_ON_DIFFERENT_SEQUENCER = R"([
  {
    "type": "Parallel",
    "sequencer": "secondary",
    "commands": [
      {
        "type": "SendEvent",
        "delay": 100,
        "sequencer": "tertiary",
        "arguments": [3]
      },
      {
        "type": "SendEvent",
        "delay": 200,
        "arguments": [2]
      }
    ]
  }
])";

TEST_F(SequencerTest, ParallelOnDifferentSequencer)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(PARALLEL_ON_DIFFERENT_SEQUENCER, false);
    // Terminate "parent" sequencer.
    execute(TERMINATE_SECONDARY, false);

    // Overcome timeout.
    loop->advanceToEnd();

    // One that was scheduled on separate sequencer should still fire.
    ASSERT_TRUE(CheckSendEvent(root, 3));

    ASSERT_FALSE(root->hasEvent());
}

TEST_F(SequencerTest, ParallelOnDifferentSequencerTerminate)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(PARALLEL_ON_DIFFERENT_SEQUENCER, false);
    // Terminate "parent" sequencer.
    execute(TERMINATE_TERTIARY, false);

    // Overcome timeout.
    loop->advanceToEnd();

    // One that was scheduled on separate sequencer should still fire.
    ASSERT_TRUE(CheckSendEvent(root, 2));

    ASSERT_FALSE(root->hasEvent());
}

static const char *SELECT_ON_DIFFERENT_SEQUENCER = R"([
  {
    "type": "Select",
    "sequencer": "secondary",
    "commands": [
      {
        "when": "${environment.agentVersion == '1.0'}",
        "type": "SendEvent",
        "delay": 100,
        "sequencer": "tertiary",
        "arguments": [3]
      },
      {
        "when": "${environment.agentVersion == '1.1'}",
        "type": "SendEvent",
        "delay": 200,
        "arguments": [2]
      }
    ]
  }
])";

TEST_F(SequencerTest, SelectOnDifferentSequencer)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(SELECT_ON_DIFFERENT_SEQUENCER, false);

    // Overcome timeout.
    loop->advanceToEnd();

    // One that was scheduled on separate sequencer should still fire.
    ASSERT_TRUE(CheckSendEvent(root, 3));

    ASSERT_FALSE(root->hasEvent());
}

TEST_F(SequencerTest, SelectOnDifferentSequencerTerminate)
{
    config->agent("Unit tests", "1.1");
    loadDocument(BASIC);

    // Should schedule send event
    execute(SELECT_ON_DIFFERENT_SEQUENCER, false);

    // Overcome timeout.
    loop->advanceToEnd();

    // One that was scheduled on separate sequencer should still fire.
    ASSERT_TRUE(CheckSendEvent(root, 2));

    ASSERT_FALSE(root->hasEvent());
}

static const char *SELECT_OTHERWISE = R"([
  {
    "type": "Select",
    "sequencer": "secondary",
    "commands": [
      {
        "when": "${environment.agentVersion == '1.0'}",
        "type": "SendEvent",
        "delay": 100,
        "sequencer": "tertiary",
        "arguments": [3]
      },
      {
        "when": "${environment.agentVersion == '1.1'}",
        "type": "SendEvent",
        "delay": 200,
        "arguments": [2]
      }
    ],
    "otherwise": [
      {
        "type": "SendEvent",
        "arguments": [0]
      }
    ]
  }
])";

TEST_F(SequencerTest, SelectOtherwise)
{
    config->agent("Unit tests", "1.2");
    loadDocument(BASIC);

    // Should schedule send event
    execute(SELECT_OTHERWISE, false);

    // Terminate on main will be ignored
    execute(TERMINATE_MAIN, false);

    // Overcome timeout.
    loop->advanceToEnd();

    // One that was scheduled on separate sequencer should still fire.
    ASSERT_TRUE(CheckSendEvent(root, 0));

    ASSERT_FALSE(root->hasEvent());
}

static const char *MAIN_AND_SECONDARY = R"([
  {
    "type": "SendEvent",
    "delay": 100,
    "arguments": [1]
  },
  {
    "type": "SendEvent",
    "delay": 200,
    "sequencer": "secondary",
    "arguments": [2]
  }
])";

TEST_F(SequencerTest, EscalateToNormal)
{
    loadDocument(BASIC);

    // Should schedule send event
    execute(MAIN_AND_SECONDARY, true);

    // Overcome timeout.
    loop->advanceToEnd();

    // We ignore one that was on the main sequencer
    ASSERT_TRUE(session->checkAndClear());

    // One that was scheduled on separate sequencer should still fire.
    ASSERT_TRUE(CheckSendEvent(root, 2));

    ASSERT_FALSE(root->hasEvent());
}

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

TEST_F(SequencerTest, SpeakItemAndPlayMediaForeground)
{
    loadDocument(SPEAK_ITEM_AND_VIDEO);

    execute(SPEAK_ITEM, false);

    loop->advanceToEnd();

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypePreroll, event.getType());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSpeak, event.getType());
    auto speakItem = event.getActionRef();

    ASSERT_FALSE(speakItem.isTerminated());

    // Same resource
    execute(PLAY_MEDIA_FOREGROUND, false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypePlayMedia, event.getType());
    auto playMedia = event.getActionRef();

    loop->advanceToEnd();

    // We are on different sequencers, but on the same resource, so first will be terminated.
    ASSERT_TRUE(speakItem.isTerminated());
    ASSERT_TRUE(playMedia.isPending());
}

static const char *TWO_VIDEO = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": [
        {
          "type": "Video",
          "id": "video1",
          "source": ["URL1", "URL2"]
        },
        {
          "type": "Video",
          "id": "video2",
          "source": ["URL1", "URL2"]
        }
      ]
    }
  }
})";

static const char *PLAY_MEDIA_BACKGROUND_1 = R"([
  {
    "type": "PlayMedia",
    "componentId": "video1",
    "source": "http://music.amazon.com/s3/MAGIC_TRACK_HERE",
    "audioTrack": "foreground",
    "sequencer": "secondary"
  }
])";

static const char *CONTROL_MEDIA_PLAY_MEDIA_BACKGROUND_2 = R"([
  {
    "type": "ControlMedia",
    "componentId": "video2",
    "audioTrack": "foreground",
    "command": "play"
  }
])";

TEST_F(SequencerTest, PlayMediaControlMediaBackground)
{
    loadDocument(TWO_VIDEO);

    execute(PLAY_MEDIA_BACKGROUND_1, false);

    loop->advanceToEnd();

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypePlayMedia, event.getType());
    auto playMedia = event.getActionRef();

    ASSERT_FALSE(playMedia.isTerminated());

    // Same resource
    execute(CONTROL_MEDIA_PLAY_MEDIA_BACKGROUND_2, false);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeControlMedia, event.getType());
    auto controlMedia = event.getActionRef();

    loop->advanceToEnd();

    // We are on different sequencers, but on the same resource, so first will be terminated.
    ASSERT_TRUE(playMedia.isTerminated());
    ASSERT_TRUE(controlMedia.isPending());
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

TEST_F(SequencerTest, SpeakItemAndScroll)
{
    loadDocument(SCROLLABLE_SPEAK_ITEM);

    execute(SPEAK_ITEM, false);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypePreroll, event.getType());

    // Same resource
    execute(SCROLL_TO_POSITION, false);
    advanceTime(1000);
    ASSERT_EQ(Point(0,10), component->scrollPosition());

    // We are on different sequencers, but on the same resource, so first will be terminated and no speak will happen.
    ASSERT_FALSE(root->hasEvent());
}

static const char *SEQUENCE = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "id": "scroll",
      "height": "10dp",
      "data": [0, 1, 2, 3, 4, 5],
      "items": [
        {
          "type": "Text",
          "id": "text${data}",
          "height": "10dp",
          "text": "${data}"
        }
      ]
    }
  }
})";

static const char *SCROLL_TO_COMPONENT = R"([
  {
    "type": "ScrollToComponent",
    "componentId": "text3",
    "sequencer": "secondary"
  }
])";

TEST_F(SequencerTest, SequenceToComponent)
{
    loadDocument(SEQUENCE);

    execute(SCROLL_TO_COMPONENT, false);

    // Same resource
    execute(SCROLL_TO_POSITION, false);
    advanceTime(1000);
    ASSERT_EQ(Point(0,10), component->scrollPosition());
}

static const char *SCROLL_TO_INDEX = R"([
  {
    "type": "ScrollToIndex",
    "componentId": "scroll",
    "index": 3,
    "sequencer": "secondary"
  }
])";

TEST_F(SequencerTest, SequenceToIndex)
{
    loadDocument(SEQUENCE);

    execute(SCROLL_TO_INDEX, false);

    // Same resource
    execute(SCROLL_TO_POSITION, false);
    advanceTime(1000);
    ASSERT_EQ(Point(0,10), component->scrollPosition());
}

static const char *PAGER = R"(
{
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "id": "pager",
      "height": "10dp",
      "width": "10dp",
      "data": [0, 1, 2, 3, 4, 5],
      "items": [
        {
          "type": "Text",
          "id": "text${data}",
          "text": "${data}"
        }
      ]
    }
  }
})";

static const char *AUTO_PAGE = R"([
  {
    "type": "AutoPage",
    "componentId": "pager",
    "duration": 100,
    "sequencer": "secondary"
  }
])";

static const char *SET_PAGE = R"([
  {
    "type": "SetPage",
    "componentId": "pager",
    "value": 3,
    "sequencer": "tertiary"
  }
])";

TEST_F(SequencerTest, Pager)
{
    loadDocument(PAGER);

    execute(AUTO_PAGE, false);

    // Same resource
    execute(SET_PAGE, false);
    advanceTime(2000);
    ASSERT_EQ(3, component->pagePosition());
}

static const char *FRAME = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
  "item":
    {
      "type": "Frame",
      "id": "box",
      "width": 100,
      "height": 100
    }
  }
})";

static const char *ANIMATE_OPACITY = R"([
  {
    "type": "AnimateItem",
    "componentId": "box",
    "duration": 1000,
    "value": {
      "property": "opacity",
      "from": 0.5,
      "to": 0
    },
    "sequencer": "secondary"
  }
])";

static const char *SET_OPACITY = R"([
  {
    "type": "SetValue",
    "componentId": "box",
    "property": "opacity",
    "value": 0.75,
    "sequencer": "tertiary"
  }
])";

TEST_F(SequencerTest, Animate)
{
    loadDocument(FRAME);

    execute(ANIMATE_OPACITY, false);

    loop->advanceToTime(500);

    ASSERT_TRUE(CheckDirty(component, kPropertyOpacity));

    ASSERT_EQ(0.25, component->getCalculated(kPropertyOpacity).asNumber());

    execute(SET_OPACITY, false);

    loop->advanceToEnd();

    ASSERT_TRUE(CheckDirty(component, kPropertyOpacity));

    ASSERT_EQ(0.75, component->getCalculated(kPropertyOpacity).asNumber());
}

static const char *ANIMATE_TRANSFORM = R"([
  {
    "type": "AnimateItem",
    "componentId": "box",
    "duration": 1000,
    "value": {
      "property": "transform",
      "from": {
        "translateX": "100vw"
      },
      "to": {
        "translateX": 0
      }
    },
    "sequencer": "secondary"
  }
])";

TEST_F(SequencerTest, AnimateInParalell)
{
    loadDocument(FRAME);

    execute(ANIMATE_TRANSFORM, false);

    loop->advanceToTime(500);

    ASSERT_TRUE(CheckDirty(component, kPropertyTransform));

    ASSERT_EQ(Transform2D::translateX(512), component->getCalculated(kPropertyTransform).getTransform2D());

    execute(SET_OPACITY, false);

    loop->advanceToEnd();

    ASSERT_TRUE(CheckDirty(component, kPropertyOpacity, kPropertyTransform));

    ASSERT_EQ(Transform2D::translateX(0), component->getCalculated(kPropertyTransform).getTransform2D());
    ASSERT_EQ(0.75, component->getCalculated(kPropertyOpacity).asNumber());
}

static const char *BASIC_MACRO = R"({
  "type": "APL",
  "version": "1.0",
  "commands": {
    "basic": {
      "parameters": [],
      "commands": {
        "type": "SendEvent",
        "arguments": [
          "Hello"
        ]
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "basic",
        "delay": 200,
        "sequencer": "secondary"
      }
    }
  }
})";

TEST_F(SequencerTest, BasicMacro)
{
    loadDocument(BASIC_MACRO);

    auto map = component->getCalculated();
    auto onPress = map[kPropertyOnPress];

    ASSERT_TRUE(onPress.isArray());
    ASSERT_EQ(1, onPress.size());

    component->update(kUpdatePressed, 1);

    // Submit idle on main sequencer will terminate it (it's empty anyway)
    execute(TERMINATE_MAIN, false);

    loop->advanceToEnd();

    ASSERT_TRUE(CheckSendEvent(root, "Hello"));
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


TEST_F(SequencerTest, Pager_1_3)
{
    loadDocument(PAGER_1_3);

    execute(PAGER_1_3_CMD, false);

    loop->advanceToEnd();

    // speak item preroll
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypePreroll, event.getType());

    advanceTime(600);
    ASSERT_EQ(1, component->pagePosition());

    // expect speak
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSpeak, event.getType());
    auto speak = event.getActionRef();
    ASSERT_FALSE(speak.isTerminated());
}

static const char *BASIC_1_3 = R"({
      "type": "APL",
      "version": "1.3",
      "mainTemplate": {
        "item": {
          "type": "Container"
        }
      }
    }
  )";

TEST_F(SequencerTest, SequentialOnSequencer13)
{
    loadDocument(BASIC_1_3);

    // Should schedule send event
    execute(SEQUENTIAL_ON_SECONDARY, false);
    loop->advanceToEnd();

    auto sequencer = context->sequencer();
    ASSERT_TRUE(sequencer.empty("magic"));
    ASSERT_FALSE(sequencer.empty(MAIN_SEQUENCER_NAME));

    CheckSendEvent(root, 1);
    CheckSendEvent(root, 2);

    sequencer.reset();
    ASSERT_TRUE(sequencer.empty("magic"));
    ASSERT_TRUE(sequencer.empty(MAIN_SEQUENCER_NAME));
}
