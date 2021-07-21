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

#ifndef _APL_DYNAMIC_LIST_DATA_SOURCE_PROVIDER_H
#define _APL_DYNAMIC_LIST_DATA_SOURCE_PROVIDER_H

#include "apl/apl.h"
#include "apl/datasource/offsetindexdatasourceconnection.h"
#include "apl/datasource/dynamiclistdatasourcecommon.h"

namespace apl {

class DynamicListDataSourceProvider;
class DynamicListDataSourceConnection;

// Aliases to shorten some writing
using DLConnectionPtr = std::shared_ptr<DynamicListDataSourceConnection>;
using DLProviderWPtr = std::weak_ptr<DynamicListDataSourceProvider>;

class DynamicListDataSourceConnection : public OffsetIndexDataSourceConnection,
                                        public NonCopyable,
                                        public std::enable_shared_from_this<DynamicListDataSourceConnection> {
public:
    /**
     * Constructor. DynamicIndexListDataSourceConnection
     * @param context owning context.
     * @param listId as per dynamicIndexList specification.
     * @param provider parent provider.
     * @param configuration DataSource configuration.
     * @param liveArray data containing array.
     * @param offset first loaded child offset.
     * @param maxItems maximum number of items.
     */
    DynamicListDataSourceConnection(
        std::weak_ptr<Context> context,
        const std::string& listId,
        DLProviderWPtr provider,
        const DynamicListConfiguration& configuration,
        std::weak_ptr<LiveArray> liveArray,
        size_t offset,
        size_t maxItems);

    /**
     * Add update for specific version in cache to be applied later as source spec suggests.
     * @param version target list version.
     * @param payload update payload.
     */
    void putCacheUpdate(int version, const Object& payload);

    /**
     * Get cached update for specific list version if any.
     * @param version target list version.
     * @return Payload if exists or NULL object otherwise.
     */
    Object retrieveCachedUpdate(int version);

    /**
     * @param correlationToken token to find.
     * @return true if can process, false otherwise.
     */
    bool canProcess(const Object& correlationToken) {
        return mPendingFetchRequests.find(correlationToken.asString()) != mPendingFetchRequests.end();
    }

    /**
      * @return list ID.
      */
    std::string getListId() { return mListId; }

    /**
     * @return get current list version.
     */
    int getListVersion() { return mListVersion; }

    /**
     * Advance list version.
     */
    void advanceListVersion() { mListVersion++; }

    /**
     * @return context object
     */
    std::shared_ptr<Context> getContext() { return mContext.lock(); }

protected:
    /**
     * Retry fetch request.
     * @return true if request sent, false otherwise.
     */
    bool retryFetchRequest(const std::string& correlationToken);

    void sendFetchRequest(const ObjectMap& requestData);
    void clearTimeouts(const ContextPtr& context, const std::string& correlationToken);
    timeout_id scheduleUpdateExpiry(int version);
    void reportUpdateExpired(int version);
    void constructAndReportError(const std::string& reason, const Object& operationIndex, const std::string& message);

    std::weak_ptr<Context> mContext;
    DynamicListConfiguration mConfiguration;

private:
    void enqueueFetchRequestEvent(const ContextPtr& context, const ObjectMapPtr& request);
    timeout_id scheduleTimeout(const std::string& correlationToken);

    /**
     * Internal Utility to keep fetch request related information.
     */
    struct PendingFetchRequest {
        ObjectMapPtr request;
        int retries;
        timeout_id timeoutId;
        std::vector<std::string> relatedTokens;
    };

    // Pending fetch requests per correlation token.
    std::map<std::string, std::shared_ptr<PendingFetchRequest>> mPendingFetchRequests;

    /**
     * Internal utility to keep cached updates.
     */
    struct Update {
        Object update;
        timeout_id expiryTimeout;
    };

    std::map<int, Update> mUpdatesCache; // Cached updates per list version if any.

    std::string mListId;
    DLProviderWPtr mProvider;
    int mListVersion;
};

class DynamicListDataSourceProvider : public DataSourceProvider {
public:
    DynamicListDataSourceProvider(const DynamicListConfiguration& config);

    /**
     * @return Data source configuration.
     */
    DynamicListConfiguration getConfiguration() { return mConfiguration; }

    /// DataSourceProvider overrides
    Object getPendingErrors() override;
    std::shared_ptr<DataSourceConnection> create(const Object& sourceDefinition, std::weak_ptr<Context> context,
                                                 std::weak_ptr<LiveArray> liveArray) override;
    bool processUpdate(const Object& payload) override;

protected:
    DLConnectionPtr getConnection(const std::string& listId);
    DLConnectionPtr getConnection(const std::string& listId, const Object& correlationToken);

    void constructAndReportError(
        const std::string& reason,
        const std::string& listId,
        const Object& listVersion,
        const Object& operationIndex,
        const std::string& message);
    void constructAndReportError(const std::string& reason, const std::string& listId, const std::string& message);
    void constructAndReportError(
        const std::string& reason, const DLConnectionPtr& connection, const Object& operationIndex, const std::string& message);

    bool canFetch(const Object& correlationToken, const DLConnectionPtr& connection);

    virtual std::shared_ptr<DynamicListDataSourceConnection> createConnection(
        const Object& sourceDefinition,
        std::weak_ptr<Context> context,
        std::weak_ptr<LiveArray> liveArray,
        const std::string& listId) = 0;
    virtual bool process(const Object& responseMap) = 0;

    DynamicListConfiguration mConfiguration;

private:
    void clearStaleConnections();

    int mRequestToken; // Monotonically increasing request correlation token.

    int getAndIncrementCorrelationToken() {
        mRequestToken++;
        return mRequestToken;
    }

    std::map<std::string, std::weak_ptr<DynamicListDataSourceConnection>> mConnections;
    std::vector<Object> mPendingErrors;

    friend class DynamicListDataSourceConnection;
};

} // namespace apl

#endif //_APL_DYNAMIC_LIST_DATA_SOURCE_PROVIDER_H
