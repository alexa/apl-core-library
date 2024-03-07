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

#include "alexaext/threadsafeextensionregistrar.h"

#include <algorithm>

namespace alexaext {

ThreadSafeExtensionRegistrar::ThreadSafeExtensionRegistrar(const std::set<ExtensionProviderPtr>& providers, const std::set<ExtensionProxyPtr>& proxies) : mProviders(providers) {
    std::lock_guard<std::mutex> lock(mExtensionMutex);
    for (const auto& proxy : proxies) {
        for (const auto& uri : proxy->getURIs()) {
            mExtensions.emplace(uri, proxy);
        }
    }
}

bool
ThreadSafeExtensionRegistrar::hasExtension(const std::string& uri)
{
    // Check if extension is cached first
    {
        std::lock_guard<std::mutex> lock(mExtensionMutex);
        if (mExtensions.count(uri) > 0) return true;
    }

    // Now check if any providers support it
    return std::any_of(mProviders.cbegin(), mProviders.cend(), [&uri](const ExtensionProviderPtr& it) {
        return it->hasExtension(uri);
    });
}

ExtensionProxyPtr
ThreadSafeExtensionRegistrar::getExtension(const std::string& uri)
{
    // Check if extension is cached first.
    ExtensionProxyPtr proxy = nullptr;
    {
        std::lock_guard<std::mutex> lock(mExtensionMutex);
        auto it = mExtensions.find(uri);
        if (it != mExtensions.end()) {
            proxy = it->second;
        }
    }

    // Now go to providers
    if (!proxy) {
        auto provider = std::find_if(mProviders.cbegin(), mProviders.cend(), [&uri](const ExtensionProviderPtr& it) {
            return it->hasExtension(uri);
        });

        if (provider != mProviders.cend()) {
            proxy = provider->get()->getExtension(uri);
            if (proxy) {
                std::lock_guard<std::mutex> lock(mExtensionMutex);
                mExtensions.emplace(uri, proxy);
            }
        }
    }

    if (proxy && !proxy->isInitialized(uri)) proxy->initializeExtension(uri);
    return proxy;
}

} // namespace alexaext