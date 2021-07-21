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

#include "testdatasourceprovider.h"
#include "apl/datasource/datasource.h"

using namespace apl;

static const int TEST_CHUNK_SIZE = 5;

static const bool TEST_DATA_SOURCE_VERBOSE = false;

TestDataSourceConnection::TestDataSourceConnection(
        std::weak_ptr<LiveArray> liveArray,
        int offset,
        int maxItems,
        const std::vector<std::string>& data) :
        OffsetIndexDataSourceConnection(std::move(liveArray), offset, maxItems, TEST_CHUNK_SIZE) {
    LOG_IF(TEST_DATA_SOURCE_VERBOSE) << "Base data";
    for (auto& p : data) {
        rapidjson::Document doc;
        doc.Parse(p.c_str());
        mData.emplace_back(std::move(doc));
        LOG_IF(TEST_DATA_SOURCE_VERBOSE) << p;
    }
    LOG_IF(TEST_DATA_SOURCE_VERBOSE) << "\n";
}

void
TestDataSourceConnection::fetch(size_t index, size_t count) {
    std::pair<size_t, size_t> request = {index, count};
    for (auto entry : mRequests) {
        if (entry.second == request) {
            // Do not process repeat requests.
            return;
        }
    }
    mRequests.emplace(mRequestToken++, request);
}

std::shared_ptr<DataSourceConnection>
TestDataSourceProvider::create(const Object& sourceDefinition, std::weak_ptr<Context>,
        std::weak_ptr<LiveArray> liveArray) {
    auto sourceMap = sourceDefinition.getMap();
    int offset = sourceMap.at("offset").getInteger();
    size_t maxItems = sourceMap.at("maxItems").getInteger();

    auto connection = std::make_shared<TestDataSourceConnection>(liveArray, offset, maxItems, *mData);
    mConnection = connection;
    return connection;
}

bool
TestDataSourceConnection::processResponseInternal(size_t index, size_t count) {
    std::vector<Object> result;
    result.reserve(count);
    for (size_t c = 0; c < count; c++) {
        result.emplace_back(mData.at(index + c));
    }

    return update(index, result);
}

bool
TestDataSourceConnection::processResponse(int requestToken, size_t index, size_t count) {
    if (requestToken < 0) {
        // Just fulfill all responses
        for (auto entry : mRequests) {
            processResponseInternal(entry.second.first, entry.second.second);
        }
        mRequests.clear();
        return true;
    }

    // Process response or just do "source initiated update".
    if (mRequests.find(requestToken) != mRequests.end()) mRequests.erase(requestToken);
    return processResponseInternal(index, count);
}

bool
TestDataSourceConnection::replace(size_t index, const std::vector<std::string>& items) {
    std::vector<Object> result;
    result.reserve(items.size());
    LOG_IF(TEST_DATA_SOURCE_VERBOSE) << "Replace on index:" << index;
    for (auto& item : items) {
        rapidjson::Document doc;
        doc.Parse(item.c_str());
        result.emplace_back(std::move(doc));
        LOG_IF(TEST_DATA_SOURCE_VERBOSE) << item;
    }
    LOG_IF(TEST_DATA_SOURCE_VERBOSE) << "\n";

    return update(index, result, true);
}

bool
TestDataSourceConnection::insert(size_t index, const std::string& item) {
    LOG_IF(TEST_DATA_SOURCE_VERBOSE) << "Insert on index:" << index << " : " << item;

    rapidjson::Document doc;
    doc.Parse(item.c_str());

    return OffsetIndexDataSourceConnection::insert(index, std::move(doc));
}