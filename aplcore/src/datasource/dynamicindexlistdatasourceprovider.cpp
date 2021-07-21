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

#include "apl/datasource/dynamicindexlistdatasourceprovider.h"
#include "apl/datasource/datasource.h"
#include "apl/datasource/datasourceprovider.h"
#include "apl/engine/evaluate.h"
#include "apl/time/timemanager.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <climits>

using namespace apl;
using namespace DynamicIndexListConstants;
using namespace DynamicListConstants;

const std::map<std::string, DynamicIndexListUpdateType> sDatasourceUpdateType = {
    {"InsertListItem", kTypeInsert},
    {"ReplaceListItem", kTypeReplace},
    {"DeleteListItem", kTypeDelete},
    {"InsertItem", kTypeInsert},
    {"SetItem", kTypeReplace},
    {"DeleteItem", kTypeDelete},
    {"InsertMultipleItems", kTypeInsertMultiple},
    {"DeleteMultipleItems", kTypeDeleteMultiple},
};

DynamicIndexListDataSourceConnection::DynamicIndexListDataSourceConnection(
        DILProviderWPtr provider,
        const DynamicIndexListConfiguration& configuration,
        std::weak_ptr<Context> context,
        std::weak_ptr<LiveArray> liveArray,
        const std::string& listId,
        int minimumInclusiveIndex,
        int maximumExclusiveIndex,
        size_t offset,
        size_t maxItems) :
        DynamicListDataSourceConnection(std::move(context), listId, std::move(provider), configuration, std::move(liveArray), offset, maxItems),
        mMinimumInclusiveIndex(minimumInclusiveIndex),
        mMaximumExclusiveIndex(maximumExclusiveIndex),
        mInFailState(false),
        mLazyLoadingOnly(false)
        {}

void
DynamicIndexListDataSourceConnection::fetch(size_t index, size_t count) {
    int idx = index + mMinimumInclusiveIndex;

    auto requestData = ObjectMap{{START_INDEX, idx}, {COUNT, count}};

    sendFetchRequest(requestData);
}

bool
DynamicIndexListDataSourceConnection::processLazyLoad(int index, const Object& data, const Object& correlationToken) {
    auto context = mContext.lock();
    if (!context)
        return false;

    auto items = evaluateRecursive(*context, data);

    bool result = false;
    bool outOfRange = false;

    if (items.isArray() && !items.empty()) {
        std::vector<Object> dataArray(items.getArray());

        // Shrink the update to fit the bounds. If any change done - report an LOAD_INDEX_OUT_OF_RANGE error
        int startAdjust = mMinimumInclusiveIndex > index ? mMinimumInclusiveIndex - index : 0;
        if (startAdjust > 0) {
            dataArray.erase(dataArray.begin(), dataArray.begin() + startAdjust);
            index += startAdjust;
            outOfRange = true;
        }

        int endAdjust = mMaximumExclusiveIndex < (index + (int) dataArray.size()) ?
                (index + dataArray.size()) - mMaximumExclusiveIndex : 0;
        if (endAdjust > 0) {
            dataArray.erase(dataArray.end() - endAdjust, dataArray.end());
            outOfRange = true;
        }

        size_t idx = index - mMinimumInclusiveIndex;
        if (overlaps(idx, dataArray.size())) {
            constructAndReportError(ERROR_REASON_OCCUPIED_LIST_INDEX, index,
                    "Load range overlaps existing items. New items for existing range discarded.");
        }
        result = update(idx, dataArray, false);
    } else {
        constructAndReportError(ERROR_REASON_MISSING_LIST_ITEMS, index,
                "No items provided to load.");
        retryFetchRequest(correlationToken.asString());
        return result;
    }

    if (result)
        clearTimeouts(context, correlationToken.asString());

    if (!result || outOfRange) {
        constructAndReportError(ERROR_REASON_LOAD_INDEX_OUT_OF_RANGE, index,
                "Requested index out of bounds.");
    }
    return result;
}

bool
DynamicIndexListDataSourceConnection::processUpdate(DynamicIndexListUpdateType type, int index, const Object& data,
        int count) {
    if (index < mMinimumInclusiveIndex) {
        constructAndReportError(ERROR_REASON_LIST_INDEX_OUT_OF_RANGE, index,
                                            "Requested index out of bounds.");
        return false;
    }
    size_t idx = index - mMinimumInclusiveIndex;

    auto context = mContext.lock();
    if (!context) {
        return false;
    }
    auto items = evaluateRecursive(*context, data);

    bool result = false;

    switch (type) {
        case kTypeInsert:
            // We explicitly prohibit insertion below start of existing range to avoid index shifting ambiguity.
            if (idx < mOffset) {
                break;
            }

            result = insert(idx, items);
            if (result && mMaximumExclusiveIndex < INT_MAX)
                mMaximumExclusiveIndex++;
            break;
        case kTypeReplace:
            result = update(idx, ObjectArray({items}), true);
            break;
        case kTypeDelete:
            result = remove(idx);
            if (result && mMaximumExclusiveIndex < INT_MAX)
                mMaximumExclusiveIndex--;
            break;
        case kTypeInsertMultiple:
            // We explicitly prohibit insertion below start of existing range to avoid index shifting ambiguity.
            if (idx < mOffset) {
                break;
            }

            if (!items.isArray()) {
                constructAndReportError(ERROR_REASON_INTERNAL_ERROR, index,
                        "No array provided for range insert.");
                return false;
            }

            // We are inserting relative to 0.
            if (index < 0)
                idx++;

            result = insert(idx, items.getArray());
            if (result && mMaximumExclusiveIndex < INT_MAX)
                mMaximumExclusiveIndex += items.size();
            break;
        case kTypeDeleteMultiple:
            result = remove(idx, count);
            if (result && mMaximumExclusiveIndex < INT_MAX)
                mMaximumExclusiveIndex -= count;
            break;
        default:
            break;
    }
    if (!result) {
        constructAndReportError(ERROR_REASON_LIST_INDEX_OUT_OF_RANGE, index,
                "Requested index out of bounds.");
    }
    return result;
}

bool
DynamicIndexListDataSourceConnection::updateBounds(
        const Object& minimumInclusiveIndexObj,
        const Object& maximumExclusiveIndexObj) {

    double minimumInclusiveIndex = minimumInclusiveIndexObj.isNumber()
            ? minimumInclusiveIndexObj.getInteger() : mMinimumInclusiveIndex;
    double maximumExclusiveIndex = maximumExclusiveIndexObj.isNumber()
            ? maximumExclusiveIndexObj.getInteger() : mMaximumExclusiveIndex;

    bool wasChanged = false;
    double oldOffset = mOffset;
    auto liveArray = mLiveArray.lock();
    if (minimumInclusiveIndex > mMinimumInclusiveIndex) {
        if (mOffset > 0) {
            mOffset = std::max((oldOffset - std::abs(mMinimumInclusiveIndex - minimumInclusiveIndex)), (double)0);
        }
        mMinimumInclusiveIndex = minimumInclusiveIndex;
        wasChanged = true;

        if (liveArray && oldOffset == 0) {
            liveArray->remove(0, mOffset - oldOffset);
            mOffset = 0;
        }
    } else if (minimumInclusiveIndex < mMinimumInclusiveIndex) {
        mOffset += (mMinimumInclusiveIndex - minimumInclusiveIndex);
        mMinimumInclusiveIndex = minimumInclusiveIndex;
        wasChanged = true;
    }

    if (maximumExclusiveIndex != mMaximumExclusiveIndex) {
        mMaximumExclusiveIndex = maximumExclusiveIndex;
        wasChanged = true;
    }

    if (mMaximumExclusiveIndex <= mMinimumInclusiveIndex) {
        mMaxItems = 0;
        mMaximumExclusiveIndex = mMinimumInclusiveIndex = 0;
    } else {
        mMaxItems = std::abs(mMaximumExclusiveIndex - mMinimumInclusiveIndex);
    }

    if (liveArray && mMaxItems == 0) {
        liveArray->remove(0, liveArray->size());
    } else if (liveArray && mOffset + liveArray->size() > mMaxItems) {
        int topShrink = mOffset + liveArray->size() - mMaxItems;
        liveArray->remove(liveArray->size() - topShrink, topShrink);
    }

    return wasChanged;
}

DynamicIndexListDataSourceProvider::DynamicIndexListDataSourceProvider(const DynamicIndexListConfiguration& config)
        : DynamicListDataSourceProvider(config) {}

DynamicIndexListDataSourceProvider::DynamicIndexListDataSourceProvider(const std::string& type, size_t cacheChunkSize)
        : DynamicIndexListDataSourceProvider(DynamicIndexListConfiguration(type, cacheChunkSize)) {}

DynamicIndexListDataSourceProvider::DynamicIndexListDataSourceProvider()
        : DynamicIndexListDataSourceProvider(DynamicIndexListConfiguration()) {}

std::shared_ptr<DynamicListDataSourceConnection>
DynamicIndexListDataSourceProvider::createConnection(
    const Object& sourceDefinition,
    std::weak_ptr<Context> context,
    std::weak_ptr<LiveArray> liveArray,
    const std::string& listId) {
    if (!sourceDefinition.has(START_INDEX) || !sourceDefinition.get(START_INDEX).isNumber()) {
        constructAndReportError(ERROR_REASON_INTERNAL_ERROR, "N/A", "Missing required fields.");
        return nullptr;
    }

    auto startIndex = sourceDefinition.get(START_INDEX).getInteger();
    auto minimumInclusiveIndexObj = sourceDefinition.get(MINIMUM_INCLUSIVE_INDEX);
    auto maximumExclusiveIndexObj = sourceDefinition.get(MAXIMUM_EXCLUSIVE_INDEX);
    int minimumInclusiveIndex = minimumInclusiveIndexObj.isNumber() ? minimumInclusiveIndexObj.getInteger() : INT_MIN;
    int maximumExclusiveIndex = maximumExclusiveIndexObj.isNumber() ? maximumExclusiveIndexObj.getInteger() : INT_MAX;

    // Check if boundaries and start position is valid:
    //  * startIndex should generally be in [minimumInclusiveIndex, maximumExclusiveIndex) range.
    //  * As an exception we allow for all of properties to be equal for proactive loading case.
    if (!(minimumInclusiveIndex == startIndex && maximumExclusiveIndex == startIndex) &&
         (minimumInclusiveIndex > startIndex || maximumExclusiveIndex < startIndex)) {
        constructAndReportError(ERROR_REASON_INTERNAL_ERROR, listId, "DataSource bounds configuration is wrong.");
        return nullptr;
    }

    size_t offset = std::abs((double)startIndex - (double)minimumInclusiveIndex);
    size_t maxItems = std::abs((double)maximumExclusiveIndex - (double)minimumInclusiveIndex);

    auto la = liveArray.lock();
    if (la) {
        int endBound = startIndex + la->size();
        if (endBound > maximumExclusiveIndex) {
            // Shrink initial array to fit into specified bounds.
            int maxAllowed = maximumExclusiveIndex - startIndex;
            int reminder = la->size() - maxAllowed;
            la->remove(maxAllowed, reminder);
        }
    }

    return std::make_shared<DynamicIndexListDataSourceConnection>(shared_from_this(), mConfiguration,
            context, liveArray, listId, minimumInclusiveIndex, maximumExclusiveIndex, offset, maxItems);
}

bool
DynamicIndexListDataSourceProvider::processLazyLoadInternal(
        const DILConnectionPtr& connection, const Object& responseMap) {
    int startIndex = responseMap.get(START_INDEX).getInteger();
    auto correlationToken = responseMap.get(CORRELATION_TOKEN);

    if (!canFetch(correlationToken, connection))
        return false;

    auto minimumInclusiveIndex = responseMap.get(MINIMUM_INCLUSIVE_INDEX);
    auto maximumExclusiveIndex = responseMap.get(MAXIMUM_EXCLUSIVE_INDEX);

    if(connection->updateBounds(minimumInclusiveIndex, maximumExclusiveIndex)) {
        constructAndReportError(ERROR_REASON_INCONSISTENT_RANGE, connection, startIndex,
                "Bounds were changed in runtime.");
    }

    if (!responseMap.has(ITEMS)) {
        constructAndReportError(ERROR_REASON_MISSING_LIST_ITEMS, connection, Object::NULL_OBJECT(),
                                "No items defined.");
        return true;
    }

    if (!connection->changesAllowed()) {
        constructAndReportError(ERROR_REASON_INTERNAL_ERROR, connection, Object::NULL_OBJECT(),
                "Payload has unexpected fields.");
        return true;
    }

    auto items = responseMap.get(ITEMS);
    return connection->processLazyLoad(startIndex, items, correlationToken);
}

bool
DynamicIndexListDataSourceProvider::processUpdateInternal(
        const DILConnectionPtr& connection, const Object& responseMap) {
    if (connection->isLazyLoadingOnly()) {
        constructAndReportError(ERROR_REASON_MISSING_LIST_VERSION_IN_SEND_DATA, connection, Object::NULL_OBJECT(),
                "List supports only lazy loading.");
        connection->setFailed();
        return false;
    }

    ObjectArray operations = responseMap.get(OPERATIONS).getArray();
    bool result = true;

    for (const auto& operation : operations) {
        if (!operation.has(UPDATE_TYPE) || !operation.get(UPDATE_TYPE).isString() ||
            !operation.has(UPDATE_INDEX) || !operation.get(UPDATE_INDEX).isNumber()) {
            constructAndReportError(ERROR_REASON_INVALID_OPERATION, connection, Object::NULL_OBJECT(),
                    "Operation malformed.");
            result = false;
            break;
        }

        auto typeName = operation.get(UPDATE_TYPE).asString();
        if (!sDatasourceUpdateType.count(typeName)) {
            constructAndReportError(ERROR_REASON_INVALID_OPERATION, connection, Object::NULL_OBJECT(),
                    "Wrong update type.");
            result = false;
            break;
        }

        auto type = sDatasourceUpdateType.at(typeName);
        auto index = operation.get(UPDATE_INDEX).asInt();
        auto item = operation.get(UPDATE_ITEM);
        auto items = item.isNull() ? operation.get(UPDATE_ITEMS) : item;
        auto count = operation.opt(COUNT, item.size()).asInt();
        if (!connection->processUpdate(type, index, items, count)) {
            result = false;
            break;
        }
    }

    if (!result) {
        connection->setFailed();
    }

    return result;
}

bool
DynamicIndexListDataSourceProvider::process(const Object& responseMap) {
    if (!responseMap.has(LIST_ID) ||
        !responseMap.get(LIST_ID).isString()) {
        constructAndReportError(ERROR_REASON_INVALID_LIST_ID, "N/A", "Missing listId.");
        return false;
    }

    std::string listId = responseMap.get(LIST_ID).getString();

    auto dataSourceConnection = getConnection(listId, responseMap.get(CORRELATION_TOKEN));

    if(!dataSourceConnection) {
        return false;
    }

    auto connection = std::dynamic_pointer_cast<DynamicIndexListDataSourceConnection>(dataSourceConnection);

    if(connection->inFailState()) {
        constructAndReportError(ERROR_REASON_INTERNAL_ERROR, listId, "List in fail state.");
        return false;
    }

    int listVersion = responseMap.opt(LIST_VERSION, -1).asInt();
    bool result = false;

    bool isLazyLoading = false;

    if (responseMap.has(START_INDEX) && responseMap.get(START_INDEX).isNumber()) {
        isLazyLoading = true;
    } else if (responseMap.has(OPERATIONS) && responseMap.get(OPERATIONS).isArray()) {
        isLazyLoading = false;
    } else {
        constructAndReportError(ERROR_REASON_INTERNAL_ERROR, connection,
                                Object::NULL_OBJECT(), "Payload missing required fields.");
        return false;
    }

    int currentListVersion = connection->getListVersion();
    if ((listVersion < 0 && !isLazyLoading)
    || ((currentListVersion > 0 || listVersion > 0) && listVersion != currentListVersion + 1)) {
        // Check if cachable
        if (listVersion > currentListVersion + 1) {
            connection->putCacheUpdate(listVersion - 1, responseMap);
        } else if (listVersion < 0) {
            constructAndReportError(ERROR_REASON_MISSING_LIST_VERSION_IN_SEND_DATA, connection,
                    Object::NULL_OBJECT(), "Missing list version.");
            connection->setFailed();
        } else {
            constructAndReportError(ERROR_REASON_DUPLICATE_LIST_VERSION, connection,
                    Object::NULL_OBJECT(), "Duplicate list version.");
        }

        return false;
    }

    if (isLazyLoading) {
        result = processLazyLoadInternal(connection, responseMap);
    } else {
        result = processUpdateInternal(connection, responseMap);
    }

    if (result && currentListVersion < listVersion) {
        connection->advanceListVersion();
    } else {
        // If no version was provided in directive we are in LazyLoading only mode.
        connection->setLazyLoadingOnly();
    }

    // Apply cache if any.
    auto cachedPayload = connection->retrieveCachedUpdate(connection->getListVersion());
    if (!cachedPayload.isNull()) {
        processUpdate(cachedPayload);
    }

    auto context = connection->getContext();
    if (result && context != nullptr)
        context->setDirtyDataSourceContext(
            std::dynamic_pointer_cast<DataSourceConnection>(shared_from_this()));

    return result;
}

std::pair<int, int>
DynamicIndexListDataSourceProvider::getBounds(const std::string& listId) {
    auto connection = std::dynamic_pointer_cast<DynamicIndexListDataSourceConnection>(getConnection(listId));
    if(!connection)
        return {};

    return connection->getBounds();
}

void
DynamicIndexListDataSourceConnection::serialize(rapidjson::Value& outMap,
                                                rapidjson::Document::AllocatorType& allocator) {

    outMap.AddMember("type", rapidjson::Value(DEFAULT_TYPE_NAME.c_str(), allocator).Move(), allocator);
    outMap.AddMember("listId", rapidjson::Value(DynamicListDataSourceConnection::getListId().c_str(), allocator).Move(), allocator);

    outMap.AddMember("listVersion", getListVersion(), allocator);
    outMap.AddMember("minimumInclusiveIndex", Object(mMinimumInclusiveIndex).asInt(), allocator);
    outMap.AddMember("maximumExclusiveIndex", Object(mMaximumExclusiveIndex).asInt(), allocator);
    outMap.AddMember("startIndex", Object(mOffset).asInt(), allocator);
}