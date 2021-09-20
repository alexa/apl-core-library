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

#include "alexaext/extensionregistrar.h"

namespace alexaext {

ExtensionRegistrar&
ExtensionRegistrar::registerExtension(const ExtensionProxyPtr& proxy) {
    if (proxy) {
        for (const auto& uri : proxy->getURIs()) {
            mExtensions.emplace(uri, proxy);
        }
    }
    return *this;
}

bool
ExtensionRegistrar::hasExtension(const std::string& uri)
{
    return (mExtensions.find(uri) != mExtensions.end());
}

ExtensionProxyPtr
ExtensionRegistrar::getExtension(const std::string& uri)
{
    auto proxy = mExtensions.find(uri);
    if (proxy == mExtensions.end())
        return nullptr;
    if (!mInitialized.count(uri)) {
        if (!proxy->second->initializeExtension(uri)) return nullptr;
        mInitialized.insert(uri);
    }
    return proxy->second;
}

} // namespace alexaext
