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

#include "apl/utils/actiondata.h"

using namespace apl;

class ActionDataTest : public DocumentWrapper {};

TEST_F(ActionDataTest, Basic)
{
    rapidjson::Document document(rapidjson::kObjectType);
    auto actionDetail = ActionData();
    auto actionDetailDump = actionDetail.serialize(document.GetAllocator());
    ASSERT_TRUE(actionDetailDump.HasMember("actionHint"));
    ASSERT_EQ("None", actionDetailDump["actionHint"]);
    ASSERT_FALSE(actionDetailDump.HasMember("component"));
    ASSERT_FALSE(actionDetailDump.HasMember("commandProvenance"));
}

static const char *ANIMATING_FRAME = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
  "item":
    {
      "type": "Frame",
      "id": "box",
      "width": 100,
      "height": 100,
      "onMount": {
        "type": "AnimateItem",
        "duration": 1000,
        "value": {
          "property": "transform",
          "from": {
            "translateX": "100vw"
          },
          "to": {
            "translateX": 0
          }
        }
      }
    }
  }
})";

TEST_F(ActionDataTest, BasicTarget)
{
    loadDocument(ANIMATING_FRAME);
    auto component = root->findComponentById("box");
    rapidjson::Document document(rapidjson::kObjectType);
    auto actionDetail = ActionData().target(component);
    auto actionDetailDump = actionDetail.serialize(document.GetAllocator());
    ASSERT_TRUE(actionDetailDump.HasMember("actionHint"));
    ASSERT_TRUE(actionDetailDump.HasMember("component"));
    ASSERT_STREQ("box", actionDetailDump["component"]["targetId"].GetString());
    ASSERT_STREQ("Frame", actionDetailDump["component"]["targetComponentType"].GetString());
    ASSERT_STREQ("_main/mainTemplate/item", actionDetailDump["component"]["provenance"].GetString());
    ASSERT_FALSE(actionDetailDump.HasMember("commandProvenance"));
}

TEST_F(ActionDataTest, NoTargetEmptyCommandProvenance)
{
    rapidjson::Document document(rapidjson::kObjectType);
    auto actionDetail = ActionData().actionHint("Animating").commandProvenance("");
    auto actionDetailDump = actionDetail.serialize(document.GetAllocator());
    ASSERT_TRUE(actionDetailDump.HasMember("actionHint"));
    ASSERT_EQ("Animating", actionDetailDump["actionHint"]);
    ASSERT_FALSE(actionDetailDump.HasMember("component"));
    ASSERT_FALSE(actionDetailDump.HasMember("commandProvenance"));
}

TEST_F(ActionDataTest, NoTarget)
{
    rapidjson::Document document(rapidjson::kObjectType);
    auto actionDetail = ActionData().actionHint("MediaPlayback").commandProvenance("_main/mainTemplate/item");
    auto actionDetailDump = actionDetail.serialize(document.GetAllocator());
    ASSERT_TRUE(actionDetailDump.HasMember("actionHint"));
    ASSERT_EQ("MediaPlayback", actionDetailDump["actionHint"]);
    ASSERT_FALSE(actionDetailDump.HasMember("component"));
    ASSERT_TRUE(actionDetailDump.HasMember("commandProvenance"));
    ASSERT_STREQ("_main/mainTemplate/item", actionDetailDump["commandProvenance"].GetString());
}

TEST_F(ActionDataTest, NullTarget)
{
    rapidjson::Document document(rapidjson::kObjectType);
    auto actionDetail = ActionData().target(nullptr);
    auto actionDetailDump = actionDetail.serialize(document.GetAllocator());
    ASSERT_TRUE(actionDetailDump.HasMember("actionHint"));
    ASSERT_FALSE(actionDetailDump.HasMember("component"));
    ASSERT_FALSE(actionDetailDump.HasMember("commandProvenance"));
}
