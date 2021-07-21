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

#ifndef _APL_DATASOURCE_H
#define _APL_DATASOURCE_H

#include <string>

#include "apl/datasource/datasourceconnection.h"
#include "apl/primitives/object.h"
#include "apl/livedata/livearrayobject.h"

namespace apl {

class Context;

class DataSource : public LiveArrayObject {
public:
    /**
     * Create DataSource object from provided object.
     * @param context data binding context.
     * @param object object to create from.
     * @return DataSource object or NULL_OBJECT if failed.
     */
    static Object create(const ContextPtr& context, const Object& object, const std::string& name);

    /// LiveArrayObject override
    void ensure(size_t idx) override;
    bool isPaginating() const override { return true; }

    /**
     * Internal constructor, use create() function instead.
     */
    DataSource(
            const LiveArrayPtr& liveArray,
            const ContextPtr& context,
            const DataSourceConnectionPtr& connection,
            const std::string& name);

    std::string toDebugString() const override;

    DataSourceConnectionPtr getDataSourceConnection() const override { return mSourceConnection; }

private:
    DataSourceConnectionPtr mSourceConnection;
};

} // namespace apl

#endif //_APL_DATASOURCE_H
