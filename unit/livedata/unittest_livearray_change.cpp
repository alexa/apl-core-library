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
#include "apl/livedata/livearrayobject.h"
#include "apl/livedata/livedatamanager.h"

using namespace apl;


/*
 * Return "true" if the LiveArray found at the given key and context is marked as dirty.
 */
::testing::AssertionResult
LiveArrayDirty(const std::string& key, const ContextPtr& context)
{
    auto& dataManager = context->dataManager();
    const auto& dirtySet = dataManager.dirty();
    for (const auto& t : dirtySet) {
        if (t->getContext() == context && t->getKey() == key)
            return ::testing::AssertionSuccess();
    }

    // Check to make sure it is a valid context - we might have made a mistake with the key/context
    const auto& trackers = dataManager.trackers();
    for (const auto& t : trackers) {
        if (t->getContext() == context && t->getKey() == key)
            return ::testing::AssertionFailure() << "Key " << key << " should have been dirty";
    }

    return ::testing::AssertionFailure() << "Key " << key << " not found in context";
}

/*
 * Verify that the tracking data for a particular array matches your expected indices.
 * For example, if you start with the array ['a', 'b', 'c', 'd'] and you insert 'e' at index 1, remove index 3,
 * and change index 2 to "k", then you should end up with the following::
 *
 *    values:  { 'a', 'e',   'k',  'd' }
 *    indices: {  0,   -1,    2,    3  }
 *    changed: { No,  Yes,  Yes,   No  }
 */
struct Triple {
    int index;
    bool changed;
    Object value;
};

::testing::AssertionResult
LiveArrayTrack(const std::string& key, const ContextPtr& context, const std::vector<Triple>& update)
{
    auto& dataManager = context->dataManager();
    const auto& dirtySet = dataManager.dirty();
    for (const auto& t : dirtySet) {
        if (t->getContext() == context && t->getKey() == key) {
            auto tracker = t->asArray();
            if (tracker) {
                auto array = tracker->getArray();
                if (array.size() != update.size())
                    return ::testing::AssertionFailure() << "tracking array size mismatch.  Actual " << array.size()
                        << " update " << update.size();

                int index = 0;
                for (const auto& m : update) {
                    auto oldPair = tracker->newToOld(index);
                    if (oldPair.first != m.index)
                        return ::testing::AssertionFailure() << "Array mismatch at index " << index
                            << " expected=" << m.index << " actual=" << oldPair.first;
                    if (oldPair.second != m.changed)
                        return ::testing::AssertionFailure() << "Update mismatch at index " << index
                            << " expected=" << m.changed << " actual=" << oldPair.second;
                    if (array.at(index) != m.value)
                        return ::testing::AssertionFailure() << "Value mismatch at index " << index
                            << " expected=" << m.value << " actual=" << array.at(index);
                    index++;
                }

                return ::testing::AssertionSuccess();
            }
        }
    }

    return ::testing::AssertionFailure() << "unable to find dynamic data key=" << key << " in the context";
}


class LiveArrayChangeTest : public DocumentWrapper {};

static const char *ARRAY_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.2\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${TestArray[1]}\""
    "    }"
    "  }"
    "}";

TEST_F(LiveArrayChangeTest, SmallChanges)
{
    auto myArray = LiveArray::create(ObjectArray{"a", "b", "c"});
    config->liveData("TestArray", myArray);

    loadDocument(ARRAY_TEST);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("b", component->getCalculated(kPropertyText).asString()));
    ASSERT_EQ(0, context->dataManager().dirty().size());

    // Update one item, by value
    ASSERT_TRUE(myArray->update(1, "seven"));
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{0, false, "a"},
                                                      {1, true, "seven"},
                                                      {2, false, "c"}}));
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(IsEqual("seven", component->getCalculated(kPropertyText).asString()));

    // Update one item, by reference
    auto eight = std::string("eight");
    ASSERT_TRUE(myArray->update(1, eight));
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{0, false, "a"},
                                                      {1, true, "eight"},
                                                      {2, false, "c"}}));
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(IsEqual("eight", component->getCalculated(kPropertyText).asString()));

    // Insert one item, by reference
    auto furry = std::string("furry");
    ASSERT_TRUE(myArray->insert(1, furry));
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{0,  false, "a"},
                                                      {-1, false, "furry"},
                                                      {1,  false, "eight"},
                                                      {2,  false, "c"}}));
    root->clearPending();
    ASSERT_TRUE(IsEqual("furry", component->getCalculated(kPropertyText).asString()));

    // Insert one item, by value
    ASSERT_TRUE(myArray->insert(1, "fuzzy"));
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{0,  false, "a"},
                                                      {-1, false, "fuzzy"},
                                                      {1,  false, "furry"},
                                                      {2,  false, "eight"},
                                                      {3,  false, "c"}}));
    root->clearPending();
    ASSERT_TRUE(IsEqual("fuzzy", component->getCalculated(kPropertyText).asString()));

    // Remove one item
    ASSERT_TRUE(myArray->remove(0));
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{1, false, "fuzzy"},
                                                      {2, false, "furry"},
                                                      {3, false, "eight"},
                                                      {4, false, "c"}}));
    root->clearPending();
    ASSERT_TRUE(IsEqual("furry", component->getCalculated(kPropertyText).asString()));

    // Push on back, by reference
    auto foo = std::string("foo");
    myArray->push_back(foo);
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{0, false, "fuzzy"},
                                                      {1, false, "furry"},
                                                      {2, false, "eight"},
                                                      {3, false, "c"},
                                                      {-1, false, "foo"}}));
    root->clearPending();

    // Push on back, by value
    myArray->push_back("bar");
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{0, false, "fuzzy"},
                                                      {1, false, "furry"},
                                                      {2, false, "eight"},
                                                      {3, false, "c"},
                                                      {4, false, "foo"},
                                                      {-1, false, "bar"}}));
    root->clearPending();
}

TEST_F(LiveArrayChangeTest, MultipleChanges)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2});
    config->liveData("TestArray", myArray);

    loadDocument(ARRAY_TEST);

    // Insert multiple items at a low index
    ASSERT_TRUE(myArray->insert(0, 5));  // 5,1,2
    ASSERT_TRUE(myArray->insert(0, 6));  // 6,5,1,2
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{-1, false, 6},
                                                      {-1, false, 5},
                                                      {0,  false, 1},
                                                      {1,  false, 2}}));
    root->clearPending();

    // Remove multiple items
    ASSERT_TRUE(myArray->remove(0));  // 5,1,2
    ASSERT_TRUE(myArray->remove(1));  // 5,2
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{1, false, 5},
                                                      {3, false, 2}}));
    root->clearPending();

    // Insert multiple items at several spots
    ASSERT_TRUE(myArray->insert(1, 10));  // 5,10,2
    ASSERT_TRUE(myArray->insert(0, 11));  // 11,5,10,2
    ASSERT_TRUE(myArray->insert(2, 12));  // 11,5,12,10,2
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{-1, false, 11},
                                                      {0,  false, 5},
                                                      {-1, false, 12},
                                                      {-1, false, 10},
                                                      {1,  false, 2}}));
    root->clearPending();

    // Remove multiple items in varying orders
    ASSERT_TRUE(myArray->remove(2));  // 11,5,10,2
    ASSERT_TRUE(myArray->remove(0));  // 5,10,2
    ASSERT_TRUE(myArray->remove(2));  // 5,10
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{1, false, 5},
                                                      {3, false, 10}}));
}

TEST_F(LiveArrayChangeTest, UpdateChecks)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2});
    config->liveData("TestArray", myArray);

    loadDocument(ARRAY_TEST);

    // Insert an item, update a different one, and remove
    ASSERT_TRUE(myArray->insert(0, 3));  // 3,1,2
    ASSERT_TRUE(myArray->update(1, 10));  // 3,10,2
    ASSERT_TRUE(myArray->remove(2));  // 3,10
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{-1, false, 3},
                                                      {0,  true,  10}}));
    root->clearPending();

    // Update an item then remove it
    ASSERT_TRUE(myArray->update(0, 13));   // 13,10
    ASSERT_TRUE(myArray->remove(0));       // 10
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{1, false, 10}}));
    root->clearPending();

    // Update an item and insert items around it
    ASSERT_TRUE(myArray->update(0, 20));  // 20
    ASSERT_TRUE(myArray->insert(1, 5));   // 20,5
    ASSERT_TRUE(myArray->insert(0, 6));   // 6,20,5
    ASSERT_TRUE(myArray->insert(0, 7));   // 7,6,20,5
    ASSERT_TRUE(myArray->update(1, 16));  // 7,16,20,5
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{-1, false, 7},
                                                      {-1, false, 16},
                                                      {0,  true,  20},
                                                      {-1, false, 5}}));
}

TEST_F(LiveArrayChangeTest, OutOfBounds)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2});
    config->liveData("TestArray", myArray);

    loadDocument(ARRAY_TEST);

    // Try to change invalid locations
    ASSERT_FALSE(myArray->insert(-1, 3));  // position is unsigned
    ASSERT_FALSE(myArray->insert(3, 3));
    ASSERT_FALSE(myArray->remove(-1));  // position is unsigned
    ASSERT_FALSE(myArray->remove(2));
    ASSERT_FALSE(myArray->remove(0, 3)); // Count out of bounds
    ASSERT_FALSE(myArray->update(-1, 10));  // position is unsigned
    ASSERT_FALSE(myArray->update(2, 10));

    auto foo = ObjectArray{10,20,30};
    ASSERT_FALSE(myArray->insert(-1, foo.begin(), foo.end()));
    ASSERT_FALSE(myArray->insert(3, foo.begin(), foo.end()));
    ASSERT_FALSE(myArray->update(-1, foo.begin(), foo.end()));
    ASSERT_FALSE(myArray->update(0, foo.begin(), foo.end()));
    ASSERT_FALSE(myArray->update(1, foo.begin(), foo.end()));

    auto foo2 = ObjectArray{};
    ASSERT_FALSE(myArray->update(0, foo2.begin(), foo2.end()));  // Updating nothing
    ASSERT_FALSE(myArray->insert(0, foo2.begin(), foo2.end()));  // Inserting nothing
    ASSERT_FALSE(LiveArrayDirty("TestArray", context));
}

// Check for conditions when the entire array has been replaced - everyone gets updated
TEST_F(LiveArrayChangeTest, Replaced)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2});
    config->liveData("TestArray", myArray);

    loadDocument(ARRAY_TEST);

    ASSERT_TRUE(myArray->remove(0));  // 2
    ASSERT_TRUE(myArray->remove(0));  // _empty_
    myArray->push_back(2);  // 2
    myArray->push_back(4);  // 4
    myArray->push_back(6);  // 6

    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{-1, false, 2},
                                                      {-1, false, 4},
                                                      {-1, false, 6}}));
    ASSERT_EQ(3, myArray->size());
    root->clearPending();

    myArray->clear();
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {}));  // Empty array
    ASSERT_EQ(0, myArray->size());
}

TEST_F(LiveArrayChangeTest, IteratorChanges)
{
    auto myArray = LiveArray::create(ObjectArray{"a", "b"});
    config->liveData("TestArray", myArray);

    loadDocument(ARRAY_TEST);
    auto newItems = ObjectArray{"c", "d", "e"};
    ASSERT_TRUE(myArray->insert(0, newItems.begin(), newItems.end()));  // c,d,e,a,b
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{-1, false, "c"},
                                                      {-1, false, "d"},
                                                      {-1, false, "e"},
                                                      {0, false, "a"},
                                                      {1, false, "b"}}));
    root->clearPending();

    ASSERT_TRUE(myArray->remove(0, 4));  // b
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{4, false, "b"}}));
    root->clearPending();

    ASSERT_TRUE(myArray->push_back(newItems.begin(), newItems.end()));  // b,c,d,e
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{0, false, "b"},
                                                      {-1, false, "c"},
                                                      {-1, false, "d"},
                                                      {-1, false, "e"}}));
    root->clearPending();

    ASSERT_TRUE(myArray->update(0, newItems.begin(), newItems.end()));  // c,d,e,e
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{0, true, "c"},
                                                      {1, true, "d"},
                                                      {2, true, "e"},
                                                      {3, false, "e"}}));
    root->clearPending();

    ASSERT_TRUE(myArray->insert(2, newItems.begin(), newItems.end()));  // c,d,c*,d*,e*,e,e
    ASSERT_TRUE(myArray->update(4, newItems.begin(), newItems.end()));  // c,d,c*,d*,c**,d**,e**
    ASSERT_TRUE(myArray->remove(1, 2));  // c,d*,c**,d**,e**
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {{0, false, "c"},
                                                      {-1, false, "d"},
                                                      {-1, false, "c"},
                                                      {2, true, "d"},
                                                      {3, true, "e"}}));
}

TEST_F(LiveArrayChangeTest, CombinedIteratorChanges)
{
    auto myArray = LiveArray::create(ObjectArray{"a", "b"});
    config->liveData("TestArray", myArray);

    loadDocument(ARRAY_TEST);
    auto newItemsBackwards = ObjectArray{"z", "y", "x"};
    auto newItemsForwards = ObjectArray{"c", "d", "e"};
    ASSERT_TRUE(myArray->insert(0, newItemsBackwards.begin(), newItemsBackwards.end()));  // z,y,x,a,b
    ASSERT_TRUE(myArray->push_back(newItemsForwards.begin(), newItemsForwards.end()));  // z,y,x,a,b,c,d,e
    ASSERT_TRUE(LiveArrayTrack("TestArray", context, {
                                                      {-1, false, "z"},
                                                      {-1, false, "y"},
                                                      {-1, false, "x"},
                                                      {0, false, "a"},
                                                      {1, false, "b"},
                                                      {-1, false, "c"},
                                                      {-1, false, "d"},
                                                      {-1, false, "e"},
    }));
    root->clearPending();
}
