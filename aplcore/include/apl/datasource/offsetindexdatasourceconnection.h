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

#ifndef _APL_OFFSET_INDEX_DATA_SOURCE_PROVIDER_H
#define _APL_OFFSET_INDEX_DATA_SOURCE_PROVIDER_H

#include "apl/datasource/datasourceconnection.h"

namespace apl {

class Object;
class LiveArray;

/**
 * Simple base implementation of index based data source that has concept of offset in it.
 * During lifetime of it it could call @see fetch() in order to request more items from external DataSource.
 * Specific implementation supposed to answer with call to @see update() providing what was requested.
 *
 * Current implementation has few limitations:
 * * It's not thread safe and don't have any synchronization in it.
 * * Response should be processed on the same (APL Core Engine handling) thread.
 * * Underlying array should be continuous. While bigger than requested or smaller then requested responses are allowed,
 * ones creating gaps will be rejected.
 */
class OffsetIndexDataSourceConnection : public DataSourceConnection {
public:
    /**
     * @param liveArray Base LiveArray, provided by APL Core on creation time.
     * @param offset Initial source offset if not 0. Some sources can start in the middle.
     * @param maxItems Maximum number of items available from external source.
     * @param cacheChunkSize Number of items to request at most.
     */
    OffsetIndexDataSourceConnection(
            std::weak_ptr<LiveArray> liveArray,
            size_t offset,
            size_t maxItems,
            size_t cacheChunkSize);

    /// DataSourceConnection overrides
    /**
     * Assumption: ensure will happen only on existing indexes as initiated by Core during ensureLayout routine.
     */
    void ensure(size_t index) override;

    std::shared_ptr<LiveArray> getLiveArray() override;

    /**
     * Provide an update to underlying data.
     *
     * @param index index to start update from.
     * @param data vector of data items.
     * @param replace true to replace items if overlaps with existing, false to ignore overlaps.
     * @return true if update applied, false otherwise.
     */
    bool update(size_t index, const std::vector<Object>& data, bool replace = false);

    /**
     * Insert new item into data and update internal state.
     * @param index insert index.
     * @param item data item.
     * @return true if successful, false otherwise.
     */
    bool insert(size_t index, const Object& item);

    /**
     * Remove item from data and update internal state.
     * @param index remove index.
     * @return true if successful, false otherwise.
     */
    bool remove(size_t index);

    /**
     * Insert new items into data and update internal state.
     * @param index insert index.
     * @param items data items.
     * @return true if successful, false otherwise.
     */
    bool insert(size_t index, const std::vector<Object>& items);

    /**
     * Remove items from data and update internal state.
     * @param index remove index.
     * @param count number of items to remove.
     * @return true if successful, false otherwise.
     */
    bool remove(size_t index, size_t count);

    /**
     * Check if provided range overlaps with currently filled area.
     * @param index range start index
     * @param count range size
     * @return true if overlaps, false otherwise.
     */
    bool overlaps(size_t index, size_t count);

protected:
    /**
     * Callback to request more data.
     *
     * @param index data index in external source.
     * @param count number of items to fetch.
     */
    virtual void fetch(size_t index, size_t count) = 0;

protected:
    size_t mMaxItems;
    size_t mOffset;
    std::weak_ptr<LiveArray> mLiveArray;

private:
    int mCacheChunkSize;
};

} // namespace apl

#endif //_APL_OFFSET_INDEX_DATA_SOURCE_PROVIDER_H