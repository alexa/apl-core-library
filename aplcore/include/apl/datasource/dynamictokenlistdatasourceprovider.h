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

#ifndef _APL_DYNAMIC_TOKEN_LIST_DATA_SOURCE_PROVIDER_H
#define _APL_DYNAMIC_TOKEN_LIST_DATA_SOURCE_PROVIDER_H

#include "apl/apl.h"
#include "apl/datasource/dynamiclistdatasourceprovider.h"

namespace apl {

namespace DynamicTokenListConstants {
/// Default source type name
static const std::string DEFAULT_TYPE_NAME = "dynamicTokenList";

// Directive content keys
static const std::string PAGE_TOKEN = "pageToken";
static const std::string BACKWARD_PAGE_TOKEN = "backwardPageToken";
static const std::string FORWARD_PAGE_TOKEN = "forwardPageToken";
static const std::string NEXT_PAGE_TOKEN = "nextPageToken";
} // namespace DynamicTokenListConstants

class DynamicTokenListDataSourceProvider;
class DynamicTokenListDataSourceConnection;

// Aliases to shorten some writing
using DTLConnectionPtr = std::shared_ptr<DynamicTokenListDataSourceConnection>;
using DTLProviderWPtr = std::weak_ptr<DynamicTokenListDataSourceProvider>;

class DynamicTokenListDataSourceConnection : public DynamicListDataSourceConnection {
public:
    DynamicTokenListDataSourceConnection(
        DTLProviderWPtr provider,
        const DynamicListConfiguration& configuration,
        std::weak_ptr<Context> context,
        std::weak_ptr<LiveArray> liveArray,
        const std::string& listId,
        const Object& firstToken,
        const Object& lastToken);

    /**
     * Process lazy loading response.
     * Performs adjustments required to match source parameters to internal implementation.
     * @param data data to insert/refresh.
     * @param pageToken fetch request page token.
     * @param nextPageToken next page token.
     * @param correlationToken fetch request correlation token.
     * @return true if successful, false otherwise.
     */
    bool processLazyLoad(
        const Object& data,
        const Object& pageToken,
        const Object& nextPageToken,
        const Object& correlationToken);

    /// DataSourceConnection overrides
    /**
     * Assumption: ensure will happen only on existing indexes as initiated by Core during ensureLayout routine.
     */
    void ensure(size_t index) override;

    void serialize(rapidjson::Value &outMap, rapidjson::Document::AllocatorType &allocator) override;

protected:
    /// Override fetch not used in this class
    void fetch(size_t index, size_t count) override { return; };

private:
    Object mFirstToken;
    Object mLastToken;

    bool updateLiveArray(const std::vector<Object>& data, const Object& pageToken, const Object& nextPageToken);
};

class DynamicTokenListDataSourceProvider : public DynamicListDataSourceProvider,
        public std::enable_shared_from_this<DynamicTokenListDataSourceProvider> {
public:
    /**
     * @param config Full configuration object.
     */
    explicit DynamicTokenListDataSourceProvider(const DynamicListConfiguration& config);

    /**
     * Default constructor.
     */
    DynamicTokenListDataSourceProvider();

    /// DataSourceProvider overrides
    std::shared_ptr<DynamicListDataSourceConnection> createConnection(
        const Object& sourceDefinition,
        std::weak_ptr<Context> context,
        std::weak_ptr<LiveArray> liveArray,
        const std::string& listId) override;
    bool process(const Object& responseMap) override;

private:
    bool processLazyLoadInternal(const DTLConnectionPtr& connection, const Object& responseMap);
};

} // namespace apl

#endif //_APL_DYNAMIC_TOKEN_LIST_DATA_SOURCE_PROVIDER_H