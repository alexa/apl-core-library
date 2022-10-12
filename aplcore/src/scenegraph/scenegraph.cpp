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

#include "apl/scenegraph/scenegraph.h"
#include "apl/scenegraph/layer.h"

namespace apl {
namespace sg {

rapidjson::Value
SceneGraph::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    if (mTopLayer)
        return mTopLayer->serialize(allocator);

    return rapidjson::Value(rapidjson::kObjectType);
}

} // namespace sg
} // namespace apl
