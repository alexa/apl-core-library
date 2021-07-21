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

#include "apl/livedata/livedataobject.h"
#include "apl/engine/context.h"
#include "apl/livedata/livedatamanager.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/livedata/livemapobject.h"
#include "apl/livedata/livearray.h"
#include "apl/livedata/livemap.h"

namespace apl {

std::shared_ptr<LiveDataObject>
LiveDataObject::create(const LiveObjectPtr& data,
                       const ContextPtr& context,
                       const std::string& key)
{
    std::shared_ptr<LiveDataObject> element;

    auto type = data->getType();
    if (type == Object::kArrayType)
        element = std::make_shared<LiveArrayObject>(std::static_pointer_cast<LiveArray>(data), context, key);
    else if (type == Object::kMapType)
        element = std::make_shared<LiveMapObject>(std::static_pointer_cast<LiveMap>(data), context, key);
    else {
        LOG(LogLevel::kError) << "Unexpected data type for live object key='" << key << "': " << data->getType();
        return nullptr;
    }

    context->putSystemWriteable(key, Object(element));
    context->dataManager().add(element);
    return element;
}

void LiveDataObject::markDirty()
{
    auto context = mContext.lock();
    if (context)
        context->dataManager().markDirty(shared_from_this());
}

} // namespace apl