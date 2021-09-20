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

static const char* SOURCE_TYPE = "dynamicTokenList";
static const char* LIST_ID = "listId";
static const char* CORRELATION_TOKEN = "correlationToken";
static const char* PAGE_TOKEN = "pageToken";

class DynamicTokenListTest : public DocumentWrapper {
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
        if (SOURCE_TYPE != incomingType)
            return ::testing::AssertionFailure()
                   << "DataSource type is wrong. Expected: " << SOURCE_TYPE
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

    DynamicTokenListTest() : DocumentWrapper() {
        auto cnf = DynamicListConfiguration(SOURCE_TYPE)
                       .setFetchTimeout(100);
        ds = std::make_shared<DynamicTokenListDataSourceProvider>(cnf);
        config->dataSourceProvider("dynamicTokenList", ds);
    }

    void TearDown() override {
        // Check for unprocessed errors.
        ASSERT_TRUE(ds->getPendingErrors().empty());

        // Clean any pending timeouts. Tests will check them explicitly.
        if (root) {
            loop->advanceToEnd();
            while (root->hasEvent()) {
                root->popEvent();
            }
        }

        DocumentWrapper::TearDown();
    }

    std::shared_ptr<DynamicTokenListDataSourceProvider> ds;
};

TEST_F(DynamicTokenListTest, Configuration) {
    auto expectedConfiguration = DynamicListConfiguration("")
                                     .setType("magic")
                                     .setCacheChunkSize(42)
                                     .setFetchRetries(3)
                                     .setFetchTimeout(2000);
    auto source = std::make_shared<DynamicTokenListDataSourceProvider>(expectedConfiguration);
    auto actualConfiguration = source->getConfiguration();
    ASSERT_EQ(expectedConfiguration.type, actualConfiguration.type);
    ASSERT_EQ(expectedConfiguration.cacheChunkSize, actualConfiguration.cacheChunkSize);
    ASSERT_EQ(expectedConfiguration.fetchRetries, actualConfiguration.fetchRetries);
    ASSERT_EQ(expectedConfiguration.fetchTimeout, actualConfiguration.fetchTimeout);

    // Default
    source = std::make_shared<DynamicTokenListDataSourceProvider>();
    actualConfiguration = source->getConfiguration();
    ASSERT_EQ(SOURCE_TYPE, actualConfiguration.type);
    ASSERT_EQ(10, actualConfiguration.cacheChunkSize);
    ASSERT_EQ(2, actualConfiguration.fetchRetries);
    ASSERT_EQ(5000, actualConfiguration.fetchTimeout);
}

static const char* DATA = R"({
  "dynamicSource": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok",
    "pageToken": "pageToken",
    "backwardPageToken": "backwardPageToken",
    "forwardPageToken": "forwardPageToken",
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char* BASIC = R"({
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

TEST_F(DynamicTokenListTest, Basic) {
    loadDocument(BASIC, DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_EQ("id10", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(4)->getId());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken"));

    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(101, "forwardPageToken", "forwardPageToken1",
                       "15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30")));
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(102, "backwardPageToken", "backwardPageToken1", "5, 6, 7, 8, 9")));
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 11), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(12, 14), false));

    ASSERT_EQ(26, component->getChildCount());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id30", component->getChildAt(25)->getId());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "backwardPageToken1"));
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(103, "backwardPageToken1", "backwardPageToken2", "-6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ("id-6", component->getChildAt(0)->getId());
    ASSERT_EQ("id30", component->getChildAt(36)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 11), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(12, 22), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(23, 25), false));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicTokenListTest, BasicAsMap) {
    loadDocument(BASIC, DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken"));

    ASSERT_TRUE(ds->processUpdate(
            StringToMapObject(createLazyLoad(101, "forwardPageToken", "forwardPageToken1",
                           "15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30"))));
    ASSERT_TRUE(ds->processUpdate(
            StringToMapObject(createLazyLoad(102, "backwardPageToken", "backwardPageToken1", "5, 6, 7, 8, 9"))));
    root->clearPending();

    ASSERT_EQ(26, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "backwardPageToken1"));
    ASSERT_TRUE(ds->processUpdate(
            StringToMapObject(createLazyLoad(103, "backwardPageToken1", "backwardPageToken2", "-6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4"))));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicTokenListTest, NoNextToken) {
    loadDocument(BASIC, DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_EQ("id10", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(4)->getId());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken"));

    ASSERT_TRUE(
        ds->processUpdate(createLazyLoad(101, "forwardPageToken", "", "15, 16, 17, 18, 19")));
    ASSERT_TRUE(
        ds->processUpdate(createLazyLoad(102, "backwardPageToken", "", "5, 6, 7, 8, 9")));
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 11), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(12, 14), false));

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id19", component->getChildAt(14)->getId());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char* EMPTY = R"({
  "dynamicSource": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok",
    "pageToken": "pageToken",
    "backwardPageToken": "backwardPageToken",
    "forwardPageToken": "forwardPageToken"
  }
})";

TEST_F(DynamicTokenListTest, Empty) {
    loadDocument(BASIC, EMPTY);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(
        ds->processUpdate(createLazyLoad(101, "forwardPageToken", "", "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 4), true));

    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id4", component->getChildAt(4)->getId());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken"));
    ASSERT_TRUE(
        ds->processUpdate(createLazyLoad(102, "backwardPageToken", "", "-5, -4, -3, -2, -1")));

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

static const char* FIRST_AND_LAST = R"({
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

static const char* FIRST_AND_LAST_DATA = R"({
  "dynamicSource": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok",
    "pageToken": "pageToken",
    "backwardPageToken": "backwardPageToken",
    "forwardPageToken": "forwardPageToken",
    "items": [ 10 ]
  }
})";

TEST_F(DynamicTokenListTest, WithFirstAndLast) {
    loadDocument(FIRST_AND_LAST, FIRST_AND_LAST_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 2}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken"));

    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(101, "forwardPageToken", "forwardPageToken1", "11, 12, 13, 14, 15")));
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(102, "backwardPageToken", "backwardPageToken1", "5, 6, 7, 8, 9")));
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

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "forwardPageToken1"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", "backwardPageToken1"));

    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(103, "forwardPageToken1", "forwardPageToken2", "16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26")));
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(104, "backwardPageToken1", "backwardPageToken2", "-6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(1700, component->getCalculated(kPropertyScrollPosition).asNumber());

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id-6", component->getChildAt(1)->getId());
    ASSERT_EQ("id26", component->getChildAt(33)->getId());
    ASSERT_EQ("li", component->getChildAt(34)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 27), true));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char* FIRST = R"({
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

TEST_F(DynamicTokenListTest, WithFirst) {
    loadDocument(FIRST, FIRST_AND_LAST_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken"));

    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(101, "forwardPageToken", "", "11, 12, 13, 14, 15")));
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(102, "backwardPageToken", "backwardPageToken1", "5, 6, 7, 8, 9")));
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

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "backwardPageToken1"));

    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(103, "backwardPageToken1", "", "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id0", component->getChildAt(1)->getId());
    ASSERT_EQ("id15", component->getChildAt(16)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 16), true));

    ASSERT_FALSE(root->hasEvent());
}

static const char* LAST = R"({
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

TEST_F(DynamicTokenListTest, WithLast) {
    loadDocument(LAST, FIRST_AND_LAST_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken"));

    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(101, "forwardPageToken", "forwardPageToken1", "11, 12, 13, 14, 15")));
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(102, "backwardPageToken", "backwardPageToken1", "5, 6, 7, 8, 9")));
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

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "forwardPageToken1"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", "backwardPageToken1"));

    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(103, "forwardPageToken1", "", "16, 17, 18, 19")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id15", component->getChildAt(10)->getId());
    ASSERT_EQ("li", component->getChildAt(15)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 15), true));

    ASSERT_FALSE(root->hasEvent());
}

static const char* LAST_DATA = R"({
  "dynamicSource": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok",
    "pageToken": "pageToken",
    "forwardPageToken": "forwardPageToken",
    "items": [ 0 ]
  }
})";

TEST_F(DynamicTokenListTest, WithLastOneWay) {
    loadDocument(LAST, LAST_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(101, "forwardPageToken", "forwardPageToken1", "1, 2, 3, 4, 5")));
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 6), true));

    ASSERT_EQ(7, component->getChildCount());

    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(5)->getId());
    ASSERT_EQ("li", component->getChildAt(6)->getId());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "forwardPageToken1"));
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(102, "forwardPageToken1", "forwardPageToken2", "6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 11), true));
    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(5)->getId());
    ASSERT_EQ("id10", component->getChildAt(10)->getId());
    ASSERT_EQ("id16", component->getChildAt(16)->getId());
    ASSERT_EQ("li", component->getChildAt(17)->getId());

    ASSERT_FALSE(root->hasEvent());

    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).asNumber());
    component->update(kUpdateScrollPosition, 600);
    root->clearPending();

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "forwardPageToken2"));
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(103, "forwardPageToken2", "forwardPageToken3", "17, 18, 19")));
    root->clearPending();
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", "forwardPageToken3"));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(5)->getId());
    ASSERT_EQ("id10", component->getChildAt(10)->getId());
    ASSERT_EQ("id15", component->getChildAt(15)->getId());
    ASSERT_EQ("id19", component->getChildAt(19)->getId());
    ASSERT_EQ("li", component->getChildAt(20)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 20), true));

    ASSERT_FALSE(root->hasEvent());
}

static const char* EMPTY_DATA = R"({
  "dynamicSource": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok",
    "pageToken": "pageToken",
    "backwardPageToken": "backwardPageToken",
    "forwardPageToken": "forwardPageToken",
    "items": []
  }
})";

TEST_F(DynamicTokenListTest, EmptySequence) {
    loadDocument(BASIC, EMPTY_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(101, "forwardPageToken", "forwardPageToken1", "10, 11, 12, 13, 14")));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "forwardPageToken1"));
}

static const char* MULTI = R"({
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

static const char* MULTI_DATA = R"({
  "dynamicSource1": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok1",
    "pageToken": "pageToken",
    "forwardPageToken": "forwardPageToken",
    "items": [ 10, 11, 12, 13, 14 ]
  },
  "dynamicSource2": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok2",
    "pageToken": "pageToken",
    "forwardPageToken": "forwardPageToken",
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

TEST_F(DynamicTokenListTest, Multi) {
    loadDocument(MULTI, MULTI_DATA);

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok1", "101", "forwardPageToken"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok2", "102", "forwardPageToken"));
}

static const char *WRONG_MISSING_FIELDS_DATA = R"({
  "dynamicSource": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok",
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

TEST_F(DynamicTokenListTest, MissingFields) {
    loadDocument(BASIC, WRONG_MISSING_FIELDS_DATA);
    ASSERT_TRUE(session->checkAndClear());
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_EQ(component->getChildCount(), 1);
}

static const char *MULTI_CLONED_DATA = R"({
  "dynamicSource1": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok1",
    "pageToken": "pageToken",
    "forwardPageToken": "forwardPageToken",
    "items": [ 10, 11, 12, 13, 14 ]
  },
  "dynamicSource2": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok1",
    "pageToken": "pageToken",
    "forwardPageToken": "forwardPageToken",
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

TEST_F(DynamicTokenListTest, MultiClonedData) {
    loadDocument(MULTI, MULTI_CLONED_DATA);
    ASSERT_TRUE(session->checkAndClear());
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_EQ(component->getChildCount(), 2);
}

TEST_F(DynamicTokenListTest, processInvalidPayload) {
    loadDocument(BASIC, DATA);
    ASSERT_FALSE(ds->processUpdate(Object::NULL_OBJECT()));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_EQ(component->getChildCount(), 5);
}

static const char* BASIC_CONTAINER = R"({
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

TEST_F(DynamicTokenListTest, Container) {
    loadDocument(BASIC_CONTAINER, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(
        ds->processUpdate(createLazyLoad(-1, "backwardPageToken", "", "0, 1, 2, 3, 4, 5, 6, 7, 8, 9")));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(14)->getId());

    root->clearDirty();

    ASSERT_FALSE(root->isDirty());

    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(14)->getId());
}

static const char* BASIC_PAGER = R"({
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

static const char* BASIC_PAGER_DATA = R"({
  "dynamicSource": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok",
    "pageToken": "pageToken",
    "backwardPageToken": "backwardPageToken",
    "forwardPageToken": "forwardPageToken",
    "items": [
      { "color": "blue", "text": "10" },
      { "color": "red", "text": "11" },
      { "color": "green", "text": "12" },
      { "color": "yellow", "text": "13" },
      { "color": "white", "text": "14" },
      { "color": "blue", "text": "15" },
      { "color": "red", "text": "16" },
      { "color": "green", "text": "17" },
      { "color": "yellow", "text": "18" },
      { "color": "white", "text": "19" },
      { "color": "blue", "text": "20" }
    ]
  }
})";

static const char* FIVE_TO_NINE_FOLLOWUP_PAGER = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "pageToken": "backwardPageToken",
  "nextPageToken": "backwardPageToken1",
  "items": [
    { "color": "blue", "text": "5" },
    { "color": "red", "text": "6" },
    { "color": "green", "text": "7" },
    { "color": "yellow", "text": "8" },
    { "color": "white", "text": "9" }
  ]
})";

static const char* ZERO_TO_FOUR_RESPONSE_PAGER = R"({
  "token": "presentationToken",
  "correlationToken": "102",
  "listId": "vQdpOESlok",
  "pageToken": "backwardPageToken1",
  "items": [
    { "color": "blue", "text": "0" },
    { "color": "red", "text": "1" },
    { "color": "green", "text": "2" },
    { "color": "yellow", "text": "3" },
    { "color": "white", "text": "4" }
  ]
})";

static const char* TWENTY_ONE_TO_TWENTY_FIVE_RESPONSE_PAGER = R"({
  "token": "presentationToken",
  "correlationToken": "103",
  "listId": "vQdpOESlok",
  "pageToken": "forwardPageToken",
  "nextPageToken": "forwardPageToken1",
  "items": [
    { "color": "blue", "text": "21" },
    { "color": "red", "text": "22" },
    { "color": "green", "text": "23" },
    { "color": "yellow", "text": "24" },
    { "color": "white", "text": "25" }
  ]
})";

TEST_F(DynamicTokenListTest, BasicPager) {
    loadDocument(BASIC_PAGER, BASIC_PAGER_DATA);
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(kComponentTypePager, component->getType());

    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 10}, false));

    // Load 5 pages BEFORE the current set of pages
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "backwardPageToken"));
    ASSERT_TRUE(ds->processUpdate(FIVE_TO_NINE_FOLLOWUP_PAGER));
    root->clearPending();
    ASSERT_EQ(16, component->getChildCount());
    ASSERT_EQ("frame-5", component->getChildAt(0)->getId());
    ASSERT_EQ("frame-20", component->getChildAt(15)->getId());
    ASSERT_TRUE(CheckChildLaidOutDirtyFlagsWithNotify(component, 4));  // Page 4 gets loaded because we're on page 5
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0,3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4,6}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7,15}, false));

    // Switch to the first page (index=0)
    component->update(UpdateType::kUpdatePagerByEvent, 0);
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlagsWithNotify(component, {0, 1}));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 6}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7, 15}, false));

    // Load 5 more pages BEFORE the current set of pages
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken1"));
    ASSERT_TRUE(ds->processUpdate(ZERO_TO_FOUR_RESPONSE_PAGER));
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 6}, true));  // Page 4 gets loaded because we're on page 5
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7, 8}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {9, 11}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {12, 20}, false));

    // Switch to the last page (index=20)
    component->update(UpdateType::kUpdatePagerByEvent, 20);
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlagsWithNotify(component, {19, 20}));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 6}, true));  // Page 4 gets loaded because we're on page 5
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7, 8}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {9, 11}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {12, 18}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {19, 20}, true));

    // Load 5 more pages AFTER the current set of pages
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "forwardPageToken"));
    ASSERT_TRUE(ds->processUpdate(TWENTY_ONE_TO_TWENTY_FIVE_RESPONSE_PAGER));
    root->clearPending();
    ASSERT_TRUE(CheckChildLaidOutDirtyFlagsWithNotify(component, 21));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 6}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7, 8}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {9, 11}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {12, 18}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {19, 21}, true));  // Page 15 gets loaded because we're on page 14
    ASSERT_TRUE(CheckChildrenLaidOut(component, {22, 25}, false));

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(component));
    ASSERT_EQ(1, component->getDirty().count(kPropertyNotifyChildrenChanged));

    ASSERT_EQ("frame-0", component->getChildAt(0)->getId());
    ASSERT_EQ("frame-25", component->getChildAt(25)->getId());
}

static const char* EMPTY_PAGER_DATA = R"({
  "dynamicSource": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok",
    "pageToken": "pageToken",
    "backwardPageToken": "backwardPageToken",
    "forwardPageToken": "forwardPageToken",
    "items": []
  }
})";

static const char* TEN_TO_TWENTY_RESPONSE_PAGER = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "pageToken": "forwardPageToken",
  "nextPageToken": "forwardPageToken1",
  "items": [
    { "color": "blue", "text": "10" },
    { "color": "red", "text": "11" },
    { "color": "green", "text": "12" },
    { "color": "yellow", "text": "13" },
    { "color": "white", "text": "14" },
    { "color": "blue", "text": "15" },
    { "color": "red", "text": "16" },
    { "color": "green", "text": "17" },
    { "color": "yellow", "text": "18" },
    { "color": "white", "text": "19" },
    { "color": "blue", "text": "20" }
  ]
})";

TEST_F(DynamicTokenListTest, EmptyPager) {
    loadDocument(BASIC_PAGER, EMPTY_PAGER_DATA);

    ASSERT_EQ(kComponentTypePager, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(ds->processUpdate(TEN_TO_TWENTY_RESPONSE_PAGER));
    root->clearPending();

    ASSERT_EQ(11, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 4}, false));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken"));
}

static const char* SMALLER_DATA = R"({
  "dynamicSource": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok",
    "pageToken": "pageToken",
    "forwardPageToken": "forwardPageToken",
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char* SMALLER_DATA_BACK = R"({
  "dynamicSource": {
    "type": "dynamicTokenList",
    "listId": "vQdpOESlok",
    "pageToken": "pageToken",
    "backwardPageToken": "backwardPageToken",
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

TEST_F(DynamicTokenListTest, GarbageCollection) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(
        ds->processUpdate(createLazyLoad(101, "forwardPageToken", "", "15, 16, 17, 18, 19")));
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

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardPageToken"));
    ASSERT_TRUE(
        ds->processUpdate(createLazyLoad(102, "backwardPageToken", "", "5, 6, 7, 8, 9")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIFTEEN_TO_NINETEEN_WRONG_LIST_AND_TOKEN_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "76",
  "listId": "vQdpOESlok1",
  "pageToken": "forwardPageToken",
  "items": [ 15, 16, 17, 18, 19 ]
})";

static const char *FIFTEEN_TO_NINETEEN_WRONG_LIST_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok1",
  "pageToken": "forwardPageToken",
  "items": [ 15, 16, 17, 18, 19 ]
})";

TEST_F(DynamicTokenListTest, CorrelationTokenSubstitute) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_FALSE(ds->processUpdate(FIFTEEN_TO_NINETEEN_WRONG_LIST_AND_TOKEN_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INVALID_LIST_ID" }));

    ASSERT_TRUE(ds->processUpdate(FIFTEEN_TO_NINETEEN_WRONG_LIST_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_LIST_ID" }));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIFTEEN_EMPTY_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "pageToken": "pageToken",
  "items": []
})";

TEST_F(DynamicTokenListTest, EmptyLazyResponseRetryFail) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(101, "forwardPageToken", "", "")));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "forwardPageToken"));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(102, "forwardPageToken", "", "")));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "forwardPageToken"));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(103, "forwardPageToken", "", "")));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicTokenListTest, EmptyLazyResponseRetryResolved) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_FALSE(ds->processUpdate(FIFTEEN_EMPTY_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "forwardPageToken"));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(101, "forwardPageToken", "", "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicTokenListTest, LazyResponseTimeout) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    // Not yet
    advanceTime(50);
    ASSERT_TRUE(CheckErrors({}));

    // Should go from here
    advanceTime(40);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "forwardPageToken"));
    advanceTime(100);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "forwardPageToken"));
    advanceTime(100);
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicTokenListTest, LazyResponseTimeoutResolvedAfterLost) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    // Not yet
    advanceTime(50);
    ASSERT_TRUE(CheckErrors({}));

    // Should go from here
    advanceTime(40);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "forwardPageToken"));

    // Retry response arrives
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(102, "forwardPageToken", "", "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicTokenListTest, LazyResponseTimeoutResolvedAfterDelayed) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    // Not yet
    advanceTime(50);
    ASSERT_TRUE(CheckErrors({}));

    // Should go from here
    advanceTime(40);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "forwardPageToken"));

    // Original response arrives
    ASSERT_TRUE(ds->processUpdate(
        createLazyLoad(101, "forwardPageToken", "", "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // Retry arrives
    ASSERT_FALSE(ds->processUpdate(
        createLazyLoad(102, "forwardPageToken", "", "15, 16, 17, 18, 19")));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *BASIC_CONFIG_CHANGE = R"({
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
  },
  "onConfigChange": [
    {
      "type": "Reinflate"
    }
  ]
})";

TEST_F(DynamicTokenListTest, Reinflate) {
    loadDocument(BASIC_CONFIG_CHANGE, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken"));
    ASSERT_TRUE(
        ds->processUpdate(createLazyLoad(101, "forwardPageToken", "", "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // re-inflate should get same result.
    configChangeReinflate(ConfigurationChange(100, 100));
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(10, component->getChildCount());
}

static const char * BIT_BY_A_BIT_DEEP = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "parameters": ["dynamicSource"],
    "items": [
      {
        "onMount": [
          {
            "type": "Sequential",
            "commands": [
              {"componentId": "dynamicSequence", "minimumDwellTime": "200", "type": "SpeakItem"},
              {"delay": 500, "type": "Idle"},
              {"type": "ScrollToIndex", "componentId": "dynamicSequence", "index": 0, "align": "center"}
            ]
          }
        ],
        "type": "Container",
        "width": "100%",
        "height": "100%",
        "id": "root",
        "direction": "row",
        "items": [
          {
            "type": "Container",
            "grow": 1,
            "item": [
              {
                "type": "Pager",
                "id": "viewPager",
                "navigation": "none",
                "width": "100%",
                "grow": 1,
                "item": [
                  {
                    "type": "Sequence",
                    "id": "dynamicSequence",
                    "speech": "https://example.com/test.mp3",
                    "navigation": "none",
                    "scrollDirection": "vertical",
                    "numbered": true,
                    "data": "${dynamicSource}",
                    "item": [
                      {
                        "type": "Container",
                        "id": "container${data}",
                        "height": 150,
                        "width": "100%",
                        "data": "${data}",
                        "items": [
                          {
                            "type": "Container",
                            "paddingTop": "50dp",
                            "paddingBottom": "50dp",
                            "item": [{"type": "Text", "text": "${data}"}]
                          }
                        ]
                      }
                    ]
                  }
                ]
              }
            ]
          }
        ]
      }
    ]
  }
})";

static const char* BIT_BY_A_BIT_DATA = R"({
  "dynamicSource": {
    "listId": "vQdpOESlok",
    "pageToken": "currentPageToken",
    "backwardPageToken": "backwardsPageToken1",
    "type": "testList",
    "forwardPageToken": "forwardPageToken1",
    "items": [100]
  }
})";

TEST_F(DynamicTokenListTest, DeepProgressive) {
    // Set different source, just to avoid config overrides
    auto source = std::make_shared<DynamicTokenListDataSourceProvider>();
    metrics.size(750, 750);
    config->dataSourceProvider("testList", source);

    loadDocument(BIT_BY_A_BIT_DEEP, BIT_BY_A_BIT_DATA);
    auto sequence = std::static_pointer_cast<CoreComponent>(root->findComponentById("dynamicSequence"));
    ASSERT_TRUE(sequence);
    ASSERT_EQ(1, sequence->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken1"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardsPageToken1"));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypePreroll, event.getType());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSpeak, event.getType());

    auto checker = [sequence](int sIdx, int eIdx) {
        int shift = 0;
        for (int i = sIdx; i <= eIdx; i++) {
            auto c = sequence->getCoreChildAt(i);
            if (!c->getCalculated(kPropertyLaidOut).getBoolean()) return false;
            if (Object(Rect(0,shift*150,750,150)) !=c->getCalculated(kPropertyBounds)) return false;
            auto cc = c->getCoreChildAt(0);
            if (!cc->getCalculated(kPropertyLaidOut).getBoolean()) return false;
            if (Object(Rect(0,0,750,110)) != cc->getCalculated(kPropertyBounds)) return false;
            auto ccc = cc->getCoreChildAt(0);
            if (!ccc->getCalculated(kPropertyLaidOut).getBoolean()) return false;
            if (Object(Rect(0,50,750,10)) !=ccc->getCalculated(kPropertyBounds)) return false;
            shift++;
        }
        return true;
    };

    advanceTime(600);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(101, "forwardPageToken1", "forwardPageToken2", "101, 102, 103")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "forwardPageToken2"));
    advanceTime(50);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(102, "backwardsPageToken1", "backwardsPageToken2", "97, 98, 99")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", "backwardsPageToken2"));

    ASSERT_EQ(7, sequence->getChildCount());
    ASSERT_EQ(Point(0, 450), sequence->scrollPosition());
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 6}, true));
    ASSERT_TRUE(checker(0, 6));

    advanceTime(600);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(103, "forwardPageToken2", "forwardPageToken3", "104, 105, 106")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "105", "forwardPageToken3"));
    advanceTime(50);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(104, "backwardsPageToken2", "backwardsPageToken3", "94, 95, 96")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "106", "backwardsPageToken3"));

    ASSERT_EQ(13, sequence->getChildCount());
    ASSERT_EQ(Point(0, 900), sequence->scrollPosition());
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 12}, true));
    ASSERT_TRUE(checker(0, 12));

    advanceTime(600);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(105, "forwardPageToken3", "forwardPageToken4", "107, 108, 109")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "107", "forwardPageToken4"));
    advanceTime(50);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(106, "backwardsPageToken3", "backwardsPageToken4", "91, 92, 93")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "108", "backwardsPageToken4"));

    ASSERT_EQ(19, sequence->getChildCount());
    ASSERT_EQ(Point(0, 900), sequence->scrollPosition());

    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 2}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {3, 18}, true));
    ASSERT_TRUE(checker(3, 18));

    advanceTime(600);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(107, "forwardPageToken4", "forwardPageToken5", "110, 111, 112")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "109", "forwardPageToken5"));
    advanceTime(50);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(108, "backwardsPageToken4", "backwardsPageToken5", "88, 89, 90")));
    advanceTime(26);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "110", "backwardsPageToken5"));

    ASSERT_EQ(25, sequence->getChildCount());
    ASSERT_EQ(Point(0, 900), sequence->scrollPosition());
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 5}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {6, 22}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {23, 24}, false));
    ASSERT_TRUE(checker(6, 22));

    event.getActionRef().resolve();
    ASSERT_EQ(Point(0, 900), sequence->scrollPosition());

    advanceTime(500);

    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 22}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {23, 24}, false));

    ASSERT_EQ(Point(0, 1800), sequence->scrollPosition());
    ASSERT_TRUE(checker(0, 22));

    advanceTime(1000);

    ASSERT_EQ(Point(0, 0), sequence->scrollPosition()); // Current 0 index effectively
    ASSERT_TRUE(source->processUpdate(createLazyLoad(109, "forwardPageToken5", "", "113, 114, 115")));
    ASSERT_TRUE(source->processUpdate(createLazyLoad(110, "backwardsPageToken5", "", "85, 86, 87")));

    advanceTime(16);
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 25}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {26, 28}, false));
    ASSERT_TRUE(checker(0, 25));
}

static const char *DOUBLE_PAGER_GALORE = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "parameters": ["dynamicSource"],
    "bind": [
      {
        "name": "CurrentItem",
        "value": "${dynamicSource[0]}"
      }
    ],
    "items": [
      {
        "type": "Container",
        "width": "100%",
        "height": "100%",
        "id": "document",
        "direction": "column",
        "items": [
          {
            "type": "Container",
            "width": "100%",
            "justifyContent": "center",
            "grow": 1,
            "items": [
              {
                "type": "Pager",
                "id": "topPager",
                "data": "${dynamicSource}",
                "navigation": "none",
                "grow": 1,
                "items": [
                  {
                    "type": "Container",
                    "id": "TopId_${data.id}",
                    "data": "${data.topItems}",
                    "width": "100%",
                    "paddingLeft": "1vw",
                    "direction": "row",
                    "items": [
                      {
                        "type": "Text",
                        "text": "Page${data}"
                      }
                    ]
                  }
                ]
              },
              {
                "type": "Pager",
                "id": "bottomPager",
                "height": "100%",
                "width": "100%",
                "grow": 1,
                "navigation": "normal",
                "data": "${dynamicSource}",
                "onPageChanged": [
                  {
                    "type": "SetValue",
                    "property": "CurrentItem",
                    "value": "${dynamicSource[event.source.page]}"
                  },
                  {
                    "type": "Sequential",
                    "sequencer": "LoadDayColumnSequencer",
                    "commands": [
                      {
                        "type": "ScrollToComponent",
                        "componentId": "TopId_${CurrentItem.id}"
                      }
                    ]
                  }
                ],
                "items": [
                  {
                    "type": "Text",
                    "height": "100dp",
                    "text": "${data.id}"
                  }
                ]
              }
            ]
          }
        ]
      }
    ]
  }
})";

static const char *DOUBLE_PAGER_GALORE_DATA = R"({
  "dynamicSource": {
    "listId": "vQdpOESlok",
    "pageToken": "currentPageToken",
    "backwardPageToken": "tokenBack",
    "forwardPageToken": "tokenForward",
    "type": "testList",
    "items": [
      {
        "id": "2021_08_04",
        "topItems": [1]
      }
    ]
  }
})";

static const char *PAGE_FORWARD_UPDATE = R"({
  "listId": "vQdpOESlok",
  "pageToken": "tokenForward",
  "correlationToken": "101",
  "type": "testList",
  "items": [
    {
      "id": "2021_08_05",
      "topItems": [2]
    }
  ]
})";

static const char *PAGE_BACKWARD_UPDATE = R"({
  "listId": "vQdpOESlok",
  "pageToken": "tokenBack",
  "correlationToken": "102",
  "type": "testList",
  "items": [
    {
      "id": "2021_08_03",
      "topItems": [0]
    }
  ]
})";

TEST_F(DynamicTokenListTest, DoublePager) {
    // Set different source, just to avoid config overrides
    auto source = std::make_shared<DynamicTokenListDataSourceProvider>();
    metrics.size(750, 750);
    config->set(RootProperty::kPagerChildCache, 0);
    config->dataSourceProvider("testList", source);

    loadDocument(DOUBLE_PAGER_GALORE, DOUBLE_PAGER_GALORE_DATA);
    auto topPager = std::static_pointer_cast<CoreComponent>(root->findComponentById("topPager"));
    ASSERT_TRUE(topPager);
    ASSERT_EQ(1, topPager->getChildCount());
    ASSERT_EQ(Object(Rect(0,-50,100,100)), topPager->getCalculated(kPropertyBounds));

    auto bottomPager = std::static_pointer_cast<CoreComponent>(root->findComponentById("bottomPager"));
    ASSERT_TRUE(bottomPager);
    ASSERT_EQ(1, bottomPager->getChildCount());
    ASSERT_EQ(Object(Rect(0,50,750,850)), bottomPager->getCalculated(kPropertyBounds));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "tokenForward"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "tokenBack"));

    auto checker = [topPager, bottomPager](const std::string& topText, const std::string& bottomText) {
        auto c = topPager->getCoreChildAt(topPager->pagePosition());
        if (kComponentTypeContainer != c->getType()) return false;
        if (!c->getCalculated(kPropertyLaidOut).getBoolean()) return false;
        if (Object(Rect(0,0,100,100)) != c->getCalculated(kPropertyBounds)) return false;
        auto cc = c->getCoreChildAt(0);
        if (kComponentTypeText != cc->getType()) return false;
        if (!cc->getCalculated(kPropertyLaidOut).getBoolean()) return false;
        if (Object(Rect(7,0,50,100)) != cc->getCalculated(kPropertyBounds)) return false;
        if (topText != cc->getCalculated(kPropertyText).asString()) return false;

        c = bottomPager->getCoreChildAt(bottomPager->pagePosition());
        if (kComponentTypeText != c->getType()) return false;
        if (!c->getCalculated(kPropertyLaidOut).getBoolean()) return false;
        if (Object(Rect(0,0,750,850)) != c->getCalculated(kPropertyBounds)) return false;
        if (bottomText != c->getCalculated(kPropertyText).asString()) return false;
        return true;
    };

    advanceTime(600);
    ASSERT_TRUE(source->processUpdate(PAGE_FORWARD_UPDATE));
    advanceTime(50);
    ASSERT_TRUE(source->processUpdate(PAGE_BACKWARD_UPDATE));
    advanceTime(50);

    ASSERT_EQ(1, bottomPager->pagePosition());
    ASSERT_EQ(1, topPager->pagePosition());

    ASSERT_EQ(3, topPager->getChildCount());
    ASSERT_EQ(3, bottomPager->getChildCount());

    ASSERT_TRUE(checker("Page1", "2021_08_04"));

    auto c = topPager->getCoreChildAt(2);
    ASSERT_FALSE(c->getCalculated(kPropertyLaidOut).getBoolean());
    ASSERT_EQ(0, c->getChildCount());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,300)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(100,300)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(100,300)));
    advanceTime(600);
    ASSERT_EQ(2, bottomPager->pagePosition());
    advanceTime(600);
    ASSERT_EQ(2, topPager->pagePosition());

    ASSERT_TRUE(c->getCalculated(kPropertyNotifyChildrenChanged).size() > 0);
    root->clearDirty();

    ASSERT_TRUE(checker("Page2", "2021_08_05"));

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,300)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,300)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,300)));
    advanceTime(600);
    ASSERT_EQ(1, bottomPager->pagePosition());
    advanceTime(600);
    ASSERT_EQ(1, topPager->pagePosition());

    c = topPager->getCoreChildAt(0);
    ASSERT_FALSE(c->getCalculated(kPropertyLaidOut).getBoolean());
    ASSERT_EQ(0, c->getChildCount());

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(100,300)));
    advanceTime(100);
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(400,300)));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,300)));
    advanceTime(600);
    ASSERT_EQ(0, bottomPager->pagePosition());
    advanceTime(600);
    ASSERT_EQ(0, topPager->pagePosition());

    ASSERT_TRUE(c->getCalculated(kPropertyNotifyChildrenChanged).size() > 0);
    root->clearDirty();

    ASSERT_TRUE(checker("Page0", "2021_08_03"));
}