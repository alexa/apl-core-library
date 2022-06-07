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

#ifndef _ALEXAEXT_LOCAL_EXTENSION_PROXY_H
#define _ALEXAEXT_LOCAL_EXTENSION_PROXY_H


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
    bool getRegistration(const std::string& uri,
                         const rapidjson::Value& registrationRequest,
                         RegistrationSuccessCallback success,
                         RegistrationFailureCallback error) override;
    bool getRegistration(const ActivityDescriptor& activity,
                         const rapidjson::Value& registrationRequest,
                         RegistrationSuccessActivityCallback&& success,
                         RegistrationFailureActivityCallback&& error) override;
    bool initializeExtension(const std::string& uri) override;
    bool isInitialized(const std::string& uri) const override;
    bool invokeCommand(const std::string& uri, const rapidjson::Value& command,
                       CommandSuccessCallback success, CommandFailureCallback error) override;
    bool invokeCommand(const ActivityDescriptor& activity,
                       const rapidjson::Value& command,
                       CommandSuccessActivityCallback&& success,
                       CommandFailureActivityCallback&& error) override;
    void registerEventCallback(Extension::EventCallback callback) override;
    void registerLiveDataUpdateCallback(Extension::LiveDataUpdateCallback callback) override;
    void registerEventCallback(const ActivityDescriptor& activity, Extension::EventActivityCallback&& callback) override;
    void registerLiveDataUpdateCallback(const ActivityDescriptor& activity, Extension::LiveDataUpdateActivityCallback&& callback) override;
    void onRegistered(const std::string& uri, const std::string& token) override;
    void onRegistered(const ActivityDescriptor& activity) override;
    void onUnregistered(const std::string& uri, const std::string& token) override;
    void onUnregistered(const ActivityDescriptor& activity) override;

    bool sendComponentMessage(const std::string &uri, const rapidjson::Value &message) override;
    bool sendComponentMessage(const ActivityDescriptor &activity, const rapidjson::Value &message) override;

    void onResourceReady(const std::string& uri, const ResourceHolderPtr& resourceHolder) override;
    void onResourceReady(const ActivityDescriptor& activity, const ResourceHolderPtr& resourceHolder) override;

    void onSessionStarted(const SessionDescriptor& session) override;
    void onSessionEnded(const SessionDescriptor& session) override;
    void onForeground(const ActivityDescriptor& activity) override;
    void onBackground(const ActivityDescriptor& activity) override;
    void onHidden(const ActivityDescriptor& activity) override;

private:
    using ProcessRegistrationCallback = std::function<rapidjson::Document(const rapidjson::Value& registrationRequest)>;
    using ProcessCommandCallback = std::function<bool(const rapidjson::Value& command)>;

    bool getRegistrationInternal(const std::string& uri,
                                 const rapidjson::Value& registrationRequest,
                                 RegistrationSuccessCallback&& success,
                                 RegistrationFailureCallback&& error,
                                 ProcessRegistrationCallback&& processRegistration);

    bool invokeCommandInternal(const std::string& uri,
                               const rapidjson::Value& command,
                               CommandSuccessCallback&& success,
                               CommandFailureCallback&& error,
                               ProcessCommandCallback&& processCommand);

private:
    using EventCallbacks = std::shared_ptr<std::vector<Extension::EventActivityCallback>>;
    using LiveDataCallbacks = std::shared_ptr<std::vector<Extension::LiveDataUpdateActivityCallback>>;

    ExtensionPtr mExtension;
    ExtensionFactory mFactory;
    std::set<std::string> mURIs;
    std::set<std::string> mInitialized;
    std::vector<Extension::EventCallback> mEventCallbacks; // For backwards compatibility
    std::map<ActivityDescriptor, EventCallbacks, ActivityDescriptor::Compare> mEventActivityCallbacks;
    std::vector<Extension::LiveDataUpdateCallback> mLiveDataCallbacks; // For backwards compatibility
    std::map<ActivityDescriptor, LiveDataCallbacks, ActivityDescriptor::Compare> mLiveDataActivityCallbacks;
};

using LocalExtensionProxyPtr = std::shared_ptr<LocalExtensionProxy>;

} // namespace alexaext

#endif //_ALEXAEXT_LOCAL_EXTENSION_PROXY_H
