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

#ifndef _APL_EXECUTION_RESOURCE_H
#define _APL_EXECUTION_RESOURCE_H

#include "apl/common.h"
#include "apl/component/componentproperties.h"

namespace apl {

enum ExecutionResourceKey {
    kExecutionResourceForegroundAudio,
    kExecutionResourceBackgroundAudio,
    kExecutionResourcePosition,
    kExecutionResourceProperty,
};

extern Bimap<int, std::string> sExecutionResourceMap;

class ExecutionResource {
public:
    ExecutionResource(ExecutionResourceKey key, const ComponentPtr& component = nullptr, PropertyKey propKey = kPropertyId);

    bool operator<(const ExecutionResource& other) const { return mResourceId < other.mResourceId; }
    std::string toString() const { return mResourceId; }

private:
    static std::string constructResourceId(ExecutionResourceKey key, const ComponentPtr& component, PropertyKey propKey);

    std::string mResourceId;
};

} // namespace apl

#endif //_APL_EXECUTION_RESOURCE_H
