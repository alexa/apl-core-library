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

#ifndef _APL_DYNAMIC_INDEX_LIST_TEST_H
#define _APL_DYNAMIC_INDEX_LIST_TEST_H

#include "../testeventloop.h"

#include "apl/apl.h"
#include "apl/dynamicdata.h"

namespace apl {

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

    struct ExpectedPage {
        std::string id;
        bool isTransforming;
        ExpectedPage(std::string _id, bool _isTransforming = false) : id(_id), isTransforming(_isTransforming) {}
    };

    ::testing::AssertionResult
    CheckPager(int expectedCurrentPage, std::vector<ExpectedPage> expectedPages) {
        if (expectedPages.size() != component->getChildCount()) {
            return ::testing::AssertionFailure() << "Expected " << expectedPages.size() << " page(s), "
                                                 << "found " << component->getChildCount();
        }
        if (expectedCurrentPage != component->getCalculated(kPropertyCurrentPage).asNumber()) {
            return ::testing::AssertionFailure() << "Expected the current page to be "
                                                 << expectedCurrentPage << " but was "
                                                 << component->getCalculated(kPropertyCurrentPage).asNumber();
        }
        for (int i = 0; i < expectedPages.size(); i++) {
            auto child = component->getChildAt(i);
            auto expectedPage = expectedPages[i];
            if (expectedPage.id != child->getId()) {
                return ::testing::AssertionFailure() << "Expected page " << i
                                                     << " to have an id of " << expectedPage.id
                                                     << " but was " << child->getId();
            }
            if (expectedPage.isTransforming == CheckTransform(Transform2D(), child)) {
                return ::testing::AssertionFailure() << "Expected page " << i << " (id="
                                                     << expectedPage.id << ") to be"
                                                     << (expectedPage.isTransforming ? "" : " NOT")
                                                     << " transforming, but it was"
                                                     << (expectedPage.isTransforming ? " NOT" : "");
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
        auto errors = ds->getPendingErrors();
        for (const auto& error : errors.getArray()) {
            LOG(LogLevel::kError) << error;
        }
        ASSERT_TRUE(errors.empty());

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

} // namespace apl

#endif //_APL_DYNAMIC_INDEX_LIST_TEST_H
