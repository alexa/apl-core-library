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

#include "apl/engine/evaluate.h"
#include "apl/engine/arrayify.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"
#include "apl/primitives/object.h"
#include "apl/datasource/datasourceprovider.h"
#include "apl/datasource/datasource.h"
#include "apl/content/rootconfig.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/livedata/livedatamanager.h"

namespace apl {

std::string
DataSource::toDebugString() const
{
    std::string result;
    result += "{";
    result += LiveArrayObject::toDebugString();
    result += "}";

    return "DataSource<"+result+">";
}

Object
DataSource::create(const ContextPtr& context, const Object& object, const std::string& name)
{
    if (object.getLiveDataObject())
        return object;

    if (!object.isMap())
        return Object::NULL_OBJECT();

    std::string type = propertyAsString(*context, object, "type");
    if (type.empty()) {
        CONSOLE_CTP(context) << "Unrecognized type field in DataSource";
        return Object::NULL_OBJECT();
    }

    auto dataSourceProvider = context->getRootConfig().getDataSourceProvider(type);
    if(!dataSourceProvider) {
        CONSOLE_CTP(context) << "Unrecognized DataSource type";
        return Object::NULL_OBJECT();
    }

    // If no items specified (even empty ones) - propertyAsRecursive + arrayify() will give us [NULL] which is an item.
    // Check for this condition and go with empty LiveArray instead to avoid creating empty one.
    auto items = propertyAsRecursive(*context, object, "items");
    auto liveDataSourceArray = items.isNull() ? LiveArray::create() : LiveArray::create(arrayify(*context, items));
    auto dataSourceConnection = dataSourceProvider->create(object, context, liveDataSourceArray);
    if (!dataSourceConnection) {
        CONSOLE_CTP(context) << "DataSourceConnection failed to initialize.";
        return Object::NULL_OBJECT();
    }
    liveDataSourceArray = dataSourceConnection->getLiveArray();
    auto dataSource = std::make_shared<DataSource>(
            liveDataSourceArray,
            context,
            dataSourceConnection,
            name);
    context->dataManager().add(dataSource);

    // If provided with empty initial array - ask for items straight away.
    if (liveDataSourceArray->empty()) {
        dataSource->ensure(0);
    }

    return Object(dataSource);
}

DataSource::DataSource(
        const LiveArrayPtr& liveArray,
        const ContextPtr& context,
        const DataSourceConnectionPtr& connection,
        const std::string& name) :
        LiveArrayObject(liveArray, context, name),
        mSourceConnection(connection) {}

void
DataSource::ensure(size_t idx) {
    // We know that data is here as we only can call it on existing component.
    // This just allows us to fetch more if deemed appropriate.
    mSourceConnection->ensure(idx);
}

}  // namespace apl