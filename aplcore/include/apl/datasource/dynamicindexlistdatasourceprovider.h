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

#ifndef _APL_DYNAMIC_INDEX_LIST_DATA_SOURCE_PROVIDER_H
#define _APL_DYNAMIC_INDEX_LIST_DATA_SOURCE_PROVIDER_H

#include "apl/apl.h"
#include "apl/datasource/dynamiclistdatasourceprovider.h"

namespace apl {

namespace DynamicIndexListConstants {
/// Default source type name
static const std::string DEFAULT_TYPE_NAME = "dynamicIndexList";

// Directive content keys
static const std::string START_INDEX = "startIndex";
static const std::string MINIMUM_INCLUSIVE_INDEX = "minimumInclusiveIndex";
static const std::string MAXIMUM_EXCLUSIVE_INDEX = "maximumExclusiveIndex";
static const std::string COUNT = "count";
static const std::string OPERATIONS = "operations";
static const std::string UPDATE_TYPE = "type";
static const std::string UPDATE_INDEX = "index";
static const std::string UPDATE_ITEM = "item";
static const std::string UPDATE_ITEMS = "items";

// Error content definitions.
static const std::string ERROR_REASON_INVALID_OPERATION = "INVALID_OPERATION";
static const std::string ERROR_REASON_MISSING_LIST_VERSION_IN_SEND_DATA = "MISSING_LIST_VERSION_IN_SEND_DATA";
static const std::string ERROR_REASON_LIST_INDEX_OUT_OF_RANGE = "LIST_INDEX_OUT_OF_RANGE";
static const std::string ERROR_REASON_OCCUPIED_LIST_INDEX = "OCCUPIED_LIST_INDEX";
static const std::string ERROR_REASON_LOAD_INDEX_OUT_OF_RANGE = "LOAD_INDEX_OUT_OF_RANGE";
static const std::string ERROR_REASON_INCONSISTENT_RANGE = "INCONSISTENT_RANGE";
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
class DynamicIndexListConfiguration : public DynamicListConfiguration {
public:
    DynamicIndexListConfiguration() : DynamicListConfiguration(DynamicIndexListConstants::DEFAULT_TYPE_NAME) {}

    // Backward compatibility constructor
    DynamicIndexListConfiguration(const std::string& type, size_t cacheChunkSize) :
        DynamicListConfiguration(type, cacheChunkSize) {}

    DynamicIndexListConfiguration(const DynamicListConfiguration& config) : DynamicListConfiguration(config) {}
};

/**
 * dynamicIndexList DataSourceConnection implementation.
 */
class DynamicIndexListDataSourceConnection : public DynamicListDataSourceConnection {
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

    void serialize(rapidjson::Value& outMap,
                    rapidjson::Document::AllocatorType& allocator) override;

protected:
    /// Override to convert internal->source specific parameters.
    void fetch(size_t index, size_t count) override;

private:
    double mMinimumInclusiveIndex;
    double mMaximumExclusiveIndex;

    bool mInFailState;
    bool mLazyLoadingOnly;
};

/**
 * dynamicIndexList DataSourceProvider implementation.
 */
class DynamicIndexListDataSourceProvider : public DynamicListDataSourceProvider,
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
    explicit DynamicIndexListDataSourceProvider(const DynamicIndexListConfiguration& config);

    /**
     * Default constructor.
     */
    DynamicIndexListDataSourceProvider();

    /// DataSourceProvider overrides
    std::shared_ptr<DynamicListDataSourceConnection> createConnection(
        const Object& sourceDefinition,
        std::weak_ptr<Context> context,
        std::weak_ptr<LiveArray> liveArray,
        const std::string& listId) override;
    bool process(const Object& responseMap) override;

    /**
     * @internal
     * @param listId ID of list requested.
     * @return list data range bounds as defined by source. For testing only.
     */
    std::pair<int, int> getBounds(const std::string& listId);

private:
    bool processLazyLoadInternal(const DILConnectionPtr& connection,
            const Object& responseMap);
    bool processUpdateInternal(const DILConnectionPtr& connection,
            const Object& responseMap);
};

} // namespace apl

#endif //_APL_DYNAMIC_INDEX_LIST_DATA_SOURCE_PROVIDER_H