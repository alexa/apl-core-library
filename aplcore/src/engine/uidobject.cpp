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

#include "apl/engine/context.h"
#include "apl/engine/uidobject.h"
#include "apl/engine/uidmanager.h"

namespace apl {

UIDObject::UIDObject(const ContextPtr& context)
    : mUniqueId(context->uniqueIdManager().create(this)),
      mContext(context)
{}

UIDObject::~UIDObject()
{
    mContext->uniqueIdManager().remove(mUniqueId, this);
}

rapidjson::Value
UIDObject::serializeContext(int depth, rapidjson::Document::AllocatorType& allocator)
{
    rapidjson::Value array(rapidjson::kArrayType);

    if (depth == 0) { // All but the top context
        for (auto context = mContext; context->parent(); context = context->parent())
            array.PushBack(context->serialize(allocator), allocator);
    }
    else {
        for (auto context = mContext; context && depth > 0; context = context->parent(), --depth)
            array.PushBack(context->serialize(allocator), allocator);
    }
    return array;
}

} // namespace apl