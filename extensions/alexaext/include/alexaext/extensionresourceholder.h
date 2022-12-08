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

#ifndef APL_EXTENSION_RESOURCE_HOLDER_H
#define APL_EXTENSION_RESOURCE_HOLDER_H

#include <memory>
#include <string>

namespace alexaext {

/**
 * Holder for a single shared system resource.  Resources are platform specific.  Example resources
 * may include: a shared rendering surface, a component in a view hierarchy.  Not all platforms
 * support shared resources.
 */
class ResourceHolder {

public:
    /**
     * Construct a resource holder.
     * @param resourceId The unique resource identifier.
     */
    explicit ResourceHolder(std::string resourceId) : mResourceId(std::move(resourceId)) {}

    virtual ~ResourceHolder() = default;

    /**
     * @return The unique resource identifier.
     */
    std::string resourceId() const { return mResourceId; }

private:
    std::string mResourceId;
};

using ResourceHolderPtr = std::shared_ptr<ResourceHolder>;

} // namespace alexaext

#endif // APL_EXTENSION_RESOURCE_HOLDER_H
