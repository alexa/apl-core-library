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

#include "apl/time/executionresource.h"
#include "apl/component/component.h"

namespace apl {

Bimap<int, std::string> sExecutionResourceMap = {
    {kExecutionResourceForegroundAudio, "foregroundAudio"},
    {kExecutionResourceBackgroundAudio, "backgroundAudio"},
    {kExecutionResourcePosition,        "position"},
    {kExecutionResourceProperty,        "property"},
};

std::string
ExecutionResource::constructResourceId(ExecutionResourceKey key, const ComponentPtr& component, PropertyKey propKey)
{
    std::string resourceId;
    auto resourceString = sExecutionResourceMap.get(key, "");
    if (resourceString.empty()) {
        // Should not happen
        assert(false);
        return resourceId;
    }

    if (component) {
        resourceId += component->getUniqueId();
        resourceId += (":" + std::to_string(propKey));
    }

    return resourceId;
}

ExecutionResource::ExecutionResource(ExecutionResourceKey key, const ComponentPtr& component, PropertyKey propKey)
{
    mResourceId = constructResourceId(key, component, propKey);
}

} // namespace apl
