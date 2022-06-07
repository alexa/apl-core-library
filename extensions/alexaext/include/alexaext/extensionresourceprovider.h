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

#ifndef APL_EXTENSION_RESOURCE_PROVIDER_H
#define APL_EXTENSION_RESOURCE_PROVIDER_H

#include "extensionresourceholder.h"

namespace alexaext {
/**
 * ExtensionResourceProvider enables the extension and the execution environment to share system
 * resources, such as display for Extension rendered Components.
 */
class ExtensionResourceProvider {

public:
    explicit ExtensionResourceProvider() = default;

    virtual ~ExtensionResourceProvider() = default;

    /**
     * Callback for returned resource from @c requestResource
     *
     * @param uri The extension URI.
     * @param resourceHolder The system resource.
     */
    using ExtensionResourceSuccessCallback =
        std::function<
            void(const std::string& uri, const ResourceHolderPtr& resourceHolder)>;

    /**
     * Callback for failure to provide resource from @c requestResource
     *
     * @param uri The extension URI.
     * @param resourceId The resource identifier assigned by the execution environment.
     * @param errorCode Error identifier.
     * @param error Error message.
     */
    using ExtensionResourceFailureCallback =
        std::function<void(const std::string& uri, const std::string& resourceId,
                           int errorCode, const std::string& error)>;

    /**
     * Request a shared resource.
     *
     * @param uri The extension URI
     * @param resourceId The unique id of the resource, assigned by the execution environment.
     * @param success The callback for success, provides the requested resource.
     * @param error The callback for failure, identifies the resource error.
     * @return true, if the request for resource can be processed.
     */
    virtual bool requestResource(const std::string& uri, const std::string& resourceId,
                                 ExtensionResourceSuccessCallback success, ExtensionResourceFailureCallback error) {
        return false;
    };
};

using ExtensionResourceProviderPtr = std::shared_ptr<ExtensionResourceProvider>;

} // namespace alexaext
#endif // APL_EXTENSION_RESOURCE_PROVIDER_H
