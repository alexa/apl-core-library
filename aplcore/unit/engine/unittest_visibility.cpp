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

#include <algorithm>

using namespace apl;

class ViewabilityTest : public DocumentWrapper {
public:
    void TearDown() override
    {
        changes.clear();
        DocumentWrapper::TearDown();
    }

    bool collectVisibilityChanges() {
        root->clearPending();
        auto hasChanges = false;
        while (root->hasEvent()) {
            auto event = root->popEvent();
            auto args = event.getValue(apl::kEventPropertyArguments).getArray();
            changes.emplace_back(args.at(0).asString());
            hasChanges = true;
        }
        return hasChanges;
    }

    ::testing::AssertionResult CheckVisibilityChange(const std::string& change) {
        if (changes.empty()) return ::testing::AssertionFailure() << "No changes available";

        auto it = std::find(changes.begin(), changes.end(), "Visibility:" + change);
        if (it == changes.end()) {
            auto fail = ::testing::AssertionFailure() << "Have no expected VC: " << "Visibility:" + change;
            for (const auto& c : changes)
                fail << "\n" << c;

            return fail;
        }

        changes.erase(it);

        return ::testing::AssertionSuccess();
    }

    std::vector<std::string> changes;
};

static const char *BASIC_TEST = R"({
  "type": "APL",
  "version": "2024.1",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": 1200,
      "height": 800,
      "direction": "row",
      "wrap": "wrap",
      "items": [
        {
          "type": "Frame",
          "id": "parent0",
          "opacity": 0.75,
          "width": 600,
          "height": 450,
          "borderColor": "green",
          "borderWidth": 5,
          "item": {
            "type": "Frame",
            "opacity": 0.75,
            "width": "100%",
            "height": "100%",
            "borderColor": "red",
            "borderWidth": 5,
            "id": "fullViewTransparent",
            "handleVisibilityChange": {
              "commands": {
                "type": "SendEvent",
                "sequencer": "VC",
                "arguments": [ "Visibility:${event.source.id}:${event.visibleRegionPercentage}:${event.cumulativeOpacity}" ]
              }
            }
          }
        },
        {
          "type": "Frame",
          "id": "parent1",
          "width": 600,
          "height": 450,
          "borderColor": "green",
          "borderWidth": 5,
          "item": {
            "type": "Sequence",
            "id": "parentSequence",
            "width": "100%",
            "height": "100%",
            "data": [
              "red",
              "yellow",
              "blue"
            ],
            "items": {
              "type": "Frame",
              "width": "100%",
              "height": 250,
              "borderColor": "${data}",
              "borderWidth": 5,
              "id": "inSequence${data}",
              "handleVisibilityChange": {
                "commands": {
                  "type": "SendEvent",
                  "sequencer": "VC",
                  "arguments": [ "Visibility:${event.source.id}:${event.visibleRegionPercentage}:${event.cumulativeOpacity}" ]
                }
              }
            }
          }
        },
        {
          "type": "Frame",
          "id": "parent2",
          "width": 600,
          "height": 450,
          "borderColor": "green",
          "borderWidth": 5,
          "item": {
            "type": "Frame",
            "opacity": 0.75,
            "width": "100%",
            "height": "100%",
            "borderColor": "red",
            "borderWidth": 5,
            "id": "cutOutByGlobalViewport",
            "handleVisibilityChange": {
              "commands": {
                "type": "SendEvent",
                "sequencer": "VC",
                "arguments": [ "Visibility:${event.source.id}:${event.visibleRegionPercentage}:${event.cumulativeOpacity}" ]
              }
            }
          }
        },
        {
          "type": "Frame",
          "id": "parent3",
          "width": 600,
          "height": 450,
          "borderColor": "green",
          "borderWidth": 5,
          "item": {
            "type": "Frame",
            "opacity": 0.75,
            "width": "200%",
            "height": "100%",
            "borderColor": "red",
            "borderWidth": 5,
            "id": "cutOutByInception",
            "handleVisibilityChange": {
              "commands": {
                "type": "SendEvent",
                "sequencer": "VC",
                "arguments": [ "Visibility:${event.source.id}:${event.visibleRegionPercentage}:${event.cumulativeOpacity}" ]
              }
            },
            "item": {
              "type": "Frame",
              "opacity": 0.75,
              "width": "200%",
              "height": "100%",
              "borderColor": "blue",
              "borderWidth": 5,
              "id": "cutOutByDeepInception",
              "handleVisibilityChange": {
                "commands": {
                  "type": "SendEvent",
                  "sequencer": "VC",
                  "arguments": [ "Visibility:${event.source.id}:${event.visibleRegionPercentage}:${event.cumulativeOpacity}" ]
                }
              }
            }
          }
        }
      ]
    }
  }
})";

TEST_F(ViewabilityTest, Changes)
{
    // Set for changes to ensure ordering.
    auto changesBag = std::set<std::string>();

    metrics.size(1200, 800);

    loadDocument(BASIC_TEST);

    ASSERT_TRUE(component);

    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("fullViewTransparent:1:0.5625"));
    ASSERT_TRUE(CheckVisibilityChange("inSequencered:1:1"));
    ASSERT_TRUE(CheckVisibilityChange("inSequenceyellow:0.76:1"));
    ASSERT_TRUE(CheckVisibilityChange("inSequenceblue:0:1"));
    ASSERT_TRUE(CheckVisibilityChange("cutOutByGlobalViewport:0.784091:0.75"));
    ASSERT_TRUE(CheckVisibilityChange("cutOutByInception:0.395368:0.75"));
    ASSERT_TRUE(CheckVisibilityChange("cutOutByDeepInception:0.199364:0.5625"));

    executeCommand("SetValue", {{"componentId", "fullViewTransparent"}, {"property", "opacity"}, {"value", 1.0}}, true);
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("fullViewTransparent:1:0.75"));

    executeCommand("SetValue", {{"componentId", "parent0"}, {"property", "opacity"}, {"value", 1.0}}, true);
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("fullViewTransparent:1:1"));

    executeCommand("Scroll", {{"componentId", "parentSequence"}, {"distance", 2}}, false);
    advanceTime(5000);
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("inSequencered:0:1"));
    ASSERT_TRUE(CheckVisibilityChange("inSequenceblue:1:1"));

    executeCommand("SetValue", {{"componentId", "cutOutByInception"}, {"property", "width"}, {"value", "100%"}}, true);
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("cutOutByInception:0.784091:0.75"));
    ASSERT_TRUE(CheckVisibilityChange("cutOutByDeepInception:0.398757:0.5625"));
}

static const char *VISIBLE_FOR_TIME = R"({
  "type": "APL",
  "version": "2024.1",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "height": 250,
      "width": 100,
      "bind": [
        { "name": "VisibleStartTime", "value": 0, "type": "number" },
        { "name": "EndOfListVisible", "value": false, "type": "boolean" },
        { "name": "TimeReached", "value": false, "type": "boolean" }
      ],
      "handleTick": {
        "when": "${!TimeReached && EndOfListVisible}",
        "minimumDelay": 100,
        "commands": {
          "type": "Sequential",
          "when": "${elapsedTime - VisibleStartTime >= 1000}",
          "commands": [
            {
              "type": "SetValue",
              "property": "TimeReached",
              "value": true
            },
            {
              "type": "SendEvent",
              "sequencer": "NOTIFY_ME",
              "arguments": [ "LastItem was visible for ${elapsedTime - VisibleStartTime} ms" ]
            }
          ]
        }
      },
      "data": [ "red", "yellow", "green", "blue" ],
      "items": {
        "type": "Frame",
        "backgroundColor": "${data}",
        "height": 100,
        "width": 100
      },
      "lastItem": {
        "type": "Frame",
        "backgroundColor": "pink",
        "height": 100,
        "width": 100,
        "handleVisibilityChange": {
          "when": "${!TimeReached}",
          "commands": [
            {
              "type": "SetValue",
              "property": "EndOfListVisible",
              "value": "${event.visibleRegionPercentage > 0.5 && event.cumulativeOpacity > 0}"
            },
            {
              "when": "${EndOfListVisible && VisibleStartTime < 0}",
              "type": "SetValue",
              "property": "VisibleStartTime",
              "value": "${elapsedTime}"
            },
            {
              "when": "${!EndOfListVisible && VisibleStartTime > 0}",
              "type": "SetValue",
              "property": "VisibleStartTime",
              "value": -1
            }
          ]
        }
      }
    }
  }
})";

TEST_F(ViewabilityTest, VisibleForTime)
{
    loadDocument(VISIBLE_FOR_TIME);

    ASSERT_TRUE(component);
    advanceTime(16);

    executeCommand("Scroll", {{"componentId", ":root"}, {"distance", 2}}, false);

    // Advance 100 frames
    for (auto i = 0; i < 100; i++) {
        advanceTime(16);
    }

    ASSERT_TRUE(root->hasEvent());
    ASSERT_TRUE(CheckSendEvent(root, "LastItem was visible for 1088 ms"));

    advanceTime(100);

    // No new events, only fired once
    ASSERT_FALSE(root->hasEvent());
}

static const char *UPDATES_ON_CHANGES_ONLY = R"({
  "type": "APL",
  "version": "2024.1",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "id": "level1",
      "opacity": 1,
      "width": 500,
      "height": 500,
      "borderColor": "blue",
      "borderWidth": 10,
      "items": [
        {
          "type": "Frame",
          "id": "level2",
          "opacity": 0.75,
          "width": 480,
          "height": 480,
          "borderColor": "green",
          "borderWidth": 10,
          "item": {
            "type": "Frame",
            "opacity": 0.75,
            "width": 460,
            "height": 460,
            "borderColor": "red",
            "borderWidth": 10,
            "id": "level3",
            "handleVisibilityChange": {
              "commands": {
                "type": "SendEvent",
                "sequencer": "VC",
                "arguments": [
                  "Visibility:${event.source.id}:${event.visibleRegionPercentage}:${event.cumulativeOpacity}"
                ]
              }
            }
          }
        }
      ]
    }
  }
})";

TEST_F(ViewabilityTest, UpdatesOnChangesOnly)
{
    loadDocument(UPDATES_ON_CHANGES_ONLY);

    ASSERT_TRUE(component);

    // Initial visibility state
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("level3:1:0.5625"));

    executeCommand("SetValue", {{"componentId", "level2"}, {"property", "opacity"}, {"value", 1.0}}, true);

    advanceTime(16);
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("level3:1:0.75"));

    // Dirty property which results in same values make no difference
    executeCommand("SetValue", {{"componentId", "level2"}, {"property", "opacity"}, {"value", 0.75}}, true);
    executeCommand("SetValue", {{"componentId", "level2"}, {"property", "opacity"}, {"value", 1.0}}, true);

    advanceTime(16);
    ASSERT_FALSE(collectVisibilityChanges());

    // Change size
    executeCommand("SetValue", {{"componentId", "level3"}, {"property", "width"}, {"value", 920}}, true);
    advanceTime(16);
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("level3:0.51087:0.75"));

    // Swap
    executeCommand("SetValue", {{"componentId", "level3"}, {"property", "width"}, {"value", 460}}, true);
    executeCommand("SetValue", {{"componentId", "level3"}, {"property", "height"}, {"value", 920}}, true);

    advanceTime(16);
    ASSERT_FALSE(collectVisibilityChanges());
}

static const char *UPDATE_ONCE = R"({
  "type": "APL",
  "version": "2024.1",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": 480,
      "height": 480,
      "items": {
        "type": "Frame",
        "bind": [
          {
            "name": "VisibilityReported",
            "type": "boolean",
            "value": false
          }
        ],
        "id": "level1",
        "opacity": 0,
        "width": 480,
        "height": 480,
        "borderColor": "green",
        "borderWidth": 10,
        "handleVisibilityChange": {
          "when": "${!VisibilityReported}",
          "commands": [
            {
              "type": "SendEvent",
              "sequencer": "VC",
              "arguments": [
                "Visibility:${event.source.id}:${event.visibleRegionPercentage}:${event.cumulativeOpacity}"
              ]
            },
            {
              "type": "SetValue",
              "property": "VisibilityReported",
              "value": "${event.visibleRegionPercentage > 0 && event.cumulativeOpacity > 0}"
            }
          ]
        }
      }
    }
  }
})";

TEST_F(ViewabilityTest, UpdatesOnce)
{
    loadDocument(UPDATE_ONCE);

    ASSERT_TRUE(component);

    // Initial visibility state
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("level1:1:0"));

    executeCommand("SetValue", {{"componentId", "level1"}, {"property", "opacity"}, {"value", 1.0}}, true);

    advanceTime(16);
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("level1:1:1"));

    // Dirty property which results in same values make no difference
    executeCommand("SetValue", {{"componentId", "level1"}, {"property", "opacity"}, {"value", 0.75}}, true);

    advanceTime(16);
    ASSERT_FALSE(collectVisibilityChanges());
}

static const char *DEREGISTER = R"({
  "type": "APL",
  "version": "2024.1",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "data": "${TestArray}",
      "width": 240,
      "height": 480,
      "items": {
        "type": "Frame",
        "id": "box${data}",
        "width": 240,
        "height": 240,
        "borderColor": "${data}",
        "borderWidth": 10,
        "handleVisibilityChange": {
          "commands": [
            {
              "type": "SendEvent",
              "sequencer": "VC",
              "arguments": [
                "Visibility:${event.source.id}:${event.visibleRegionPercentage}:${event.cumulativeOpacity}"
              ]
            }
          ]
        }
      }
    }
  }
})";

TEST_F(ViewabilityTest, Deregister)
{
    auto myArray = LiveArray::create(ObjectArray{"red", "green"});
    config->liveData("TestArray", myArray);

    loadDocument(DEREGISTER);

    // Initial visibility state
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("boxred:1:1"));
    ASSERT_TRUE(CheckVisibilityChange("boxgreen:1:1"));

    myArray->remove(0);
    advanceTime(10);
    ASSERT_FALSE(collectVisibilityChanges());

    executeCommand("SetValue", {{"componentId", ":root"}, {"property", "opacity"}, {"value", 0.75}}, true);

    advanceTime(10);
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_FALSE(CheckVisibilityChange("boxred:1:0.75"));
    ASSERT_TRUE(CheckVisibilityChange("boxgreen:1:0.75"));
}

static const char *ROOT_VISIBILITY_AND_REINFLATION = R"({
  "type": "APL",
  "version": "2024.1",
  "theme": "dark",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "preserve": ["opacity"],
      "id": "root",
      "width": 500,
      "height": 400,
      "borderColor": "red",
      "borderWidth": 10,
      "handleVisibilityChange": {
        "commands": [
          {
            "type": "SendEvent",
            "sequencer": "VC",
            "arguments": [
              "Visibility:${event.source.id}:${event.visibleRegionPercentage}:${event.cumulativeOpacity}"
            ]
          }
        ]
      }
    }
  }
})";

TEST_F(ViewabilityTest, RootVisibility)
{
    metrics.size(400, 400);

    loadDocument(ROOT_VISIBILITY_AND_REINFLATION);

    // Initial visibility state
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("root:0.8:1"));

    executeCommand("SetValue", {{"componentId", ":root"}, {"property", "opacity"}, {"value", 0.75}}, true);

    advanceTime(10);
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("root:0.8:0.75"));

    configChangeReinflate(ConfigurationChange(500, 500));

    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("root:1:0.75"));
}

static const char *CHILDREN_CHANGE_AND_MOUNT = R"({
  "type": "APL",
  "version": "2024.1",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "data": "${TestArray}",
      "width": 240,
      "height": 480,
      "onChildrenChanged": {
        "type": "Sequential",
        "sequencer": "CHILD_CHANGE",
        "data": "${event.changes}",
        "commands": {
          "type": "SendEvent",
          "arguments": [
            "childChange:${data.action}"
          ]
        }
      },
      "items": {
        "type": "Frame",
        "id": "box${data}",
        "width": 240,
        "height": 240,
        "borderColor": "${data}",
        "borderWidth": 10,
        "onMount": {
          "type": "SendEvent",
          "sequencer": "MOUNT",
          "arguments": [
            "onMount:${event.source.id}"
          ]
        },
        "handleVisibilityChange": {
          "commands": [
            {
              "type": "SendEvent",
              "sequencer": "VC",
              "arguments": [
                "Visibility:${event.source.id}:${event.visibleRegionPercentage}:${event.cumulativeOpacity}"
              ]
            }
          ]
        }
      }
    }
  }
})";

TEST_F(ViewabilityTest, EventOrdering)
{
    auto myArray = LiveArray::create(ObjectArray{"red", "green"});
    config->liveData("TestArray", myArray);

    loadDocument(CHILDREN_CHANGE_AND_MOUNT);

    // onMount happens first
    ASSERT_TRUE(CheckSendEvent(root, "onMount:boxred"));
    ASSERT_TRUE(CheckSendEvent(root, "onMount:boxgreen"));


    // Initial visibility state
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_TRUE(CheckVisibilityChange("boxred:1:1"));
    ASSERT_TRUE(CheckVisibilityChange("boxgreen:1:1"));

    myArray->remove(0);
    advanceTime(10);

    ASSERT_TRUE(CheckSendEvent(root, "childChange:remove"));

    ASSERT_FALSE(collectVisibilityChanges());

    executeCommand("SetValue", {{"componentId", ":root"}, {"property", "opacity"}, {"value", 0.75}}, true);

    advanceTime(10);
    ASSERT_TRUE(collectVisibilityChanges());
    ASSERT_FALSE(CheckVisibilityChange("boxred:1:0.75"));
    ASSERT_TRUE(CheckVisibilityChange("boxgreen:1:0.75"));
}

static const char *SIMPLE_SCROLL_VIEW = R"({
  "type": "APL",
  "version": "2024.1",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "bind": [
        {
          "name": "Percentage",
          "value": -1
        }
      ],
      "items": [
        {
          "type": "Text",
          "text": "${Percentage}"
        },
        {
          "type": "ScrollView",
          "width": "100%",
          "height": 500,
          "item": {
            "type": "Container",
            "width": "100%",
            "height": 1000,
            "items": {
              "type": "Frame",
              "width": 100,
              "height": 100,
              "position": "absolute",
              "top": 500,
              "borderWidth": 2,
              "borderColor": "blue",
              "handleVisibilityChange": {
                "commands": [
                  {
                    "type": "SetValue",
                    "componentId": ":root",
                    "property": "Percentage",
                    "value": "${event.visibleRegionPercentage}"
                  }
                ]
              }
            }
          }
        }
      ]
    }
  }
})";

TEST_F(ViewabilityTest, SimpleScrollView)
{
    loadDocument(SIMPLE_SCROLL_VIEW);

    ASSERT_TRUE(component);
    advanceTime(16);

    ASSERT_EQ("0", component->getChildAt(0)->getCalculated(apl::kPropertyText).asString());

    executeCommand("Scroll", {{"componentId", ":root:child(1)"}, {"distance", 2}}, false);

    // Advance 100 frames
    for (auto i = 0; i < 100; i++) {
        advanceTime(16);
    }

    ASSERT_EQ("1", component->getChildAt(0)->getCalculated(apl::kPropertyText).asString());
}