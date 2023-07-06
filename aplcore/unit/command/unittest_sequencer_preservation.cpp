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

#include "apl/animation/coreeasing.h"

using namespace apl;

class SequencerPreservationTest : public DocumentWrapper {};

static const char *COMMAND_ENGINE = R"apl({
 "type": "APL",
 "version": "2022.1",
 "theme": "dark",
 "onConfigChange": {
   "type": "Reinflate",
   "preservedSequencers": ["MAGIC"]
 },
 "mainTemplate": {
   "items": [
     {
       "type": "Container",
       "items": {
         "when": "${viewport.pixelWidth > 350}",
         "type": "Frame",
         "id": "framy",
         "opacity": 1
       }
     }
   ]
 }
})apl";

TEST_F(SequencerPreservationTest, Delay)
{
   loadDocument(COMMAND_ENGINE);

   auto action = executeCommand("SendEvent", {{"sequencer", "MAGIC"}, {"delay", 1000}}, false);

   advanceTime(250);

   configChange(ConfigurationChange(1000,1000));
   processReinflate();

   advanceTime(750);
   ASSERT_TRUE(root->hasEvent());
   auto event = root->popEvent();
   ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

TEST_F(SequencerPreservationTest, DelayNoTarget)
{
    loadDocument(COMMAND_ENGINE);

    auto action = executeCommand("SendEvent", {{"sequencer", "MAGIC"}, {"delay", 1000}}, false);

    advanceTime(250);

    configChange(ConfigurationChange(300,1000));
    processReinflate();

    advanceTime(750);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

TEST_F(SequencerPreservationTest, Animate)
{
    loadDocument(COMMAND_ENGINE);

    auto valueArray = std::make_shared<ObjectArray>();
    auto valueMap = std::make_shared<ObjectMap>();
    valueMap->emplace("property", "opacity");
    valueMap->emplace("to", 0);
    valueArray->emplace_back(valueMap);

    auto action = executeCommand("AnimateItem", {
                                                    {"sequencer", "MAGIC"},
                                                    {"duration", 1000},
                                                    {"componentId", "framy"},
                                                    {"easing", "linear"},
                                                    {"value", valueArray}
                                                }, false);

    auto framy = component->getCoreChildAt(0);

    advanceTime(250);

    ASSERT_EQ(0.75, framy->getCalculated(kPropertyOpacity).asFloat());

    configChange(ConfigurationChange(1000,1000));
    advanceTime(500);
    processReinflate();

    advanceTime(250);
    framy = component->getCoreChildAt(0);
    ASSERT_EQ(0, framy->getCalculated(kPropertyOpacity).asFloat());
}

TEST_F(SequencerPreservationTest, AnimateNoTarget)
{
    loadDocument(COMMAND_ENGINE);

    auto valueArray = std::make_shared<ObjectArray>();
    auto valueMap = std::make_shared<ObjectMap>();
    valueMap->emplace("property", "opacity");
    valueMap->emplace("to", 0);
    valueArray->emplace_back(valueMap);

    auto action = executeCommand("AnimateItem", {
                                                    {"sequencer", "MAGIC"},
                                                    {"duration", 1000},
                                                    {"componentId", "framy"},
                                                    {"easing", "linear"},
                                                    {"value", valueArray}
                                                }, false);

    auto framy = component->getCoreChildAt(0);

    advanceTime(250);

    ASSERT_EQ(0.75, framy->getCalculated(kPropertyOpacity).asFloat());

    configChange(ConfigurationChange(300,1000));
    advanceTime(500);
    processReinflate();

    advanceTime(250);
    ASSERT_FALSE(component->getChildCount());

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(SequencerPreservationTest, AnimateWithRepeat)
{
    loadDocument(COMMAND_ENGINE);

    auto valueArray = std::make_shared<ObjectArray>();
    auto valueMap = std::make_shared<ObjectMap>();
    valueMap->emplace("property", "opacity");
    valueMap->emplace("to", 0);
    valueArray->emplace_back(valueMap);

    auto action = executeCommand("AnimateItem", {
                                                    {"sequencer", "MAGIC"},
                                                    {"duration", 1000},
                                                    {"componentId", "framy"},
                                                    {"easing", "linear"},
                                                    {"repeatCount", 1},
                                                    {"repeatMode", "reverse"},
                                                    {"value", valueArray}
                                                }, false);

    auto framy = component->getCoreChildAt(0);

    advanceTime(250);

    ASSERT_EQ(0.75, framy->getCalculated(kPropertyOpacity).asFloat());

    configChange(ConfigurationChange(1000,1000));
    advanceTime(500);
    processReinflate();

    advanceTime(250);
    framy = component->getCoreChildAt(0);
    ASSERT_EQ(0, framy->getCalculated(kPropertyOpacity).asFloat());

    advanceTime(250);
    ASSERT_EQ(0.25, framy->getCalculated(kPropertyOpacity).asFloat());

    configChange(ConfigurationChange(500,500));
    advanceTime(500);
    processReinflate();

    advanceTime(250);
    framy = component->getCoreChildAt(0);
    ASSERT_EQ(1, framy->getCalculated(kPropertyOpacity).asFloat());
}

static const char *COMMAND_ENGINE_FUNKY_REINFLATE = R"apl({
  "type": "APL",
  "version": "2022.1",
  "theme": "dark",
  "onConfigChange": {
    "type": "Sequential",
    "sequencer": "MAGIC",
    "commands": [
      {
        "type": "Reinflate",
        "preservedSequencers": ["MAGIC"]
      },
      {
        "type": "SendEvent"
      }
    ]
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "items": {
          "when": "${viewport.pixelWidth > 350}",
          "type": "Frame",
          "id": "framy",
          "opacity": 1
        }
      }
    ]
  }
})apl";

TEST_F(SequencerPreservationTest, PreserveSequencerThatReinflates)
{
    loadDocument(COMMAND_ENGINE_FUNKY_REINFLATE);

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    advanceTime(250);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

static const char *COMMAND_PARALLEL_EVENT = R"([{
  "type": "Parallel",
  "sequencer": "MAGIC",
  "delay": 500,
  "commands": [
    {
      "type": "SendEvent",
      "delay": 500
    },
    {
      "type": "SendEvent",
      "delay": 250
    }
  ]
}])";

TEST_F(SequencerPreservationTest, Parallel)
{
    loadDocument(COMMAND_ENGINE);

    rapidjson::Document doc;
    doc.Parse(COMMAND_PARALLEL_EVENT);
    auto action = executeCommands(apl::Object(doc), false);

    advanceTime(250);

    ASSERT_FALSE(root->hasEvent());

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    advanceTime(250);
    ASSERT_FALSE(root->hasEvent());

    configChange(ConfigurationChange(500,500));
    processReinflate();

    advanceTime(250);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());

    ASSERT_FALSE(root->hasEvent());

    configChange(ConfigurationChange(500,500));
    processReinflate();

    advanceTime(250);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

static const char *COMMAND_SEQUENTIAL_EVENT = R"([{
  "type": "Sequential",
  "sequencer": "MAGIC",
  "delay": 500,
  "commands": [
    {
      "type": "SendEvent",
      "delay": 500
    },
    {
      "type": "SendEvent",
      "delay": 250
    }
  ]
}])";

TEST_F(SequencerPreservationTest, Sequential)
{
    loadDocument(COMMAND_ENGINE);

    rapidjson::Document doc;
    doc.Parse(COMMAND_SEQUENTIAL_EVENT);
    auto action = executeCommands(apl::Object(doc), false);

    advanceTime(250);

    ASSERT_FALSE(root->hasEvent());

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    advanceTime(250);
    ASSERT_FALSE(root->hasEvent());

    configChange(ConfigurationChange(500,500));
    processReinflate();

    advanceTime(500);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());

    ASSERT_FALSE(root->hasEvent());

    configChange(ConfigurationChange(500,500));
    processReinflate();

    advanceTime(250);
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

static const char *COMMAND_SEQUENTIAL_ANIMATE = R"([{
  "type": "Sequential",
  "sequencer": "MAGIC",
  "commands": [
    {
      "type": "AnimateItem",
      "duration": 1000,
      "componentId": "framy",
      "easing": "linear",
      "value": [ { "property": "opacity", "to": 0 } ]
    },
    {
      "type": "SendEvent"
    }
  ]
}])";

TEST_F(SequencerPreservationTest, AnimateNoTargetSequential)
{
    loadDocument(COMMAND_ENGINE);

    rapidjson::Document doc;
    doc.Parse(COMMAND_SEQUENTIAL_ANIMATE);
    auto action = executeCommands(apl::Object(doc), false);

    auto framy = component->getCoreChildAt(0);

    advanceTime(250);

    ASSERT_EQ(0.75, framy->getCalculated(kPropertyOpacity).asFloat());

    configChange(ConfigurationChange(300,1000));
    advanceTime(500);
    processReinflate();

    advanceTime(250);
    ASSERT_FALSE(component->getChildCount());

    ASSERT_TRUE(CheckSendEvent(root));
}

static const char *COMMAND_PARALLEL_ANIMATE = R"([{
  "type": "Parallel",
  "sequencer": "MAGIC",
  "commands": [
    {
      "type": "AnimateItem",
      "duration": 1000,
      "componentId": "framy",
      "easing": "linear",
      "value": [ { "property": "opacity", "to": 0 } ]
    },
    {
      "type": "SendEvent",
      "delay": 1000
    }
  ]
}])";

TEST_F(SequencerPreservationTest, AnimateNoTargetParallel)
{
    loadDocument(COMMAND_ENGINE);

    rapidjson::Document doc;
    doc.Parse(COMMAND_PARALLEL_ANIMATE);
    auto action = executeCommands(apl::Object(doc), false);

    auto framy = component->getCoreChildAt(0);

    advanceTime(250);

    ASSERT_EQ(0.75, framy->getCalculated(kPropertyOpacity).asFloat());

    configChange(ConfigurationChange(300,1000));
    advanceTime(500);
    processReinflate();

    advanceTime(250);
    ASSERT_FALSE(component->getChildCount());

    ASSERT_TRUE(CheckSendEvent(root));
}

static const char *COMMAND_PAGER = R"apl({
 "type": "APL",
 "version": "1.9",
 "theme": "dark",
 "onConfigChange": {
   "type": "Reinflate",
   "preservedSequencers": ["MAGIC"]
 },
 "mainTemplate": {
   "items": [
     {
       "type": "Pager",
       "when": "${viewport.pixelWidth > 350}",
       "preserve": ["pageIndex"],
       "id": "root",
       "data": [0,1,2,3,4,5,6],
       "items": {
         "when": "${index < 6 || viewport.pixelWidth > 500}",
         "type": "Frame"
       }
     }
   ]
 }
})apl";

static const char *COMMAND_PAGER_WITHOUT_IDX = R"apl({
 "type": "APL",
 "version": "1.9",
 "theme": "dark",
 "onConfigChange": {
   "type": "Reinflate",
   "preservedSequencers": ["MAGIC"]
 },
 "mainTemplate": {
   "items": [
     {
       "type": "Pager",
       "when": "${viewport.pixelWidth > 350}",
       "id": "root",
       "data": [0,1,2,3,4,5,6],
       "items": {
         "when": "${index < 6 || viewport.pixelWidth > 500}",
         "type": "Frame"
       }
     }
   ]
 }
})apl";

TEST_F(SequencerPreservationTest, SetPage)
{
    loadDocument(COMMAND_PAGER_WITHOUT_IDX);

    ASSERT_EQ(0, component->pagePosition());

    auto action = executeCommand("SetPage", {
                                                    {"sequencer", "MAGIC"},
                                                    {"position", "relative"},
                                                    {"componentId", "root"},
                                                    {"value", 1}
                                                }, false);

    advanceTime(300);

    configChange(ConfigurationChange(1000,1000));
    advanceTime(100);
    processReinflate();

    advanceTime(200);
    ASSERT_EQ(1, component->pagePosition());


    action = executeCommand("SetPage", {
                                           {"sequencer", "MAGIC"},
                                           {"position", "relative"},
                                           {"componentId", "root"},
                                           {"value", 1}
                                       }, false);

    advanceTime(300);

    configChange(ConfigurationChange(800,800));
    advanceTime(100);
    processReinflate();

    advanceTime(200);
    ASSERT_EQ(2, component->pagePosition());
}

TEST_F(SequencerPreservationTest, SetPageNoTarget)
{
    loadDocument(COMMAND_PAGER_WITHOUT_IDX);

    ASSERT_EQ(0, component->pagePosition());

    auto action = executeCommand("SetPage", {
                                                {"sequencer", "MAGIC"},
                                                {"position", "relative"},
                                                {"componentId", "root"},
                                                {"value", 1}
                                            }, false);

    advanceTime(300);

    configChange(ConfigurationChange(300,1000));
    advanceTime(100);
    processReinflate();

    advanceTime(200);

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(SequencerPreservationTest, SetPageNoTargetIndex)
{
    loadDocument(COMMAND_PAGER_WITHOUT_IDX);

    ASSERT_EQ(0, component->pagePosition());

    auto action = executeCommand("SetPage", {
                                                {"sequencer", "MAGIC"},
                                                {"position", "relative"},
                                                {"componentId", "root"},
                                                {"value", 6}
                                            }, false);

    advanceTime(300);

    ASSERT_EQ(7, component->getChildCount());

    configChange(ConfigurationChange(400,1000));
    advanceTime(100);
    processReinflate();

    ASSERT_EQ(6, component->getChildCount());

    advanceTime(200);
    ASSERT_EQ(0, component->pagePosition());

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(SequencerPreservationTest, AutoPage)
{
    loadDocument(COMMAND_PAGER);

    ASSERT_EQ(0, component->pagePosition());

    auto action = executeCommand("AutoPage", {
                                                {"sequencer", "MAGIC"},
                                                {"componentId", "root"},
                                                {"duration", 1000}
                                            }, false);
    advanceTime(600);
    ASSERT_EQ(1, component->pagePosition());

    advanceTime(1000);

    advanceTime(300);

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    advanceTime(1000);
    ASSERT_EQ(1, component->pagePosition());
    advanceTime(600);
    ASSERT_EQ(2, component->pagePosition());
}

TEST_F(SequencerPreservationTest, AutoPageNoTargetIndex)
{
    loadDocument(COMMAND_PAGER);

    ASSERT_EQ(0, component->pagePosition());

    auto action = executeCommand("AutoPage", {
                                                 {"sequencer", "MAGIC"},
                                                 {"componentId", "root"},
                                                 {"duration", 1000}
                                             }, false);

    advanceTime(8000);
    ASSERT_EQ(5, component->pagePosition());

    advanceTime(300);

    configChange(ConfigurationChange(400,1000));
    processReinflate();

    advanceTime(1600);
    ASSERT_EQ(5, component->pagePosition());

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(SequencerPreservationTest, ScrollToIdxPager)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_PAGER_WITHOUT_IDX);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("ScrollToIndex", {
                                                      {"sequencer", "MAGIC"},
                                                      {"componentId", "root"},
                                                      {"index", 2}
                                                  }, false);

    advanceTime(300);

    ASSERT_EQ(0, component->pagePosition());

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    advanceTime(1000);
    ASSERT_EQ(2, component->pagePosition());

    action = executeCommand("ScrollToIndex", {
                                                 {"sequencer", "MAGIC"},
                                                 {"componentId", "root"},
                                                 {"index", 4}
                                             }, false);

    advanceTime(300);

    ASSERT_EQ(2, component->pagePosition());

    configChange(ConfigurationChange(500,500));
    processReinflate();

    advanceTime(1000);
    ASSERT_EQ(4, component->pagePosition());
}

TEST_F(SequencerPreservationTest, ScrollToIdxPagerNoTarget)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_PAGER_WITHOUT_IDX);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("ScrollToIndex", {
                                                      {"sequencer", "MAGIC"},
                                                      {"componentId", "root"},
                                                      {"index", 2}
                                                  }, false);

    advanceTime(300);

    ASSERT_EQ(0, component->pagePosition());

    configChange(ConfigurationChange(300,1000));
    processReinflate();

    advanceTime(1000);

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(SequencerPreservationTest, ScrollToIdxPagerNoTargetIndex)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_PAGER_WITHOUT_IDX);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("ScrollToIndex", {
                                                      {"sequencer", "MAGIC"},
                                                      {"componentId", "root"},
                                                      {"index", 6}
                                                  }, false);

    advanceTime(300);

    ASSERT_EQ(0, component->pagePosition());

    configChange(ConfigurationChange(400,1000));
    processReinflate();

    advanceTime(1000);
    ASSERT_EQ(0, component->pagePosition());

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(SequencerPreservationTest, AutoPageNoCurrentPagePreserve)
{
    loadDocument(COMMAND_PAGER_WITHOUT_IDX);

    ASSERT_EQ(0, component->pagePosition());

    auto action = executeCommand("AutoPage", {
                                                 {"sequencer", "MAGIC"},
                                                 {"componentId", "root"},
                                                 {"duration", 1000}
                                             }, false);

    advanceTime(600);
    ASSERT_EQ(1, component->pagePosition());

    advanceTime(1000);

    advanceTime(300);

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    advanceTime(1000);
    ASSERT_EQ(0, component->pagePosition());
    advanceTime(600);
    ASSERT_EQ(1, component->pagePosition());
}

static const char *COMMAND_SCROLLABLE_WITH_PRESERVE = R"apl({
 "type": "APL",
 "version": "1.9",
 "theme": "dark",
 "onConfigChange": {
   "type": "Reinflate",
   "preservedSequencers": ["MAGIC"]
 },
 "mainTemplate": {
   "items": [
     {
       "type": "Sequence",
       "preserve": ["scrollOffset"],
       "when": "${viewport.pixelWidth > 350}",
       "id": "root",
       "data": [0,1,2,3,4,5,6,7,8,9],
       "height": 250,
       "width": 500,
       "items": {
         "type": "Frame",
         "when": "${index < 7 || viewport.pixelWidth > 500}",
         "id": "f${index}",
         "width": "100%",
         "height": 100
       }
     }
   ]
 }
})apl";

TEST_F(SequencerPreservationTest, ScrollSequenceOffsetPreserve)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_SCROLLABLE_WITH_PRESERVE);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("Scroll", {
                                               {"sequencer", "MAGIC"},
                                               {"componentId", "root"},
                                               {"distance", 3}
                                           }, false);

    advanceTime(500);

    ASSERT_EQ(Point(0,375), component->scrollPosition());

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    ASSERT_EQ(Point(0,375), component->scrollPosition());

    advanceTime(500);
    ASSERT_EQ(Point(0,750), component->scrollPosition());
}

TEST_F(SequencerPreservationTest, ScrollSequenceShortenedDistanceOffsetPreserve)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_SCROLLABLE_WITH_PRESERVE);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("Scroll", {
                                               {"sequencer", "MAGIC"},
                                               {"componentId", "root"},
                                               {"distance", 3}
                                           }, false);

    advanceTime(500);

    ASSERT_EQ(Point(0,375), component->scrollPosition());

    configChange(ConfigurationChange(400,1000));
    processReinflate();

    ASSERT_EQ(Point(0,375), component->scrollPosition());

    advanceTime(500);
    ASSERT_EQ(Point(0,450), component->scrollPosition());
}

static const char *COMMAND_SCROLLABLE = R"apl({
 "type": "APL",
 "version": "1.9",
 "theme": "dark",
 "onConfigChange": {
   "type": "Reinflate",
   "preservedSequencers": ["MAGIC"]
 },
 "mainTemplate": {
   "items": [
     {
       "type": "Sequence",
       "when": "${viewport.pixelWidth > 350}",
       "id": "root",
       "data": [0,1,2,3,4,5,6,7,8,9],
       "height": 250,
       "width": 500,
       "items": {
         "type": "Frame",
         "when": "${index < 7 || viewport.pixelWidth > 500}",
         "id": "f${index}",
         "width": "100%",
         "height": 100
       }
     }
   ]
 }
})apl";

TEST_F(SequencerPreservationTest, ScrollSequence)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_SCROLLABLE);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("Scroll", {
                                                {"sequencer", "MAGIC"},
                                                {"componentId", "root"},
                                                {"distance", 3}
                                            }, false);

    advanceTime(500);

    ASSERT_EQ(Point(0,375), component->scrollPosition());

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    advanceTime(500);
    ASSERT_EQ(Point(0,375), component->scrollPosition());
}

TEST_F(SequencerPreservationTest, ScrollSequenceShortenedDistance)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_SCROLLABLE);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("Scroll", {
                                               {"sequencer", "MAGIC"},
                                               {"componentId", "root"},
                                               {"distance", 3}
                                           }, false);

    advanceTime(500);

    ASSERT_EQ(Point(0,375), component->scrollPosition());

    configChange(ConfigurationChange(400,1000));
    processReinflate();

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    advanceTime(500);
    ASSERT_EQ(Point(0,375), component->scrollPosition());
}

TEST_F(SequencerPreservationTest, ScrollSequenceNoTarget)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_SCROLLABLE);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("Scroll", {
                                               {"sequencer", "MAGIC"},
                                               {"componentId", "root"},
                                               {"distance", 3}
                                           }, false);

    advanceTime(500);

    ASSERT_EQ(Point(0,375), component->scrollPosition());

    configChange(ConfigurationChange(300,1000));
    processReinflate();

    advanceTime(500);

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(SequencerPreservationTest, ScrollToIdxSequence)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_SCROLLABLE);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("ScrollToIndex", {
                                               {"sequencer", "MAGIC"},
                                               {"componentId", "root"},
                                               {"index", 7},
                                               {"align", "center"}
                                           }, false);

    advanceTime(500);

    ASSERT_EQ(Point(0,312.5), component->scrollPosition());

    configChange(ConfigurationChange(1000,1000));
    processReinflate();
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    advanceTime(1000);
    // 7 * 100 - 500/2
    ASSERT_EQ(Point(0,625), component->scrollPosition());
}

static const char *COMMAND_SCROLLABLE_WITH_POSITION_PRESERVE = R"apl({
 "type": "APL",
 "version": "1.9",
 "theme": "dark",
 "onConfigChange": {
   "type": "Reinflate",
   "preservedSequencers": ["MAGIC"]
 },
 "mainTemplate": {
   "items": [
     {
       "type": "Sequence",
       "preserve": ["scrollOffset"],
       "when": "${viewport.pixelWidth > 350}",
       "id": "root",
       "data": [0,1,2,3,4,5,6,7,8,9],
       "height": 250,
       "width": 500,
       "items": {
         "type": "Frame",
         "when": "${index < 7 || viewport.pixelWidth > 500}",
         "id": "f${index}",
         "width": "100%",
         "height": 100
       }
     }
   ]
 }
})apl";

TEST_F(SequencerPreservationTest, ScrollToIdxSequenceWithPositionPreserve)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_SCROLLABLE_WITH_POSITION_PRESERVE);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("ScrollToIndex", {
                                                      {"sequencer", "MAGIC"},
                                                      {"componentId", "root"},
                                                      {"index", 7},
                                                      {"align", "center"}
                                                  }, false);

    advanceTime(500);

    ASSERT_EQ(Point(0,312.5), component->scrollPosition());

    configChange(ConfigurationChange(1000,1000));
    processReinflate();
    ASSERT_EQ(Point(0,312.5), component->scrollPosition());

    advanceTime(1000);
    // 7 * 100 - 500/2
    ASSERT_EQ(Point(0,625), component->scrollPosition());
}

TEST_F(SequencerPreservationTest, ScrollToIdxSequenceNoTargetIndex)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_SCROLLABLE);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("ScrollToIndex", {
                                                      {"sequencer", "MAGIC"},
                                                      {"componentId", "root"},
                                                      {"index", 7},
                                                      {"align", "center"}
                                                  }, false);

    advanceTime(500);

    ASSERT_EQ(Point(0,312.5), component->scrollPosition());

    configChange(ConfigurationChange(400,1000));
    processReinflate();

    advanceTime(1000);
    // 7 * 100 - 500/2
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(SequencerPreservationTest, ScrollToComponentSequence)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_SCROLLABLE);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("ScrollToComponent", {
                                                      {"sequencer", "MAGIC"},
                                                      {"componentId", "f7"},
                                                      {"align", "center"}
                                                  }, false);

    advanceTime(500);

    ASSERT_EQ(Point(0,312.5), component->scrollPosition());

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    advanceTime(1000);
    // 7 * 100 - 500/2
    ASSERT_EQ(Point(0,625), component->scrollPosition());
}

TEST_F(SequencerPreservationTest, ScrollToComponentSequenceNoTargetComponent)
{
    config->set(RootProperty::kUEScrollerDurationEasing, CoreEasing::linear());

    loadDocument(COMMAND_SCROLLABLE);

    ASSERT_EQ(Point(0,0), component->scrollPosition());

    auto action = executeCommand("ScrollToComponent", {
                                                          {"sequencer", "MAGIC"},
                                                          {"componentId", "f7"},
                                                          {"align", "center"}
                                                      }, false);

    advanceTime(500);

    ASSERT_EQ(Point(0,312.5), component->scrollPosition());

    configChange(ConfigurationChange(400,1000));
    processReinflate();

    advanceTime(1000);
    // 7 * 100 - 500/2
    ASSERT_EQ(Point(0,0), component->scrollPosition());

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}

static const char *COMMAND_NO_ID = R"apl({
 "type": "APL",
 "version": "2022.1",
 "theme": "dark",
 "onConfigChange": {
   "type": "Reinflate",
   "preservedSequencers": ["MAGIC"]
 },
 "mainTemplate": {
   "items": [
     {
       "type": "Container",
       "items": {
         "type": "Frame",
         "opacity": 1,
         "onMount": {
           "sequencer": "MAGIC",
           "type": "AnimateItem",
           "duration": 1000,
           "easing": "linear",
           "value": {
             "property": "opacity",
             "from": 1,
             "to": 0
           }
         }
       }
     }
   ]
 }
})apl";

TEST_F(SequencerPreservationTest, AnimateItemNoTargetComponent)
{
    loadDocument(COMMAND_NO_ID);

    auto framy = component->getCoreChildAt(0);

    advanceTime(250);

    ASSERT_EQ(0.75, framy->getCalculated(kPropertyOpacity).asFloat());

    configChange(ConfigurationChange(1000,1000));
    processReinflate();

    auto reinflatedFramy = component->getCoreChildAt(0);
    ASSERT_EQ(0, reinflatedFramy->getCalculated(kPropertyOpacity).asFloat());

    // complaint about failed preserve
    ASSERT_TRUE(ConsoleMessage());
}