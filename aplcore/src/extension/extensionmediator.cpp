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
#include "apl/extension/extensioncomponent.h"
#include "apl/extension/extensionmediator.h"
#include "apl/primitives/objectdata.h"

using namespace alexaext;

namespace apl {

static const bool DEBUG_EXTENSION_MEDIATOR = false;

// TODO
// TODO Experimental class.  This represents the integration point between the Alexa Extension library,
// TODO and the existing extension messaging client.  This class will change significantly.
// TODO

void
ExtensionMediator::bindContext(const RootContextPtr& context)
{
    // Called by RootContext on create to create an association for event and data updates.
    // This goes away when ExtensionManager registers callbacks directly.
    mRootContext = context;
    for (auto& client : mClients) {
        client.second->bindContext(context);
    }
}

void
ExtensionMediator::initializeExtensions(const RootConfigPtr& rootConfig, const ContentPtr& content,
                                        const ExtensionGrantRequestCallback& grantHandler) {

    auto uris = content->getExtensionRequests();
    auto extensionProvider = mProvider.lock();
    if (rootConfig == nullptr || uris.empty() || extensionProvider == nullptr) {
        return;
    }

    std::weak_ptr<ExtensionMediator> weak_this = shared_from_this();
    std::weak_ptr<RootConfig> weak_config = rootConfig;
    for (const auto& uri : uris) {
        if (mPendingRegistrations.count(uri)) continue;

        LOG_IF(DEBUG_EXTENSION_MEDIATOR) << "initialize extension: " << uri
                                         << " has extension: " << extensionProvider->hasExtension(uri);

        mPendingGrants.insert(uri);
        if (grantHandler) {
            // callback to grant/deny access to extension
            grantHandler(
                uri,
                [weak_this, weak_config](const std::string& grantedUri) {
                    if (auto mediator = weak_this.lock())
                        mediator->grantExtension(weak_config.lock(), grantedUri);
                },
                [weak_this](const std::string& deniedUri) {
                  if (auto mediator = weak_this.lock())
                      mediator->denyExtension(deniedUri);
                });
        } else {
            // auto-grant when no grant handler
            grantExtension(rootConfig, uri);
        }
    }
}

void
ExtensionMediator::grantExtension(const RootConfigPtr& rootConfig, const std::string& uri) {

    if (!mPendingGrants.count(uri))
        return;
    LOG_IF(DEBUG_EXTENSION_MEDIATOR) << "Extension granted: " << uri;

    mPendingGrants.erase(uri);
    auto extensionProvider = mProvider.lock();
    if (rootConfig == nullptr  || extensionProvider == nullptr) {
        return;
    }

    if (extensionProvider->hasExtension(uri)) {
        // First get will call initialize.
        auto proxy = extensionProvider->getExtension(uri);
        if (!proxy) {
            CONSOLE_S(rootConfig->getSession()) << "Failed to retrieve proxy for extension: " << uri;
            return;
        }

        // create a client for message processing
        auto client = ExtensionClient::create(rootConfig, uri);
        if (!client) {
            CONSOLE_S(rootConfig->getSession()) << "Failed to create client for extension: " << uri;
            return;
        }
        mClients.emplace(uri, client);
        mPendingRegistrations.insert(uri);
    }
}

void
ExtensionMediator::denyExtension(const std::string& uri) {
    mPendingGrants.erase(uri);
    LOG_IF(DEBUG_EXTENSION_MEDIATOR) << "Extension denied: " << uri;
}

void
ExtensionMediator::loadExtensionsInternal(const RootConfigPtr& rootConfig, const ContentPtr& content)
{

    if (!mPendingGrants.empty()) {
        LOG(LogLevel::kWarn) << "Loading extensions with pending grant requests.  "
            << "Failure to grant extension use makes the extension unavailable for the session.";
        mPendingGrants.clear();
    }

    // No extensions to load
    auto extensionProvider = mProvider.lock();
    if (rootConfig == nullptr || mPendingRegistrations.empty() || extensionProvider == nullptr) {
        if (mLoadedCallback) {
            mLoadedCallback();
        }
        return;
    }

    auto session = rootConfig->getSession();
    auto pendingRegistrations = mPendingRegistrations;
    for (const auto& uri : pendingRegistrations) {
        LOG_IF(DEBUG_EXTENSION_MEDIATOR) << "load extension: " << uri
                                         << " has extension: " << extensionProvider->hasExtension(uri);
        // Get the extension from the registration
        auto proxy = extensionProvider->getExtension(uri);
        if (!proxy) {
            CONSOLE_S(session) << "Failed to retrieve proxy for extension: " << uri;
            mPendingRegistrations.erase(uri);
            continue;
        }

        //  Send a registration request to the extension.
        rapidjson::Document settingsDoc;
        auto settings = content->getExtensionSettings(uri).serialize(settingsDoc.GetAllocator());
        rapidjson::Document flagsDoc;
        auto flags = rootConfig->getExtensionFlags(uri).serialize(flagsDoc.GetAllocator());
        rapidjson::Document regReq;
        // TODO: It was uri instead of the version here. Funny. We need to figure if it's schema or interface here.
        regReq.Swap(RegistrationRequest("1.0").uri(uri).settings(settings).flags(flags).getDocument());
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

        if (!success) {
            mPendingRegistrations.erase(uri);
            // call to extension failed without failure callback
            CONSOLE_S(session) << "Extension registration failure - code: " << kErrorInvalidMessage
                               << " message: " << sErrorMessage[kErrorInvalidMessage] + uri;
        }
    }

    if (mPendingRegistrations.empty() && mLoadedCallback) {
        mLoadedCallback();
        mLoadedCallback = nullptr;
    }
}

void
ExtensionMediator::loadExtensions(const RootConfigPtr& rootConfig, const ContentPtr& content,
                                  const std::set<std::string>* grantedExtensions)
{
    mRootConfig = rootConfig;
    if (!content->isReady()) {
        CONSOLE_CFGP(rootConfig) << "Cannot load extensions when Content is not ready";
        return;
    }

    initializeExtensions(rootConfig, content,
                         [&grantedExtensions](const std::string& uri, ExtensionMediator::ExtensionGrantResult grant,
                                             ExtensionMediator::ExtensionGrantResult deny) {
                           if (grantedExtensions == nullptr) {
                               // without explicit grant list, all are granted
                               grant(uri);
                           }
                           else if (grantedExtensions->find(uri) != grantedExtensions->end()) {
                               // extension has been explicit grant
                               grant(uri);
                           } else {
                               // extension is denied
                               deny(uri);
                           }
                         });
    loadExtensionsInternal(rootConfig, content);
}

void
ExtensionMediator::loadExtensions(
        const RootConfigPtr& rootConfig,
        const ContentPtr& content,
        ExtensionsLoadedCallback loaded)
{
    mRootConfig = rootConfig;
    mLoadedCallback = std::move(loaded);
    loadExtensionsInternal(rootConfig, content);
}

bool
ExtensionMediator::invokeCommand(const apl::Event& event)
{
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

inline ExtensionProxyPtr
ExtensionMediator::getProxy(const std::string &uri)
{
    auto extPro = mProvider.lock();
    if (extPro == nullptr || !extPro->hasExtension(uri)) {
        auto config = mRootConfig.lock();
        CONSOLE_CFGP(config) << "Proxy does not exist for uri: " << uri;
        return nullptr;
    }
    return extPro->getExtension(uri);
}

inline ExtensionClientPtr
ExtensionMediator::getClient(const std::string &uri)
{
    auto itr = mClients.find(uri);
    if (itr == mClients.end()) {
        auto config = mRootConfig.lock();
        CONSOLE_CFGP(config) << "Attempt to use an unavailable extension - uri: " << uri;
        return nullptr;
    }
    return  itr->second;
}

void
ExtensionMediator::notifyComponentUpdate(const ExtensionComponentPtr& component, bool resourceNeeded)
{
    auto uri = component->getUri();
    auto resourceId = component->getResourceID();
    auto proxy = getProxy(uri);
    auto client = getClient(uri);
    if (!proxy || !client)
        return;

    rapidjson::Document document;
    rapidjson::Value message;
    message = client->createComponentChange(document.GetAllocator(), *component);

    // Notify the extension of the component change
    auto sent = proxy->sendMessage(uri, message);
    if (!sent) {
        auto config = mRootConfig.lock();
        CONSOLE_CFGP(config) << "Extension message failure - code: " << kErrorInvalidMessage
                                      << " message: " << sErrorMessage[kErrorInvalidMessage] + uri;
        return;
    }

    // acquire the resource for the extension component
    if (!resourceNeeded) {
        return;
    }

    auto requested = false;
    if (auto resourceProvider = mResourceProvider.lock()) {
        std::weak_ptr<ExtensionMediator> weak_this = shared_from_this();
        std::weak_ptr<ExtensionComponent> weak_component = component;
        requested = resourceProvider->requestResource(uri, resourceId,
                [weak_this](const std::string& uri, const ResourceHolderPtr& resourceHolder) {
                  if (auto mediator = weak_this.lock())
                      mediator->sendResourceReady(uri, resourceHolder);
                  },
                [weak_this, weak_component](const std::string& uri, const std::string& resourceId, int errorCode,
                            const std::string& error) {
                  // resource acquisition failed
                  if (auto mediator = weak_this.lock())
                      mediator->resourceFail(weak_component.lock(), errorCode, error);
                }
            );
    }
    if (!requested) {

    }

}

void
ExtensionMediator::sendResourceReady(const std::string& uri, const alexaext::ResourceHolderPtr& resourceHolder) {

    if (auto proxy = getProxy(uri))
        proxy->onResourceReady(uri, resourceHolder);
}

void
ExtensionMediator::resourceFail(const ExtensionComponentPtr& component, int errorCode, const std::string& error) {
    auto root = mRootContext.lock();
    if (!component)
        return;
    CONSOLE_S(root->getSession()) << "Extension resource failure - uri:"
                                  << component->getUri() << " resourceId:" << component->getResourceID();
    component->updateResourceState(kResourceError, errorCode, error);
}


void
ExtensionMediator::registerExtension(const std::string& uri, const ExtensionProxyPtr& extension,
                                     const ExtensionClientPtr& client)
{

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
    extension->onRegistered(uri, client->getConnectionToken());
    LOG_IF(DEBUG_EXTENSION_MEDIATOR) << "registered: " << uri << " clients: " << mClients.size();
}

void
ExtensionMediator::enqueueResponse(const std::string& uri, const rapidjson::Value& message)
{
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
ExtensionMediator::processMessage(const std::string& uri, JsonData&& processMessage)
{
    auto client = mClients.find(uri);
    if (client == mClients.end())
        return;

    // capture the current registration state
    bool needsRegistration = !client->second->registered() && !client->second->registrationMessageProcessed();

    // client handles null root
    auto root = mRootContext.lock();
    client->second->processMessage(root, std::move(processMessage));

    // register handlers if registered for first time
    if (needsRegistration) {
        if (client->second->registered()) {
            if (auto provider = mProvider.lock()) {
                auto proxy = provider->getExtension(uri);
                registerExtension(uri, proxy, client->second);
            }
        }

        if (mPendingRegistrations.count(uri)) {
            mPendingRegistrations.erase(uri);
            if (mPendingRegistrations.empty() && mLoadedCallback) {
                mLoadedCallback();
                mLoadedCallback = nullptr;
            }
        }
    }
}

void
ExtensionMediator::finish()
{
    auto provider = mProvider.lock();
    if (!provider) return;

    for (const auto& u2c : mClients) {
        auto proxy = provider->getExtension(u2c.first);
        if (proxy) {
            proxy->onUnregistered(u2c.first, u2c.second->getConnectionToken());
        }
    }

    mClients.clear();
}

} // namespace apl

#endif
