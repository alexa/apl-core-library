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

using namespace apl;

/**
 * The unit tests in this file address a bug found in deep evaluation of command
 * properties.  Certain commands such as SetValue have arguments that may include
 * nested data structures.  Data-binding expressions inside of those nested data
 * structures often refer to properties in the event data-binding context such
 * as "event.source.XXX".
 *
 * Please note that not all command properties should be deeply evalauted when they
 * are first calculated.  For example, the Sequential command has a list of commands
 * to be executed.  The properties in each of the subcommands are evaluated when
 * that subcommand is executed because they may, in fact, depend on properties set by
 * earlier subcommands.
 */
class CommandEventBinding : public DocumentWrapper {};

static const char *EVENT_DATA_IN_TRANSFORM = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "ScrollView",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Text",
            "id": "TEXT",
            "width": 100,
            "height": 400
          },
          "onScroll": [
            {
              "type": "SetValue",
              "componentId": "TEXT",
              "property": "text",
              "value": "${event.source.position}"
            },
            {
              "type": "SetValue",
              "componentId": "TEXT",
              "property": "transform",
              "value": {
                "translateX": "${event.source.position}"
              }
            }
          ]
        }
      }
    }
)apl";

/**
 * Verify that we can call SetValue with event data-binding inside
 * of a transform object.  In this case we use a scroll view to set
 * a transform that is proportional to how far we've scrolled.
 */
TEST_F(CommandEventBinding, EventDataInTransform)
{
    loadDocument(EVENT_DATA_IN_TRANSFORM);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);
    ASSERT_TRUE(IsEqual("", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Transform2D(), text->getCalculated(kPropertyTransform)));

    component->update(kUpdateScrollPosition, 100);  // Scroll down exactly one page
    root->clearPending();

    ASSERT_TRUE(IsEqual("1", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Transform2D::translateX(1), text->getCalculated(kPropertyTransform)));
}


static const char *SEND_EVENT_ARGUMENTS = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "ScrollView",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 400
          },
          "onScroll": [
            {
              "type": "SendEvent",
              "sequencer": "DUMMY",
              "arguments": [
                "${event.source.position}",
                {
                  "inner": "${event.source.position}"
                },
                [
                  "${event.source.position}"
                ]
              ]
            }
          ]
        }
      }
    }
)apl";

/**
 * The arguments array of SendEvent can have complicated data structures.  These
 * should all be recursively evaluated.
 */
TEST_F(CommandEventBinding, SendEventArguments)
{
    loadDocument(SEND_EVENT_ARGUMENTS);
    ASSERT_TRUE(component);

    component->update(kUpdateScrollPosition, 100);
    root->clearPending();

    rapidjson::Document doc;
    rapidjson::Value v(rapidjson::kObjectType);
    v.AddMember("inner", 1, doc.GetAllocator());
    rapidjson::Value v2(rapidjson::kArrayType);
    v2.PushBack(1, doc.GetAllocator());
    ASSERT_TRUE(CheckSendEvent(root, 1, Object(v), Object(v2)));
}


static const char *SEND_EVENT_ARRAY_INTERPOLATION = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "ScrollView",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Text",
            "id": "TEXT",
            "width": 100,
            "height": 400
          },
          "bind": {
            "name": "FOO",
            "value": [
              8,
              9
            ]
          },
          "onScroll": [
            {
              "type": "SendEvent",
              "sequencer": "DUMMY",
              "arguments": [
                "${event.source.position}",
                "${FOO}",
                [ 2, 3 ],
                ["${FOO}"]
              ]
            }
          ]
        }
      }
    }
)apl";

/**
 * FOO = [8,9]
 * event.source.position = 1
 * arguments = [ "${event.source.position}", "${FOO}", [2,3], ["${FOO}"]]
 *         ==> [ 1, 8, 9, [2,3], [8, 9]]
 */
TEST_F(CommandEventBinding, SendEventArrayInterpolation)
{
    loadDocument(SEND_EVENT_ARRAY_INTERPOLATION);
    ASSERT_TRUE(component);

    component->update(kUpdateScrollPosition, 100);
    root->clearPending();

    rapidjson::Document doc;
    rapidjson::Value v(rapidjson::kArrayType);
    v.PushBack(8, doc.GetAllocator());
    v.PushBack(9, doc.GetAllocator());

    rapidjson::Value v2(rapidjson::kArrayType);
    v2.PushBack(2, doc.GetAllocator());
    v2.PushBack(3, doc.GetAllocator());

    ASSERT_TRUE(CheckSendEvent(root, 1, 8, 9, Object(v2), Object(v)));
}


static const char *VIDEO_COMPONENT_EVENT_INTERPOLATION = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Video",
          "bind": [
            {
              "name": "OTHER",
              "value": 13
            },
            {
              "name": "ClipList",
              "value": [
                "track2",
                "track${OTHER}"
              ]
            }
          ],
          "source": [
            "${ClipList}",
            "track3"
          ],
          "width": 100,
          "height": 100,
          "onPause": [
            {
              "type": "SetValue",
              "property": "source",
              "value": [
                "clip${event.trackIndex}-${event.trackCount}"
              ]
            }
          ]
        }
      }
    }
)apl";

/**
 * The "source" field for a video component takes simple text strings and rich data objects.
 * These should be recursively evaluated in the event context when they are evaluated as media source objects.
 */
TEST_F(CommandEventBinding, VideoComponentEventInterpolation)
{
    loadDocument(VIDEO_COMPONENT_EVENT_INTERPOLATION);
    ASSERT_TRUE(component);

    auto array = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(3, array.size());
    ASSERT_EQ("track2", array.at(0).getMediaSource().getUrl());
    ASSERT_EQ("track13", array.at(1).getMediaSource().getUrl());
    ASSERT_EQ("track3", array.at(2).getMediaSource().getUrl());

    // Start playback
    component->updateMediaState(MediaState(0,
                                           3,
                                           0,
                                           12000,
                                           false,
                                           false));

    // Pause the video
    component->updateMediaState(MediaState(0,
                                           3,
                                           230,
                                           12000,
                                           true,
                                           false));
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertySource));
    ASSERT_TRUE(CheckDirty(root, component));

    array = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(1, array.size());
    ASSERT_EQ("clip0-3", array.at(0).getMediaSource().getUrl());
}
