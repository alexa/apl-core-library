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

#include "apl/datasource/dynamiclistdatasourceprovider.h"
#include "apl/datasource/datasource.h"
#include "apl/time/timemanager.h"

using namespace apl;
using namespace DynamicListConstants;


DynamicListDataSourceConnection::DynamicListDataSourceConnection(
    std::weak_ptr<Context> context,
    const std::string& listId,
    DLProviderWPtr provider,
    const DynamicListConfiguration& configuration,
    std::weak_ptr<LiveArray> liveArray,
    size_t offset,
    size_t maxItems) :
    OffsetIndexDataSourceConnection(std::move(liveArray), offset, maxItems, configuration.cacheChunkSize),
    mContext(std::move(context)),
    mConfiguration(configuration),
    mListId(listId),
    mProvider(std::move(provider)),
    mListVersion(0) {}

void
DynamicListDataSourceConnection::enqueueFetchRequestEvent(const ContextPtr& context, const ObjectMapPtr& request) {
    EventBag bag;
    bag.emplace(kEventPropertyName, mConfiguration.type);
    bag.emplace(kEventPropertyValue, request);
    context->pushEvent(Event(kEventTypeDataSourceFetchRequest, std::move(bag)));
}

bool
DynamicListDataSourceConnection::retryFetchRequest(const std::string& correlationToken) {
    auto context = mContext.lock();
    auto provider = mProvider.lock();

    if (!context || !provider || correlationToken.empty())
        return false;

    auto liveArray = mLiveArray.lock();
    if (!liveArray || liveArray->size() == mMaxItems)
        return false;

    std::string newToken = correlationToken;
    auto it = mPendingFetchRequests.find(correlationToken);
    if (it == mPendingFetchRequests.end())
        return false;

    auto pendingRequest = it->second;
    pendingRequest->retries--;
    context->getRootConfig().getTimeManager()->clearTimeout(pendingRequest->timeoutId);

    // Replace token with new one as it should be unique.
    newToken = std::to_string(provider->getAndIncrementCorrelationToken());
    (*pendingRequest->request)[CORRELATION_TOKEN] = newToken;
    pendingRequest->relatedTokens.emplace_back(newToken);

    enqueueFetchRequestEvent(context, pendingRequest->request);

    if (pendingRequest->retries > 0) {
        // Reset timeout
        pendingRequest->timeoutId = scheduleTimeout(newToken);
        mPendingFetchRequests.emplace(newToken, pendingRequest);
    } else {
        // Clean up all related
        for (auto& token : pendingRequest->relatedTokens) {
            mPendingFetchRequests.erase(token);
        }
    }
    return true;
}

timeout_id
DynamicListDataSourceConnection::scheduleTimeout(const std::string& correlationToken) {
    auto context = mContext.lock();
    if (!context)
        return 0;

    auto timeManager = context->getRootConfig().getTimeManager();
    std::weak_ptr<DynamicListDataSourceConnection> weak_ptr(shared_from_this());
    std::weak_ptr<Context> weak_ctx(context);
    return timeManager->setTimeout([weak_ptr, weak_ctx, correlationToken]() {
      auto self = weak_ptr.lock();
      auto ctx = weak_ctx.lock();
      if (!self || !ctx)
          return;

      if (self->retryFetchRequest(correlationToken)) {
          self->constructAndReportError(ERROR_REASON_LOAD_TIMEOUT, Object::NULL_OBJECT(),
                                        "Retrying timed out request: " + correlationToken);
      }
    }, mConfiguration.fetchTimeout);
}

void
DynamicListDataSourceConnection::clearTimeouts(const ContextPtr& context, const std::string& correlationToken) {
    if (!context || correlationToken.empty())
        return;
    auto it = mPendingFetchRequests.find(correlationToken);
    if (it != mPendingFetchRequests.end()) {
        context->getRootConfig().getTimeManager()->clearTimeout(it->second->timeoutId);
        for (auto& token : it->second->relatedTokens) {
            mPendingFetchRequests.erase(token);
        }
    }
}

void
DynamicListDataSourceConnection::sendFetchRequest(const ObjectMap& requestData) {
    auto context = mContext.lock();
    auto provider = mProvider.lock();

    if (!context || !provider)
        return;

    for (const auto& request : mPendingFetchRequests) {
        auto requestMap = request.second->request;
        if (requestMap->at(LIST_ID) != mListId)
            continue;

        std::weak_ptr<ObjectMap> weak_request(requestMap);
        auto allMatch = std::all_of(requestData.begin(), requestData.end(),
                                    [weak_request](std::pair<std::string, Object> const& i) {
                                      auto request = weak_request.lock();
                                      return request->at(i.first) == i.second;
                                    });

        // If request such as that already in the go - ignore.
        if (allMatch)
            return;
    }

    int requestToken = provider->getAndIncrementCorrelationToken();

    std::string correlationToken = std::to_string(requestToken);
    ObjectMap request;
    request.emplace(CORRELATION_TOKEN, correlationToken);
    request.emplace(LIST_ID, mListId);

    request.insert(requestData.begin(), requestData.end());

    auto requestMap = std::make_shared<ObjectMap>(request);
    auto timeoutId = scheduleTimeout(correlationToken);

    enqueueFetchRequestEvent(context, requestMap);

    PendingFetchRequest pendingFetchRequest = {requestMap, mConfiguration.fetchRetries, timeoutId,
                                               {correlationToken}};
    mPendingFetchRequests.emplace(correlationToken, std::make_shared<PendingFetchRequest>(pendingFetchRequest));
}

timeout_id
DynamicListDataSourceConnection::scheduleUpdateExpiry(int version) {
    auto context = mContext.lock();
    if (context) {
        // Schedule "expiry" of list version cache
        auto timeManager = context->getRootConfig().getTimeManager();
        std::weak_ptr<DynamicListDataSourceConnection> weak_ptr(shared_from_this());

        return timeManager->setTimeout([weak_ptr, version]() {
          auto self = weak_ptr.lock();
          if (!self)
              return;

          self->reportUpdateExpired(version);
        }, mConfiguration.cacheExpiryTimeout);
    }
    return 0;
}

void
DynamicListDataSourceConnection::reportUpdateExpired(int version) {
    auto context = mContext.lock();

    if (!context)
        return;

    auto it = mUpdatesCache.find(version);
    if (it != mUpdatesCache.end()) {
        context->getRootConfig().getTimeManager()->clearTimeout(it->second.expiryTimeout);
        constructAndReportError(ERROR_REASON_MISSING_LIST_VERSION, Object::NULL_OBJECT(),
                                "Update to version " + std::to_string(version + 1) + " buffered longer than expected.");
    }
}

void
DynamicListDataSourceConnection::putCacheUpdate(int version, const Object& payload) {
    auto provider = mProvider.lock();
    if (!provider) {
        // Provider dead. Should not happen.
        LOG(LogLevel::kError) << "DataSource provider for " << mConfiguration.type
                             << " is dead while trying to process update cache.";
        return;
    }

    if (mUpdatesCache.size() >= mConfiguration.listUpdateBufferSize) {
        // Remove highest or discard current one if it's one.
        constructAndReportError(ERROR_REASON_MISSING_LIST_VERSION, Object::NULL_OBJECT(),
                                "Too many updates buffered. Discarding highest version.");
        auto it = mUpdatesCache.rbegin();
        if (it->first > version) {
            retrieveCachedUpdate(it->first);
        } else {
            return;
        }
    }

    if (!mUpdatesCache.count(version)) {
        auto timeoutId = scheduleUpdateExpiry(version);
        Update update = {payload, timeoutId};
        mUpdatesCache.emplace(version, update);
    } else {
        constructAndReportError(ERROR_REASON_DUPLICATE_LIST_VERSION, Object::NULL_OBJECT(),
                                "Trying to cache existing list version.");
    }
}

Object
DynamicListDataSourceConnection::retrieveCachedUpdate(int version) {
    auto it = mUpdatesCache.find(version);
    if (it != mUpdatesCache.end()) {
        auto context = mContext.lock();
        if (context) {
            context->getRootConfig().getTimeManager()->clearTimeout(it->second.expiryTimeout);
        }

        Object payload = it->second.update;
        mUpdatesCache.erase(version);

        return payload;
    }
    return Object::NULL_OBJECT();
}

void
DynamicListDataSourceConnection::constructAndReportError(
    const std::string& reason,
    const Object& operationIndex,
    const std::string& message) {
    auto provider = mProvider.lock();
    if (!provider) {
        // Provider dead. Should not happen.
        LOG(LogLevel::kError) << "DataSource provider for " << mConfiguration.type
                             << " is dead while trying to report error.";
        return;
    }

    provider->constructAndReportError(reason, shared_from_this(), operationIndex, message);
}

DynamicListDataSourceProvider::DynamicListDataSourceProvider(const DynamicListConfiguration& config)
    : mConfiguration(config), mRequestToken(STARTING_REQUEST_TOKEN) {}

std::shared_ptr<DataSourceConnection>
DynamicListDataSourceProvider::create(
    const Object& sourceDefinition,
    std::weak_ptr<Context> context,
    std::weak_ptr<LiveArray> liveArray) {
    clearStaleConnections();
    if (!sourceDefinition.has(LIST_ID) || !sourceDefinition.get(LIST_ID).isString()) {
        constructAndReportError(ERROR_REASON_INTERNAL_ERROR, "N/A", "Missing required fields.");
        return nullptr;
    }

    auto listId = sourceDefinition.get(LIST_ID).getString();
    auto existingConnection = getConnection(listId);
    if (existingConnection) {
        // this datasource allows for data reuse on reinflate.
        auto contextPtr = context.lock();
        if (contextPtr && contextPtr->getReinflationFlag()) {
            return existingConnection;
        }
        // Trying to reuse existing listId/DataSource. Should not happen.
        constructAndReportError(ERROR_REASON_INTERNAL_ERROR, listId, "Trying to reuse existing listId.");
        return nullptr;
    }

    auto connection = createConnection(sourceDefinition, context, liveArray, listId);

    mConnections.emplace(listId, connection);
    return connection;
}

bool
DynamicListDataSourceProvider::processUpdate(const Object &payload) {
    clearStaleConnections();

    auto result = false;
    if (payload.isString()) {
        rapidjson::Document doc;
        rapidjson::ParseResult parseResult =
                doc.Parse<rapidjson::kParseValidateEncodingFlag | rapidjson::kParseStopWhenDoneFlag>(
                        payload.asString().c_str());

        if (parseResult.IsError()) {
            LOG(LogLevel::kError) << "Failed to parse update JSON: "
                << std::string(rapidjson::GetParseError_En(parseResult.Code()));
            return false;
        }
        result = process(Object(std::move(doc)));
    } else if (payload.isMap()) {
        result = process(payload);
    } else {
        constructAndReportError(ERROR_REASON_INTERNAL_ERROR, "N/A", "Can't process payload.");
    }

    return result;
}

void
DynamicListDataSourceProvider::clearStaleConnections() {
    auto cIt = mConnections.cbegin();
    while (cIt != mConnections.cend()) {
        if (cIt->second.expired()) {
            // Link is dead. Clean it up.
            cIt = mConnections.erase(cIt);
        } else {
            ++cIt;
        }
    }
}

DLConnectionPtr
DynamicListDataSourceProvider::getConnection(const std::string& listId) {
    auto connectionIter = mConnections.find(listId);
    if (connectionIter == mConnections.end()) {
        // Non-existing
        return nullptr;
    }
    auto connection = connectionIter->second.lock();
    if (!connection)
        return nullptr;

    return connection;
}

DLConnectionPtr
DynamicListDataSourceProvider::getConnection(const std::string& listId, const Object& correlationToken) {
    bool hasValidListId = true;
    auto connectionIter = mConnections.find(listId);
    if (connectionIter == mConnections.end()) {
        hasValidListId = false;

        // Non-existing. Give it a chance to figure out by correlationToken.
        if (!correlationToken.isNull()) {
            connectionIter = mConnections.begin();
            while (connectionIter != mConnections.end()) {
                    auto connection = connectionIter->second.lock();
                    if (connection && connection->canProcess(correlationToken)) {
                        break;
                    }
                    connectionIter++;
                }
            }
    }

    if (!hasValidListId && connectionIter != mConnections.end()) {
        constructAndReportError(ERROR_REASON_INCONSISTENT_LIST_ID, listId, "Non-existing listId.");
    } else if (connectionIter == mConnections.end()) {
        constructAndReportError(ERROR_REASON_INVALID_LIST_ID, listId, "Unexpected response.");
        return nullptr;
    }

    auto connection = connectionIter->second.lock();
    if (!connection) {
        // Link is dead. Clean it up. Unlikely to happen but clean anyway.
        mConnections.erase(connectionIter);
        constructAndReportError(ERROR_REASON_INTERNAL_ERROR, listId, "DataSource context lost.");
        return nullptr;
    }

    return connection;
}

Object
DynamicListDataSourceProvider::getPendingErrors() {
    Object result = Object(std::make_shared<ObjectArray>(mPendingErrors));
    mPendingErrors.clear();
    return result;
}

void
DynamicListDataSourceProvider::constructAndReportError(
    const std::string& reason,
    const std::string& listId,
    const Object& listVersion,
    const Object& operationIndex,
    const std::string& message) {
    auto error = std::make_shared<ObjectMap>();

    error->emplace(ERROR_TYPE, ERROR_TYPE_LIST_ERROR);
    error->emplace(ERROR_REASON, reason);
    error->emplace(LIST_ID, listId);
    if (listVersion.isNumber()) {
        error->emplace(LIST_VERSION, listVersion);
    }
    if (operationIndex.isNumber()) {
        error->emplace(ERROR_OPERATION_INDEX, operationIndex);
    }
    error->emplace(ERROR_MESSAGE, message);

    mPendingErrors.emplace_back(std::move(error));
    // Throw errors into log to help debugging on device
    LOG(LogLevel::kWarn) << "Datasource " << listId << "; Error: " << message;
}

void
DynamicListDataSourceProvider::constructAndReportError(
    const std::string& reason,
    const std::string& listId,
    const std::string& message) {
    constructAndReportError(reason, listId, Object::NULL_OBJECT(), Object::NULL_OBJECT(), message);
}

void
DynamicListDataSourceProvider::constructAndReportError(
    const std::string& reason,
    const DLConnectionPtr& connection,
    const Object& operationIndex,
    const std::string& message) {
    constructAndReportError(reason, connection->getListId(), connection->getListVersion(), operationIndex, message);
}

bool
DynamicListDataSourceProvider::canFetch(const Object& correlationToken, const DLConnectionPtr& connection) {
    if (correlationToken.isNull() || connection->canProcess(correlationToken.asString()))
        return true;

    int tokenNumber = correlationToken.asInt();
    if (tokenNumber < mRequestToken && tokenNumber > STARTING_REQUEST_TOKEN) {
        // Most likely duplicate response because of timeout. Skill can't really track it so just exit.
        return false;
    }

    constructAndReportError(ERROR_REASON_INTERNAL_ERROR, connection, Object::NULL_OBJECT(), "Wrong correlation token.");
    return false;
}