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

#ifndef _ALEXAEXT_EXTENSION_REGISTRAR_H
#define _ALEXAEXT_EXTENSION_REGISTRAR_H

#include <map>
#include <memory>
#include <string>

#include "extensionprovider.h"
#include "extensionproxy.h"

namespace alexaext {

/**
 * Default implementation of ExtensionProvider, maintained by the runtime.
 * Provides a registry of directly registered extension URI to extension proxy plus provider
 * delegation.
 */
class ExtensionRegistrar : public ExtensionProvider {

public:
    /**
     * Add a specific ExtensionProvider.
     *
     * @param provider Provider implementation.
     * @return This object for chaining.
     */
    ExtensionRegistrar& addProvider(const ExtensionProviderPtr& provider);

    /**
     * Register an extension. Called by the runtime to register a known extension.
     *
     * @param proxy The extension proxy.
     * @return This object for chaining.
     */
    ExtensionRegistrar& registerExtension(const ExtensionProxyPtr& proxy);

    /**
     * Identifies the presence of an extension.  Called when a document has
     * requested an extension. This method returns true if an extension matching
     * the given uri has been registered or is available through any of known providers.
     *
     * @param uri The requsted extension URI.
     * @return true if the extension is registered.
     */
    bool hasExtension(const std::string& uri) override;

    /**
     * Get a proxy to the extension.  Called when a document has requested an extension.
     * If an extension that supports the specified URI has been directly registered with this
     * registrar, it will be returned. If not, the providers added to this registrar prior to
     * this call will be queried inthe hash order (undefined). The first provider to have an
     * extension with the specified URI will be used. Any remaining providers will not be queried.
     *
     * @param uri The extension URI.
     * @return An extension proxy of a registered or provider-held extension.
     */
    ExtensionProxyPtr getExtension(const std::string& uri) override;

private:
    std::set<ExtensionProviderPtr> mProviders;
    std::map<std::string, ExtensionProxyPtr> mExtensions;
};

using ExtensionRegistrarPtr = std::shared_ptr<ExtensionRegistrar>;

} // namespace alexaext

#endif // _ALEXAEXT_EXTENSION_REGISTRAR_H
