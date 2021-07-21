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

#include "rapidjson/document.h"

#include "../testeventloop.h"

#include "apl/dynamicdata.h"
#include "apl/livedata/livedatamanager.h"

using namespace apl;

static const char* DTL_SOURCE_TYPE = "dynamicTokenList";
static const char* DIL_SOURCE_TYPE = "dynamicIndexList";
static const char* LIST_ID = "listId";
static const char* CORRELATION_TOKEN = "correlationToken";
static const char* PAGE_TOKEN = "pageToken";
static const char* START_INDEX = "startIndex";
static const char* COUNT = "count";
static const size_t TEST_CHUNK_SIZE = 2;

class DatasourceContextTest : public DocumentWrapper {
public:
    DatasourceContextTest() : DocumentWrapper(),
    dcDoc(rapidjson::kObjectType) {}

    void postInflate() override {
        ASSERT_FALSE(root->isDataSourceContextDirty());
        serializeDatasourceContext();
    }

    void serializeDatasourceContext() {
        root->clearPending();
        datasourceContext = root->serializeDataSourceContext(dcDoc.GetAllocator());
    }


    void TearDown() override {
        // Clean any pending timeouts. Tests will check them explicitly.
        if (root) {
            loop->advanceToEnd();
            while (root->hasEvent()) {
                root->popEvent();
            }
        }

        DocumentWrapper::TearDown();
    }

    rapidjson::Document dcDoc;
    rapidjson::Value datasourceContext;

protected:
    ::testing::AssertionResult CheckFetchRequest(const std::string& listId,
                                                 const std::string& correlationToken,
                                                 const std::string& pageToken) {
        bool fetchCalled = root->hasEvent();
        auto event = root->popEvent();
        fetchCalled &= (event.getType() == kEventTypeDataSourceFetchRequest);

        if (!fetchCalled)
            return ::testing::AssertionFailure() << "Fetch was not called.";

        auto incomingType = event.getValue(kEventPropertyName).getString();
        if (DTL_SOURCE_TYPE != incomingType)
            return ::testing::AssertionFailure()
                    << "DataSource type is wrong. Expected: dynamicTokenList"
                    << ", actual: " << incomingType;

        auto request = event.getValue(kEventPropertyValue);

        auto incomingListId = request.opt(LIST_ID, "");
        if (incomingListId != listId)
            return ::testing::AssertionFailure()
                    << "listId is wrong. Expected: " << listId << ", actual: " << incomingListId;

        auto incomingCorrelationToken = request.opt(CORRELATION_TOKEN, "");
        if (incomingCorrelationToken != correlationToken)
            return ::testing::AssertionFailure()
                    << "correlationToken is wrong. Expected: " << correlationToken
                    << ", actual: " << incomingCorrelationToken;

        auto incomingPageToken = request.opt(PAGE_TOKEN, "");
        if (incomingPageToken != pageToken)
            return ::testing::AssertionFailure() << "pageToken is wrong. Expected: " << pageToken
                                                 << ", actual: " << incomingPageToken;

        return ::testing::AssertionSuccess();
    }

    static std::string createLazyLoad(int correlationToken, const std::string& pageToken,
                                      const std::string& nextPageToken, const std::string& items) {
        std::string ctString =
                correlationToken < 0
                ? ""
                : ("\"correlationToken\": \"" + std::to_string(correlationToken) + "\",");
        std::string nptString =
                nextPageToken == "" ? "" : ("\"nextPageToken\": \"" + nextPageToken + "\",");
        std::string ptString = "\"pageToken\": \"" + pageToken + "\",";
        return "{"
               "  \"presentationToken\": \"presentationToken\","
               "  \"listId\": \"vQdpOESlok\"," +
               ctString + ptString + nptString + "  \"items\": [" + items + "]"
                                                                            "}";
    }

    ::testing::AssertionResult
    CheckFetchRequest(const std::string& listId, const std::string& correlationToken, int startIndex, int count) {
        bool fetchCalled = root->hasEvent();
        auto event = root->popEvent();
        fetchCalled &= (event.getType() == kEventTypeDataSourceFetchRequest);

        if (!fetchCalled)
            return ::testing::AssertionFailure() << "Fetch was not called.";

        auto incomingType = event.getValue(kEventPropertyName).getString();
        if (DIL_SOURCE_TYPE != incomingType)
            return ::testing::AssertionFailure()
                << "DataSource type is wrong. Expected: " << DIL_SOURCE_TYPE
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
    serializeAndCheckDTLContext(int index, const std::string& type, const std::string& listId,
                                const std::string& backwardPageToken, const std::string& forwardPageToken) {

        serializeDatasourceContext();

        if (datasourceContext[index]["type"].GetString() != type)
            return ::testing::AssertionFailure()
                << "DataSource type is wrong. Expected: " << type
                << ", actual: " << datasourceContext[index]["type"].GetString();

        if (datasourceContext[index]["listId"].GetString() != listId)
            return ::testing::AssertionFailure()
                << "DataSource listId is wrong. Expected: " << listId
                << ", actual: " << datasourceContext[index]["listId"].GetString();

        if (datasourceContext[index]["backwardPageToken"].GetString() != backwardPageToken)
            return ::testing::AssertionFailure()
                << "DataSource backwardPageToken is wrong. Expected: " << backwardPageToken
                << ", actual: " << datasourceContext[index]["backwardPageToken"].GetString();

        if (datasourceContext[index]["forwardPageToken"].GetString() != forwardPageToken)
            return ::testing::AssertionFailure()
                << "DataSource forwardPageToken is wrong. Expected: " << forwardPageToken
                << ", actual: " << datasourceContext[index]["forwardPageToken"].GetString();

        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    serializeAndCheckDILContext(int index, const std::string& type, const std::string& listId, int listVersion,
                            int minimumInclusiveIndex, int maximumExclusiveIndex, int startIndex) {

        serializeDatasourceContext();

        if (datasourceContext[index]["type"].GetString() != type)
            return ::testing::AssertionFailure()
                << "DataSource type is wrong. Expected: " << type
                << ", actual: " << datasourceContext[index]["type"].GetString();

        if (datasourceContext[index]["listId"].GetString() != listId)
            return ::testing::AssertionFailure()
                << "DataSource listId is wrong. Expected: " << listId
                << ", actual: " << datasourceContext[index]["listId"].GetString();

        if (datasourceContext[index]["listVersion"].GetInt() != listVersion)
            return ::testing::AssertionFailure()
                << "DataSource listVersion is wrong. Expected: " << listVersion
                << ", actual: " << datasourceContext[index]["listVersion"].GetString();

        if (datasourceContext[index]["minimumInclusiveIndex"].GetInt() != minimumInclusiveIndex)
            return ::testing::AssertionFailure()
                << "DataSource minimumInclusiveIndex is wrong. Expected: " << minimumInclusiveIndex
                << ", actual: " << datasourceContext[index]["minimumInclusiveIndex"].GetString();

        if (datasourceContext[index]["maximumExclusiveIndex"].GetInt() != maximumExclusiveIndex)
            return ::testing::AssertionFailure()
                << "DataSource maximumExclusiveIndex is wrong. Expected: " << maximumExclusiveIndex
                << ", actual: " << datasourceContext[index]["maximumExclusiveIndex"].GetString();

        if (datasourceContext[index]["startIndex"].GetInt() != startIndex)
            return ::testing::AssertionFailure()
                << "DataSource startIndex is wrong. Expected: " << startIndex
                << ", actual: " << datasourceContext[index]["startIndex"].GetString();

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
};

static const char* NO_DATASOURCE_DOC = R"apl(
{
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "width": 400,
      "height": 400,
      "text": "Hello World!"
    }
  }
}
)apl";

TEST_F(DatasourceContextTest, NoDatasource) {
    loadDocument(NO_DATASOURCE_DOC);

    // Check number of datasources registered
    ASSERT_EQ(0, datasourceContext.GetArray().Size());
    ASSERT_FALSE(root->isDataSourceContextDirty());
}

static const char* DYNAMIC_TOKEN_LIST_DATA = R"({
  "dynamicSource": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok",
    "pageToken": "pageToken",
    "backwardPageToken": "backwardPageToken",
    "forwardPageToken": "forwardPageToken",
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char* DYNAMIC_TOKEN_LIST_DOC = R"({
  "type": "APL",
  "version": "1.6",
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

TEST_F(DatasourceContextTest, DynamicTokenList) {
    auto cnf = DynamicListConfiguration(DTL_SOURCE_TYPE).setFetchTimeout(100);
    auto ds = std::make_shared<DynamicTokenListDataSourceProvider>(cnf);
    config->dataSourceProvider(DTL_SOURCE_TYPE, ds);

    loadDocument(DYNAMIC_TOKEN_LIST_DOC, DYNAMIC_TOKEN_LIST_DATA);

    // Check number of datasources registered
    ASSERT_EQ(1, datasourceContext.GetArray().Size());

    // Check datasource context value
    ASSERT_TRUE(serializeAndCheckDTLContext(0, DTL_SOURCE_TYPE, "vQdpOESlok", "backwardPageToken", "forwardPageToken"));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(ds->processUpdate(
            createLazyLoad(101, "forwardPageToken", "forwardPageToken1",
                           "15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30")));
    ASSERT_TRUE(root->isDataSourceContextDirty());
    ASSERT_TRUE(serializeAndCheckDTLContext(0, DTL_SOURCE_TYPE, "vQdpOESlok", "backwardPageToken", "forwardPageToken1"));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken"));
    ASSERT_TRUE(ds->processUpdate(
            createLazyLoad(102, "backwardPageToken", "backwardPageToken1", "5, 6, 7, 8, 9")));
    ASSERT_TRUE(root->isDataSourceContextDirty());
    ASSERT_TRUE(serializeAndCheckDTLContext(0, DTL_SOURCE_TYPE, "vQdpOESlok", "backwardPageToken1", "forwardPageToken1"));

    // Check for unprocessed errors.
    ASSERT_TRUE(ds->getPendingErrors().empty());
    // Check after serialization datasource context should not be dirty
    ASSERT_FALSE(root->isDataSourceContextDirty());
}

static const char *DYNAMIC_INDEX_LIST_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *DYNAMIC_INDEX_LIST_DOC = R"({
  "type": "APL",
  "version": "1.6",
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

TEST_F(DatasourceContextTest, DynamicIndexList) {
    auto cnf = DynamicIndexListConfiguration()
        .setType(DIL_SOURCE_TYPE)
        .setCacheChunkSize(TEST_CHUNK_SIZE)
        .setListUpdateBufferSize(5)
        .setFetchRetries(2)
        .setFetchTimeout(100)
        .setCacheExpiryTimeout(500);
    auto ds = std::make_shared<DynamicIndexListDataSourceProvider>(cnf);
    config->dataSourceProvider(DIL_SOURCE_TYPE, ds);

    loadDocument(DYNAMIC_INDEX_LIST_DOC, DYNAMIC_INDEX_LIST_DATA);

    // Check number of datasources registered
    ASSERT_EQ(1, datasourceContext.GetArray().Size());

    // Check datasource context value
    ASSERT_TRUE(serializeAndCheckDILContext(0, DIL_SOURCE_TYPE, "vQdpOESlok", 0, 0, 20, 10));

    // Update dynamicIndexListData and check new context value
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 8, 2));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(1, 101, 8, "8, 9")));
    ASSERT_TRUE(root->isDataSourceContextDirty());
    ASSERT_TRUE(serializeAndCheckDILContext(0, DIL_SOURCE_TYPE, "vQdpOESlok", 1, 0, 20, 8));

    // Update dynamicIndexListData with listVersion more than expected and check new context value (update fail case)
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 2));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(3, 102, 15, "15, 16")));
    ASSERT_FALSE(root->isDataSourceContextDirty());
    ASSERT_TRUE(serializeAndCheckDILContext(0, DIL_SOURCE_TYPE, "vQdpOESlok", 1, 0, 20, 8));

    // Update dynamicIndexListData with missing listVersion and check new context value
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 6, 2));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(2, 103, 6, "6, 7")));
    ASSERT_TRUE(root->isDataSourceContextDirty());
    ASSERT_TRUE(serializeAndCheckDILContext(0, DIL_SOURCE_TYPE, "vQdpOESlok", 3, 0, 20, 6));

    // Check for unprocessed errors.
    ASSERT_TRUE(ds->getPendingErrors().empty());
    // Check after serialization datasource context should not be dirty
    ASSERT_FALSE(root->isDataSourceContextDirty());
}

static const char *LIVE_ARRAY_DOC = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "text": "${TestArray[1]}"
    }
  }
})";

TEST_F(DatasourceContextTest, LiveArrayChangeTest)
{
    auto myArray = LiveArray::create(ObjectArray{"a", "b", "c"});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_ARRAY_DOC);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("b", component->getCalculated(kPropertyText).asString()));
    ASSERT_EQ(0, context->dataManager().dirty().size());

    // Update one item, by value and serialize Datasource context
    ASSERT_TRUE(myArray->update(1, "seven"));

    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyText));
    ASSERT_TRUE(IsEqual("seven", component->getCalculated(kPropertyText).asString()));

    ASSERT_FALSE(root->isDataSourceContextDirty());
    serializeDatasourceContext();
    ASSERT_EQ(0, datasourceContext.Size());
}