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

#ifndef _APL_DATA_SOURCE_H
#define _APL_DATA_SOURCE_H

#include <memory>

#include "apl/primitives/object.h"
#include "apl/livedata/livearray.h"
#include "apl/utils/noncopyable.h"

namespace apl {

class DataSourceConnection;

class DataSourceProvider : public NonCopyable {
public:
    virtual ~DataSourceProvider() = default;

    /**
     * Create DataSource connection.
     *
     * @param dataSourceDefinition metadata required for connection.
     * @param context owning context.
     * @param liveArray pointer to base LiveArray.
     * @return connection if succeeded, nullptr otherwise.
     */
    virtual std::shared_ptr<DataSourceConnection> create(const Object& dataSourceDefinition, std::weak_ptr<Context>, std::weak_ptr<LiveArray> liveArray) = 0;

    /**
     * Parse update payload and pass it to relevant connection.
     * @param payload update payload.
     * @return true if successful, false otherwise.
     */
    virtual bool processUpdate(const Object& payload) { return false; }

    /**
     * Retrieve any errors pending.
     * @return Object containing array of errors.
     *
     */
    virtual Object getPendingErrors() { return Object::EMPTY_ARRAY(); }
};

} // namespace apl

#endif //_APL_DATA_SOURCE_H
