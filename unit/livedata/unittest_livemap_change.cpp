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

#include "gtest/gtest.h"

#include "../testeventloop.h"
#include "apl/livedata/livedatamanager.h"
#include "apl/livedata/livemapobject.h"

using namespace apl;

/*
 * Verify that the tracking data for the map matches the expected values.
 * For example, if you start with a map containing { "A": "One", "B": "Two", "C": "Three" } and you
 * delete "A", change the value of "B" to "TwoPlus", and add "D": "Four", then you should
 * end up with the following:
 *
 *   changed: "A", "B", "D"
 *   new values": { "B": "TwoPlus", "C": "Three", "D": "Four" }
 */
struct Update {
    std::string key;
    Object value;
    bool changed;
};

::testing::AssertionResult
LiveMapTrack(const std::string& key,
             const ContextPtr& context,
             const std::vector<Update>& update)
{
    auto& dataManager = context->dataManager();
    const auto& dirtySet = dataManager.dirty();
    for (const auto& t : dirtySet) {
        if (t->getContext() == context && t->getKey() == key) {
            auto liveMap = t->asMap();
            if (liveMap) {
                const auto& map = liveMap->getMap();
                if (map.size() != update.size())
                    return ::testing::AssertionFailure() << "tracking map value mismatch. "
                        << "Actual " << map.size() << " Expected " << update.size();

                int changeCount = 0;
                for (const auto& m : update) {
                    if (!liveMap->has(m.key))
                        return ::testing::AssertionFailure() << "Expected to find key '" << m.key << "'";

                    auto v = liveMap->get(m.key);
                    if (v != m.value)
                        return ::testing::AssertionFailure() << "Value mismatch for key '" << m.key << "'"
                                                             << " expected=" << m.value << " actual=" << v;

                    bool hasChanged = (liveMap->getChanged().count(m.key) == 1);
                    if (hasChanged)
                        changeCount++;

                    if (m.changed != hasChanged)
                        return ::testing::AssertionFailure() << "Change mismatch for key '" << m.key << "'"
                                                             << " expected=" << m.changed << " actual=" << hasChanged;
                }

                if (changeCount != liveMap->getChanged().size())
                    return ::testing::AssertionFailure() << "Mismatch in change count";

                return ::testing::AssertionSuccess();
            }
        }
    }

    return ::testing::AssertionFailure() << "unable to find dynamic data key=" << key << " in the context";
}

class LiveMapChangeTest : public DocumentWrapper {};

static const char *MAP_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${TestMap.adjective} ${TestMap.noun}\""
    "    }"
    "  }"
    "}";

TEST_F(LiveMapChangeTest, SmallChange)
{
    auto myMap = LiveMap::create(ObjectMap{{"adjective", "happy"},
                                           {"noun",      "dog"}});
    config->liveData("TestMap", myMap);

    loadDocument(MAP_TEST);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("happy dog", component->getCalculated(kPropertyText).asString()));

    // Check the basic "has" and "get" methods
    ASSERT_TRUE(myMap->has("noun"));
    ASSERT_FALSE(myMap->has("verb"));

    ASSERT_TRUE(IsEqual("happy", myMap->get("adjective")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), myMap->get("verb")));

    // Change one item
    myMap->set("noun", "cat");
    ASSERT_TRUE(LiveMapTrack("TestMap", context,
                             {{"adjective", "happy", false},
                              {"noun",      "cat",   true}}));

    root->clearPending();
    ASSERT_TRUE(IsEqual("happy cat", component->getCalculated(kPropertyText).asString()));

    // Insert a new item
    myMap->set("other", "tiger");
    ASSERT_TRUE(LiveMapTrack("TestMap", context,
                             {{"adjective", "happy", false},
                              {"noun",      "cat",   false},
                              {"other",     "tiger", true}}));
    root->clearPending();
    ASSERT_TRUE(IsEqual("happy cat", component->getCalculated(kPropertyText).asString()));

    // Remove an item
    myMap->remove("noun");
    ASSERT_TRUE(LiveMapTrack("TestMap", context,
                             {{"adjective", "happy", false},
                              {"other",     "tiger", false}}));
    root->clearPending();
    ASSERT_TRUE(IsEqual("happy ", component->getCalculated(kPropertyText).asString()));
}

// Change several items at one time
TEST_F(LiveMapChangeTest, MultipleChanges)
{
    auto myMap = LiveMap::create(ObjectMap{{"adjective", "happy"},
                                           {"noun",      "dog"}});
    config->liveData("TestMap", myMap);

    loadDocument(MAP_TEST);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("happy dog", component->getCalculated(kPropertyText).asString()));

    // Insert multiple items
    myMap->set("verb", "run");
    myMap->set("article", "the");
    myMap->set("noun", "cat");

    ASSERT_TRUE(LiveMapTrack("TestMap", context,
                             {{"adjective", "happy", false},
                              {"noun",      "cat",   true},
                              {"verb",      "run",   true},
                              {"article",   "the",   true}}));
    root->clearPending();

    // Remove multiple items
    ASSERT_TRUE(myMap->remove("article"));
    ASSERT_TRUE(myMap->remove("noun"));
    ASSERT_FALSE(myMap->remove("article"));

    ASSERT_TRUE(LiveMapTrack("TestMap", context,
                             {{"adjective", "happy", false},
                              {"verb",      "run",   false}}));
    root->clearPending();

    // Update a group of items
    myMap->update({{"noun",    "bird"},
                   {"article", "a"},
                   {"verb",    "flew"}});

    ASSERT_TRUE(LiveMapTrack("TestMap", context,
                             {{"noun",      "bird",  true},
                              {"article",   "a",     true},
                              {"adjective", "happy", false},
                              {"verb",      "flew",  true}}));
}

// Replace all of the items and verify that everyone is marked as "changed"
TEST_F(LiveMapChangeTest, Replaced)
{
    auto myMap = LiveMap::create(ObjectMap{{"adjective", "happy"},
                                           {"noun",      "dog"}});
    config->liveData("TestMap", myMap);

    loadDocument(MAP_TEST);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("happy dog", component->getCalculated(kPropertyText).asString()));

    // Remove all items
    myMap->clear();
    ASSERT_TRUE(LiveMapTrack("TestMap", context,
                             {}));
    root->clearPending();

    // Add them all back
    myMap->set("noun", "dog");
    myMap->set("adjective", "happy");
    ASSERT_TRUE(LiveMapTrack("TestMap", context,
                             {{"adjective", "happy", true},
                              {"noun",      "dog",   true}}));
    root->clearPending();

    // Replace the entire map
    myMap->replace({{"adjective", "sad"},
                    {"pronoun",   "it"}});
    ASSERT_TRUE(LiveMapTrack("TestMap", context,
                             {{"adjective", "sad", true},
                              {"pronoun",   "it",  true}}));
}

static const char *PASSED_THROUGH_PARAMETERS = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "layouts": {
    "TestText": {
      "parameters": [
        "Label"
      ],
      "items": [
        {
          "type": "Text",
          "width": "100%",
          "id": "${Label}",
          "text": "${Label}",
          "textAlign": "center",
          "textAlignVertical": "center"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "TestText",
      "Label": "${IAmLive.check}"
    }
  }
})";

TEST_F(LiveMapChangeTest, ReplaceLayoutMap)
{
    auto myMap = LiveMap::create(ObjectMap{{"check", "maybe"}});
    config->liveData("IAmLive", myMap);

    loadDocument(PASSED_THROUGH_PARAMETERS);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("maybe", component->getCalculated(kPropertyText).asString()));

    myMap->set("check", "think so");
    root->clearPending();

    ASSERT_TRUE(IsEqual("think so", component->getCalculated(kPropertyText).asString()));
}

TEST_F(LiveMapChangeTest, PopulateLayoutMap)
{
    auto myMap = LiveMap::create(ObjectMap{});
    config->liveData("IAmLive", myMap);

    loadDocument(PASSED_THROUGH_PARAMETERS);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("", component->getCalculated(kPropertyText).asString()));

    myMap->set("check", "think so");
    root->clearPending();

    ASSERT_TRUE(IsEqual("think so", component->getCalculated(kPropertyText).asString()));
}