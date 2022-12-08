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
    Object element;

    auto type = data->getType();
    if (type == LiveObject::ObjectType::kArrayType)
        element = std::make_shared<LiveArrayObject>(std::static_pointer_cast<LiveArray>(data), context, key);
    else if (type == LiveObject::ObjectType::kMapType)
        element = std::make_shared<LiveMapObject>(std::static_pointer_cast<LiveMap>(data), context, key);
    else {
        LOG(LogLevel::kError).session(context) << "Unexpected data type for live object key='" << key << "': " << static_cast<int>(data->getType());
        return nullptr;
    }

    context->putSystemWriteable(key, element);
    context->dataManager().add(element.getLiveDataObject());
    return element.getLiveDataObject();
}

void
LiveDataObject::markDirty()
{
    auto context = mContext.lock();
    if (context)
        context->dataManager().markDirty(shared_from_this());
}

void
LiveDataObject::flush()
{
    auto context = mContext.lock();
    mIsFlushing = true;
    if (context)
        context->recalculateDownstream(mKey, true);

    // Make a copy to ensure sane iteration because it's possible that calling a callback will add more callbacks
    std::map<int, FlushCallback> flushCallbacksCopy{mFlushCallbacks};
    for (const auto& m : flushCallbacksCopy) {
        // Enforce the rule that we should not call any callbacks that have been added during this overall flush cycle.
        // See LiveDataObject::preFlush() for more details.
        //
        // Note that this rule is still necessary despite making a copy of this object's flush callbacks because other
        // live data object flushes can also result in adding callbacks to this object's map.
        if (m.first < mMaxWatcherTokenBeforeFlush)
            m.second(mKey, *this);
    }

    mReplaced = false;
    mIsFlushing = false;
}

int
LiveDataObject::addFlushCallback(FlushCallback&& callback)
{
    int token = mWatcherToken++;
    mFlushCallbacks.emplace(token, std::move(callback));
    return token;
}

void
LiveDataObject::removeFlushCallback(int token)
{
    mFlushCallbacks.erase(mFlushCallbacks.find(token));
}

} // namespace apl
