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

#ifndef _APL_DATA_SOURCE_CONNECTION_H
#define _APL_DATA_SOURCE_CONNECTION_H

#include <memory>

#include "rapidjson/document.h"

#include "apl/common.h"
#include "apl/utils/counter.h"

namespace apl {

class LiveArray;
/**
 * Dynamic DataSource connection. Aimed to provide fetch and update interface to particular data set.
 */
class DataSourceConnection : public Counter<DataSourceConnection> {
public:
    virtual ~DataSourceConnection() = default;

    /**
     * Ensure that source knows that index is in use. Could trigger fetching of more items. It's up to source
     * implementation to decide if it's needed but we recommend keeping reasonable buffer (for example equal to
     * initial array size) around to allow for faster scrolling/etc.
     *
     * @param index index to fetch items around.
     */
    virtual void ensure(size_t index) = 0;

    /**
     * LiveArray is owned and maintained by each data source connection. It is dynamically updated during runtime.
     * LiveArray may be retrieved for reuse if preservation of dynamically loaded data is required (for example during reinflate).
     *
     * @return LiveArray used as data storage for this connection.
     */
    virtual std::shared_ptr<LiveArray> getLiveArray() = 0;

    /**
     * Retrieve datasource context as a JSON object. Should be called by RootContext->serializeDatasourceContext()
     */
    virtual void serialize(rapidjson::Value& outMap,
                           rapidjson::Document::AllocatorType& allocator) = 0;
};

} // namespace apl

#endif //_APL_DATA_SOURCE_H
