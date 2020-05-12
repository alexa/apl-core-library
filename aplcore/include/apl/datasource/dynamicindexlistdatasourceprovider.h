/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef _APL_DYNAMIC_INDEX_LIST_DATA_SOURCE_PROVIDER_H
#define _APL_DYNAMIC_INDEX_LIST_DATA_SOURCE_PROVIDER_H

#include "apl/apl.h"
#include "apl/datasource/offsetindexdatasourceconnection.h"

namespace apl {

namespace DynamicIndexListConstants {
/// Default source type name
static const std::string DEFAULT_TYPE_NAME = "dynamicIndexList";
/// Number of data items to cache on Lazy Loading fetch requests.
static const size_t DEFAULT_CACHE_CHUNK_SIZE = 10;
/// Maximum number of directives to buffer in case of unbound receival. Arbitrary number but considers highly unlikely
/// occurrence and capability to recover.
static const int DEFAULT_MAX_LIST_UPDATE_BUFFER = 5;
/// Number of retries to attempt on fetch requests.
static const int DEFAULT_FETCH_RETRIES = 2;
/// Fetch request timeout.
static const int DEFAULT_FETCH_TIMEOUT_MS = 5000;
/// Cache expiry timeout.
static const int DEFAULT_CACHE_EXPIRY_TIMEOUT_MS = 5000;

// Directive content keys
static const std::string LIST_ID = "listId";
static const std::string CORRELATION_TOKEN = "correlationToken";
static const std::string START_INDEX = "startIndex";
static const std::string MINIMUM_INCLUSIVE_INDEX = "minimumInclusiveIndex";
static const std::string MAXIMUM_EXCLUSIVE_INDEX = "maximumExclusiveIndex";
static const std::string ITEMS = "items";
static const std::string COUNT = "count";
static const std::string LIST_VERSION = "listVersion";
static const std::string OPERATIONS = "operations";
static const std::string UPDATE_TYPE = "type";
static const std::string UPDATE_INDEX = "index";
static const std::string UPDATE_ITEM = "item";
static const std::string UPDATE_ITEMS = "items";

// Error content definitions.
static const std::string ERROR_TYPE = "type";
static const std::string ERROR_TYPE_LIST_ERROR = "LIST_ERROR";
static const std::string ERROR_REASON = "reason";
static const std::string ERROR_REASON_INVALID_PRESENTATION_TOKEN = "INVALID_PRESENTATION_TOKEN";
static const std::string ERROR_REASON_INVALID_LIST_ID = "INVALID_LIST_ID";
static const std::string ERROR_REASON_INVALID_DATASOURCE = "INVALID_DATASOURCE";
static const std::string ERROR_REASON_INVALID_OPERATION = "INVALID_OPERATION";
static const std::string ERROR_REASON_MISSING_LIST_VERSION = "MISSING_LIST_VERSION";
static const std::string ERROR_REASON_DUPLICATE_LIST_VERSION = "DUPLICATE_LIST_VERSION";
static const std::string ERROR_REASON_LIST_INDEX_OUT_OF_RANGE = "LIST_INDEX_OUT_OF_RANGE";
static const std::string ERROR_REASON_MISSING_LIST_VERSION_IN_SEND_DATA = "MISSING_LIST_VERSION_IN_SEND_DATA";
static const std::string ERROR_REASON_INTERNAL_ERROR = "INTERNAL_ERROR";
static const std::string ERROR_OPERATION_INDEX = "operationIndex";
static const std::string ERROR_MESSAGE = "message";
} // DynamicIndexListConstants

/**
 * Possible update operations.
 */
enum DynamicIndexListUpdateType {
    kTypeInsert = 0,
    kTypeReplace = 1,
    kTypeDelete = 2,
    kTypeInsertMultiple = 3,
    kTypeDeleteMultiple = 4,
};

class DynamicIndexListDataSourceProvider;
class DynamicIndexListDataSourceConnection;

// Aliases to shorten some writing
using DILConnectionPtr = std::shared_ptr<DynamicIndexListDataSourceConnection>;
using DILProviderWPtr = std::weak_ptr<DynamicIndexListDataSourceProvider>;

/**
 * Simple configuration object
 */
class DynamicIndexListConfiguration {
public:
    DynamicIndexListConfiguration() = default;

    // Backward compatibility constructor
    DynamicIndexListConfiguration(const std::string& type, size_t cacheChunkSize) :
        type(type), cacheChunkSize(cacheChunkSize) {}

    // Link setters
    DynamicIndexListConfiguration& setType(const std::string& v) { type = v; return *this; }
    DynamicIndexListConfiguration& setCacheChunkSize(size_t v) { cacheChunkSize = v; return *this; }
    DynamicIndexListConfiguration& setListUpdateBufferSize(int v) { listUpdateBufferSize = v; return *this; }
    DynamicIndexListConfiguration& setFetchRetries(int v) { fetchRetries = v; return *this; }
    DynamicIndexListConfiguration& setFetchTimeout(apl_duration_t v) { fetchTimeout = v; return *this; }
    DynamicIndexListConfiguration& setCacheExpiryTimeout(apl_duration_t v) { cacheExpiryTimeout = v; return *this; }

    /// Source type name.
    std::string type = DynamicIndexListConstants::DEFAULT_TYPE_NAME;
    /// Fetch cache chunk size.
    size_t cacheChunkSize = DynamicIndexListConstants::DEFAULT_CACHE_CHUNK_SIZE;
    /// Size of the list for buffered update operations.
    int listUpdateBufferSize = DynamicIndexListConstants::DEFAULT_MAX_LIST_UPDATE_BUFFER;
    /// Number of retries for fetch requests.
    int fetchRetries = DynamicIndexListConstants::DEFAULT_FETCH_RETRIES;
    /// Fetch request timeout in milliseconds.
    apl_duration_t fetchTimeout = DynamicIndexListConstants::DEFAULT_FETCH_TIMEOUT_MS;
    /// Cached updates expiry timeout in milliseconds.
    apl_duration_t cacheExpiryTimeout = DynamicIndexListConstants::DEFAULT_CACHE_EXPIRY_TIMEOUT_MS;
};

/**
 * dynamicIndexList DataSourceConnection implementation.
 */
class DynamicIndexListDataSourceConnection : public OffsetIndexDataSourceConnection,
        public std::enable_shared_from_this<DynamicIndexListDataSourceConnection>{
public:
    /**
     * Constructor. Pretty much copy of OffsetIndexDataSourceConnection one but with additional parameters specific
     * to dynamicSourceList below.
     * @param provider parent provider.
     * @param configuration DataSource configuration.
     * @param context owning context.
     * @param liveArray data containing array.
     * @param listId as per dynamicIndexList specification.
     * @param minimumInclusiveIndex as per dynamicIndexList specification.
     * @param maximumExclusiveIndex as per dynamicIndexList specification.
     * @param offset first loaded child offset.
     * @param maxItems maximum number of items.
     */
    DynamicIndexListDataSourceConnection(
            DILProviderWPtr provider,
            const DynamicIndexListConfiguration& configuration,
            std::weak_ptr<Context> context,
            std::weak_ptr<LiveArray> liveArray,
            const std::string& listId,
            int minimumInclusiveIndex,
            int maximumExclusiveIndex,
            size_t offset,
            size_t maxItems);

    /**
     * Process items update passed through provider.
     * Performs adjustments required to match source parameters to internal implementation.
     * @param type type of update.
     * @param index starting index of update.
     * @param data data to insert/refresh.
     * @param count number of items affected.
     * @return true if successful, false otherwise.
     */
    bool processUpdate(DynamicIndexListUpdateType type, int index, const Object& data, int count);

    /**
     * Process lazy loading response.
     * Performs adjustments required to match source parameters to internal implementation.
     * @param index starting index of update.
     * @param data data to insert/refresh.
     * @param correlationToken fetch request correlation token.
     * @return true if successful, false otherwise.
     */
    bool processLazyLoad(int index, const Object& data, const Object& correlationToken);

    /**
     * Update data range bounds as per source specification. Internal variables will also be updated.
     * @return true if bounds were changed, false otherwise.
     */
    bool updateBounds(const Object& minimumInclusiveIndexObj, const Object& maximumExclusiveIndexObj);

    /**
     * @return get current list version.
     */
    int getListVersion() { return mListVersion; }

    /**
     * Advance list version.
     */
    void advanceListVersion() { mListVersion++; }

    /**
     * @return list ID.
     */
    std::string getListId() { return mListId; }

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
     * @param correlationToken Token to process.
     * @return true if request exists, false otherwise.
     */
    bool canProcess(const std::string& correlationToken) {
        return mPendingFetchRequests.find(correlationToken) != mPendingFetchRequests.end();
    }

    /**
     * @internal
     * @return Current data range bounds as defined by source. For testing only.
     */
    std::pair<int, int> getBounds() { return { mMinimumInclusiveIndex, mMaximumExclusiveIndex }; }

    /**
     * @return true if connection in fail state and cant process any more updates, false otherwise.
     */
    bool inFailState() const { return mInFailState; }

    /**
     * Set connection to be in fail state.
     */
     void setFailed() { mInFailState = true; }

     /**
      * @return true if list support only lazy loading, false otherwise.
      */
     bool isLazyLoadingOnly() const { return mLazyLoadingOnly; }

     /**
      * Set connection to only support lazy loading.
      */
     void setLazyLoadingOnly() { mLazyLoadingOnly = true; }

     /**
      * @return true if inserts or other operations possible, false otherwise.
      */
     bool changesAllowed() { return mMaxItems > 0; }

protected:
    /// Override to convert internal->source specific parameters.
    void fetch(size_t index, size_t count) override;

private:
    /**
     * Internal utility to keep fetch request related information.
     */
    struct PendingFetchRequest {
        ObjectMapPtr request;
        int retries;
        timeout_id timeoutId;
        std::vector<std::string> relatedTokens;
    };

    void sendFetchRequest(const ContextPtr& context, int index, int count);
    void enqueueFetchRequestEvent(const ContextPtr& context, const ObjectMapPtr& request);
    void retryFetchRequest(const ContextPtr& context, const std::string& correlationToken);
    timeout_id scheduleTimeout(const ContextPtr& context, const std::string& correlationToken);
    timeout_id scheduleUpdateExpiry(int version);
    void reportUpdateExpired(int version);

    std::string mListId;
    DILProviderWPtr mProvider;
    DynamicIndexListConfiguration mConfiguration;
    std::weak_ptr<Context> mContext;
    double mMinimumInclusiveIndex;
    double mMaximumExclusiveIndex;
    // Pending fetch requests per correlation token.
    std::map<std::string, std::shared_ptr<PendingFetchRequest>> mPendingFetchRequests;
    int mListVersion;

    /**
     * Internal utility to keep cached updates.
     */
     struct Update {
         Object update;
         timeout_id expiryTimeout;
     };

    std::map<int, Update> mUpdatesCache; // Cached updates per list version if any.
    bool mInFailState;
    bool mLazyLoadingOnly;
};

/**
 * dynamicIndexList DataSourceProvider implementation.
 */
class DynamicIndexListDataSourceProvider : public DataSourceProvider,
        public std::enable_shared_from_this<DynamicIndexListDataSourceProvider> {
public:
    /**
     * @deprecated Will be deleted, please use one below.
     * @param type DataSource type.
     * @param cacheChunkSize size of cache chunk. Effectively means how many items around ensured one should be available.
     */
    DynamicIndexListDataSourceProvider(const std::string& type, size_t cacheChunkSize);

    /**
     * @param config Full configuration object.
     */
    DynamicIndexListDataSourceProvider(const DynamicIndexListConfiguration& config);

    /**
     * Default constructor.
     */
    DynamicIndexListDataSourceProvider();

    /// DataSourceProvider overrides
    std::shared_ptr<DataSourceConnection> create(const Object& sourceDefinition, std::weak_ptr<Context> context,
            std::weak_ptr<LiveArray> liveArray) override;
    bool processUpdate(const Object& payload) override;
    Object getPendingErrors() override;

    /**
     * @return Data source configuration.
     */
    DynamicIndexListConfiguration getConfiguration() { return mConfiguration; }

    /**
     * @internal
     * @param listId ID of list requested.
     * @return list data range bounds as defined by source. For testing only.
     */
    std::pair<int, int> getBounds(const std::string& listId);

private:
    std::map<std::string, std::weak_ptr<DynamicIndexListDataSourceConnection>> mConnections;
    DynamicIndexListConfiguration mConfiguration;
    std::vector<Object> mPendingErrors;
    int mRequestToken; // Monotonically increasing request correlation token.

    bool processLazyLoadInternal(const DILConnectionPtr& connection,
            const Object& responseMap);
    bool processUpdateInternal(const DILConnectionPtr& connection,
            const Object& responseMap);

    void clearStaleConnections();
    void constructAndReportError(const std::string& reason, const std::string& listId, const Object& listVersion,
            const Object& operationIndex, const std::string& message);
    void constructAndReportError(const std::string& reason, const std::string& listId, const std::string& message);
    void constructAndReportError(const std::string& reason, const DILConnectionPtr& connection,
            const Object& operationIndex, const std::string& message);
    int getAndIncrementCorrelationToken() {
        mRequestToken++;
        return mRequestToken;
    }

    friend class DynamicIndexListDataSourceConnection;
};

} // namespace apl

#endif //_APL_DYNAMIC_INDEX_LIST_DATA_SOURCE_PROVIDER_H