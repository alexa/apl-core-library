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

#include <algorithm>

namespace alexaext {

ExtensionRegistrar&
ExtensionRegistrar::addProvider(const ExtensionProviderPtr& provider)
{
    if (provider) {
        mProviders.emplace(provider);
    }
    return *this;
}

ExtensionRegistrar&
ExtensionRegistrar::registerExtension(const ExtensionProxyPtr& proxy)
{
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
    if (mExtensions.find(uri) != mExtensions.end()) return true;
    return std::any_of(
            mProviders.begin(),
            mProviders.end(),
            [uri](const ExtensionProviderPtr& provider) {
                return provider->hasExtension(uri);
            });
}

ExtensionProxyPtr
ExtensionRegistrar::getExtension(const std::string& uri)
{
    ExtensionProxyPtr proxy;
    auto it = mExtensions.find(uri);

    if (it == mExtensions.end()) {
        for (auto& provider : mProviders) {
            proxy = provider->getExtension(uri);
            if (proxy) {
                mExtensions.emplace(uri, proxy);
                break;
            }
        }
    } else {
        proxy = it->second;
    }

    if (!proxy) return nullptr;

    if (!proxy->isInitialized(uri)) {
        if (!proxy->initializeExtension(uri)) return nullptr;
    }
    return proxy;
}

} // namespace alexaext
