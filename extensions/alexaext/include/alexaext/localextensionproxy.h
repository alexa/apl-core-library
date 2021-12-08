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

#ifndef _ALEXAEXT_LOCALEXTENSIONPROXY_H
#define _ALEXAEXT_LOCALEXTENSIONPROXY_H


#include <functional>
#include <rapidjson/document.h>
#include <set>
#include <string>


#include "extension.h"
#include "extensionproxy.h"


namespace alexaext {

/**
 * Default implementation of Extension proxy, used for built-in extensions. This
 * class forwards all calls from the extension framework directly to the extension.
 */
class LocalExtensionProxy final : public ExtensionProxy,
                                  public std::enable_shared_from_this<LocalExtensionProxy> {

public:
    /**
     * Factory method provided by the extension author to create the extension described
     * by this definition and the uri.
     *
     * @param The extension URI.
     * @return The extension instance.
     */
    using ExtensionFactory = std::function<ExtensionPtr(const std::string& uri)>;

    /**
     * Proxy constructor for a local extension.
     *
     * @param extension The extension.
     */
    explicit LocalExtensionProxy(const ExtensionPtr& extension);

    /**
     * Proxy constructor for an extension using deferred creation. The extension
     * supports a single URI.
     *
     * @param factory Factory to create the extension.
     */
    explicit LocalExtensionProxy(const std::string& uri, ExtensionFactory factory);

    /**
     * Proxy constructor for an extension using deferred creation. The extension supports
     * multiple URI.
     *
     * @param factory Factory to create the extension.
     */
    explicit LocalExtensionProxy(std::set<std::string> uri, ExtensionFactory factory);

    std::set<std::string> getURIs() const override;
    bool getRegistration(const std::string& uri, const rapidjson::Value& registrationRequest,
                         RegistrationSuccessCallback success,
                         RegistrationFailureCallback error) override;
    bool initializeExtension(const std::string& uri) override;
    bool isInitialized(const std::string& uri) const override;
    bool invokeCommand(const std::string& uri, const rapidjson::Value& command,
                       CommandSuccessCallback success, CommandFailureCallback error) override;
    void registerEventCallback(Extension::EventCallback callback) override;
    void registerLiveDataUpdateCallback(Extension::LiveDataUpdateCallback callback) override;
    void onRegistered(const std::string &uri, const std::string &token) override;
    void onUnregistered(const std::string &uri, const std::string &token) override;

    bool sendMessage(const std::string &uri, const rapidjson::Value &message) override {
        if (mExtension)
            return mExtension->updateComponent(uri, message);
        return false;
    }

    void onResourceReady(const std::string &uri, const ResourceHolderPtr& resourceHolder) override {
        if (mExtension)
            mExtension->onResourceReady(uri, resourceHolder);
    }

private:
    ExtensionPtr mExtension;
    ExtensionFactory mFactory;
    std::set<std::string> mURIs;
    std::set<std::string> mInitialized;
    std::vector<Extension::EventCallback> mEventCallbacks;
    std::vector<Extension::LiveDataUpdateCallback> mLiveDataCallbacks;
};

using LocalExtensionProxyPtr = std::shared_ptr<LocalExtensionProxy>;

} // namespace alexaext

#endif //_ALEXAEXT_LOCALEXTENSIONPROXY_H
