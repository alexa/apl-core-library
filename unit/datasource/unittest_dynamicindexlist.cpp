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

/**
 * The purpose of this include statement is to verify that apl/dataprovider.h includes
 * required file(s) that a consumer will need in order to use the datasource provider
 * functionality of APL.
 */
#include "apl/dynamicdata.h"

using namespace apl;

static const char *SOURCE_TYPE = "dynamicIndexList";
static const char *LIST_ID = "listId";
static const char *CORRELATION_TOKEN = "correlationToken";
static const char *START_INDEX = "startIndex";
static const char *COUNT = "count";
static const size_t TEST_CHUNK_SIZE = 5;

class DynamicIndexListTest : public DocumentWrapper {
public:
    ::testing::AssertionResult
    CheckFetchRequest(const std::string& listId, const std::string& correlationToken, int startIndex, int count) {
        bool fetchCalled = root->hasEvent();
        auto event = root->popEvent();
        fetchCalled &= (event.getType() == kEventTypeDataSourceFetchRequest);

        if (!fetchCalled)
            return ::testing::AssertionFailure() << "Fetch was not called.";

        auto incomingType = event.getValue(kEventPropertyName).getString();
        if (SOURCE_TYPE != incomingType)
            return ::testing::AssertionFailure()
                    << "DataSource type is wrong. Expected: " << SOURCE_TYPE
                    << ", actual: " << incomingType;

        auto request = event.getValue(kEventPropertyValue);

        auto incomingListId = request.opt(LIST_ID, "");
        if (incomingListId != listId)
            return ::testing::AssertionFailure()
                << "listId is wrong. Expected: " << listId
                << ", actual: " << incomingListId;

        auto incomingCorrelationToken = request.opt(CORRELATION_TOKEN, "");
        if (incomingCorrelationToken != correlationToken)
            return ::testing::AssertionFailure()
                << "correlationToken is wrong. Expected: " << correlationToken
                << ", actual: " << incomingCorrelationToken;

        auto incomingStartIndex = request.opt(START_INDEX, -1);
        if (incomingStartIndex != startIndex)
            return ::testing::AssertionFailure()
                    << "startIndex is wrong. Expected: " << startIndex
                    << ", actual: " << incomingStartIndex;

        auto incomingCount = request.opt(COUNT, -1);
        if (incomingCount != count)
            return ::testing::AssertionFailure()
                    << "count is wrong. Expected: " << count
                    << ", actual: " << incomingCount;

        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    CheckChild(size_t idx, int exp) {
        auto text = std::to_string(exp);
        auto actualText = component->getChildAt(idx)->getCalculated(kPropertyText).asString();
        if (actualText != text) {
            return ::testing::AssertionFailure()
                    << "text " << idx
                    << " is wrong. Expected: " << text
                    << ", actual: " << actualText;
        }

        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    CheckChildren(size_t startIdx, std::vector<int> values) {
        if (values.size() != component->getChildCount()) {
            return ::testing::AssertionFailure()
                    << "Wrong child number. Expected: " << values.size()
                    << ", actual: " << component->getChildCount();
        }
        int idx = startIdx;
        for (int exp : values) {
            auto result = CheckChild(idx, exp);
            if (!result) {
                return result;
            }
            idx++;
        }

        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    CheckChildren(std::vector<int> values) {
        return CheckChildren(0, values);
    }

    ::testing::AssertionResult
    CheckBounds(int minInclusive, int maxExclusive) {
        auto actual = ds->getBounds("vQdpOESlok");
        std::pair<int, int> expected(minInclusive, maxExclusive);

        if (actual != expected) {
            return ::testing::AssertionFailure()
                    << "bounds is wrong. Expected: (" << expected.first << "," << expected.second
                    << "), actual: (" << actual.first << "," << actual.second << ")";
        }
        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    CheckErrors(std::vector<std::string> reasons) {
        auto errors = ds->getPendingErrors().getArray();

        if (errors.size() != reasons.size()) {
            return ::testing::AssertionFailure()
                    << "Number of errors is wrong. Expected: " << reasons.size()
                    << ", actual: " << errors.size();
        }

        for (int i = 0; i<errors.size(); i++) {
            auto actual = errors.at(i).get("reason").asString();
            auto expected = reasons.at(i);
            if (actual != expected) {
                return ::testing::AssertionFailure()
                        << "error " << i << " reason is wrong. Expected: " << expected
                        << ", actual: " << actual;
            }
        }

        return ::testing::AssertionSuccess();
    }

    static std::string
    createLazyLoad(int listVersion, int correlationToken, int index, const std::string& items ) {
        std::string listVersionString = listVersion < 0 ? "" : ("\"listVersion\": " + std::to_string(listVersion) + ",");
        std::string ctString = correlationToken < 0 ? "" : ("\"correlationToken\": \"" + std::to_string(correlationToken) + "\",");
        return "{"
               "  \"presentationToken\": \"presentationToken\","
               "  \"listId\": \"vQdpOESlok\","
               + listVersionString + ctString +
               "  \"startIndex\": " + std::to_string(index) + ","
               "  \"items\": [" + items + "]"
               "}";
    }

    static std::string
    createInsert(int listVersion, int index, int item ) {
        return "{"
                "  \"presentationToken\": \"presentationToken\","
                "  \"listId\": \"vQdpOESlok\","
                "  \"listVersion\": " + std::to_string(listVersion) + ","
                "  \"operations\": ["
                "    {"
                "      \"type\": \"InsertItem\","
                "      \"index\": " + std::to_string(index) + ","
                "      \"item\": " + std::to_string(item) +
                "    }"
                "  ]"
                "}";
    }

    static std::string
    createReplace(int listVersion, int index, int item ) {
        return "{"
               "  \"presentationToken\": \"presentationToken\","
               "  \"listId\": \"vQdpOESlok\","
               "  \"listVersion\": " + std::to_string(listVersion) + ","
               "  \"operations\": ["
               "    {"
               "      \"type\": \"SetItem\","
               "      \"index\": " + std::to_string(index) + ","
               "      \"item\": " + std::to_string(item) +
               "    }"
               "  ]"
               "}";
    }

    static std::string
    createDelete(int listVersion, int index ) {
        return "{"
               "  \"presentationToken\": \"presentationToken\","
               "  \"listId\": \"vQdpOESlok\","
               "  \"listVersion\": " + std::to_string(listVersion) + ","
               "  \"operations\": ["
               "    {"
               "      \"type\": \"DeleteItem\","
               "      \"index\": " + std::to_string(index) +
               "    }"
               "  ]"
               "}";
    }

    static std::string
    createMultiInsert(int listVersion, int index, const std::vector<int>& items ) {
        std::string itemsString;
        for (size_t i = 0; i<items.size(); i++) {
            itemsString += std::to_string(items.at(i));
            if (i != items.size() - 1) {
                itemsString += ",";
            }
        }
        return "{"
               "  \"presentationToken\": \"presentationToken\","
               "  \"listId\": \"vQdpOESlok\","
               "  \"listVersion\": " + std::to_string(listVersion) + ","
               "  \"operations\": ["
               "    {"
               "      \"type\": \"InsertMultipleItems\","
               "      \"index\": " + std::to_string(index) + ","
               "      \"items\": [" + itemsString + "]"
               "    }"
               "  ]"
               "}";
    }

    static std::string
    createMultiDelete(int listVersion, int index, int count ) {
        return "{"
               "  \"presentationToken\": \"presentationToken\","
               "  \"listId\": \"vQdpOESlok\","
               "  \"listVersion\": " + std::to_string(listVersion) + ","
               "  \"operations\": ["
               "    {"
               "      \"type\": \"DeleteMultipleItems\","
               "      \"index\": " + std::to_string(index) + ","
               "      \"count\": " + std::to_string(count) +
               "    }"
               "  ]"
               "}";
    }

    DynamicIndexListTest() : DocumentWrapper() {
        auto cnf = DynamicIndexListConfiguration()
            .setType(SOURCE_TYPE)
            .setCacheChunkSize(TEST_CHUNK_SIZE)
            .setListUpdateBufferSize(5)
            .setFetchRetries(2)
            .setFetchTimeout(100)
            .setCacheExpiryTimeout(500);
        ds = std::make_shared<DynamicIndexListDataSourceProvider>(cnf);
        config->dataSourceProvider(SOURCE_TYPE, ds);
    }

    void TearDown() override {
        // Check for unprocessed errors.
        ASSERT_TRUE(ds->getPendingErrors().empty());

        // Clean any pending timeouts. Tests will check them explicitly.
        if(root) {
            loop->advanceToEnd();
            while (root->hasEvent()) {
                root->popEvent();
            }
        }
        DocumentWrapper::TearDown();
    }

    std::shared_ptr<DynamicIndexListDataSourceProvider> ds;
};

TEST_F(DynamicIndexListTest, Configuration) {
    // Backward compatibility
    auto source = std::make_shared<DynamicIndexListDataSourceProvider>("magic", 42);
    auto actualConfiguration = source->getConfiguration();
    ASSERT_EQ("magic", actualConfiguration.type);
    ASSERT_EQ(42, actualConfiguration.cacheChunkSize);
    ASSERT_EQ(5, actualConfiguration.listUpdateBufferSize);
    ASSERT_EQ(2, actualConfiguration.fetchRetries);
    ASSERT_EQ(5000, actualConfiguration.fetchTimeout);
    ASSERT_EQ(5000, actualConfiguration.cacheExpiryTimeout);

    // Full config
    auto expectedConfiguration = DynamicIndexListConfiguration()
            .setType("magic")
            .setCacheChunkSize(42)
            .setListUpdateBufferSize(7)
            .setFetchRetries(3)
            .setFetchTimeout(2000)
            .setCacheExpiryTimeout(10000);
    source = std::make_shared<DynamicIndexListDataSourceProvider>(expectedConfiguration);
    actualConfiguration = source->getConfiguration();
    ASSERT_EQ(expectedConfiguration.type, actualConfiguration.type);
    ASSERT_EQ(expectedConfiguration.cacheChunkSize, actualConfiguration.cacheChunkSize);
    ASSERT_EQ(expectedConfiguration.listUpdateBufferSize, actualConfiguration.listUpdateBufferSize);
    ASSERT_EQ(expectedConfiguration.fetchRetries, actualConfiguration.fetchRetries);
    ASSERT_EQ(expectedConfiguration.fetchTimeout, actualConfiguration.fetchTimeout);
    ASSERT_EQ(expectedConfiguration.cacheExpiryTimeout, actualConfiguration.cacheExpiryTimeout);

    // Default
    source = std::make_shared<DynamicIndexListDataSourceProvider>();
    actualConfiguration = source->getConfiguration();
    ASSERT_EQ(SOURCE_TYPE, actualConfiguration.type);
    ASSERT_EQ(10, actualConfiguration.cacheChunkSize);
    ASSERT_EQ(5, actualConfiguration.listUpdateBufferSize);
    ASSERT_EQ(2, actualConfiguration.fetchRetries);
    ASSERT_EQ(5000, actualConfiguration.fetchTimeout);
    ASSERT_EQ(5000, actualConfiguration.cacheExpiryTimeout);
}

static const char *DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *SMALLER_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 10,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *RESTRICTED_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 10,
    "maximumExclusiveIndex": 15,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *BASIC = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "height": 300,
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  }
})";

TEST_F(DynamicIndexListTest, Basic)
{
    loadDocument(BASIC, DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 11), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(12, 14), false));

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(9)->getId());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ(400, component->getCalculated(kPropertyScrollPosition).asNumber());
    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id19", component->getChildAt(19)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 5), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(6, 16), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(17, 19), false));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListTest, BasicAsMap)
{
    loadDocument(BASIC, DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19"))));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9"))));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4"))));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(400, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *BASIC_HORIZONTAL_RTL = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "width": 300,
      "scrollDirection": "horizontal",
      "layoutDirection": "RTL",
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  }
})";

TEST_F(DynamicIndexListTest, BasicRTL)
{
    loadDocument(BASIC_HORIZONTAL_RTL, DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 11), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(12, 14), false));

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(9)->getId());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ(-400, component->getCalculated(kPropertyScrollPosition).asNumber());
    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id19", component->getChildAt(19)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 5), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(6, 16), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(17, 19), false));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListTest, BasicAsMapRTL)
{
    loadDocument(BASIC_HORIZONTAL_RTL, DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19"))));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9"))));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4"))));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(-400, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *EMPTY = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "minimumInclusiveIndex": -5,
    "maximumExclusiveIndex": 5,
    "startIndex": 0
  }
})";

TEST_F(DynamicIndexListTest, Empty)
{
    loadDocument(BASIC, EMPTY);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 0, "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 4), true));

    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id4", component->getChildAt(4)->getId());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, -5, "-5, -4, -3, -2, -1")));

    root->clearPending();

    ASSERT_EQ(10, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 9), true));

    ASSERT_EQ("id-5", component->getChildAt(0)->getId());
    ASSERT_EQ("id4", component->getChildAt(9)->getId());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIRST_AND_LAST = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "height": 300,
      "data": "${dynamicSource}",
      "firstItem": {
        "type": "Text",
        "id": "fi",
        "width": 100,
        "height": 100,
        "text": "FI"
      },
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      },
      "lastItem": {
        "type": "Text",
        "id": "li",
        "width": 100,
        "height": 100,
        "text": "LI"
      }
    }
  }
})";

static const char *FIRST_AND_LAST_DATA =
R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [ 10 ]
  }
})";

TEST_F(DynamicIndexListTest, WithFirstAndLast)
{
    loadDocument(FIRST_AND_LAST, FIRST_AND_LAST_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 2}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 11, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 11, "11, 12, 13, 14, 15")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    // Whole range is laid out as we don't allow gaps
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 12), true));

    ASSERT_EQ(13, component->getChildCount());

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(1)->getId());
    ASSERT_EQ("id15", component->getChildAt(11)->getId());
    ASSERT_EQ("li", component->getChildAt(12)->getId());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    component->update(kUpdateScrollPosition, 600);
    root->clearPending();


    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", 16, 4));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 104, 16, "16, 17, 18, 19")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(1100, component->getCalculated(kPropertyScrollPosition).asNumber());

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id0", component->getChildAt(1)->getId());
    ASSERT_EQ("id19", component->getChildAt(20)->getId());
    ASSERT_EQ("li", component->getChildAt(21)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 21), true));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIRST_AND_LAST_HORIZONTAL_RTL = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "scrollDirection": "horizontal",
      "layoutDirection": "RTL",
      "width": 300,
      "data": "${dynamicSource}",
      "firstItem": {
        "type": "Text",
        "id": "fi",
        "width": 100,
        "height": 100,
        "text": "FI"
      },
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      },
      "lastItem": {
        "type": "Text",
        "id": "li",
        "width": 100,
        "height": 100,
        "text": "LI"
      }
    }
  }
})";

TEST_F(DynamicIndexListTest, WithFirstAndLastHorizontalRTL)
{
    loadDocument(FIRST_AND_LAST_HORIZONTAL_RTL, FIRST_AND_LAST_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 2}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 11, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 11, "11, 12, 13, 14, 15")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    // Whole range is laid out as we don't allow gaps
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 12), true));

    ASSERT_EQ(13, component->getChildCount());

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(1)->getId());
    ASSERT_EQ("id15", component->getChildAt(11)->getId());
    ASSERT_EQ("li", component->getChildAt(12)->getId());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    component->update(kUpdateScrollPosition, -600);
    root->clearPending();


    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", 16, 4));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 104, 16, "16, 17, 18, 19")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(-1100, component->getCalculated(kPropertyScrollPosition).asNumber());

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id0", component->getChildAt(1)->getId());
    ASSERT_EQ("id19", component->getChildAt(20)->getId());
    ASSERT_EQ("li", component->getChildAt(21)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 21), true));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIRST = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "height": 300,
      "data": "${dynamicSource}",
      "firstItem": {
        "type": "Text",
        "id": "fi",
        "width": 100,
        "height": 100,
        "text": "FI"
      },
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  }
})";

TEST_F(DynamicIndexListTest, WithFirst)
{
    loadDocument(FIRST, FIRST_AND_LAST_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 11, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 11, "11, 12, 13, 14, 15")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    // Whole range is laid out as we don't allow gaps
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 6), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(7, 11), false));

    ASSERT_EQ(12, component->getChildCount());

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(1)->getId());
    ASSERT_EQ("id15", component->getChildAt(11)->getId());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    component->update(kUpdateScrollPosition, 600);
    root->clearPending();

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", 16, 4));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id0", component->getChildAt(1)->getId());
    ASSERT_EQ("id15", component->getChildAt(16)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 16), true));

    ASSERT_FALSE(root->hasEvent());
}

static const char *LAST = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "height": 300,
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      },
      "lastItem": {
        "type": "Text",
        "id": "li",
        "width": 100,
        "height": 100,
        "text": "LI"
      }
    }
  }
})";

TEST_F(DynamicIndexListTest, WithLast)
{
    loadDocument(LAST, FIRST_AND_LAST_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 11, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 11, "11, 12, 13, 14, 15")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    // Whole range is laid out as we don't allow gaps
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 11), true));

    ASSERT_EQ(12, component->getChildCount());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id15", component->getChildAt(10)->getId());
    ASSERT_EQ("li", component->getChildAt(11)->getId());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(400, component->getCalculated(kPropertyScrollPosition).asNumber());

    component->update(kUpdateScrollPosition, 600);
    root->clearPending();

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 16, 4));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 16, "16, 17, 18, 19")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id15", component->getChildAt(10)->getId());
    ASSERT_EQ("li", component->getChildAt(15)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 15), true));

    ASSERT_FALSE(root->hasEvent());
}

static const char *LAST_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [ 0 ]
  }
})";

TEST_F(DynamicIndexListTest, WithLastOneWay)
{
    loadDocument(LAST, LAST_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 1, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 1, "1, 2, 3, 4, 5")));
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 6), true));

    ASSERT_EQ(7, component->getChildCount());

    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(5)->getId());
    ASSERT_EQ("li", component->getChildAt(6)->getId());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 6, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 6, "6, 7, 8, 9, 10")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 11), true));
    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(5)->getId());
    ASSERT_EQ("id10", component->getChildAt(10)->getId());
    ASSERT_EQ("li", component->getChildAt(11)->getId());

    ASSERT_FALSE(root->hasEvent());


    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).asNumber());
    component->update(kUpdateScrollPosition, 600);
    root->clearPending();

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 11, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 11, "11, 12, 13, 14, 15")));
    root->clearPending();
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", 16, 4));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(5)->getId());
    ASSERT_EQ("id10", component->getChildAt(10)->getId());
    ASSERT_EQ("id15", component->getChildAt(15)->getId());
    ASSERT_EQ("li", component->getChildAt(16)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 16), true));

    ASSERT_FALSE(root->hasEvent());
}

static const char *SHRINKABLE_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 10,
    "maximumExclusiveIndex": 15,
    "items": [ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 ]
  }
})";

TEST_F(DynamicIndexListTest, ShrinkData)
{
    loadDocument(BASIC, SHRINKABLE_DATA);
    advanceTime(10);
    ASSERT_TRUE(CheckBounds(10, 15));
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
}

static const char *EMPTY_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": []
  }
})";

TEST_F(DynamicIndexListTest, EmptySequence)
{
    loadDocument(BASIC, EMPTY_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 10, "10, 11, 12, 13, 14")));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 5, 5));
}

static const char *MULTI = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource1", "dynamicSource2"
    ],
    "item": {
      "type": "Container",
      "id": "container",
      "items": [
        {
          "type": "Sequence",
          "id": "sequence",
          "height": 300,
          "data": "${dynamicSource1}",
          "items": {
            "type": "Text",
            "id": "id${data}",
            "width": 100,
            "height": 100,
            "text": "${data}"
          }
        },
        {
          "type": "Sequence",
          "id": "sequence",
          "height": 300,
          "data": "${dynamicSource2}",
          "items": {
            "type": "Text",
            "id": "id${data}",
            "width": 100,
            "height": 100,
            "text": "${data}"
          }
        }
      ]
    }
  }
})";

static const char *MULTI_DATA = R"({
  "dynamicSource1": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok1",
    "startIndex": 10,
    "minimumInclusiveIndex": 10,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  },
  "dynamicSource2": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok2",
    "startIndex": 10,
    "minimumInclusiveIndex": 5,
    "maximumExclusiveIndex": 15,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

TEST_F(DynamicIndexListTest, Multi) {
    loadDocument(MULTI, MULTI_DATA);

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok1", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok2", "102", 5, 5));
}

static const char *WRONG_NIN_INDEX_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 15,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *WRONG_MISSING_FIELDS_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "minimumInclusiveIndex": 15,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *WRONG_MAX_INDEX_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 15,
    "maximumExclusiveIndex": 15,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *MULTI_CLONED_DATA = R"({
  "dynamicSource1": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  },
  "dynamicSource2": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

TEST_F(DynamicIndexListTest, WrongMissingFieldsData) {
    loadDocument(BASIC, WRONG_MISSING_FIELDS_DATA);
    ASSERT_TRUE(session->checkAndClear());
    ASSERT_TRUE(CheckErrors({"INTERNAL_ERROR"}));
    ASSERT_EQ(component->getChildCount(), 1);
}

TEST_F(DynamicIndexListTest, WrongNINIndexData) {

    loadDocument(BASIC, WRONG_NIN_INDEX_DATA);
    ASSERT_TRUE(session->checkAndClear());
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_EQ(component->getChildCount(), 1);
}

TEST_F(DynamicIndexListTest,WrongMaxIndexData) {
    loadDocument(BASIC, WRONG_MAX_INDEX_DATA);
    ASSERT_TRUE(session->checkAndClear());
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_EQ(component->getChildCount(), 1);
}

TEST_F(DynamicIndexListTest,MultiCloneData) {
    loadDocument(MULTI, MULTI_CLONED_DATA);
    ASSERT_TRUE(session->checkAndClear());
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_EQ(component->getChildCount(), 2);
}

static const char *BASIC_CONTAINER = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Container",
      "id": "container",
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  }
})";

TEST_F(DynamicIndexListTest, Container)
{
    loadDocument(BASIC_CONTAINER, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, -1, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    ASSERT_EQ(10, component->getChildCount());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(9)->getId());

    root->clearDirty();

    ASSERT_FALSE(root->isDirty());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(9)->getId());
}

static const char *WRONG_CORRELATION_TOKEN = R"({
  "token": "presentationToken",
  "correlationToken": "76",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "items": [ 5, 6, 7, 8, 9 ]
})";

static const char *TEN_TO_FOURTEEN_RANGE = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "minimumInclusiveIndex": 10,
  "maximumExclusiveIndex": 15
})";

static const char *INCOMPLETE_FOLLOWUP = R"({
  "token": "presentationToken",
  "startIndex": 5,
  "items": [ 5, 6, 7, 8, 9 ]
})";

static const char *UNCORRELATED_FOLLOWUP = R"({
  "token": "presentationToken",
  "correlationToken": "42",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "items": [ 5, 6, 7, 8, 9 ]
})";

static const char *WRONG_LIST_FOLLOWUP = R"({
  "token": "presentationToken",
  "listId": "DEADBEEF",
  "startIndex": 5,
  "items": [ 5, 6, 7, 8, 9 ]
})";

static const char *NOT_ARRAY_ITEMS_FOLLOWUP = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "items": { "abr": 1 }
})";

TEST_F(DynamicIndexListTest, WrongUpdates)
{
    loadDocument(BASIC, DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));

    ASSERT_EQ("id10", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(4)->getId());

    ASSERT_FALSE(ds->processUpdate(7)); // Should do nothing, type is wrong.
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_FALSE(ds->processUpdate(INCOMPLETE_FOLLOWUP)); // Should do nothing, missing fields.
    ASSERT_TRUE(CheckErrors({ "INVALID_LIST_ID" }));
    ASSERT_FALSE(ds->processUpdate(UNCORRELATED_FOLLOWUP)); // Should do nothing, wrong correlation token.
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_FALSE(ds->processUpdate(WRONG_LIST_FOLLOWUP)); // Should do nothing, wrong list.
    ASSERT_TRUE(CheckErrors({ "INVALID_LIST_ID" }));
    ASSERT_FALSE(ds->processUpdate(NOT_ARRAY_ITEMS_FOLLOWUP)); // Should do nothing, not an items array.
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_FALSE(ds->processUpdate(WRONG_CORRELATION_TOKEN));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    root->clearPending();

    ASSERT_FALSE(root->isDirty());

    // Adjust boundaries and try to update around it.
    ASSERT_TRUE(ds->processUpdate(TEN_TO_FOURTEEN_RANGE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE", "MISSING_LIST_ITEMS" }));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(-1, -1, 5, "5, 6, 7, 8, 9")));
    ASSERT_TRUE(CheckErrors({ "LOAD_INDEX_OUT_OF_RANGE" }));
}

static const char *DATA_PARTIAL_OOR = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 10,
    "maximumExclusiveIndex": 15,
    "items": []
  }
})";

TEST_F(DynamicIndexListTest, PartialOutOfRange)
{
    loadDocument(BASIC, DATA_PARTIAL_OOR);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(CheckBounds(10, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 9, "9, 10, 11, 12, 13, 14, 15")));
    ASSERT_TRUE(CheckErrors({ "LOAD_INDEX_OUT_OF_RANGE" }));

    root->clearPending();
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_EQ("id10", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(4)->getId());
}


static const char *UNKNOWN_BOUNDS_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": -10,
    "items": [ -10, -9, -8, -7, -6 ]
  }
})";

static const char *RESPONSE_AND_BOUND_UNKNOWN_DOWN = R"({
  "token": "presentationToken",
  "correlationToken": "103",
  "listId": "vQdpOESlok",
  "startIndex": -20,
  "minimumInclusiveIndex": -20,
  "maximumExclusiveIndex": 5,
  "items": [ -20, -19, -18, -17, -16 ]
})";

TEST_F(DynamicIndexListTest, UnknownBounds)
{
    loadDocument(BASIC, UNKNOWN_BOUNDS_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(INT_MIN, INT_MAX));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", -5, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, -1, -15, "-15, -14, -13, -12, -11")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, -1, -5, "-5, -4, -3, -2, -1")));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_EQ("id-15", component->getChildAt(0)->getId());
    ASSERT_EQ("id-1", component->getChildAt(14)->getId());

    ASSERT_TRUE(ds->processUpdate(RESPONSE_AND_BOUND_UNKNOWN_DOWN));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", -20, 5));

    // Scroll down to get it fetching again
    ASSERT_EQ(400, component->getCalculated(kPropertyScrollPosition).asNumber());
    component->update(kUpdateScrollPosition, 550); // + 5 children down
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "105", 0, 5));
    ASSERT_TRUE(CheckBounds(-20, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 104, 0, "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(component));
    ASSERT_EQ(1, component->getDirty().count(kPropertyNotifyChildrenChanged));

    ASSERT_EQ(25, component->getChildCount());

    ASSERT_EQ("id-20", component->getChildAt(0)->getId());
    ASSERT_EQ("id4", component->getChildAt(24)->getId());
}

static const char *SIMPLE_UPDATE = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": -17,
  "items": [ "-17U", "-16U", "-15U", "-14U", "-13U" ]
})";

TEST_F(DynamicIndexListTest, SimpleUpdate)
{
    loadDocument(BASIC, UNKNOWN_BOUNDS_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(INT_MIN, INT_MAX));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", -5, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, -1, -15, "-15, -14, -13, -12, -11")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, -1, -5, "-5, -4, -3, -2, -1")));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_EQ("-15", component->getChildAt(0)->getCalculated(kPropertyText).asString());
    ASSERT_EQ("-11", component->getChildAt(4)->getCalculated(kPropertyText).asString());
    ASSERT_EQ("-1", component->getChildAt(14)->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", -20, 5));

    ASSERT_TRUE(ds->processUpdate(SIMPLE_UPDATE));
    ASSERT_TRUE(CheckErrors({ "OCCUPIED_LIST_INDEX" }));
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    ASSERT_EQ(17, component->getChildCount());

    ASSERT_EQ("-17U", component->getChildAt(0)->getCalculated(kPropertyText).asString());
    ASSERT_EQ("-16U", component->getChildAt(1)->getCalculated(kPropertyText).asString());
    ASSERT_EQ("-15", component->getChildAt(2)->getCalculated(kPropertyText).asString());
}

static const char *POSITIVE_BOUNDS_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 7,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *RESPONSE_AND_BOUND_EXTEND = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 7,
  "minimumInclusiveIndex": 7,
  "maximumExclusiveIndex": 15,
  "items": [ 7, 8, 9 ]
})";

TEST_F(DynamicIndexListTest, PositiveBounds)
{
    loadDocument(BASIC, POSITIVE_BOUNDS_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(7, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 7, 3));

    ASSERT_TRUE(ds->processUpdate(RESPONSE_AND_BOUND_EXTEND));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    ASSERT_TRUE(CheckBounds(7, 15));
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(component));
    ASSERT_EQ(1, component->getDirty().count(kPropertyNotifyChildrenChanged));

    ASSERT_EQ(8, component->getChildCount());

    ASSERT_EQ("id7", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(7)->getId());
}

static const char *BASIC_CRUD_SERIES = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1,
  "operations": [
    {
      "type": "InsertListItem",
      "index": 11,
      "item": 111
    },
    {
      "type": "ReplaceListItem",
      "index": 13,
      "item": 113
    },
    {
      "type": "DeleteListItem",
      "index": 12
    }
  ]
})";

TEST_F(DynamicIndexListTest, CrudBasicSeries)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(10, 15));

    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));

    ASSERT_TRUE(ds->processUpdate(BASIC_CRUD_SERIES));
    root->clearPending();

    ASSERT_TRUE(CheckChildren({10, 111, 113, 13, 14}));
}

static const char *BROKEN_CRUD_SERIES = R"({
 "presentationToken": "presentationToken",
 "listId": "vQdpOESlok",
 "listVersion": 1,
 "operations": [
   {
     "type": "InsertListItem",
     "index": 11,
     "item": 111
   },
   {
     "type": "InsertListItem",
     "index": 27,
     "item": 27
   },
   {
     "type": "ReplaceListItem",
     "index": 13,
     "item": 113
   },
   {
     "type": "DeleteListItem",
     "index": 27,
     "item": 27
   },
   {
     "type": "DeleteListItem",
     "index": 12
   }
 ]
})";

TEST_F(DynamicIndexListTest, CrudInvalidInbetweenSeries)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(10, 15));

    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));

    ASSERT_FALSE(ds->processUpdate(BROKEN_CRUD_SERIES));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));
    root->clearPending();

    ASSERT_TRUE(CheckChildren({10, 111, 11, 12, 13, 14}));
}

static const char *STARTING_BOUNDS_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": -5,
    "minimumInclusiveIndex": -5,
    "maximumExclusiveIndex": 5,
    "items": [ -5, -4, -3, -2, -1, 0, 1, 2, 3, 4 ]
  }
})";

TEST_F(DynamicIndexListTest, CrudBoundsVerification)
{
    loadDocument(BASIC, STARTING_BOUNDS_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));

    ASSERT_TRUE(CheckBounds(-5, 5));

    // Negative insert
    ASSERT_TRUE(ds->processUpdate(createInsert(1, -3, -103)));
    root->clearPending();
    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 6));
    ASSERT_TRUE(CheckChildren({-5, -4, -103, -3, -2, -1, 0, 1, 2, 3, 4}));

    // Positive insert
    ASSERT_TRUE(ds->processUpdate(createInsert(2, 3, 103)));
    root->clearPending();
    ASSERT_EQ(12, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 7));
    ASSERT_TRUE(CheckChildren({-5, -4, -103, -3, -2, -1, 0, 1, 103, 2, 3, 4}));

    // Insert on 0
    ASSERT_TRUE(ds->processUpdate(createInsert(3, 0, 100)));
    root->clearPending();
    ASSERT_EQ(13, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 8));
    ASSERT_TRUE(CheckChildren({-5, -4, -103, -3, -2, 100, -1, 0, 1, 103, 2, 3, 4}));

    // Negative delete
    ASSERT_TRUE(ds->processUpdate(createDelete(4, -5)));
    root->clearPending();
    ASSERT_EQ(12, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 7));
    ASSERT_TRUE(CheckChildren({-4, -103, -3, -2, 100, -1, 0, 1, 103, 2, 3, 4}));

    // Positive delete
    ASSERT_TRUE(ds->processUpdate(createDelete(5, 3)));
    root->clearPending();
    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 6));
    ASSERT_TRUE(CheckChildren({-4, -103, -3, -2, 100, -1, 0, 1, 2, 3, 4}));

    // Delete on 0
    ASSERT_TRUE(ds->processUpdate(createDelete(6, 0)));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 5));
    ASSERT_TRUE(CheckChildren({-4, -103, -3, -2, 100, 0, 1, 2, 3, 4}));
}

TEST_F(DynamicIndexListTest, CrudPayloadGap)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Insert with gap
    ASSERT_FALSE(ds->processUpdate(createInsert(1, 17, 17)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListTest, CrudPayloadInsertOOB)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Insert out of bounds
    ASSERT_FALSE(ds->processUpdate(createInsert(1, 21, 21)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListTest, CrudPayloadRemoveOOB)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Remove out of bounds
    ASSERT_FALSE(ds->processUpdate(createDelete(1, 21)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListTest, CrudPayloadReplaceOOB)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Replace out of bounds
    ASSERT_FALSE(ds->processUpdate(createReplace(1, 21, 1000)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

static const char *WRONG_TYPE_CRUD = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1,
  "operations": [
    {
      "type": "7",
      "index": 10,
      "item": 101
    }
  ]
})";

TEST_F(DynamicIndexListTest, CrudPayloadInvalidOperation)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Specify wrong operation
    ASSERT_FALSE(ds->processUpdate(WRONG_TYPE_CRUD));
    ASSERT_TRUE(CheckErrors({ "INVALID_OPERATION" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

static const char *MALFORMED_OPERATION_CRUD = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1,
  "operations": [
    {
      "type": "InsertItem",
      "item": 101
    }
  ]
})";

TEST_F(DynamicIndexListTest, CrudPayloadMalformedOperation)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Specify wrong operation
    ASSERT_FALSE(ds->processUpdate(MALFORMED_OPERATION_CRUD));
    ASSERT_TRUE(CheckErrors({ "INVALID_OPERATION" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

static const char *MISSING_OPERATIONS_CRUD = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1
})";

TEST_F(DynamicIndexListTest, CrudPayloadNoOperation)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Don't specify any operations
    ASSERT_FALSE(ds->processUpdate(MISSING_OPERATIONS_CRUD));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

static const char *MISSING_LIST_VERSION_CRUD = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "operations": [
    {
      "type": "InsertItem",
      "index": 10,
      "item": 101
    }
  ]
})";

TEST_F(DynamicIndexListTest, CrudPayloadNoListVersion)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    ASSERT_FALSE(ds->processUpdate(MISSING_LIST_VERSION_CRUD));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION_IN_SEND_DATA" }));
}

TEST_F(DynamicIndexListTest, CrudMultiInsert) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Negative insert
    ASSERT_TRUE(ds->processUpdate(createMultiInsert(1, -3, {-31, -32})));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -31, -32, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 7));

    // Positive insert
    ASSERT_TRUE(ds->processUpdate(createMultiInsert(2, 3, {31, 32})));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -31, -32, -2, -1, 0, 31, 32, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 9));

    // Above loaded adjust insert
    ASSERT_TRUE(ds->processUpdate(createMultiInsert(3, 9, {71, 72})));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -31, -32, -2, -1, 0, 31, 32, 1, 2, 3, 4, 71, 72}));
    ASSERT_TRUE(CheckBounds(-5, 11));
}

TEST_F(DynamicIndexListTest, CrudMultiInsertAbove) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Attach at the end
    ASSERT_FALSE(ds->processUpdate(createMultiInsert(1, 10, {100, 101})));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListTest, CrudMultiInsertBelow) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Below loaded insert
    ASSERT_FALSE(ds->processUpdate(createMultiInsert(1, -10, {-100, -101})));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

static const char *NON_ARRAY_MULTI_INSERT = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1,
  "operations": [
    {
      "type": "InsertMultipleItems",
      "index": 11,
      "items": 111
    }
  ]
})";

TEST_F(DynamicIndexListTest, CrudMultiInsertNonArray) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Below loaded insert
    ASSERT_FALSE(ds->processUpdate(NON_ARRAY_MULTI_INSERT));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListTest, CrudMultiDelete) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Remove across
    ASSERT_TRUE(ds->processUpdate(createMultiDelete(1, -1, 3)));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 2));

    // Delete negative
    ASSERT_TRUE(ds->processUpdate(createMultiDelete(2, -5, 2)));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-3, -2, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 0));

    // Delete at the end
    ASSERT_TRUE(ds->processUpdate(createMultiDelete(3, -2, 2)));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-3, -2, 2}));
    ASSERT_TRUE(CheckBounds(-5, -2));
}

TEST_F(DynamicIndexListTest, CrudMultiDeleteOOB) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Out of range
    ASSERT_FALSE(ds->processUpdate(createMultiDelete(1, 7, 2)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListTest, CrudMultiDeletePartialOOB) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Some out of range
    ASSERT_FALSE(ds->processUpdate(createMultiDelete(1, 15, 3)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListTest, CrudMultiDeleteAll) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createMultiDelete(1, -5, 10)));
    root->clearPending();
    ASSERT_EQ(0, component->getChildCount());
}


static const char *SINGULAR_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": -5,
    "maximumExclusiveIndex": 5,
    "items": [ 0 ]
  }
})";

TEST_F(DynamicIndexListTest, CrudMultiDeleteMore) {
    loadDocument(BASIC, SINGULAR_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckChildren({0}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Some out of range
    ASSERT_FALSE(ds->processUpdate(createMultiDelete(1, 15, 3)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));

    ASSERT_EQ(1, component->getChildCount());
}

TEST_F(DynamicIndexListTest, CrudMultiDeleteLast) {
    loadDocument(BASIC, SINGULAR_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckChildren({0}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createMultiDelete(1, 0, 1)));
    root->clearPending();
    ASSERT_EQ(0, component->getChildCount());
}

TEST_F(DynamicIndexListTest, CrudDeleteLast) {
    loadDocument(BASIC, SINGULAR_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckChildren({0}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createDelete(1, 0)));
    root->clearPending();
    ASSERT_EQ(0, component->getChildCount());
}

TEST_F(DynamicIndexListTest, CrudInsertAdjascent) {
    loadDocument(BASIC, SINGULAR_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckChildren({0}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createInsert(1, 1, 1))); // This allowed (N+1)
    ASSERT_TRUE(ds->processUpdate(createInsert(2, 0, 11))); // This is also allowed (M)
    ASSERT_FALSE(ds->processUpdate(createInsert(3, -1, -1))); // This is not (M-1)
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));
    root->clearPending();

    ASSERT_TRUE(CheckChildren({11, 0, 1}));
    ASSERT_TRUE(CheckBounds(-5, 7));
    ASSERT_EQ(3, component->getChildCount());
}

static const char *LAZY_CRUD_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": -2,
    "minimumInclusiveIndex": -5,
    "maximumExclusiveIndex": 5,
    "items": [ -2, -1, 0, 1, 2 ]
  }
})";

TEST_F(DynamicIndexListTest, CrudLazyCombination)
{
    loadDocument(BASIC, LAZY_CRUD_DATA);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 3, 2));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -5, 3));

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-2, -1, 0, 1, 2}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createLazyLoad(1, 101, 3, "3, 4" )));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(2, 102, -5, "-5, -4, -3" )));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));

    ASSERT_TRUE(ds->processUpdate(createInsert(3, -2, -103)));
    root->clearPending();
    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 6));
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -103, -2, -1, 0, 1, 2, 3, 4}));

    ASSERT_TRUE(ds->processUpdate(createInsert(4, 4, 103)));
    root->clearPending();
    ASSERT_EQ(12, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 7));
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -103, -2, -1, 0, 1, 2, 103, 3, 4}));


}

static const char *LAZY_WITHOUT_VERSION = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "correlationToken": "102",
  "startIndex": -5,
  "items": [ -5, -4, -3 ]
})";

TEST_F(DynamicIndexListTest, CrudAfterNoVersionLazy)
{
    loadDocument(BASIC, LAZY_CRUD_DATA);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 3, 2));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -5, 3));

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-2, -1, 0, 1, 2}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(LAZY_WITHOUT_VERSION));
    root->clearPending();

    ASSERT_EQ(8, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2}));

    ASSERT_FALSE(ds->processUpdate(createInsert(1, 0, 101)));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION_IN_SEND_DATA" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListTest, CrudBeforeNoVersionLazy)
{
    loadDocument(BASIC, LAZY_CRUD_DATA);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 3, 2));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -5, 3));

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-2, -1, 0, 1, 2}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createInsert(1, 0, 101)));
    root->clearPending();

    ASSERT_EQ(6, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-2, -1, 101, 0, 1, 2}));

    ASSERT_FALSE(ds->processUpdate(LAZY_WITHOUT_VERSION));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION_IN_SEND_DATA" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListTest, CrudWrongData)
{
    loadDocument(BASIC, LAZY_CRUD_DATA);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 3, 2));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -5, 3));

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-2, -1, 0, 1, 2}));
    ASSERT_TRUE(CheckBounds(-5, 5));


    ASSERT_TRUE(ds->processUpdate(createInsert(1, -2, -103)));
    root->clearPending();
    ASSERT_EQ(6, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 6));
    ASSERT_TRUE(CheckChildren({-103, -2, -1, 0, 1, 2}));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 4, 2));

    // Wrong version crud will not fly
    ASSERT_FALSE(ds->processUpdate(createInsert(3, 0, 100))); // This is cached
    ASSERT_FALSE(ds->processUpdate(createInsert(1, 0, 100))); // This is not
    ASSERT_TRUE(CheckErrors({ "DUPLICATE_LIST_VERSION" }));
}

TEST_F(DynamicIndexListTest, CrudOutOfOrder)
{
    loadDocument(BASIC, STARTING_BOUNDS_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));

    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_FALSE(ds->processUpdate(createInsert(2, 4, 103)));
    ASSERT_FALSE(ds->processUpdate(createInsert(3, 2, 100)));
    ASSERT_FALSE(ds->processUpdate(createDelete(5, 5)));

    // Duplicate version in cache
    ASSERT_FALSE(ds->processUpdate(createDelete(5, 5)));
    ASSERT_TRUE(CheckErrors({ "DUPLICATE_LIST_VERSION" }));

    ASSERT_TRUE(ds->processUpdate(createInsert(1, -3, -103)));
    ASSERT_TRUE(ds->processUpdate(createDelete(4, -5)));

    ASSERT_TRUE(ds->processUpdate(createDelete(6, 2)));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 5));
    ASSERT_TRUE(CheckChildren({-4, -103, -3, -2, -1, 0, 100, 2, 103, 4}));
}

TEST_F(DynamicIndexListTest, CrudBadOutOfOrder)
{
    loadDocument(BASIC, STARTING_BOUNDS_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_FALSE(ds->processUpdate(createInsert(6, 0, 7)));
    loop->advanceToTime(500);

    // Update 6 will expire
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION" }));

    ASSERT_FALSE(ds->processUpdate(createInsert(5, 0, 6)));
    ASSERT_FALSE(ds->processUpdate(createInsert(4, 0, 5)));
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 0, 3)));
    ASSERT_FALSE(ds->processUpdate(createInsert(7, 0, 8)));
    ASSERT_FALSE(ds->processUpdate(createInsert(3, 0, 4)));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION" }));
    ASSERT_FALSE(ds->processUpdate(createInsert(8, 0, 9)));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION" }));

    ASSERT_TRUE(ds->processUpdate(createInsert(1, 0, 2)));
    loop->advanceToEnd();
    ASSERT_TRUE(CheckErrors({}));

    root->clearPending();
    ASSERT_EQ(16, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 7, 6, 5, 4, 3, 2, 0, 1, 2, 3, 4}));
}

static const char *BASIC_PAGER = R"({
  "type": "APL",
  "version": "1.2",
  "theme": "light",
  "layouts": {
    "square": {
      "parameters": ["color", "text"],
      "item": {
        "type": "Frame",
        "width": 200,
        "height": 200,
        "id": "frame-${text}",
        "backgroundColor": "${color}",
        "item": {
          "type": "Text",
          "text": "${text}",
          "color": "black",
          "width": 200,
          "height": 200
        }
      }
    }
  },
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Pager",
      "id": "pager",
      "data": "${dynamicSource}",
      "width": "100%",
      "height": "100%",
      "navigation": "normal",
      "items": {
        "type": "square",
        "index": "${index}",
        "color": "${data.color}",
        "text": "${data.text}"
      }
    }
  }
})";

static const char *BASIC_PAGER_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [
      { "color": "blue", "text": "10" },
      { "color": "red", "text": "11" },
      { "color": "green", "text": "12" },
      { "color": "yellow", "text": "13" },
      { "color": "white", "text": "14" }
    ]
  }
})";

static const char *FIVE_TO_NINE_FOLLOWUP_PAGER =
R"({
"token": "presentationToken",
"listId": "vQdpOESlok",
"startIndex": 5,
"items": [
  { "color": "blue", "text": "5" },
  { "color": "red", "text": "6" },
  { "color": "green", "text": "7" },
  { "color": "yellow", "text": "8" },
  { "color": "white", "text": "9" }
]
})";

TEST_F(DynamicIndexListTest, BasicPager)
{
    loadDocument(BASIC_PAGER, BASIC_PAGER_DATA);

    ASSERT_EQ(kComponentTypePager, component->getType());
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 20));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 4}, false));

    // Load 5 pages BEFORE the current set of pages
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));
    ASSERT_TRUE(ds->processUpdate(FIVE_TO_NINE_FOLLOWUP_PAGER));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_EQ("frame-5", component->getChildAt(0)->getId());
    ASSERT_EQ("frame-14", component->getChildAt(9)->getId());
    ASSERT_TRUE(CheckChildLaidOutDirtyFlagsWithNotify(component, 4));  // Page 4 gets loaded because we're on page 5
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0,3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4,6}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7,9}, false));

    // Switch to the first page (index=0)
    component->update(UpdateType::kUpdatePagerByEvent, 0);
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlagsWithNotify(component, {0, 1}));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 6}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7, 9}, false));

    // Load 5 more pages BEFORE the current set of pages
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 102, 15,
                                                 R"({ "color": "blue", "text": "15" },
                                                    { "color": "red", "text": "16" },
                                                    { "color": "green", "text": "17" },
                                                    { "color": "yellow", "text": "18" },
                                                    { "color": "white", "text": "19" })" )));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 103, 0,
                                                 R"({ "color": "blue", "text": "0" },
                                                    { "color": "red", "text": "1" },
                                                    { "color": "green", "text": "2" },
                                                    { "color": "yellow", "text": "3" },
                                                    { "color": "white", "text": "4" })" )));
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 6}, true));  // Page 4 gets loaded because we're on page 5
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7, 8}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {9, 11}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {12, 14}, false));

    // Switch to the last page (index=14)
    component->update(UpdateType::kUpdatePagerByEvent, 14);
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlagsWithNotify(component, {13, 14}));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 6}, true));  // Page 4 gets loaded because we're on page 5
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7, 8}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {9, 11}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {12, 12}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {13, 15}, true));  // Page 15 gets loaded because we're on page 14
    ASSERT_TRUE(CheckChildrenLaidOut(component, {16, 19}, false));

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(component));
    ASSERT_EQ(1, component->getDirty().count(kPropertyNotifyChildrenChanged));

    ASSERT_EQ("frame-0", component->getChildAt(0)->getId());
    ASSERT_EQ("frame-19", component->getChildAt(19)->getId());
}

static const char *EMPTY_PAGER_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": []
  }
})";

static const char *TEN_TO_FIFTEEN_RESPONSE_PAGER = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 10,
  "items": [
    { "color": "blue", "text": "10" },
    { "color": "red", "text": "11" },
    { "color": "green", "text": "12" },
    { "color": "yellow", "text": "13" },
    { "color": "white", "text": "14" }
  ]
})";

TEST_F(DynamicIndexListTest, EmptyPager)
{
    loadDocument(BASIC_PAGER, EMPTY_PAGER_DATA);

    ASSERT_EQ(kComponentTypePager, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 10, 5));
    ASSERT_TRUE(ds->processUpdate(TEN_TO_FIFTEEN_RESPONSE_PAGER));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 4}, false));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
}

static const char *WRAPPING_PAGER = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "light",
  "layouts": {
    "square": {
      "parameters": ["color", "text"],
      "item": {
        "type": "Frame",
        "width": 200,
        "height": 200,
        "id": "frame-${text}",
        "backgroundColor": "${color}",
        "item": {
          "type": "Text",
          "text": "${text}",
          "color": "black",
          "width": "100%",
          "height": "100%"
        }
      }
    }
  },
  "mainTemplate": {
    "parameters": [ "dynamicSource" ],
    "item": {
      "type": "Pager",
      "id": "pager",
      "data": "${dynamicSource}",
      "width": "100%",
      "height": "100%",
      "navigation": "wrap",
      "items": {
        "type": "square",
        "index": "${index}",
        "color": "${data.color}",
        "text": "${data.text}"
      }
    }
  }
})";

static const char *WRAPPING_PAGER_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 5,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [
      { "color": "blue", "text": "5" },
      { "color": "red", "text": "6" },
      { "color": "green", "text": "7" },
      { "color": "yellow", "text": "8" },
      { "color": "white", "text": "9" }
    ]
  }
})";

TEST_F(DynamicIndexListTest, WrappedPager)
{
    loadDocument(WRAPPING_PAGER, WRAPPING_PAGER_DATA);

    ASSERT_EQ(kComponentTypePager, component->getType());
    ASSERT_EQ(kNavigationWrap, static_cast<Navigation>(component->getCalculated(kPropertyNavigation).getInteger()));

    ASSERT_EQ(5, component->getChildCount());
    advanceTime(10);
    root->clearDirty();

    // Load 5 pages every direction the current set of pages
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 101, 0,
                                                 R"({ "color": "blue", "text": "0" },
                                                    { "color": "red", "text": "1" },
                                                    { "color": "green", "text": "2" },
                                                    { "color": "yellow", "text": "3" },
                                                    { "color": "white", "text": "4" })" )));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 102, 10,
                                                 R"({ "color": "blue", "text": "10" },
                                                    { "color": "red", "text": "11" },
                                                    { "color": "green", "text": "12" },
                                                    { "color": "yellow", "text": "13" },
                                                    { "color": "white", "text": "14" })" )));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());

    // Go back to 0
    component->update(UpdateType::kUpdatePagerByEvent, 0);
    root->clearPending();

    // We need to wrap to load from 15 to 20
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 103, 15,
                                                 R"({ "color": "blue", "text": "15" },
                                                    { "color": "red", "text": "16" },
                                                    { "color": "green", "text": "17" },
                                                    { "color": "yellow", "text": "18" },
                                                    { "color": "white", "text": "19" })" )));
    root->clearPending();

    ASSERT_EQ(20, component->getChildCount());
}

static const char *OLD_WRAPPING_PAGER = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "light",
  "layouts": {
    "square": {
      "parameters": ["color", "text"],
      "item": {
        "type": "Frame",
        "width": 200,
        "height": 200,
        "id": "frame-${text}",
        "backgroundColor": "${color}",
        "item": {
          "type": "Text",
          "text": "${text}",
          "color": "black",
          "width": "100%",
          "height": "100%"
        }
      }
    }
  },
  "mainTemplate": {
    "parameters": [ "dynamicSource" ],
    "item": {
      "type": "Pager",
      "id": "pager",
      "data": "${dynamicSource}",
      "width": "100%",
      "height": "100%",
      "navigation": "wrap",
      "items": {
        "type": "square",
        "index": "${index}",
        "color": "${data.color}",
        "text": "${data.text}"
      }
    }
  }
})";

TEST_F(DynamicIndexListTest, OldWrappedPager)
{
    loadDocument(OLD_WRAPPING_PAGER, WRAPPING_PAGER_DATA);

    ASSERT_EQ(kComponentTypePager, component->getType());
    // Check the override
    ASSERT_EQ(kNavigationNormal, static_cast<Navigation>(component->getCalculated(kPropertyNavigation).getInteger()));

    ASSERT_EQ(5, component->getChildCount());
    advanceTime(10);
    root->clearDirty();

    // Load 5 pages every direction the current set of pages
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 102, 10,
                                                 R"({ "color": "blue", "text": "10" },
                                                    { "color": "red", "text": "11" },
                                                    { "color": "green", "text": "12" },
                                                    { "color": "yellow", "text": "13" },
                                                    { "color": "white", "text": "14" })" )));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 101, 0,
                                                 R"({ "color": "blue", "text": "0" },
                                                    { "color": "red", "text": "1" },
                                                    { "color": "green", "text": "2" },
                                                    { "color": "yellow", "text": "3" },
                                                    { "color": "white", "text": "4" })" )));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());
}

static const char *SMALLER_DATA_BACK = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 5,
    "maximumExclusiveIndex": 15,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

TEST_F(DynamicIndexListTest, GarbageCollection) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // Kill RootContext and re-inflate.
    component = nullptr;
    context = nullptr;
    root = nullptr;

    loop = std::make_shared<TestTimeManager>();
    config->timeManager(loop);
    loadDocument(BASIC, SMALLER_DATA_BACK);
    advanceTime(20);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(5, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIFTEEN_TO_NINETEEN_WRONG_LIST_AND_TOKEN_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "76",
  "listId": "vQdpOESlok1",
  "startIndex": 15,
  "items": [ 15, 16, 17, 18, 19 ]
})";

static const char *FIFTEEN_TO_NINETEEN_WRONG_LIST_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok1",
  "startIndex": 15,
  "items": [ 15, 16, 17, 18, 19 ]
})";

TEST_F(DynamicIndexListTest, CorrelationTokenSubstitute) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_FALSE(ds->processUpdate(FIFTEEN_TO_NINETEEN_WRONG_LIST_AND_TOKEN_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INVALID_LIST_ID" }));

    ASSERT_TRUE(ds->processUpdate(FIFTEEN_TO_NINETEEN_WRONG_LIST_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_LIST_ID" }));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIFTEEN_TO_TWENTY_FOUR_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 15,
  "items": [ 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 ]
})";

TEST_F(DynamicIndexListTest, BigLazyLoad) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(ds->processUpdate(FIFTEEN_TO_TWENTY_FOUR_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "LOAD_INDEX_OUT_OF_RANGE" }));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIFTEEN_TO_NINETEEN_SHRINK_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 15,
  "minimumInclusiveIndex": 12,
  "items": [ 15, 16, 17, 18, 19 ]
})";

TEST_F(DynamicIndexListTest, BoundsShrinkBottom) {
    loadDocument(BASIC, SMALLER_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(ds->processUpdate(FIFTEEN_TO_NINETEEN_SHRINK_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE", "OCCUPIED_LIST_INDEX" }));
    root->clearPending();

    ASSERT_EQ(8, component->getChildCount());
    ASSERT_TRUE(CheckBounds(12, 20));
}

static const char *FIVE_TO_NINE_SHRINK_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "maximumExclusiveIndex": 13,
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListTest, BoundsShrinkTop) {
    loadDocument(BASIC, SMALLER_DATA_BACK);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(5, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));
    ASSERT_TRUE(ds->processUpdate(FIVE_TO_NINE_SHRINK_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    root->clearPending();

    ASSERT_EQ(8, component->getChildCount());
    ASSERT_TRUE(CheckBounds(5, 13));
}

static const char *SHRINK_FULL_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "minimumInclusiveIndex": 0,
  "maximumExclusiveIndex": 0,
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListTest, BoundsShrinkFull) {
    loadDocument(BASIC, SMALLER_DATA_BACK);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(5, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));
    ASSERT_TRUE(ds->processUpdate(SHRINK_FULL_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE", "INTERNAL_ERROR" }));
    root->clearPending();

    ASSERT_EQ(0, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 0));
}

static const char *EXPAND_BOTTOM_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 15,
  "minimumInclusiveIndex": 5,
  "items": [ 15, 16, 17, 18, 19 ]
})";

TEST_F(DynamicIndexListTest, BoundsExpandBottom) {
    loadDocument(BASIC, SMALLER_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(ds->processUpdate(EXPAND_BOTTOM_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    root->clearPending();

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckBounds(5, 20));
}

static const char *EXPAND_TOP_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "maximumExclusiveIndex": 20,
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListTest, BoundsExpandTop) {
    loadDocument(BASIC, SMALLER_DATA_BACK);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(5, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));
    ASSERT_TRUE(ds->processUpdate(EXPAND_TOP_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    root->clearPending();

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckBounds(5, 20));
}

static const char *EXPAND_FULL_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "minimumInclusiveIndex": -5,
  "maximumExclusiveIndex": 20,
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListTest, BoundsExpandFull) {
    loadDocument(BASIC, SMALLER_DATA_BACK);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(5, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));
    ASSERT_TRUE(ds->processUpdate(EXPAND_FULL_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    root->clearPending();

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 20));
}

static const char *FIFTEEN_EMPTY_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 15,
  "items": []
})";

TEST_F(DynamicIndexListTest, EmptyLazyResponseRetryFail) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(0, 101, 15, "")));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(0, 102, 15, "")));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 15, 5));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(0, 103, 15, "")));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListTest, EmptyLazyResponseRetryResolved) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_FALSE(ds->processUpdate(FIFTEEN_EMPTY_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIFTEEN_SHRINK_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "102",
  "listId": "vQdpOESlok",
  "startIndex": 15,
  "minimumInclusiveIndex": 10,
  "maximumExclusiveIndex": 15,
  "items": []
})";

TEST_F(DynamicIndexListTest, EmptyLazyResponseRetryBoundsUpdated) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_FALSE(ds->processUpdate(FIFTEEN_EMPTY_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_FALSE(ds->processUpdate(FIFTEEN_SHRINK_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE", "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckBounds(10, 15));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListTest, LazyResponseTimeout) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    // Not yet
    advanceTime(50);
    ASSERT_TRUE(CheckErrors({}));

    // Should go from here
    advanceTime(40);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    advanceTime(100);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 15, 5));
    advanceTime(100);
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListTest, LazyResponseTimeoutResolvedAfterLost) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    // Not yet
    advanceTime(50);
    ASSERT_TRUE(CheckErrors({}));

    // Should go from here
    advanceTime(40);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));

    // Retry response arrives
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 15, "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListTest, LazyResponseTimeoutResolvedAfterDelayed) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    // Not yet
    advanceTime(50);
    ASSERT_TRUE(CheckErrors({}));

    // Should go from here
    advanceTime(40);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));

    // Original response arrives
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // Retry arrives
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(-1, 102, 15, "15, 16, 17, 18, 19")));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *SWIPE_TO_DELETE = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "layouts": {
    "swipeAway" : {
      "parameters": ["text1", "text2"],
      "item": {
        "type": "TouchWrapper",
        "width": 200,
        "item": {
          "type": "Frame",
          "backgroundColor": "blue",
          "height": 100,
          "items": {
            "type": "Text",
            "text": "${text1}",
            "fontSize": 60
          }
        },
        "gestures": [
          {
            "type": "SwipeAway",
            "direction": "left",
            "action":"reveal",
            "items": {
              "type": "Frame",
              "backgroundColor": "purple",
              "width": "100%",
              "items": {
                "type": "Text",
                "text": "${text2}",
                "fontSize": 60,
                "color": "white"
              }
            },
            "onSwipeDone": {
              "type": "SendEvent",
              "arguments": ["${event.source.uid}", "${index}"]
            }
          }
        ]
      }
    }
  },
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "items": [
      {
        "type": "Sequence",
        "width": "100%",
        "height": 500,
        "alignItems": "center",
        "justifyContent": "spaceAround",
        "data": "${dynamicSource}",
        "items": [
          {
            "type": "swipeAway",
            "text1": "${data}",
            "text2": "${data}"
          }
        ]
      }
    ]
  }
})";

static const char *SWIPE_TO_DELETE_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 5,
    "items": [ 1, 2, 3, 4, 5 ]
  }
})";

TEST_F(DynamicIndexListTest, SwipeToDelete)
{
    config->set({
        {RootProperty::kSwipeAwayAnimationEasing, "linear"},
        {RootProperty::kPointerSlopThreshold, 5},
        {RootProperty::kSwipeVelocityThreshold, 5},
        {RootProperty::kTapOrScrollTimeout, 10},
        {RootProperty::kPointerInactivityTimeout, 1000}
    });
    loadDocument(SWIPE_TO_DELETE, SWIPE_TO_DELETE_DATA);

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_EQ(5, component->getDisplayedChildCount());

    auto idToDelete = component->getChildAt(0)->getUniqueId();

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(200,1), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(190,1), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(140,1), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(140,1), true));

    advanceTime(800);
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    int indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(0, indexToDelete);

    ASSERT_TRUE(ds->processUpdate(createDelete(1, indexToDelete)));
    root->clearPending();
    ASSERT_EQ(4, component->getChildCount());
    ASSERT_EQ(4, component->getDisplayedChildCount());
    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyBounds, kPropertyNotifyChildrenChanged));
    root->clearDirty();


    // Repeat for very first
    idToDelete = component->getChildAt(0)->getUniqueId();

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(200,1), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(190,1), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(140,1), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(140,1), true));

    advanceTime(800);
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(0, indexToDelete);
    root->clearDirty();

    ASSERT_TRUE(ds->processUpdate(createDelete(2, indexToDelete)));
    root->clearPending();
    ASSERT_EQ(3, component->getChildCount());
    ASSERT_EQ(3, component->getDisplayedChildCount());
    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyBounds,kPropertyNotifyChildrenChanged));
    root->clearDirty();


    // Remove one at the end
    idToDelete = component->getChildAt(2)->getUniqueId();

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(200,201), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(190,201), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(140,201), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(140,201), true));

    advanceTime(800);
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(2, indexToDelete);
    root->clearDirty();

    ASSERT_TRUE(ds->processUpdate(createDelete(3, indexToDelete)));
    root->clearPending();
    root->clearDirty();

    ASSERT_EQ(2, component->getChildCount());
    ASSERT_EQ(2, component->getDisplayedChildCount());

    // again
    idToDelete = component->getChildAt(0)->getUniqueId();

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(200,1), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(190,1), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(140,1), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(140,1), true));

    advanceTime(800);
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(0, indexToDelete);
    root->clearDirty();

    ASSERT_TRUE(ds->processUpdate(createDelete(4, indexToDelete)));
    root->clearPending();
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyBounds,kPropertyNotifyChildrenChanged));
    root->clearDirty();

    // empty the list
    idToDelete = component->getChildAt(0)->getUniqueId();

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(200,1), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(190,1), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(140,1), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(140,1), true));

    advanceTime(800);
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(0, indexToDelete);
    root->clearDirty();

    ASSERT_TRUE(ds->processUpdate(createDelete(5, indexToDelete)));
    root->clearPending();
    ASSERT_EQ(0, component->getChildCount());
    ASSERT_EQ(0, component->getDisplayedChildCount());
    root->clearDirty();
}

static const char *PROACTIVE_LOAD_ONLY = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 5,
    "minimumInclusiveIndex": 5,
    "maximumExclusiveIndex": 5
  }
})";

static const char *PROACTIVE_EXPAND_RESPONSE = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "minimumInclusiveIndex": 5,
  "maximumExclusiveIndex": 10,
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListTest, ProactiveLoadOnly)
{
    loadDocument(BASIC, PROACTIVE_LOAD_ONLY);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(ds->processUpdate(PROACTIVE_EXPAND_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(5, 10));

    ASSERT_FALSE(root->hasEvent());
}

static const char *PROACTIVE_EXPAND_BAD_RESPONSE = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "minimumInclusiveIndex": 5
  "maximumExclusiveIndex": 10
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListTest, ProactiveLoadOnlyBadJson)
{
    loadDocument(BASIC, PROACTIVE_LOAD_ONLY);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_FALSE(root->hasEvent());

    ASSERT_FALSE(ds->processUpdate(PROACTIVE_EXPAND_BAD_RESPONSE));
}

static const char *BASIC_CONFIG_CHANGE = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "preserve": ["centerIndex"],
      "height": 300,
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  },
  "onConfigChange": [
    {
      "type": "Reinflate"
    }
  ]
})";

TEST_F(DynamicIndexListTest, Reinflate) {
    loadDocument(BASIC_CONFIG_CHANGE, DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 20));
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();
    ASSERT_EQ(15, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4")));
    root->clearPending();
    ASSERT_EQ(20, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // re-inflate should get same result.
    auto oldComponent = component;
    configChangeReinflate(ConfigurationChange(100, 100));
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getId(), oldComponent->getId());
    ASSERT_EQ(20, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 20));
    ASSERT_FALSE(root->hasEvent());
}

static const char *TYPED_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [
      { "type": "TYPE1", "value": 0 },
      { "type": "TYPE2", "value": 1 },
      { "type": "TYPE2", "value": 2 },
      { "type": "TYPE1", "value": 3 },
      { "type": "TYPE1", "value": 4 },
      { "type": "TYPE1", "value": 5 },
      { "type": "TYPE1", "value": 6 },
      { "type": "TYPE1", "value": 7 },
      { "type": "TYPE1", "value": 8 },
      { "type": "TYPE1", "value": 9 }
    ]
  }
})";

static const char *MULTITYPE_SEQUENCE = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "height": 200,
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "when": "${data.type == 'TYPE2'}",
        "id": "id${data.value}",
        "width": 100,
        "height": 100,
        "text": "${data.value}"
      }
    }
  }
})";

TEST_F(DynamicIndexListTest, ConditionalSequenceChildren)
{
    loadDocument(MULTITYPE_SEQUENCE, TYPED_DATA);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 10,
        "{\"type\": \"TYPE1\", \"value\": 10},"
        "{\"type\": \"TYPE1\", \"value\": 11},"
        "{\"type\": \"TYPE1\", \"value\": 12},"
        "{\"type\": \"TYPE1\", \"value\": 13},"
        "{\"type\": \"TYPE1\", \"value\": 14}")));
    root->clearPending();

    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 15,
         "{\"type\": \"TYPE1\", \"value\": 15},"
         "{\"type\": \"TYPE1\", \"value\": 16},"
         "{\"type\": \"TYPE1\", \"value\": 17},"
         "{\"type\": \"TYPE2\", \"value\": 18},"
         "{\"type\": \"TYPE2\", \"value\": 19}")));
    root->clearPending();

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *TYPED_DATA_BACK = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": -15,
    "maximumExclusiveIndex": 2,
    "items": [
      { "type": "TYPE2", "value": 0 },
      { "type": "TYPE1", "value": 1 }
    ]
  }
})";

TEST_F(DynamicIndexListTest, ConditionalSequenceChildrenBackwards)
{
    loadDocument(MULTITYPE_SEQUENCE, TYPED_DATA_BACK);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(-15, 2));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", -5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, -5,
                                                 "{\"type\": \"TYPE1\", \"value\": -5},"
                                                 "{\"type\": \"TYPE1\", \"value\": -4},"
                                                 "{\"type\": \"TYPE1\", \"value\": -3},"
                                                 "{\"type\": \"TYPE1\", \"value\": -2},"
                                                 "{\"type\": \"TYPE1\", \"value\": -1}")));
    root->clearPending();

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, -10,
                                                 "{\"type\": \"TYPE1\", \"value\": -10},"
                                                 "{\"type\": \"TYPE1\", \"value\": -9},"
                                                 "{\"type\": \"TYPE1\", \"value\": -8},"
                                                 "{\"type\": \"TYPE2\", \"value\": -7},"
                                                 "{\"type\": \"TYPE2\", \"value\": -6}")));
    root->clearPending();

    ASSERT_EQ(3, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", -15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, -15,
                                                 "{\"type\": \"TYPE1\", \"value\": -15},"
                                                 "{\"type\": \"TYPE1\", \"value\": -14},"
                                                 "{\"type\": \"TYPE1\", \"value\": -13},"
                                                 "{\"type\": \"TYPE2\", \"value\": -12},"
                                                 "{\"type\": \"TYPE2\", \"value\": -11}")));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *TYPED_DATA_START_EMPTY = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 5,
    "items": [
      { "type": "TYPE1", "value": 0 },
      { "type": "TYPE1", "value": 1 }
    ]
  }
})";

TEST_F(DynamicIndexListTest, ConditionalSequenceChildrenStartEmpty)
{
    loadDocument(MULTITYPE_SEQUENCE, TYPED_DATA_START_EMPTY);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(0, 5));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 2, 3));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 2,
                                                 "{\"type\": \"TYPE1\", \"value\": 2},"
                                                 "{\"type\": \"TYPE2\", \"value\": 3},"
                                                 "{\"type\": \"TYPE1\", \"value\": 4}")));
    root->clearPending();
    ASSERT_EQ(1, component->getChildCount());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *MULTITYPE_PAGER = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Pager",
      "height": 200,
      "width": 200,
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "when": "${data.type == 'TYPE2'}",
        "id": "id${data.value}",
        "width": 100,
        "height": 100,
        "text": "${data.value}"
      }
    }
  }
})";

TEST_F(DynamicIndexListTest, ConditionalPagerChildren)
{
    loadDocument(MULTITYPE_PAGER, TYPED_DATA);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 10,
                                                 "{\"type\": \"TYPE1\", \"value\": 10},"
                                                 "{\"type\": \"TYPE1\", \"value\": 11},"
                                                 "{\"type\": \"TYPE1\", \"value\": 12},"
                                                 "{\"type\": \"TYPE1\", \"value\": 13},"
                                                 "{\"type\": \"TYPE1\", \"value\": 14}")));
    root->clearPending();

    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 15,
                                                 "{\"type\": \"TYPE1\", \"value\": 15},"
                                                 "{\"type\": \"TYPE1\", \"value\": 16},"
                                                 "{\"type\": \"TYPE1\", \"value\": 17},"
                                                 "{\"type\": \"TYPE2\", \"value\": 18},"
                                                 "{\"type\": \"TYPE2\", \"value\": 19}")));
    root->clearPending();

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListTest, ConditionalPagerChildrenBackwards)
{
    loadDocument(MULTITYPE_PAGER, TYPED_DATA_BACK);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(-15, 2));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", -5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, -5,
                                                 "{\"type\": \"TYPE1\", \"value\": -5},"
                                                 "{\"type\": \"TYPE1\", \"value\": -4},"
                                                 "{\"type\": \"TYPE1\", \"value\": -3},"
                                                 "{\"type\": \"TYPE1\", \"value\": -2},"
                                                 "{\"type\": \"TYPE1\", \"value\": -1}")));
    root->clearPending();

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, -10,
                                                 "{\"type\": \"TYPE1\", \"value\": -10},"
                                                 "{\"type\": \"TYPE1\", \"value\": -9},"
                                                 "{\"type\": \"TYPE1\", \"value\": -8},"
                                                 "{\"type\": \"TYPE2\", \"value\": -7},"
                                                 "{\"type\": \"TYPE2\", \"value\": -6}")));
    root->clearPending();

    ASSERT_EQ(3, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", -15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, -15,
                                                 "{\"type\": \"TYPE1\", \"value\": -15},"
                                                 "{\"type\": \"TYPE1\", \"value\": -14},"
                                                 "{\"type\": \"TYPE1\", \"value\": -13},"
                                                 "{\"type\": \"TYPE2\", \"value\": -12},"
                                                 "{\"type\": \"TYPE2\", \"value\": -11}")));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListTest, ConditionalPagerChildrenStartEmpty)
{
    loadDocument(MULTITYPE_PAGER, TYPED_DATA_START_EMPTY);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(0, 5));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 2, 3));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 2,
                                                 "{\"type\": \"TYPE1\", \"value\": 2},"
                                                 "{\"type\": \"TYPE2\", \"value\": 3},"
                                                 "{\"type\": \"TYPE1\", \"value\": 4}")));
    root->clearPending();
    ASSERT_EQ(1, component->getChildCount());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *SEQUENCE_RECREATE_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 1,
    "items": [
      { "label": "I am a label.", "sequence": ["red", "green", "blue", "yellow", "purple"] }
    ]
  }
})";

static const char *SEQUENCE_RECREATE = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Container",
      "height": 300,
      "width": 300,
      "data": "${dynamicSource}",
      "items": {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "items": [
          {
            "type": "Sequence",
            "height": "50%",
            "width": "100%",
            "data": "${data.sequence}",
            "items": {
              "type": "Frame",
              "backgroundColor": "${data}",
              "height": 10,
              "width": "100%"
            }
          }
        ]
      }
    }
  }
})";

static const char *REPLACE_SEQUENCE_CRUD = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1,
  "operations": [
    {
      "type": "DeleteListItem",
      "index": 0
    },
    {
      "type": "InsertListItem",
      "index": 0,
      "item": { "sequence": ["purple", "yellow", "blue", "green", "red"] }
    }
  ]
})";

TEST_F(DynamicIndexListTest, SequenceRecreate)
{
    loadDocument(SEQUENCE_RECREATE, SEQUENCE_RECREATE_DATA);
    advanceTime(10);

    ASSERT_EQ(1, component->getChildCount());
    auto sequence = component->getCoreChildAt(0)->getCoreChildAt(0);
    ASSERT_EQ(5, sequence->getChildCount());

    ASSERT_EQ(Rect(0,  0, 300, 300), component->getCoreChildAt(0)->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 0, 300, 150), sequence->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0,  0, 300, 10), sequence->getCoreChildAt(0)->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 10, 300, 10), sequence->getCoreChildAt(1)->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 20, 300, 10), sequence->getCoreChildAt(2)->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 30, 300, 10), sequence->getCoreChildAt(3)->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 40, 300, 10), sequence->getCoreChildAt(4)->getCalculated(kPropertyBounds).getRect());

    ASSERT_TRUE(ds->processUpdate(REPLACE_SEQUENCE_CRUD));
    root->clearPending();

    sequence = component->getCoreChildAt(0)->getCoreChildAt(0);
    ASSERT_EQ(5, sequence->getChildCount());

    ASSERT_EQ(Rect(0,  0, 300, 300).toDebugString(), component->getCoreChildAt(0)->getCalculated(kPropertyBounds).getRect().toDebugString());
    ASSERT_EQ(Rect(0,  0, 300, 150), sequence->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0,   0, 300, 10), sequence->getCoreChildAt(0)->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 10, 300, 10), sequence->getCoreChildAt(1)->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 20, 300, 10), sequence->getCoreChildAt(2)->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 30, 300, 10), sequence->getCoreChildAt(3)->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 40, 300, 10), sequence->getCoreChildAt(4)->getCalculated(kPropertyBounds).getRect());
}

static const char *FILLED_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 5,
    "items": [ 0, 1, 2, 3, 4 ]
  }
})";

TEST_F(DynamicIndexListTest, DeleteMultipleAll)
{
    loadDocument(BASIC, FILLED_DATA);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(0, 5));
    ASSERT_EQ(5, component->getChildCount());

    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(ds->processUpdate(createMultiDelete(1, 0, 100)));
    root->clearPending();

    ASSERT_EQ(0, component->getChildCount());
}



static const char *FORWARD_ONLY_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 10,
    "items": [ 0, 1, 2, 3, 4 ]
  }
})";

static const char *SHRINK_BOUNDS_WITHOUT_ITEMS = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": 0,
  "minimumInclusiveIndex": 0,
  "maximumExclusiveIndex": 5
})";

TEST_F(DynamicIndexListTest, ShrinkWithoutItems)
{
    loadDocument(BASIC, FORWARD_ONLY_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));

    ASSERT_TRUE(ds->processUpdate(SHRINK_BOUNDS_WITHOUT_ITEMS));
    ASSERT_TRUE(CheckErrors({"INCONSISTENT_RANGE", "MISSING_LIST_ITEMS"}));
    root->clearPending();

    advanceTime(10000);
    ASSERT_FALSE(root->hasEvent());
}