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

#ifndef _APL_DYNAMIC_LIST_DATA_SOURCE_COMMON_H
#define _APL_DYNAMIC_LIST_DATA_SOURCE_COMMON_H

#include "apl/apl.h"

namespace apl {

namespace DynamicListConstants {
/// Semi-magic number to seed correlation tokens
static const int STARTING_REQUEST_TOKEN = 100;
/// Number of data items to cache on Lazy Loading fetch requests.
static const size_t DEFAULT_CACHE_CHUNK_SIZE = 10;
/// Number of retries to attempt on fetch requests.
static const int DEFAULT_FETCH_RETRIES = 2;
/// Fetch request timeout.
static const int DEFAULT_FETCH_TIMEOUT_MS = 5000;
/// Maximum number of directives to buffer in case of unbound receival. Arbitrary number but considers highly unlikely
/// occurrence and capability to recover.
static const int DEFAULT_MAX_LIST_UPDATE_BUFFER = 5;
/// Cache expiry timeout.
static const int DEFAULT_CACHE_EXPIRY_TIMEOUT_MS = 5000;

// Directive content keys
static const std::string LIST_ID = "listId";
static const std::string LIST_VERSION = "listVersion";
static const std::string CORRELATION_TOKEN = "correlationToken";
static const std::string ITEMS = "items";

// Error content definitions.
static const std::string ERROR_TYPE = "type";
static const std::string ERROR_TYPE_LIST_ERROR = "LIST_ERROR";
static const std::string ERROR_REASON = "reason";
static const std::string ERROR_OPERATION_INDEX = "operationIndex";
static const std::string ERROR_MESSAGE = "message";

static const std::string ERROR_REASON_DUPLICATE_LIST_VERSION = "DUPLICATE_LIST_VERSION";
static const std::string ERROR_REASON_INVALID_LIST_ID = "INVALID_LIST_ID";
static const std::string ERROR_REASON_INCONSISTENT_LIST_ID = "INCONSISTENT_LIST_ID";
static const std::string ERROR_REASON_LOAD_TIMEOUT = "LOAD_TIMEOUT";
static const std::string ERROR_REASON_MISSING_LIST_ITEMS = "MISSING_LIST_ITEMS";
static const std::string ERROR_REASON_MISSING_LIST_VERSION = "MISSING_LIST_VERSION";
static const std::string ERROR_REASON_INTERNAL_ERROR = "INTERNAL_ERROR";
} // DynamicListConstants

/**
 * Simple configuration object
 */
class DynamicListConfiguration {
public:
    explicit DynamicListConfiguration(const std::string& type) : type(type) {}

    // Backward compatibility constructor
    DynamicListConfiguration(const std::string& type, size_t cacheChunkSize) :
          type(type), cacheChunkSize(cacheChunkSize) {}

    // Link setters
    DynamicListConfiguration& setType(const std::string& v) { type = v; return *this; }
    DynamicListConfiguration& setCacheChunkSize(size_t v) { cacheChunkSize = v; return *this; }
    DynamicListConfiguration& setListUpdateBufferSize(int v) { listUpdateBufferSize = v; return *this; }
    DynamicListConfiguration& setFetchRetries(int v) { fetchRetries = v; return *this; }
    DynamicListConfiguration& setFetchTimeout(apl_duration_t v) { fetchTimeout = v; return *this; }
    DynamicListConfiguration& setCacheExpiryTimeout(apl_duration_t v) { cacheExpiryTimeout = v; return *this; }

    /// Source type name.
    std::string type;
    /// Fetch cache chunk size.
    size_t cacheChunkSize = DynamicListConstants::DEFAULT_CACHE_CHUNK_SIZE;
    /// Number of retries for fetch requests.
    int fetchRetries = DynamicListConstants::DEFAULT_FETCH_RETRIES;
    /// Fetch request timeout in milliseconds.
    apl_duration_t fetchTimeout = DynamicListConstants::DEFAULT_FETCH_TIMEOUT_MS;
    /// Size of the list for buffered update operations.
    int listUpdateBufferSize = DynamicListConstants::DEFAULT_MAX_LIST_UPDATE_BUFFER;
    /// Cached updates expiry timeout in milliseconds.
    apl_duration_t cacheExpiryTimeout = DynamicListConstants::DEFAULT_CACHE_EXPIRY_TIMEOUT_MS;
};

} // namespace apl

#endif //_APL_DYNAMIC_LIST_DATA_SOURCE_COMMON_H
