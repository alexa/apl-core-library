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

#include <cassert>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <rapidjson/document.h>

#include <alexaext/alexaext.h>
#include "apl/extension/extensioncomponent.h"
#include "apl/extension/extensionmediator.h"
#include "apl/primitives/objectdata.h"

using namespace alexaext;

namespace apl {

static const bool DEBUG_EXTENSION_MEDIATOR = false;

/**
 * Possible lifecycle states for extensions. A session manages a state machine for each registered
 * extension (expressed as an activity). State transitions are typically used to ensure that
 * the appropriate notifications are sent to extensions exactly once.
 */
enum class ExtensionLifecycleStage {
    /**
     * Not part of the state machine. Only used to report the state of an unknown extension.
     */
    kExtensionUnknown,

    /**
     * Initial state for a requested extension
     */
    kExtensionInitialized,

    /**
     * Assigned to an extension once registration has successfully been completed.
     */
    kExtensionRegistered,

    /**
     * Assigned to an extension once it was notified that the current document unregistered.
     *
     * This is a terminal state.
     */
    kExtensionUnregistered,

    /**
     * Assigned to an extension that has reached terminal state without going through a successful
     * registration. For example, the extension was denied, or an error was encountered before
     * registration could be completed.
     *
     * This is a terminal state.
     */
    kExtensionFinalized,
};

enum class SessionLifecycleStage {
    kSessionStarted,
    kSessionEnding,
    kSessionEnded,
};

/**
 * Describes the state of a specific extension within a session.
 */
struct ActivityState {
    ActivityState(const ActivityDescriptorPtr& activity)
        : activity(activity),
          state(ExtensionLifecycleStage::kExtensionInitialized)
    {}

    ActivityDescriptorPtr activity;
    ExtensionLifecycleStage state;
};

struct ExtensionState {
    ~ExtensionState() {
        proxy = nullptr;
        activities.clear();
    }

    SessionLifecycleStage state;
    alexaext::ExtensionProxyPtr proxy;
    std::vector<ActivityState> activities;
};

/**
 * Defines the state of all extensions within a session.
 */
class ExtensionSessionState final {
public:
    ExtensionSessionState(const alexaext::SessionDescriptorPtr& sessionDescriptor)
        : mSessionDescriptor(sessionDescriptor)
    {}
    ~ExtensionSessionState() = default;

    /**
     * Ends the specified session
     *
     * @param session The session that ended
     */
    void endSession() {
        if (mSessionState == SessionLifecycleStage::kSessionEnded) {
            /* The session has already ended, and the notification has been sent to all associated
             * extensions, so simply ignore this duplicate signal */
            return;
        }

        alexaext::ExtensionProxyPtr proxy = nullptr;

        mSessionState = SessionLifecycleStage::kSessionEnding;

        bool allEnded = true;
        for (const auto& entry : mExtensionStateByURI) {
            if (!tryEndSession(*entry.second)) {
                allEnded = false;
            }
        }

        if (allEnded) {
            mSessionState = SessionLifecycleStage::kSessionEnded;
        }
    }

    /**
     * Returns the state of the extension with the specified URI.
     *
     * @param uri The extension URI
     * @return The current state for this extension, or unknown if not found.
     */
    ExtensionLifecycleStage getState(const ActivityDescriptorPtr& activity) {
        auto it = mExtensionStateByURI.find(activity->getURI());
        if (it != mExtensionStateByURI.end()) {
            for (const auto& entry : it->second->activities) {
                if (*entry.activity == *activity) {
                    return entry.state;
                }
            }
        }

        return ExtensionLifecycleStage::kExtensionUnknown;
    }

    /**
     * Initialize the specified extension
     * @param activity The extension activity
     * @param proxy The proxy used to communicate with the extension
     */
    void initialize(const ActivityDescriptorPtr& activity, const alexaext::ExtensionProxyPtr& proxy) {
        if (mSessionState == SessionLifecycleStage::kSessionEnding
                || mSessionState == SessionLifecycleStage::kSessionEnded) {
            // The session is ending or has ended, prevent new extensions from being initialized
            LOG(LogLevel::kWarn) << "Ignoring attempt to initialize extension in inactive session, uri: " << activity->getURI();
            return;
        }

        auto it = mExtensionStateByURI.find(activity->getURI());
        if (it == mExtensionStateByURI.end()) {
            // This is the first activity for this extension, initialize the state
            auto state = std::make_shared<ExtensionState>();
            state->proxy = proxy;
            state->state = SessionLifecycleStage::kSessionStarted;

            state->activities.emplace_back(ActivityState(activity));

            mExtensionStateByURI.emplace(activity->getURI(), state);

            // Notify the extension that a session has started. This is only done
            // after at least one extension is requested during the session to cut down on
            // unnecessary noise
            proxy->onSessionStarted(*mSessionDescriptor);
        } else {
            for (const auto& entry : it->second->activities) {
                if (*entry.activity == *activity) {
                    // This activity was already registered
                    LOG(LogLevel::kWarn) << "Ignoring attempt to re-initialize extension, uri: " << activity->getURI();
                    return;
                }
            }

            it->second->activities.emplace_back(ActivityState(activity));
        }
    }

    /**
     * Attempts to update the state of the specified extension.
     *
     * @param uri The extension URI
     * @param lifecycleStage The new extension state
     * @return @c true if the update succeeded, @c false if the state transition isn't permitted.
     */
    bool updateState(const alexaext::ActivityDescriptorPtr& activity, ExtensionLifecycleStage lifecycleStage) {
        auto it = mExtensionStateByURI.find(activity->getURI());
        if (it != mExtensionStateByURI.end()) {
            for (auto& entry : it->second->activities) {
                if (*entry.activity == *activity && canUpdate(entry.state, lifecycleStage)) {
                    entry.state = lifecycleStage;

                    if (lifecycleStage == ExtensionLifecycleStage::kExtensionFinalized
                            && mSessionState == SessionLifecycleStage::kSessionEnding) {
                        // An activity was just finalized, and there was a previous attempt to end
                        // the session that could not be successfully completed. Force a check to
                        // see if the session can now end
                        endSession();
                    }

                    return true;
                }
            }
        }

        return false;
    }

private:
    bool tryEndSession(ExtensionState& state) {
        if (state.state == SessionLifecycleStage::kSessionEnded) {
            // Already ended the session for this URI, nothing to do
            return true;
        }

        for (const auto& activity : state.activities) {
            if (activity.state == ExtensionLifecycleStage::kExtensionInitialized
                    || activity.state == ExtensionLifecycleStage::kExtensionRegistered) {
                // Found an extension that hasn't been unregistered yet, so don't end the session
                return false;
            }
        }

        state.state = SessionLifecycleStage::kSessionEnded;
        state.proxy->onSessionEnded(*mSessionDescriptor);

        return true;
    }

    /**
     * Determines if a state transition is permitted.
     *
     * @param fromState The current state
     * @param toState The candidate state
     * @return @c true if the state transition is allowed, @c false otherwise
     */
    bool canUpdate(ExtensionLifecycleStage fromState, ExtensionLifecycleStage toState) {
        switch (fromState) {
            case ExtensionLifecycleStage::kExtensionInitialized:
                return toState == ExtensionLifecycleStage::kExtensionRegistered
                        || toState == ExtensionLifecycleStage::kExtensionFinalized;
            case ExtensionLifecycleStage::kExtensionRegistered:
                return toState == ExtensionLifecycleStage::kExtensionUnregistered;
            case ExtensionLifecycleStage::kExtensionUnregistered: // terminal state
            case ExtensionLifecycleStage::kExtensionFinalized: // terminal state
            default:
                return false;
        }
    }

private:
    alexaext::SessionDescriptorPtr mSessionDescriptor;
    SessionLifecycleStage mSessionState = SessionLifecycleStage::kSessionStarted;
    std::unordered_map<std::string, std::shared_ptr<ExtensionState>> mExtensionStateByURI;
};

ExtensionMediator::ExtensionMediator(const ExtensionProviderPtr& provider,
                                     const ExtensionResourceProviderPtr& resourceProvider,
                                     const ExecutorPtr& messageExecutor)
    : mProvider(provider),
      mResourceProvider(resourceProvider),
      mMessageExecutor(messageExecutor)
{}

ExtensionMediator::ExtensionMediator(const ExtensionProviderPtr& provider,
                                     const ExtensionResourceProviderPtr& resourceProvider,
                                     const ExecutorPtr& messageExecutor,
                                     const ExtensionSessionPtr& extensionSession)
    : mProvider(provider),
      mResourceProvider(resourceProvider),
      mMessageExecutor(messageExecutor),
      mExtensionSession(extensionSession)
{
    if (mExtensionSession && mExtensionSession->getSessionDescriptor()) {
        auto sessionState = extensionSession->getSessionState();
        if (!sessionState) {
            sessionState = std::make_shared<ExtensionSessionState>(extensionSession->getSessionDescriptor());
            extensionSession->setSessionState(sessionState);
            extensionSession->onSessionEnded([](ExtensionSession& session) {
                if (auto state = session.getSessionState()) {
                    state->endSession();
                }
            });
        }
    }
}

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
    onDisplayStateChanged(context->getDisplayState());
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

        LOG_IF(DEBUG_EXTENSION_MEDIATOR).session(rootConfig) << "initialize extension: " << uri
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
                [weak_this, weak_config](const std::string& deniedUri) {
                  if (auto mediator = weak_this.lock())
                      mediator->denyExtension(weak_config.lock(), deniedUri);
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
    LOG_IF(DEBUG_EXTENSION_MEDIATOR).session(rootConfig) << "Extension granted: " << uri;

    mPendingGrants.erase(uri);
    auto extensionProvider = mProvider.lock();
    if (rootConfig == nullptr  || extensionProvider == nullptr) {
        return;
    }

    if (extensionProvider->hasExtension(uri)) {
        // First get will call initialize.
        auto proxy = extensionProvider->getExtension(uri);
        if (!proxy) {
            CONSOLE(rootConfig) << "Failed to retrieve proxy for extension: " << uri;
            return;
        }

        // create a client for message processing
        auto client = ExtensionClient::create(rootConfig, uri);
        if (!client) {
            CONSOLE(rootConfig) << "Failed to create client for extension: " << uri;
            return;
        }
        auto activity = getActivity(uri);
        mClients.emplace(uri, client);
        mPendingRegistrations.insert(uri);

        if (auto sessionState = getExtensionSessionState()) {
            sessionState->initialize(activity, proxy);
        }
    }
}

void
ExtensionMediator::denyExtension(const RootConfigPtr& rootConfig, const std::string& uri) {
    mPendingGrants.erase(uri);
    LOG_IF(DEBUG_EXTENSION_MEDIATOR).session(rootConfig) << "Extension denied: " << uri;
}

void
ExtensionMediator::loadExtensionsInternal(const RootConfigPtr& rootConfig, const ContentPtr& content)
{

    if (!mPendingGrants.empty()) {
        LOG(LogLevel::kWarn).session(content->getSession()) << "Loading extensions with pending grant requests.  "
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
        LOG_IF(DEBUG_EXTENSION_MEDIATOR).session(rootConfig) << "load extension: " << uri
                                         << " has extension: " << extensionProvider->hasExtension(uri);

        auto activity = getActivity(uri);
        auto sessionState = getExtensionSessionState();

        if (sessionState && sessionState->getState(activity) != ExtensionLifecycleStage::kExtensionInitialized) {
            LOG(LogLevel::kError) << "Ignoring registration for uninitialized extension: " << uri;
            mPendingRegistrations.erase(uri);
            continue;
        }

        // Get the extension from the registration
        auto proxy = extensionProvider->getExtension(uri);
        if (!proxy) {
            CONSOLE(session) << "Failed to retrieve proxy for extension: " << uri;
            mPendingRegistrations.erase(uri);
            if (sessionState) {
                // The update shouldn't fail because we know that the activity is known and
                // the transition should be allowed.
                sessionState->updateState(activity, ExtensionLifecycleStage::kExtensionFinalized);
            }
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

        auto success = proxy->getRegistration(*activity, regReq,
                [weak_this, activity](const ActivityDescriptor& descriptor, const rapidjson::Value& registrationSuccess) {
                    if (auto mediator = weak_this.lock())
                        mediator->enqueueResponse(activity, registrationSuccess);
                },
                [weak_this, activity](const ActivityDescriptor& descriptor, const rapidjson::Value& registrationFailure) {
                    if (auto mediator = weak_this.lock())
                        mediator->enqueueResponse(activity, registrationFailure);
                }
        );

        if (!success) {
            // call to extension failed without failure callback
            CONSOLE(session) << "Extension registration failure - code: " << kErrorInvalidMessage
                             << " message: " << sErrorMessage[kErrorInvalidMessage] + uri;
            mPendingRegistrations.erase(uri);
            if (sessionState) {
                // The update shouldn't fail because we know that the activity is known and
                // the transition should be allowed.
                sessionState->updateState(activity, ExtensionLifecycleStage::kExtensionFinalized);
            }
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
        CONSOLE(rootConfig) << "Cannot load extensions when Content is not ready";
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
        CONSOLE(root->getSession()) << "Attempt to execute command on unavailable extension - uri: " << uri;
        return false;
    }
    auto client = itr->second;

    // Get the Extension
    auto proxy = extPro->getExtension(uri);
    if (!proxy) {
        CONSOLE(root->getSession()) << "Attempt to execute command on unavailable extension - uri: " << uri;
        return false;
    }

    // create the message
    rapidjson::Document document;
    auto cmd = client->processCommand(document.GetAllocator(), event);
    std::weak_ptr<ExtensionMediator> weak_this = shared_from_this();

    auto activity = getActivity(uri);
    // Forward to the extension
    auto invoke = proxy->invokeCommand(*activity, cmd,
        [weak_this, activity](const ActivityDescriptor& descriptor, const rapidjson::Value& commandSuccess) {
           if (auto mediator = weak_this.lock())
               mediator->enqueueResponse(activity, commandSuccess);
        },
        [weak_this, activity](const ActivityDescriptor& descriptor, const rapidjson::Value& commandFailure) {
           if (auto mediator = weak_this.lock())
               mediator->enqueueResponse(activity, commandFailure);
        });

    if (!invoke) {
        CONSOLE(root->getSession()) << "Extension command failure - code: " << kErrorInvalidMessage
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
        CONSOLE(config) << "Proxy does not exist for uri: " << uri;
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
        CONSOLE(config) << "Attempt to use an unavailable extension - uri: " << uri;
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

    auto activity = getActivity(uri);

    rapidjson::Document document;
    rapidjson::Value message;
    message = client->createComponentChange(document.GetAllocator(), *component);

    // Notify the extension of the component change
    auto sent = proxy->sendComponentMessage(*activity, message);
    if (!sent) {
        auto config = mRootConfig.lock();
        CONSOLE(config) << "Extension message failure - code: " << kErrorInvalidMessage
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
    auto activity = getActivity(uri);

    if (auto proxy = getProxy(uri))
        proxy->onResourceReady(*activity, resourceHolder);
}

void
ExtensionMediator::resourceFail(const ExtensionComponentPtr& component, int errorCode, const std::string& error) {
    auto root = mRootContext.lock();
    if (!component)
        return;
    CONSOLE(root->getSession()) << "Extension resource failure - uri:"
                                  << component->getUri() << " resourceId:" << component->getResourceID();
    component->updateResourceState(kResourceError, errorCode, error);
}


void
ExtensionMediator::registerExtension(const std::string& uri, const ExtensionProxyPtr& extension,
                                     const ExtensionClientPtr& client)
{
    auto activity = getActivity(uri);

    //  set up callbacks for extension messages
    std::weak_ptr<ExtensionMediator> weak_this = shared_from_this();

    extension->registerEventCallback(*activity,
            [weak_this](const alexaext::ActivityDescriptor& activity, const rapidjson::Value& event) {
                if (auto mediator = weak_this.lock()) {
                    if (mediator->isEnabled()) {
                        auto activityPtr = mediator->getActivity(activity.getURI());
                        mediator->enqueueResponse(activityPtr, event);
                    }
                } else if (DEBUG_EXTENSION_MEDIATOR) {
                    LOG(LogLevel::kDebug) << "Mediator expired for event callback.";
                }
            });
    // Legacy callback for backwards compatibility with older extensions / proxies
    extension->registerEventCallback(
            [weak_this](const std::string& uri, const rapidjson::Value& event) {
                if (auto mediator = weak_this.lock()) {
                    if (mediator->isEnabled()) {
                        auto activityPtr = mediator->getActivity(uri);
                        mediator->enqueueResponse(activityPtr, event);
                    }
                } else if (DEBUG_EXTENSION_MEDIATOR) {
                    LOG(LogLevel::kDebug) << "Mediator expired for event callback.";
                }
            });
    extension->registerLiveDataUpdateCallback(*activity,
            [weak_this](const alexaext::ActivityDescriptor& activity, const rapidjson::Value& liveDataUpdate) {
                if (auto mediator = weak_this.lock()) {
                    if (mediator->isEnabled()) {
                        auto activityPtr = mediator->getActivity(activity.getURI());
                        mediator->enqueueResponse(activityPtr, liveDataUpdate);
                    }
                } else if (DEBUG_EXTENSION_MEDIATOR) {
                    LOG(LogLevel::kDebug) << "Mediator expired for live data callback.";
                }
            });
    // Legacy callback for backwards compatibility with older extensions / proxies
    extension->registerLiveDataUpdateCallback(
            [weak_this](const std::string& uri, const rapidjson::Value& liveDataUpdate) {
                if (auto mediator = weak_this.lock()) {
                    if (mediator->isEnabled()) {
                        auto activityPtr = mediator->getActivity(uri);
                        mediator->enqueueResponse(activityPtr, liveDataUpdate);
                    }
                } else if (DEBUG_EXTENSION_MEDIATOR) {
                    LOG(LogLevel::kDebug) << "Mediator expired for live data callback.";
                }
            });

    mClients.emplace(uri, client);

    auto sessionState = getExtensionSessionState();
    if (sessionState) {
        if (!sessionState->updateState(activity, ExtensionLifecycleStage::kExtensionRegistered)) {
            LOG(LogLevel::kError) << "Ignoring extension registration due to incompatible state";
            return;
        }
    }

    extension->onRegistered(*activity);

    LOG_IF(DEBUG_EXTENSION_MEDIATOR).session(mRootContext.lock()) << "registered: " << uri << " clients: " << mClients.size();
}

void
ExtensionMediator::enqueueResponse(const alexaext::ActivityDescriptorPtr& activity, const rapidjson::Value& message)
{
    if (!activity) return;

    const auto& uri = activity->getURI();
    std::weak_ptr<ExtensionMediator> weak_this = shared_from_this();
    // TODO optimize
    auto copy = std::make_shared<rapidjson::Document>();
    copy->CopyFrom(message, copy->GetAllocator());
    bool enqueued = mMessageExecutor->enqueueTask([weak_this, activity, copy] () {
        if (auto mediator = weak_this.lock()) {
            mediator->processMessage(activity, *copy);
        }
    });
    if (!enqueued)
        LOG(LogLevel::kWarn).session(mRootContext.lock()) << "failed to process message for extension, uri:" << uri;
}

void
ExtensionMediator::processMessage(const alexaext::ActivityDescriptorPtr& activity, JsonData&& processMessage)
{
    if (!activity) return;

    const auto& uri = activity->getURI();
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
        } else if (client->second->registrationFailed()) {
            // Registration failed, since registration was processed but th
            if (auto state = getExtensionSessionState()) {
                // The update shouldn't fail because we know that the activity is known and
                // the transition should be allowed.
                state->updateState(activity, ExtensionLifecycleStage::kExtensionFinalized);
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
        auto activity = getActivity(u2c.first);
        unregister(activity);
    }

    mClients.clear();
    mActivitiesByURI.clear();

    /**
     * Check if the session has already ended. If it has, make sure we trigger the end of
     * session logic since any active extension from the current mediator would have prevented
     * end of session notifications from going out. If the session hasn't ended yet, there is
     * nothing to do until it does.
     */
    if (mExtensionSession && mExtensionSession->hasEnded()) {
        onSessionEnded();
    }
}

void
ExtensionMediator::onSessionEnded() {
    auto sessionState = getExtensionSessionState();
    if (!sessionState) return;

    sessionState->endSession();
}

void
ExtensionMediator::onDisplayStateChanged(DisplayState displayState) {
    for (const auto& entry : mActivitiesByURI) {
        updateDisplayState(entry.second, displayState);
    }
}

void
ExtensionMediator::updateDisplayState(const ActivityDescriptorPtr& activity,
                                         DisplayState displayState) {
    if (!activity) return;
    auto provider = mProvider.lock();
    if (!provider) return;

    if (auto sessionState = getExtensionSessionState()) {
        if (sessionState->getState(activity) == ExtensionLifecycleStage::kExtensionRegistered) {
            auto proxy = provider->getExtension(activity->getURI());
            if (!proxy) return;

            switch (displayState) {
                case kDisplayStateForeground:
                    proxy->onForeground(*activity);
                    break;
                case kDisplayStateBackground:
                    proxy->onBackground(*activity);
                    break;
                case kDisplayStateHidden:
                    proxy->onHidden(*activity);
                    break;
                default:
                    LOG(LogLevel::kWarn) << "Unknown display state, ignoring update";
                    break;
            }
        }
    }
}

std::shared_ptr<ExtensionSessionState>
ExtensionMediator::getExtensionSessionState() const {
    if (!mExtensionSession) return nullptr;
    return mExtensionSession->getSessionState();
}

alexaext::ActivityDescriptorPtr
ExtensionMediator::getActivity(const std::string& uri) {
    auto it = mActivitiesByURI.find(uri);
    if (it != mActivitiesByURI.end()) {
        return it->second;
    }

    auto activity = ActivityDescriptor::create(uri,
            mExtensionSession ? mExtensionSession->getSessionDescriptor() : nullptr);
    mActivitiesByURI.emplace(uri, activity);
    return activity;
}

void
ExtensionMediator::unregister(const alexaext::ActivityDescriptorPtr& activity) {
    if (!activity) return;
    auto provider = mProvider.lock();
    if (!provider) return;

    auto proxy = provider->getExtension(activity->getURI());
    if (!proxy) return;

    auto sessionState = getExtensionSessionState();

    auto itr = mClients.find(activity->getURI());
    if (itr == mClients.end()) return;
    if (!itr->second->registered()) return; // Nothing to do, the activity was never successfully registered

    proxy->onUnregistered(*activity);

    if (sessionState) {
        sessionState->updateState(activity, ExtensionLifecycleStage::kExtensionUnregistered);
        if (mExtensionSession->hasEnded()) {
            // The session has already ended, so make sure we notify all extensions if the activity
            // we just unregistered was the last one for the session.
            sessionState->endSession();
        }
    }
}

} // namespace apl

#endif
