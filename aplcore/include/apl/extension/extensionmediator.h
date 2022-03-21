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
#ifndef _APL_EXTENSION_MEDIATOR_H
#define _APL_EXTENSION_MEDIATOR_H

#include <alexaext/alexaext.h>

#include "apl/content/content.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/rootcontext.h"
#include "extensionclient.h"

#include <functional>
#include <set>
#include <string>

namespace apl {

class RootContext;

using ExtensionsLoadedCallback = std::function<void()>;

/**
 * This class mediates message passing between "local" alexaext::Extension and the APL engine.  It
 * is intended for internal use by the viewhost. Remote extensions are not supported.
 *
 * ExtensionMediator is an experimental class requiring RootConfig::kExperimentalExtensionProvider.
 * It is expected to be eliminated before APL 2.0.  The class temporarily supports the following
 * extension message processes:
 * - Registration: using the loadExtensions(...) API
 * - Commands: using the invokeCommand(..) API
 * - Events: handled internally after registration, no outward API
 * - LiveData Updates: handled internally after registration, no outward API.
 * - Resource sharing: using sendResourceReady(..) API
 *
 * The message executor allows for messages from the extension to be enqueued/sequenced before
 * processing. Any message from the extension is passed through the enqueue(...) call.
 *
 * This class cannot be used with more than one Document / RootContext.
 */
class ExtensionMediator : public std::enable_shared_from_this<ExtensionMediator> {

public:
    /**
     * @deprecated
     * Create a message mediator for the alexaext:Extensions registered with given
     * alexaext::ExtensionProvider.
     * @param provider The extension provider.
     */
    static ExtensionMediatorPtr create(const alexaext::ExtensionProviderPtr& provider) {
        return std::make_shared<ExtensionMediator>(provider, nullptr,
                                                   alexaext::Executor::getSynchronousExecutor());
    }

    /**
     * Create a message mediator for the alexaext:Extensions registered with given
     * alexaext::ExtensionProvider.
     *
     * @param provider The extension provider.
     * @param messageExecutor Process an extension message in a manner consistent with the APL
     * execution model.
     */
    static ExtensionMediatorPtr create(const alexaext::ExtensionProviderPtr& provider,
                                       const alexaext::ExecutorPtr& messageExecutor) {
        return std::make_shared<ExtensionMediator>(provider, nullptr, messageExecutor);
    }

    /**
     * Create a message mediator for the alexaext:Extensions registered with given
     * alexaext::ExtensionProvider.
     *
     * @param provider The extension provider.
     * @param resourceProvider The provider for resources shared with the extension.
     * @param messageExecutor Process an extension message in a manner consistent with the APL
     * execution model.
     */
    static ExtensionMediatorPtr
    create(const alexaext::ExtensionProviderPtr& provider,
           const alexaext::ExtensionResourceProviderPtr& resourceProvider,
           const alexaext::ExecutorPtr& messageExecutor) {
        return std::make_shared<ExtensionMediator>(provider, resourceProvider, messageExecutor);
    }


    /**
     * Signal the grant or deny of a requested extension.
     * @c ExtensionGrantRequestCallback
     */
    using ExtensionGrantResult = std::function<void(const std::string uri)>;

    /**
     * Request handler used to grant/deny use of the extension.
     */
    using ExtensionGrantRequestCallback = std::function<void(const std::string& uri,
                                                             ExtensionGrantResult grant,
                                                             ExtensionGrantResult deny)>;

    /**
     * Initialize extensions available in provided content. Performance gains can be made
     * by initializing extensions as each apl::Content package is loaded. Once Content is ready,
     * and all packages have been initialized, @c loadExtensions should be used to register the
     * extensions for use.
     *
     * An optional grant request handler is used to grant/deny use of the extension.  In the
     * absence of the grant handler use of the extension is automatically granted.  Calling
     * loadExtensions before a grant/deny response results in the extension being unavailable for
     * use.
     *
     * @param rootConfig The RootConfig receiving the registered extensions.
     * @param content The document content, contains requested extensions
     * @param grantHandler Callback that grants use of the extension.
     */
    void initializeExtensions(const RootConfigPtr& rootConfig, const ContentPtr& content,
                              const ExtensionGrantRequestCallback& grantHandler = nullptr);

    /**
     * Register the extensions found in the associated  alexaext::ExtensionProvider.  This method
     * should be used in conjunction with @c initializeExtensions.  Performance gains can be made
     * by initializing extensions as each Content package is loaded.  @c initializeExtensions.
     *
     * Must be called before RootContext::create();
     *
     * @param rootConfig The RootConfig receiving the registered extensions.
     * @param content The document content, contains requested extensions and extension settings.
     * @param loaded Callback to be called when all extensions required by the doc are loaded.
     */
    void loadExtensions(const RootConfigPtr& rootConfig, const ContentPtr& content,
                        ExtensionsLoadedCallback loaded);

    /**
     * Register the extensions found in the associated  alexaext::ExtensionProvider. This method
     * performs @c initializeExtensions and @c loadExtensions. It is less performant due to the
     * sequential execution of initialization and loading.
     *
     * An optional set of extension uri representing extensions that have be granted for use may
     * be provided.  In the absence of the granted extension set all extensions are automatically
     * granted.
     *
     * Must be called before RootContext::create();
     *
     * @param rootConfig The RootConfig receiving the registered extensions.
     * @param content The document content, contains requested extensions and extension settings.
     * @param grantedExtensions Pre-granted extensions, may be null.
     */
    void loadExtensions(const RootConfigPtr& rootConfig, const ContentPtr& content,
                        const std::set<std::string>* grantedExtensions = nullptr);


    /**
     * Process an extension event. The extension must be registered in the associated
     * alexaext::ExtensionProvider.
     *
     * @param even The event with type kEventTypeExtension.
     * @param root The root context.
     * @return true if the command was invoked.
     */
    bool invokeCommand(const Event& even);

    /**
     * Notify the extension that the component has changed.  Changes may be a result of document
     * command, or runtime change in the resource state.
     *
     * @param component ExtensionComponent reference.
     * @param resourceNeeded A rendering resource is needed for the component.
     */
    void notifyComponentUpdate(const ExtensionComponentPtr& component, bool resourceNeeded);

    /**
     * Use create(...)
     */
    explicit ExtensionMediator(const alexaext::ExtensionProviderPtr& provider,
                               const alexaext::ExtensionResourceProviderPtr& resourceProvider,
                               const alexaext::ExecutorPtr& messageExecutor)
        : mProvider(provider),
          mResourceProvider(resourceProvider),
          mMessageExecutor(messageExecutor) {}

    /**
     * Destructor.
     */
    ~ExtensionMediator() { mClients.clear(); }

    /**
     * @return @c true if this mediator is enabled, @c false otherwise.
     */
    bool isEnabled() const { return mEnabled; }

    /**
     * Enables or disables this mediator. Disabled mediators will not process incoming messages.
     * This is useful when the document associated with the mediator is being background-ed.
     *
     * Mediators are enabled when first created.
     *
     * @param enabled @c true if this mediator should become enabled, @c false if it should become
     * disabled.
     */
    void enable(bool enabled) { mEnabled = enabled; }


    /**
     * Clear the internal state and unregister all extensions.
     */
    void finish();

private:
    friend class RootContext;

    /**
     * Initialize an extension that was granted approval for use.
     */
    void grantExtension(const RootConfigPtr& rootConfig, const std::string& uri);

    /**
     * Stop initialization on a denied extension.
     */
    void denyExtension(const std::string& uri);

    /**
     * Perform extension registration requests.
     */
    void loadExtensionsInternal(const RootConfigPtr& rootConfig, const ContentPtr& content);

    /**
     * Associate a RootContext to the mediator for event and live data updates.
     */
    void bindContext(const RootContextPtr& context);

    /**
     * Registers the extensions found in the ExtensionProvider by calling
     * RootConfig::registerExtensionXXX().
     */
    void registerExtension(const std::string& uri, const alexaext::ExtensionProxyPtr& extension,
                           const ExtensionClientPtr& client);

    /**
     * Enqueue a message with the executor in response to an extension callback.
     */
    void enqueueResponse(const std::string& uri, const rapidjson::Value& message);

    /**
     * Delegate a message to the extension client for processing.
     * @return true if the message was processed.
     */
    void processMessage(const std::string& uri, JsonData&& message);

    /**
     * Get Proxy corresponding to requested uri.
     * @param uri extension URI.
     * @return Proxy, if exists, null otherwise.
     */
    alexaext::ExtensionProxyPtr getProxy(const std::string& uri);

    /**
     * Get the extension client corresponding to requested uri.
     * @param uri extension URI.
     * @return ExtensionClient, if exists, null otherwise.
     */
    ExtensionClientPtr getClient(const std::string& uri);

    /**
     * Send a resource to an extension.
     * @param component ExtensionComponent reference.
     * @param uri Extension URI.
     * @param resourceHolder ResourceHolder reference.
     */
    void sendResourceReady(const std::string& uri,
                           const alexaext::ResourceHolderPtr& resourceHolder);

    /**
     * Component resource could not be acquired.
     * @param component ExtensionComponent reference.
     * @param errorCode Failure error code.
     * @param error Message associated with error code.
     */
     void resourceFail(const ExtensionComponentPtr& component, int errorCode, const std::string& error);

    // access to the extensions
    std::weak_ptr<alexaext::ExtensionProvider> mProvider;
    // access to the extension resources
    std::weak_ptr<alexaext::ExtensionResourceProvider> mResourceProvider;
    // the context that events and data updates are forwarded to
    std::weak_ptr<RootContext> mRootContext;
    // reference to associated config
    std::weak_ptr<RootConfig> mRootConfig;
    // retro extension wrapper used for message passing
    std::map<std::string, std::shared_ptr<ExtensionClient>> mClients;
    // executor to enqueue/sequence message processing
    alexaext::ExecutorPtr mMessageExecutor;
    // Determines whether incoming messages from extensions should be processed.
    bool mEnabled = true;
    // Pending Extension grants
    std::set<std::string> mPendingGrants;
    // Pending Extensions to register.
    std::set<std::string> mPendingRegistrations;
    // Extensions loaded callback
    ExtensionsLoadedCallback mLoadedCallback;
};

} // namespace apl

#endif //_APL_EXTENSION_MEDIATOR_H
#endif // ALEXAEXTENSIONS
