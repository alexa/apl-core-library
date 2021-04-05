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

#ifndef _ALEXAEXT_EXTENSIONREGISTRAR_H
#define _ALEXAEXT_EXTENSIONREGISTRAR_H

#include <map>
#include <memory>
#include <string>

#include "extensionprovider.h"
#include "extensionproxy.h"

namespace alexaext {

/**
 * Default implementation of ExtensionProvider, maintained by the runtime.
 * Provides a registry of extension URI to extension proxy.
 */
class ExtensionRegistrar : public ExtensionProvider {

public:
    /**
     * Register an extension. Called by the runtime to register a known extension.
     *
     * @param proxy The extension proxy.
     * @return This object for chaining.
     */
    ExtensionRegistrar& registerExtension(const ExtensionProxyPtr& proxy) {
        if (proxy) {
            for (auto uri : proxy->getURIs()) {
                mExtensions.emplace(uri, proxy);
            }
        }
        return *this;
    }

    /**
     * Identifies the presence of an extension.  Called when a document has
     * requested an extension. This method returns true if an extension matching
     * the given uri has been registered.
     *
     * @param uri The requsted extension URI.
     * @return true if the extension is registered.
     */
    bool hasExtension(const std::string& uri) override {
        return (mExtensions.find(uri) != mExtensions.end());
    }

    /**
     * Get a proxy to the extension.  Called when a document has requested
     * an extension.
     *
     * @param uri The extension URI.
     * @return An extension proxy of a registered extension, nullptr if the extension
     *    was not registered.
     */
    ExtensionProxyPtr getExtension(const std::string& uri) override {
        auto proxy = mExtensions.find(uri);
        if (proxy == mExtensions.end())
            return nullptr;
        proxy->second->initializeExtension(uri);
        return proxy->second;
    }

private:
    std::map<std::string, ExtensionProxyPtr> mExtensions;
};

using ExtensionRegistrarPtr = std::shared_ptr<ExtensionRegistrar>;

} // namespace alexaext

#endif // _ALEXAEXT_EXTENSIONREGISTRAR_H
