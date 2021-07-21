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

#ifdef ALEXAEXTENSIONS

#include <functional>
#include <memory>

#include <rapidjson/document.h>

#include <alexaext/alexaext.h>
#include <apl/primitives/objectdata.h>

#include "apl/extension/extensionmediator.h"

using namespace alexaext;

namespace apl {

static const bool DEBUG_EXTENSION_MEDIATOR = false;

// TODO
// TODO Experimental class.  This represents the integration point between the Alexa Extension library,
// TODO and the existing extension messaging client.  This class will change significantly.
// TODO

void
ExtensionMediator::bindContext(const RootContextPtr& context) {
    // Called by RootContext on create to create an association for event and data updates.
    // This goes away when ExtensionManager registers callbacks directly.
    mRootContext = context;
}

void
ExtensionMediator::loadExtensions(const RootConfigPtr& rootConfig, const ContentPtr& content) {

    auto uris = content->getExtensionRequests();

    // No extensions to load
    auto extensionProvider = mProvider.lock();
    if (rootConfig == nullptr || uris.empty() || extensionProvider == nullptr)
        return;

    auto session = rootConfig->getSession();

    for (const auto& uri: uris) {

        LOG_IF(DEBUG_EXTENSION_MEDIATOR) << "load extension: " << uri
                                         << " has extension: " << extensionProvider->hasExtension(uri);

        // Get the extension from the registration
        if (extensionProvider->hasExtension(uri)) {
            auto proxy = extensionProvider->getExtension(uri);

            // create a client for message processing
            auto client = ExtensionClient::create(rootConfig, uri);
            if (!client) {
                CONSOLE_S(session) << "Failed to create client for extension: " << uri;
                continue;
            }
            mClients.emplace(uri, client);

            //  Send a registration request to the extension.
            rapidjson::Document regReq;
            auto settings = content->getExtensionSettings(uri).serialize(regReq.GetAllocator());
            regReq.Swap(RegistrationRequest(uri).settings(settings).getDocument());
            std::weak_ptr<ExtensionMediator> weak_this(shared_from_this());

            auto success = proxy->getRegistration(uri, regReq,
                                                  [weak_this](const std::string& uri,
                                                              const rapidjson::Value& registrationSuccess) {
                                                      if (auto mediator = weak_this.lock())
                                                          mediator->enqueueResponse(uri, registrationSuccess);
                                                  },
                                                  [weak_this](const std::string& uri,
                                                              const rapidjson::Value& registrationFailure) {
                                                      if (auto mediator = weak_this.lock())
                                                          mediator->enqueueResponse(uri, registrationFailure);
                                                  });

            if (success) {
                proxy->onRegistered(uri, client->getConnectionToken());
            } else {
                // call to extension failed without failure callback
                CONSOLE_S(session) << "Extension registration failure - code: " << kErrorInvalidMessage
                                   << " message: " << sErrorMessage[kErrorInvalidMessage] + uri;
            }
        }
    }

}

bool
ExtensionMediator::invokeCommand(const apl::Event& event) {

    auto root = mRootContext.lock();

    if (event.getType() != kEventTypeExtension || !root)
        return false;

    auto uri = event.getValue(EventProperty::kEventPropertyExtensionURI).asString();

    // Get the extension client
    auto itr = mClients.find(uri);
    auto extPro = mProvider.lock();
    if (itr == mClients.end() || extPro == nullptr || !extPro->hasExtension(uri)) {
        CONSOLE_S(root->getSession()) << "Attempt to execute command on unavailable extension - uri: " << uri;
        return false;
    }
    auto client = itr->second;

    // Get the Extension
    auto proxy = extPro->getExtension(uri);
    if (!proxy) {
        CONSOLE_S(root->getSession()) << "Attempt to execute command on unavailable extension - uri: " << uri;
        return false;
    }

    // create the message
    rapidjson::Document document;
    auto cmd = client->processCommand(document.GetAllocator(), event);
    std::weak_ptr<ExtensionMediator> weak_this = shared_from_this();

    // Forward to the extension
    auto invoke = proxy->invokeCommand(uri, cmd,
                                       [weak_this](const std::string& uri,
                                                   const rapidjson::Value& commandSuccess) {
                                           if (auto mediator = weak_this.lock())
                                               mediator->enqueueResponse(uri, commandSuccess);
                                       },
                                       [weak_this](const std::string& uri,
                                                   const rapidjson::Value& commandFailure) {
                                           if (auto mediator = weak_this.lock())
                                               mediator->enqueueResponse(uri, commandFailure);
                                       });

    if (!invoke) {
        CONSOLE_S(root->getSession()) << "Extension command failure - code: " << kErrorInvalidMessage
                                      << " message: " << sErrorMessage[kErrorInvalidMessage] + uri;
    }

    return invoke;
}

void
ExtensionMediator::registerExtension(const std::string& uri, const ExtensionProxyPtr& extension,
                                     const ExtensionClientPtr& client) {

    //  set up callbacks for extension messages
    std::weak_ptr<ExtensionMediator> weak_this = shared_from_this();

    extension->registerEventCallback(
            [weak_this](const std::string& uri, const rapidjson::Value& event) {
                if (auto mediator = weak_this.lock()) {
                    if (mediator->isEnabled())
                        mediator->enqueueResponse(uri, event);
                } else if (DEBUG_EXTENSION_MEDIATOR) {
                    LOG(LogLevel::kDebug) << "Mediator expired for event callback.";
                }
            });
    extension->registerLiveDataUpdateCallback(
            [weak_this](const std::string& uri, const rapidjson::Value& liveDataUpdate) {
                if (auto mediator = weak_this.lock()) {
                    if (mediator->isEnabled())
                        mediator->enqueueResponse(uri, liveDataUpdate);
                } else if (DEBUG_EXTENSION_MEDIATOR) {
                    LOG(LogLevel::kDebug) << "Mediator expired for live data callback.";
                }
            });

    mClients.emplace(uri, client);
    LOG_IF(DEBUG_EXTENSION_MEDIATOR) << "registered: " << uri << " clients: " << mClients.size();
}

void
ExtensionMediator::enqueueResponse(const std::string& uri, const rapidjson::Value& message) {
    std::weak_ptr<ExtensionMediator> weak_this = shared_from_this();
    // TODO optimize
    auto copy = std::make_shared<rapidjson::Document>();
    copy->CopyFrom(message, copy->GetAllocator());
    bool enqueued = mMessageExecutor->enqueueTask([weak_this, uri, copy] () {
        if (auto mediator = weak_this.lock()) {
            mediator->processMessage(uri, *copy);
        }
    });
    if (!enqueued)
        LOG(LogLevel::kWarn) << "failed to process message for extension, uri:" << uri;
}

void
ExtensionMediator::processMessage(const std::string& uri, JsonData&& processMessage) {

    auto client = mClients.find(uri);
    if (client == mClients.end())
        return;

    // capture the current registration state
    bool needsRegistration = !client->second->registered();

    // client handles null root
    auto root = mRootContext.lock();
    client->second->processMessage(root, std::move(processMessage));

    // register handlers if registered for first time
    if (needsRegistration && client->second->registered()) {
        if (auto provider = mProvider.lock()) {
            auto proxy = provider->getExtension(uri);
            registerExtension(uri, proxy, client->second);
        }
    }
}


} // namespace apl

#endif
