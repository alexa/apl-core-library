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

#ifndef _APL_EXTENSION_CLIENT_H
#define _APL_EXTENSION_CLIENT_H

#include "apl/common.h"
#include "apl/content/extensioncommanddefinition.h"
#include "apl/content/extensioncomponentdefinition.h"
#include "apl/content/extensioneventhandler.h"
#include "apl/content/extensionfilterdefinition.h"
#include "apl/content/extensionproperty.h"
#include "apl/content/jsondata.h"
#include "apl/engine/event.h"
#include "apl/livedata/livedataobjectwatcher.h"
#include "apl/livedata/liveobject.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/session.h"

namespace apl {

class LiveArrayChange;
class LiveMapChange;

/**
 * Live data types that can be processed by extension.
 */
enum ExtensionLiveDataType {
    kExtensionLiveDataTypeArray,
    kExtensionLiveDataTypeObject,
};

/**
 * Live data operations enumeration.
 */
enum ExtensionLiveDataUpdateType {
    kExtensionLiveDataUpdateTypeInsert,
    kExtensionLiveDataUpdateTypeUpdate,
    kExtensionLiveDataUpdateTypeSet,
    kExtensionLiveDataUpdateTypeRemove,
    kExtensionLiveDataUpdateTypeClear,
};

/**
 * Extension processing methods enumeration.
 */
enum ExtensionMethod {
    kExtensionMethodRegister,
    kExtensionMethodRegisterSuccess,
    kExtensionMethodRegisterFailure,
    kExtensionMethodCommand,
    kExtensionMethodCommandSuccess,
    kExtensionMethodCommandFailure,
    kExtensionMethodEvent,
    kExtensionMethodLiveDataUpdate,
    // Component state/update message from the execution environment to the extension
    kExtensionMethodComponent,
    // Component success messages from the extension to the execution environment
    kExtensionMethodComponentSuccess,
    // Component creation/operation failure message from the extension to the execution environment
    kExtensionMethodComponentFailure,
    // Component creation success messages from the extension to the execution environment
    kExtensionMethodComponentUpdate,
};

/**
 * Event execution mode.
 */
enum ExtensionEventExecutionMode {
    kExtensionEventExecutionModeNormal,
    kExtensionEventExecutionModeFast,
};

class ActionRef;
class PropertyTriggerEvent;
class LiveDataRef;

using TypePropertiesPtr = std::shared_ptr<std::map<std::string, ExtensionProperty>>;

class PropertyTriggerEvent {
public:
    std::string name;
    std::map<std::string, bool> params;
};

class LiveDataRef {
public:
    std::string name;
    ExtensionLiveDataType objectType;
    std::string type;
    LiveObjectPtr objectPtr;
    bool hasPendingUpdate;
    PropertyTriggerEvent addEvent;
    PropertyTriggerEvent updateEvent;
    PropertyTriggerEvent removeEvent;
};

class ExtensionEvent {
public:
    std::string uri;
    std::string name;
    ObjectMap data;
    bool fastMode;
    std::string resourceId;
};

/**
 * Encapsulate schema information that ExtensionClient is responsible for collecting during
 * registration. This information can be retrieved via @see ExtensionClient::extensionSchema
 */
struct ParsedExtensionSchema {
    Object environment;
    std::map<std::string, TypePropertiesPtr> types;
    std::vector<ExtensionEventHandler> eventHandlers;
    std::vector<ExtensionCommandDefinition> commandDefinitions;
    std::vector<ExtensionFilterDefinition> filterDefinitions;
    std::vector<ExtensionComponentDefinition> componentDefinitions;
    std::map<std::string, LiveObjectPtr> liveData;
    std::map<std::string, bool> eventModes;
};

/**
 * Extension processing client. Refer to unittest_extension_client.cpp for suggested lifecycle.
 */
class ExtensionClient : public Counter<ExtensionClient>,
                        public LiveDataObjectWatcher,
                        public NonCopyable,
                        public std::enable_shared_from_this<ExtensionClient> {
public:
    /**
     * @param rootConfig rootConfig pointer.
     * @param uri Requested extension URI.
     * @return ExtensionClient pointer.
     * @deprecated Extensions should be managed via ExtensionMediator
     * uri, const SessionPtr& session);
     */
    APL_DEPRECATED static ExtensionClientPtr create(const RootConfigPtr& rootConfig, const std::string& uri);

    /**
     * @param rootConfig rootConfig pointer.
     * @param uri Requested extension URI.
     * @param session Session
     * @return ExtensionClient pointer.
     * @deprecated Extensions should be managed via ExtensionMediator
     */
    static ExtensionClientPtr create(const RootConfigPtr& rootConfig, const std::string& uri, const SessionPtr& session);

    /**
     * Constructor. Do not use - let ExtensionMediator create clients instead.
     */
    ExtensionClient(const std::string& uri, const SessionPtr& session, const Object& flags);

    /**
     * Destructor
     */
    ~ExtensionClient() override = default;

    /**
     * Form a registration request for current extension.
     * @param allocator JSON allocator.
     * @param content APL content that has appropriate extension settings.
     * @return JSON representing registration request.
     */
    rapidjson::Value createRegistrationRequest(rapidjson::Document::AllocatorType& allocator,
                                               Content& content);

    /**
     * Form a registration request for an extension. Static utility method that could be used outside of connection context.
     * @param allocator JSON allocator.
     * @param uri extension URI.
     * @param settings Settings object to use.
     * @param flags Opaque data passed to the extension from the execution environment.
     * @return JSON representing registration request.
     */
    static rapidjson::Value createRegistrationRequest(rapidjson::Document::AllocatorType& allocator,
                                                      const std::string& uri,
                                                      const Object& settings,
                                                      const Object& flags = Object::NULL_OBJECT());

    /**
     * Form a component change message to be sent to the extension.  Changes may be a result of the
     * document updating the component properties via command, or the runtime changing the
     * kPropertyResource state when the system resource changes.
     *
     * @param allocator JSON allocator
     * @param component Extension component
     * @return JSON representing component update
     */
    rapidjson::Value createComponentChange(rapidjson::Document::AllocatorType& allocator,
                                            ExtensionComponent& component);

    /**
     * @return The URI of the extension
     */
    std::string getUri() const;

    /**
     * @return True if RegisterSuccess or RegisterFailure was processed. False otherwise.
     */
    bool registrationMessageProcessed() const;

    /**
     * @return True if extension was successfully registered. False otherwise.
     */
    bool registered() const;

    /**
     * @return True if extension failed to register (i.e. registration was processed but failed).
     *         False otherwise.
     */
    bool registrationFailed() const;

    /**
     * @return Extension-related information collected during registration
     */
    const ParsedExtensionSchema& extensionSchema() const;

    /**
     * @return The assigned connection token.
     */
    std::string getConnectionToken() const;

    /**
     * Process service message directed to this extension.
     * @param rootContext alive root context.
     * @param message service message in form of serialized JSON.
     * @return true if processed successfully, false otherwise.
     */
    bool processMessage(const RootContextPtr& rootContext, JsonData&& message);

    /**
     * Associate a RootContext to the mediator for events and live data triggers.
     * @param rootContext ctx to bind.
     */
    void bindContext(const CoreRootContextPtr& rootContext);

    /**
     * Process extension command into serialized command request.
     * @param allocator JSON allocator.
     * @param event Extension event.
     * @return JSON representing command request.
     */
    rapidjson::Value processCommand(rapidjson::Document::AllocatorType& allocator, const Event& event);

    /**
     * @deprecated @c createComponentChange
     */
    APL_DEPRECATED rapidjson::Value processComponentRequest(rapidjson::Document::AllocatorType& allocator,
                                                            ExtensionComponent& component);

    /**
     * @deprecated @c createComponentChange
     */
    APL_DEPRECATED rapidjson::Value processComponentUpdate(rapidjson::Document::AllocatorType& allocator,
                                                           ExtensionComponent& component);

    /**
     * @deprecated @c createComponentChange
     */
    APL_DEPRECATED rapidjson::Value processComponentRelease(rapidjson::Document::AllocatorType& allocator,
                                                            ExtensionComponent& component);

    /**
     * Handle disconnection from an extension. It could either be a graceful disconnection or
     * connection failure.
     * @param errorCode Error code associated with disconnection
     * @param message Disconnection message.
     * @return true if processed successfully, false otherwise.
     */
    bool handleDisconnection(const RootContextPtr& rootContext, int errorCode, const std::string& message);

protected:
    /// LiveDataObjectWatcher methods
    void liveDataObjectFlushed(const std::string& key, LiveDataObject& liveDataObject) override;

private:
    friend class ExtensionMediator;
    friend class ExtensionManager;

    // Parse an extension from json
    bool readExtension(const Context& context, const Object& extension);
    bool readExtensionTypes(const Context& context, const Object& types);
    bool readExtensionCommandDefinitions(const Context& context, const Object& commands);
    void readExtensionComponentCommandDefinitions(const Context& context, const Object& commands, ExtensionComponentDefinition& def);
    std::vector<ExtensionCommandDefinition> readCommandDefinitionsInternal(const Context& context,const ObjectArray& commands);
    bool readExtensionEventHandlers(const Context& context, const Object& handlers);
    bool readExtensionLiveData(const Context& context, const Object& liveData);
    bool readExtensionComponentDefinitions(const Context& context, const Object& components);
    bool readExtensionComponentEventHandlers(const Context& context, const Object& handlers, ExtensionComponentDefinition& def);

    bool processRegistrationResponse(const Context& context, const Object& connectionResponse);
    bool processEvent(const Context& context, const Object& event);
    bool processCommandResponse(const Context& context, const Object& response);
    bool processLiveDataUpdate(const Context& context, const Object& update);
    bool processComponentResponse(const Context& context, const Object& response);

    bool updateLiveMap(ExtensionLiveDataUpdateType type, const LiveDataRef& dataRef, const Object& operation);
    bool updateLiveArray(ExtensionLiveDataUpdateType type, const LiveDataRef& dataRef, const Object& operation);
    void reportLiveMapChanges(const LiveDataRef& ref, const std::vector<LiveMapChange>& changes);
    void reportLiveArrayChanges(const LiveDataRef& ref, const std::vector<LiveArrayChange>& changes);

    void sendLiveDataEvent(const std::string& event, const Object& current, const Object& changed);
    void flushPendingEvents(const CoreDocumentContextPtr& rootContext);

    std::map<std::string, bool> readPropertyTriggers(const Context& context, const TypePropertiesPtr& type, const Object& triggers);

    void invokeExtensionHandler(const std::string& uri, const std::string& name,
                                const ObjectMap& data, bool fastMode,
                                std::string resourceId = "");
    void bindContextInternal(const CoreDocumentContextPtr& documentContext);
    bool processMessageInternal(const CoreDocumentContextPtr& documentContext, JsonData&& message);
    bool handleDisconnectionInternal(const CoreDocumentContextPtr& documentContext, int errorCode, const std::string& message);

    static id_type sCommandIdGenerator;

    bool mRegistrationProcessed;
    bool mRegistered;
    std::string mUri;
    ParsedExtensionSchema mSchema;
    SessionPtr mSession;
    Object mFlags;
    std::shared_ptr<RootConfig> mInternalRootConfig;
    std::string mConnectionToken;
    std::map<std::string, LiveDataRef> mLiveData;
    std::map<id_type, ActionRef> mActionRefs;
    std::weak_ptr<CoreDocumentContext> mCachedContext;
    std::vector<ExtensionEvent> mPendingEvents;
};

} // namespace apl

#endif // _APL_EXTENSION_CLIENT_H
