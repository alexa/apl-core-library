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

#ifndef _APL_TEST_DATA_SOURCE_H
#define _APL_TEST_DATA_SOURCE_H

#include "apl/apl.h"
#include "apl/datasource/offsetindexdatasourceconnection.h"

namespace apl {

class TestDataSourceConnection : public OffsetIndexDataSourceConnection {
public:
    TestDataSourceConnection(
            std::weak_ptr<LiveArray> liveArray,
            int offset,
            int maxItems,
            const std::vector<std::string>& data);

    std::map<unsigned int, std::pair<size_t, size_t>> requests() { return mRequests; }
    bool processResponse(int requestToken = -1, size_t index = 0, size_t count = 0);
    bool replace(size_t index, const std::vector<std::string>& items);
    bool insert(size_t index, const std::string& item);
    void serialize(rapidjson::Value &outMap, rapidjson::Document::AllocatorType &allocator) override {}

protected:
    void fetch(size_t index, size_t count) override;

private:
    bool processResponseInternal(size_t index, size_t count);

    int mRequestToken = 0;
    std::vector<Object> mData;
    std::map<unsigned int, std::pair<size_t, size_t>> mRequests;
};

class TestDataSourceProvider : public DataSourceProvider {
public:
    TestDataSourceProvider() : mData(std::make_shared<std::vector<std::string>>()) {}
    explicit TestDataSourceProvider(const std::vector<std::string>& init) : mData(std::make_shared<std::vector<std::string>>(init)) {}

    std::shared_ptr<DataSourceConnection> create(const Object& sourceDefinition, std::weak_ptr<Context>,
            std::weak_ptr<LiveArray> liveArray) override;
    std::shared_ptr<TestDataSourceConnection> getConnection() {
        auto connection = mConnection.lock();
        assert(connection);
        return connection;
    }

private:
    std::shared_ptr<std::vector<std::string>> mData;
    std::weak_ptr<TestDataSourceConnection> mConnection;
};

} // namespace apl

#endif //_APL_TEST_DATA_SOURCE_H
