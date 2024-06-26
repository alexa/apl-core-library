/*
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
 *
 */

#include "../testeventloop.h"

using namespace apl;

class MapGrammarTest : public DocumentWrapper {};


static auto MAP_TESTS = std::vector<std::pair<std::string, ObjectArray>> {
    {"Map.keys()", {}},
    {"Map.keys(TEST)", {"a", "b", "c"}},
};

TEST_F(MapGrammarTest, MapFunctions)
{
    auto c = Context::createTestContext(Metrics(), RootConfig());
    auto map = std::make_shared<ObjectMap>();
    map->emplace("a", "adventure");
    map->emplace("b", "beauty");
    map->emplace("c", "culture");
    auto obj = Object(map);

    c->putConstant("TEST", obj);

    for (const auto& m : MAP_TESTS) {
        auto len = evaluate(*c, "${" + m.first + ".length}");
        ASSERT_TRUE(IsEqual(m.second.size(), len)) << m.first << " LENGTH " << len.toDebugString();

        for (int i = 0 ; i < m.second.size() ; i++ ) {
            auto valueAt = evaluate(*c, "${" + m.first + "[" + std::to_string(i) + "]}");
            ASSERT_TRUE(IsEqual(m.second.at(i), valueAt)) << m.first << " INDEX=" << i << valueAt.toDebugString();
        }
    }
}

static const char *JSON_MAP = R"apl({
   "a": "adventure",
   "b": "beauty",
   "c": "culture"
})apl";

TEST_F(MapGrammarTest, MapFunctionsWithJSONDocument)
{
    auto c = Context::createTestContext(Metrics(), RootConfig());

    rapidjson::Document doc;
    doc.Parse(JSON_MAP);
    c->putConstant("TEST", Object(std::move(doc)));  // This adds a constant which is the rapidjson Document

    for (const auto& m : MAP_TESTS) {
        auto len = evaluate(*c, "${" + m.first + ".length}");
        ASSERT_TRUE(IsEqual(m.second.size(), len)) << m.first << " LENGTH " << len.toDebugString();

        for (int i = 0 ; i < m.second.size() ; i++ ) {
            auto valueAt = evaluate(*c, "${" + m.first + "[" + std::to_string(i) + "]}");
            ASSERT_TRUE(IsEqual(m.second.at(i), valueAt)) << m.first << " INDEX=" << i << valueAt.toDebugString();
        }
    }
}


static const char *DEEP_JSON_MAP = R"apl(
{
    "TEST": {
        "a": "adventure",
        "b": "beauty",
        "c": "culture"
    }
})apl";


TEST_F(MapGrammarTest, MapFunctionsWithJSONValue)
{
    auto c = Context::createTestContext(Metrics(), RootConfig());

    rapidjson::Document doc;
    doc.Parse(DEEP_JSON_MAP);
    ASSERT_TRUE(doc.IsObject());
    auto itr = doc.FindMember("TEST");
    ASSERT_TRUE(itr != doc.MemberEnd());
    c->putConstant("TEST", itr->value);  // This adds a constant from the rapidjson::Value

    for (const auto& m : MAP_TESTS) {
        auto len = evaluate(*c, "${" + m.first + ".length}");
        ASSERT_TRUE(IsEqual(m.second.size(), len)) << m.first << " LENGTH " << len.toDebugString();

        for (int i = 0 ; i < m.second.size() ; i++ ) {
            auto valueAt = evaluate(*c, "${" + m.first + "[" + std::to_string(i) + "]}");
            ASSERT_TRUE(IsEqual(m.second.at(i), valueAt)) << m.first << " INDEX=" << i << valueAt.toDebugString();
        }
    }
}


static const char *COMPONENT_SOURCE_EVENT_KEYS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SendEvent",
        "arguments": "${Map.keys(event.source)}"
      }
    }
  }
})apl";

// The ComponentEventWrapper is exposed to the APL document in event.source
TEST_F(MapGrammarTest, SourceEventKeys)
{
    loadDocument(COMPONENT_SOURCE_EVENT_KEYS);
    ASSERT_TRUE(component);

    performTap(0,0);
    ASSERT_TRUE(CheckSendEvent(root, "bind",
                               "checked",
                               "disabled",
                               "focused",
                               "height",
                               "id",
                               "layoutDirection",
                               "opacity",
                               "pressed",
                               "type",
                               "uid",
                               "width",
                               // These values are added by the ComponentSourceEventWrapper
                               "value",
                               "handler",
                               "source"));
}


static const char *COMPONENT_TARGET_EVENT_KEYS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SendEvent",
        "arguments": "${Map.keys(event.target)}"
      }
    }
  }
})apl";

// The ComponentEventWrapper is exposed to the APL document in event.source
TEST_F(MapGrammarTest, TargetEventKeys)
{
    loadDocument(COMPONENT_TARGET_EVENT_KEYS);
    ASSERT_TRUE(component);

    performTap(0,0);
    ASSERT_TRUE(CheckSendEvent(root, "bind",
                               "checked",
                               "disabled",
                               "focused",
                               "height",
                               "id",
                               "layoutDirection",
                               "opacity",
                               "pressed",
                               "type",
                               "uid",
                               "width"));
}


static const char *COMPONENT_ON_SCROLL_SOURCE_EVENT_KEYS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "ScrollView",
      "items": {
         "type": "Frame",
         "height": 10000
      },
      "onScroll": {
        "type": "SendEvent",
        "sequencer": "S",
        "arguments": "${Map.keys(event.source)}"
      }
    }
  }
})apl";

TEST_F(MapGrammarTest, OnScrollSourceEventKeys)
{
    loadDocument(COMPONENT_ON_SCROLL_SOURCE_EVENT_KEYS);
    ASSERT_TRUE(component);

    executeCommand("Scroll", {{"componentId", ":root"}, {"distance", 1}}, false);
    advanceTime(300);

    ASSERT_TRUE(CheckSendEvent(root,
                               "allowBackwards",
                               "allowForward",
                               "bind",
                               "checked",
                               "disabled",
                               "focused",
                               "height",
                               "id",
                               "layoutDirection",
                               "opacity",
                               "position",  // This property is added by the ScrollableComponent
                               "pressed",
                               "type",
                               "uid",
                               "width",
                               // Values after this point are added by the ComponentSourceEventWrapper
                               "value",
                               "handler",
                               "source"));
}


static const char *COMPONENT_ON_SCROLL_TARGET_EVENT_KEYS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "ScrollView",
      "items": {
         "type": "Frame",
         "height": 10000
      },
      "onScroll": {
        "type": "SendEvent",
        "sequencer": "S",
        "arguments": "${Map.keys(event.target)}"
      }
    }
  }
})apl";

TEST_F(MapGrammarTest, OnScrollTargetEventKeys)
{
    loadDocument(COMPONENT_ON_SCROLL_TARGET_EVENT_KEYS);
    ASSERT_TRUE(component);

    executeCommand("Scroll", {{"componentId", ":root"}, {"distance", 1}}, false);
    advanceTime(300);

    ASSERT_TRUE(CheckSendEvent(root,
                               "allowBackwards",
                               "allowForward",
                               "bind",
                               "checked",
                               "disabled",
                               "focused",
                               "height",
                               "id",
                               "layoutDirection",
                               "opacity",
                               "position",  // This property is added by the ScrollableComponent
                               "pressed",
                               "type",
                               "uid",
                               "width"));
}