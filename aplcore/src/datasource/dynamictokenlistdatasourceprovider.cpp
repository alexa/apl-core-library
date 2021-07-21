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

#include "apl/datasource/dynamictokenlistdatasourceprovider.h"
#include "apl/datasource/datasource.h"
#include "apl/engine/evaluate.h"
#include "apl/time/timemanager.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace apl;
using namespace DynamicTokenListConstants;
using namespace DynamicListConstants;

DynamicTokenListDataSourceConnection::DynamicTokenListDataSourceConnection(
    DTLProviderWPtr provider,
    const DynamicListConfiguration& configuration,
    std::weak_ptr<Context> context,
    std::weak_ptr<LiveArray> liveArray,
    const std::string& listId,
    const Object& firstToken,
    const Object& lastToken) :
      DynamicListDataSourceConnection(
          std::move(context),
          listId,
          std::move(provider),
          configuration,
          std::move(liveArray),
          0,
          SIZE_MAX),
      mFirstToken(firstToken),
      mLastToken(lastToken) {}

bool
DynamicTokenListDataSourceConnection::processLazyLoad(
    const Object& data,
    const Object& pageToken,
    const Object& nextPageToken,
    const Object& correlationToken) {
    auto context = mContext.lock();
    if (!context) {
        return false;
    }
    auto items = evaluateRecursive(*context, data);

    bool result = false;
    if (items.isArray() && !items.empty()) {
        const std::vector<Object>& dataArray(items.getArray());

        result = updateLiveArray(dataArray, pageToken, nextPageToken);
    }
    else {
        constructAndReportError(ERROR_REASON_MISSING_LIST_ITEMS, Object::NULL_OBJECT(),
                                          "No items provided to load.");
        retryFetchRequest(correlationToken.asString());
        return result;
    }

    if (result)
        clearTimeouts(context, correlationToken.asString());

    return result;
}

bool
DynamicTokenListDataSourceConnection::updateLiveArray(
    const std::vector<Object>& data,
    const Object& pageToken,
    const Object& nextPageToken) {
    LiveArrayPtr liveArray = mLiveArray.lock();
    if (!liveArray || data.empty()) return false;

    size_t baseSize = liveArray->size();
    if (mMaxItems < baseSize || mMaxItems - baseSize < data.size()) return false;

    auto pageTokenString = pageToken.asString();
    if (pageTokenString == mLastToken.asString()) {
        liveArray->insert(baseSize, data.begin(), data.end());
        mLastToken = nextPageToken;
    }

    if (pageTokenString == mFirstToken.asString()) {
        liveArray->insert(0, data.begin(), data.end());
        mFirstToken = nextPageToken;
    }

    return true;
}

void
DynamicTokenListDataSourceConnection::ensure(size_t index) {
    LiveArrayPtr liveArray = mLiveArray.lock();
    if (!liveArray) return;

    size_t baseSize = liveArray->size();
    if (baseSize >= mMaxItems) return;

    if (index + mConfiguration.cacheChunkSize > baseSize && !mLastToken.isNull()) {
        sendFetchRequest(ObjectMap{{PAGE_TOKEN, mLastToken}});
    }

    if (index < mConfiguration.cacheChunkSize && !mFirstToken.isNull()) {
        sendFetchRequest(ObjectMap{{PAGE_TOKEN, mFirstToken}});
    }
}

DynamicTokenListDataSourceProvider::DynamicTokenListDataSourceProvider(
    const DynamicListConfiguration& config)
    : DynamicListDataSourceProvider(config) {}

DynamicTokenListDataSourceProvider::DynamicTokenListDataSourceProvider()
    : DynamicTokenListDataSourceProvider(DynamicListConfiguration(DEFAULT_TYPE_NAME)) {}

std::shared_ptr<DynamicListDataSourceConnection>
DynamicTokenListDataSourceProvider::createConnection(
    const Object& sourceDefinition,
    std::weak_ptr<Context> context,
    std::weak_ptr<LiveArray> liveArray,
    const std::string& listId) {
    auto sourceMap = sourceDefinition.getMap();

    if (!sourceDefinition.has(LIST_ID) || !sourceDefinition.get(LIST_ID).isString()||
        !sourceDefinition.has(PAGE_TOKEN) || !sourceDefinition.get(PAGE_TOKEN).isString()) {
        constructAndReportError(ERROR_REASON_INTERNAL_ERROR, "N/A", "Missing required fields.");
        return nullptr;
    }

    auto backwardPageToken = sourceDefinition.get(BACKWARD_PAGE_TOKEN);
    auto forwardPageToken = sourceDefinition.get(FORWARD_PAGE_TOKEN);

    return std::make_shared<DynamicTokenListDataSourceConnection>(
        shared_from_this(), mConfiguration, context, liveArray, listId, backwardPageToken,
        forwardPageToken);
}

bool
DynamicTokenListDataSourceProvider::processLazyLoadInternal(
    const DTLConnectionPtr& connection,
    const Object& responseMap) {
    auto correlationToken = responseMap.get(CORRELATION_TOKEN);

    if (!canFetch(correlationToken, connection))
        return false;

    if (!responseMap.has(ITEMS)) {
        constructAndReportError(ERROR_REASON_INTERNAL_ERROR, connection->getListId(),
                                "Missing required fields.");
        return true;
    }

    auto items = responseMap.get(ITEMS);
    auto pageToken = responseMap.get(PAGE_TOKEN);
    auto nextPageToken = responseMap.get(NEXT_PAGE_TOKEN);
    return connection->processLazyLoad(items, pageToken, nextPageToken, correlationToken);
}

bool
DynamicTokenListDataSourceProvider::process(const Object& responseMap) {
    if (!responseMap.has(LIST_ID) || !responseMap.get(LIST_ID).isString()) {
        constructAndReportError(ERROR_REASON_INVALID_LIST_ID, "N/A", "Missing listId.");
        return false;
    }

    std::string listId = responseMap.get(LIST_ID).getString();
    auto dataSourceConnection = getConnection(listId, responseMap.get(CORRELATION_TOKEN));

    if(!dataSourceConnection)
        return false;

    auto connection = std::dynamic_pointer_cast<DynamicTokenListDataSourceConnection>(dataSourceConnection);

    bool result = processLazyLoadInternal(connection, responseMap);
    auto context = connection->getContext();
    if (result && context != nullptr)
        context->setDirtyDataSourceContext(
            std::dynamic_pointer_cast<DataSourceConnection>(shared_from_this()));

    return result;
}

void
DynamicTokenListDataSourceConnection::serialize(rapidjson::Value& outMap,
                                                rapidjson::Document::AllocatorType& allocator) {
    outMap.AddMember("type", rapidjson::Value(DEFAULT_TYPE_NAME.c_str(), allocator).Move(), allocator);
    outMap.AddMember("listId", rapidjson::Value(DynamicListDataSourceConnection::getListId().c_str(), allocator).Move(), allocator);

    outMap.AddMember("backwardPageToken", rapidjson::Value(mFirstToken.asString().c_str(), allocator).Move(), allocator);
    outMap.AddMember("forwardPageToken", rapidjson::Value(mLastToken.asString().c_str(), allocator).Move(), allocator);
}