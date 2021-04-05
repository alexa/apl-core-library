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

#ifndef _ALEXAEXT_EXTENSIONPROVIDER_H
#define _ALEXAEXT_EXTENSIONPROVIDER_H

#include <memory>
#include <set>
#include <string>

#include "extensionproxy.h"


namespace alexaext {

/**
 * Interface that defines access to the extensions supported by the runtime.
 */
class ExtensionProvider {

public:
    /**
     * Destructor.
     */
    virtual ~ExtensionProvider() = default;

    /**
     * Check for the existence of an extension.
     *
     * @param uri The URI used to identify the extension.
     * @return true if the extension is supported by the runtime.
     */
    virtual bool hasExtension(const std::string& uri) = 0;

    /**
     * Get a proxy to a known extension. See hasExtension(...).
     *
     * @param uri The extension URI.
     * @return An extension proxy, nullptr if the extension does not exist, or has been removed.
     */
    virtual ExtensionProxyPtr getExtension(const std::string& uri) = 0;
};

using ExtensionProviderPtr = std::shared_ptr<ExtensionProvider>;

} // namespace alexaext

#endif // _ALEXAEXT_EXTENSIONPROVIDER_H
