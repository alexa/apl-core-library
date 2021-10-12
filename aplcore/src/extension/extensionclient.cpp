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

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "apl/content/rootconfig.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/componentproperties.h"
#include "apl/component/corecomponent.h"
#include "apl/content/content.h"
#include "apl/extension/extensionclient.h"
#include "apl/extension/extensionmanager.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/arrayify.h"
#include "apl/engine/binding.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/rootcontext.h"
#include "apl/livedata/livearray.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/livedata/livemap.h"
#include "apl/livedata/livemapobject.h"
#include "apl/utils/random.h"
#include "apl/utils/streamer.h"

namespace apl {

/// Simple "semi-unique" generator for command IDs.
id_type ExtensionClient::sCommandIdGenerator = 1000;

static const std::string IMPLEMENTED_INTERFACE_VERSION = "1.0";
static const std::string MAX_SUPPORTED_SCHEMA_VERSION = "1.1";

static const std::string ROOT_CONFIG_MISSING = "RootConfig unavailable. Should not happen.";

static const Bimap<ExtensionLiveDataUpdateType, std::string> sExtensionLiveDataUpdateTypeBimap = {
    {kExtensionLiveDataUpdateTypeInsert, "Insert"},
    {kExtensionLiveDataUpdateTypeUpdate, "Update"},
    {kExtensionLiveDataUpdateTypeSet,    "Set"},
    {kExtensionLiveDataUpdateTypeRemove, "Remove"},
    {kExtensionLiveDataUpdateTypeClear,  "Clear"},
};

static const Bimap<ExtensionMethod, std::string> sExtensionMethodBimap = {
    {kExtensionMethodRegister,         "Register"},
    {kExtensionMethodRegisterSuccess,  "RegisterSuccess"},
    {kExtensionMethodRegisterFailure,  "RegisterFailure"},
    {kExtensionMethodCommand,          "Command"},
    {kExtensionMethodCommandSuccess,   "CommandSuccess"},
    {kExtensionMethodCommandFailure,   "CommandFailure"},
    {kExtensionMethodEvent,            "Event"},
    {kExtensionMethodLiveDataUpdate,   "LiveDataUpdate"},
    {kExtensionMethodComponent,        "Component"},
    {kExtensionMethodComponentSuccess, "ComponentSuccess"},
    {kExtensionMethodComponentFailure, "ComponentFailure"},
    {kExtensionMethodComponentUpdate,  "ComponentUpdate"},
};

static const Bimap<ExtensionEventExecutionMode, std::string> sExtensionEventExecutionModeBimap = {
    {kExtensionEventExecutionModeNormal, "NORMAL"},
    {kExtensionEventExecutionModeFast,   "FAST"},
};

static const Bimap<ExtensionComponentResourceState, std::string> sExtensionComponentStateBimap = {
    {kResourcePending,  "Pending"},
    {kResourceReady,    "Ready"},
    {kResourceError,    "Error"},
    {kResourceReleased, "Released"}
};

ExtensionClientPtr
ExtensionClient::create(const RootConfigPtr& rootConfig, const std::string& uri)
{
    return std::make_shared<ExtensionClient>(rootConfig, uri);
}

ExtensionClient::ExtensionClient(const RootConfigPtr& rootConfig, const std::string& uri)
    :  mRegistrationProcessed(false), mRegistered(false), mUri(uri), mRootConfig(rootConfig)
{}

rapidjson::Value
ExtensionClient::createRegistrationRequest(rapidjson::Document::AllocatorType& allocator, Content& content) {
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return rapidjson::Value(rapidjson::kNullType);
    }

    const auto& settings = content.getExtensionSettings(mUri);
    const auto& flags = rootConfig->getExtensionFlags(mUri);
    return createRegistrationRequest(allocator, mUri, settings, flags);
}

rapidjson::Value
ExtensionClient::createRegistrationRequest(rapidjson::Document::AllocatorType& allocator,
        const std::string& uri, const Object& settings, const Object& flags) {
    auto request = std::make_shared<ObjectMap>();
    request->emplace("method", sExtensionMethodBimap.at(kExtensionMethodRegister));
    request->emplace("version", IMPLEMENTED_INTERFACE_VERSION);
    request->emplace("uri", uri);
    request->emplace("settings", settings);
    request->emplace("flags", flags);

    return Object(request).serialize(allocator);
}

bool
ExtensionClient::registrationMessageProcessed()
{
    return mRegistrationProcessed;
}

bool
ExtensionClient::registered()
{
    return mRegistered;
}

std::string
ExtensionClient::getConnectionToken() const
{
    return mConnectionToken;
}

bool
ExtensionClient::processMessage(const RootContextPtr& rootContext, JsonData&& message)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    if (!message) {
        CONSOLE_CFGP(rootConfig).log("Malformed offset=%u: %s.", message.offset(), message.error());
        return false;
    }

    const auto& context = rootContext ? rootContext->context() : rootConfig->evaluationContext();
    auto evaluated = Object(std::move(message).get());
    auto method = propertyAsMapped<ExtensionMethod>(context, evaluated, "method", static_cast<ExtensionMethod>(-1), sExtensionMethodBimap);

    if (!mRegistered) {
        if (mRegistrationProcessed) {
            CONSOLE_CFGP(rootConfig).log("Can't process message after failed registration.");
            return false;
        } else if (method != kExtensionMethodRegisterSuccess && method != kExtensionMethodRegisterFailure) {
            CONSOLE_CFGP(rootConfig).log("Can't process message before registration.");
            return false;
        }
    }

    if (rootContext) {
        mCachedContext = rootContext;
    }

    auto version = propertyAsObject(context, evaluated, "version");
    if (version.isNull() || version.getString() != IMPLEMENTED_INTERFACE_VERSION) {
        CONSOLE_CFGP(rootConfig) << "Interface version is wrong. Expected=" << IMPLEMENTED_INTERFACE_VERSION
                            << "; Actual=" << version.toDebugString();
        return false;
    }

    bool result = true;
    switch (method) {
        case kExtensionMethodRegisterSuccess:
            result = processRegistrationResponse(context, evaluated);
        case kExtensionMethodRegisterFailure:
            mRegistrationProcessed = true;
            break;
        case kExtensionMethodCommandSuccess: // FALL_THROUGH
        case kExtensionMethodCommandFailure:
            result = processCommandResponse(context, evaluated);
            break;
        case kExtensionMethodEvent:
            result = processEvent(context, evaluated);
            break;
        case kExtensionMethodLiveDataUpdate:
            result = processLiveDataUpdate(context, evaluated);
            break;
//        case kExtensionMethodRegister: // Outgoing commands should not be processed here.
//        case kExtensionMethodCommand:
        case kExtensionMethodComponentSuccess:
        case kExtensionMethodComponentFailure:
        case kExtensionMethodComponentUpdate:
            result = processComponentResponse(context, evaluated);
            break;
        default:
            CONSOLE_CFGP(rootConfig).log("Unknown method");
            result = false;
            break;
    }

    return result;
}

bool
ExtensionClient::processRegistrationResponse(const Context& context, const Object& connectionResponse)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    if (mRegistered) {
        CONSOLE_CFGP(rootConfig).log("Can't register extension twice.");
        return false;
    }

    auto connectionToken = propertyAsObject(context, connectionResponse, "token");
    auto schema = propertyAsObject(context, connectionResponse, "schema");
    if (connectionToken.isNull() || connectionToken.empty() || schema.isNull()) {
        CONSOLE_CFGP(rootConfig).log("Malformed connection response message.");
        return false;
    }

    if (!readExtension(context, schema)) {
        CONSOLE_CFGP(rootConfig).log("Malformed schema.");
        return false;
    }

    const auto& assignedToken = connectionToken.getString();
    if (assignedToken == "<AUTO_TOKEN>") {
        mConnectionToken = Random::generateToken(mUri);
    } else {
        mConnectionToken = assignedToken;
    }

    auto environment = propertyAsRecursive(context, connectionResponse, "environment");
    if(environment.isMap()) {
        // Override environment to one that is provided in response as we set it to nothing when initially register
        // extension.
        rootConfig->registerExtensionEnvironment(mUri, environment);
    }

    mRegistered = true;
    return true;
}

bool
ExtensionClient::processEvent(const Context& context, const Object& event)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    auto rootContext = mCachedContext.lock();
    if (!rootContext) {
        CONSOLE_CFGP(rootConfig).log("Can't process message without RootContext.");
        return false;
    }

    auto name = propertyAsObject(context, event, "name");
    if (!name.isString() || name.empty() || (mEventModes.find(name.getString()) == mEventModes.end())) {
        CONSOLE_CFGP(rootConfig) << "Invalid extension event name for extension=" << mUri
                << " name:" << name.toDebugString();
        return false;
    }

    auto target = propertyAsObject(context, event, "target");
    if (!target.isString() || target.empty() || target.getString() != mUri) {
        CONSOLE_CFGP(rootConfig) << "Invalid extension event target for extension=" << mUri;
        return false;
    }

    auto payload = propertyAsRecursive(context, event, "payload");
    if (!payload.isNull() && !payload.isMap()) {
        CONSOLE_CFGP(rootConfig) << "Invalid extension event data for extension=" << mUri;
        return false;
    }

    auto resourceId = propertyAsString(context, event, "resourceId");
    rootContext->invokeExtensionEventHandler(mUri,
                                             name.getString(),
                                             payload.isNull() ? Object::EMPTY_MAP().getMap() : payload.getMap(),
                                             mEventModes.at(name.getString()) == kExtensionEventExecutionModeFast,
                                             resourceId);
    return true;
}

rapidjson::Value
ExtensionClient::processCommand(rapidjson::Document::AllocatorType& allocator, const Event& event)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return rapidjson::Value(rapidjson::kNullType);
    }

    if (kEventTypeExtension != event.getType()) {
        CONSOLE_CFGP(rootConfig) << "Invalid extension command type for extension=" << mUri;
        return rapidjson::Value(rapidjson::kNullType);
    }

    auto extensionURI = event.getValue(kEventPropertyExtensionURI);
    if (!extensionURI.isString() || extensionURI.getString() != mUri) {
        CONSOLE_CFGP(rootConfig) << "Invalid extension command target for extension=" << mUri;
        return rapidjson::Value(rapidjson::kNullType);
    }

    auto commandName = event.getValue(kEventPropertyName);
    if (!commandName.isString() || commandName.empty()) {
        CONSOLE_CFGP(rootConfig) << "Invalid extension command name for extension=" << mUri;
        return rapidjson::Value(rapidjson::kNullType);
    }

    auto resourceId = event.getValue(kEventPropertyExtensionResourceId);
    if (!resourceId.empty() && !resourceId.isString()) {
        CONSOLE_CFGP(rootConfig) << "Invalid extension component handle for extension=" << mUri;
        return rapidjson::Value (rapidjson::kNullType);
    }

    auto result = std::make_shared<ObjectMap>();
    result->emplace("version", IMPLEMENTED_INTERFACE_VERSION);
    result->emplace("method", sExtensionMethodBimap.at(kExtensionMethodCommand));
    result->emplace("token", mConnectionToken);
    if (!resourceId.empty()) {
        result->emplace("resourceId", resourceId);
    }
    auto id = sCommandIdGenerator++;
    result->emplace("id", id  );

    auto actionRef = event.getActionRef();
    if (!actionRef.isEmpty() && actionRef.isPending()) {
        actionRef.addTerminateCallback([this, id](const TimersPtr&) {
            mActionRefs.erase(id);
        });
        mActionRefs.emplace(id, actionRef);
    }

    auto parameters = event.getValue(kEventPropertyExtension);
    result->emplace("name", commandName.getString());
    result->emplace("target", extensionURI.getString());
    result->emplace("payload", parameters);

    return Object(result).serialize(allocator);
}

bool
ExtensionClient::processCommandResponse(const Context& context, const Object& response)
{
    // Resolve any Action associated with a command response.  The action is resolved independent
    // of a Success or Failure in command execution
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    auto id = propertyAsObject(context, response, "id");
    if (!id.isNumber() || id.getInteger() > sCommandIdGenerator) {
        CONSOLE_CFGP(rootConfig) << "Invalid extension command response for extension=" << mUri << " id="
            << id.toDebugString() << " total pending=" << mActionRefs.size();
        return false;
    }

    if (!mActionRefs.count(id.getDouble())) {
        // no action ref is tracked.
        return true;
    }

    auto resultObj = propertyAsObject(context, response, "result");
    if (!resultObj.isNull()) {
        // TODO: We don't have resolves with arbitrary results.
    }

    auto ref = mActionRefs.find(id.getDouble());
    ref->second.resolve();
    mActionRefs.erase(ref);

    return true;
}

void
ExtensionClient::liveDataObjectFlushed(const std::string& key, LiveDataObject& liveDataObject)
{
    if (!mLiveData.count(key)) {
        LOG(LogLevel::kWarn) << "Received update for unhandled LiveData " << key;
        return;
    }

    auto rootContext = mCachedContext.lock();
    if (!rootContext) {
        LOG(LogLevel::kWarn) << "RootContext not available";
        return;
    }

    auto ref = mLiveData.at(key);
    switch (ref.objectType) {
        case kExtensionLiveDataTypeArray:
            reportLiveArrayChanges(rootContext, ref, liveDataObject);
            break;
        case kExtensionLiveDataTypeObject:
            reportLiveMapChanges(rootContext, ref, liveDataObject);
            break;
    }
}

void
ExtensionClient::sendLiveDataEvent(const RootContextPtr& rootContext, const std::string& event,
        const Object& current, const Object& changed)
{
    ObjectMap parameters;
    parameters.emplace("current", current);
    if (!changed.isNull()) {
        parameters.emplace("changed", changed);
    }
    rootContext->invokeExtensionEventHandler(mUri, event, parameters, true);
}

void
ExtensionClient::reportLiveMapChanges(const RootContextPtr& rootContext, const LiveDataRef& ref,
        LiveDataObject& liveDataObject)
{
    std::set<std::string> updatedCollapsed;
    std::set<std::string> removedCollapsed;
    std::string updateTriggerEvent = ref.updateEvent.name;
    std::string removeTriggerEvent = ref.removeEvent.name;

    auto mapPtr = std::make_shared<ObjectMap>(std::dynamic_pointer_cast<LiveMap>(ref.objectPtr)->getMap());
    auto changes = dynamic_cast<LiveMapObject&>(liveDataObject).getChanges();

    for (auto& change : changes) {
        auto key = change.key();
        auto changed = std::make_shared<ObjectMap>();
        switch (change.command()) {
            case LiveMapChange::Command::SET:
                if (ref.updateEvent.params.count(key)) {
                    if (ref.updateEvent.params.at(key)) {
                        updatedCollapsed.emplace(key);
                    } else {
                        changed->emplace(key, liveDataObject.get(key));
                        sendLiveDataEvent(rootContext, updateTriggerEvent, mapPtr, changed);
                    }
                }
                break;
            case LiveMapChange::Command::REMOVE:
                if (ref.removeEvent.params.count(key)) {
                    if (ref.removeEvent.params.at(key)) {
                        removedCollapsed.emplace(key);
                    } else {
                        changed->emplace(key, Object::NULL_OBJECT());
                        sendLiveDataEvent(rootContext, removeTriggerEvent, mapPtr, changed);
                    }
                }
                break;
            default:
                LOG(LogLevel::kWarn) << "Unknown LiveDataObject change type: " << change.command()
                                    << " for: " << ref.name;
                break;
        }
    }

    if (!updatedCollapsed.empty()) {
        auto changed = std::make_shared<ObjectMap>();
        for (auto c : updatedCollapsed) {
            changed->emplace(c, liveDataObject.get(c));
        }
        sendLiveDataEvent(rootContext, updateTriggerEvent, mapPtr, changed);
    }
    if (!removedCollapsed.empty()) {
        auto changed = std::make_shared<ObjectMap>();
        for (auto c : removedCollapsed) {
            changed->emplace(c, liveDataObject.get(c));
        }
        sendLiveDataEvent(rootContext, removeTriggerEvent, mapPtr, changed);
    }
}

void
ExtensionClient::reportLiveArrayChanges(const RootContextPtr& rootContext, const LiveDataRef& ref,
        LiveDataObject& liveDataObject)
{
    std::string addTriggerEvent;
    std::string updateTriggerEvent;
    std::string removeTriggerEvent;

    auto arrayPtr = std::make_shared<ObjectArray>(std::dynamic_pointer_cast<LiveArray>(ref.objectPtr)->getArray());
    auto changes = dynamic_cast<LiveArrayObject&>(liveDataObject).getChanges();

    for (auto& change : changes) {
        std::string triggerEvent;

        switch (change.command()) {
            case LiveArrayChange::Command::INSERT :
                addTriggerEvent = ref.addEvent.name;
                break;
            case LiveArrayChange::Command::UPDATE:
                updateTriggerEvent = ref.updateEvent.name;
                break;
            case LiveArrayChange::Command::REMOVE:
                removeTriggerEvent = ref.removeEvent.name;
                break;
            default:
                LOG(LogLevel::kWarn) << "Unknown LiveDataObject change type: " << change.command()
                                    << " for: " << ref.name;
                break;
        }
    }

    if (!addTriggerEvent.empty()) {
        sendLiveDataEvent(rootContext, addTriggerEvent, arrayPtr, Object::NULL_OBJECT());
    }
    if (!updateTriggerEvent.empty()) {
        sendLiveDataEvent(rootContext, updateTriggerEvent, arrayPtr, Object::NULL_OBJECT());
    }
    if (!removeTriggerEvent.empty()) {
        sendLiveDataEvent(rootContext, removeTriggerEvent, arrayPtr, Object::NULL_OBJECT());
    }
}

bool
ExtensionClient::processLiveDataUpdate(const Context& context, const Object& update)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    auto name = propertyAsObject(context, update, "name");
    if (!name.isString() || name.empty() || (mLiveData.find(name.getString()) == mLiveData.end())) {
        CONSOLE_CFGP(rootConfig) << "Invalid LiveData name for extension=" << mUri;
        return false;
    }

    auto target = propertyAsObject(context, update, "target");
    if (!target.isString() || target.empty() || target.getString() != mUri) {
        CONSOLE_CFGP(rootConfig) << "Invalid LiveData target for extension=" << mUri;
        return false;
    }

    auto operations = propertyAsRecursive(context, update, "operations");
    if (!operations.isArray()) {
        CONSOLE_CFGP(rootConfig) << "Invalid LiveData operations for extension=" << mUri;
        return false;
    }

    auto dataRef = mLiveData.at(name.getString());
    for (const auto& operation : operations.getArray()) {
        auto type = propertyAsMapped<ExtensionLiveDataUpdateType>(context, operation, "type",
                static_cast<ExtensionLiveDataUpdateType>(-1), sExtensionLiveDataUpdateTypeBimap);
        if (type == static_cast<ExtensionLiveDataUpdateType>(-1)) {
            CONSOLE_CFGP(rootConfig) << "Wrong operation type for=" << name;
            return false;
        }

        bool result;
        switch (dataRef.objectType) {
            case kExtensionLiveDataTypeObject:
                result = updateLiveMap(type, dataRef, operation);
                break;
            case kExtensionLiveDataTypeArray:
                result = updateLiveArray(type, dataRef, operation);
                break;
            default:
                result = false;
                CONSOLE_CFGP(rootConfig) << "Unknown LiveObject type=" << dataRef.objectType << " for " << dataRef.name;
                break;
        }

        if (!result) {
            CONSOLE_CFGP(rootConfig) << "LiveMap operation failed=" << dataRef.name << " operation="
                                << sExtensionLiveDataUpdateTypeBimap.at(type);
        }
    }
    return true;
}

bool
ExtensionClient::updateLiveMap(ExtensionLiveDataUpdateType type, const LiveDataRef& dataRef, const Object& operation)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    std::string triggerEvent;
    auto keyObj = operation.opt("key", "");
    if (keyObj.empty()) {
        CONSOLE_CFGP(rootConfig) << "Invalid LiveData key for=" << dataRef.name;
        return false;
    }
    const auto& key = keyObj.getString();
    auto typeDef = mTypes.at(dataRef.type);
    auto item = operation.get("item");

    auto liveMap = std::dynamic_pointer_cast<LiveMap>(dataRef.objectPtr);
    auto result = true;

    switch (type) {
        case kExtensionLiveDataUpdateTypeSet:
            liveMap->set(key, item);
            break;
        case kExtensionLiveDataUpdateTypeRemove:
            result = liveMap->remove(key);
            break;
        default:
            CONSOLE_CFGP(rootConfig) << "Unknown operation for=" << dataRef.name;
            return false;
    }

    return result;
}

bool
ExtensionClient::updateLiveArray(ExtensionLiveDataUpdateType type, const LiveDataRef& dataRef, const Object& operation)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    std::string triggerEvent;

    auto item = operation.get("item");
    if (item.isNull() && (type != kExtensionLiveDataUpdateTypeRemove && type != kExtensionLiveDataUpdateTypeClear)) {
        CONSOLE_CFGP(rootConfig) << "Malformed items on LiveData update for=" << dataRef.name;
        return false;
    }

    auto indexObj = operation.opt("index", -1);
    if (!indexObj.isNumber() && type != kExtensionLiveDataUpdateTypeClear) {
        CONSOLE_CFGP(rootConfig) << "Invalid LiveData index for=" << dataRef.name;
        return false;
    }
    auto index = indexObj.getInteger();
    auto count = operation.get("count");
    auto liveArray = std::dynamic_pointer_cast<LiveArray>(dataRef.objectPtr);
    auto result = true;

    switch (type) {
        case kExtensionLiveDataUpdateTypeInsert:
            if(item.isArray()) {
                result = liveArray->insert(index, item.getArray().begin(), item.getArray().end());
            } else {
                result = liveArray->insert(index, item);
            }
            break;
        case kExtensionLiveDataUpdateTypeUpdate:
            if(item.isArray()) {
                result = liveArray->update(index, item.getArray().begin(), item.getArray().end());
            } else {
                result = liveArray->update(index, item);
            }
            break;
        case kExtensionLiveDataUpdateTypeRemove:
            result = liveArray->remove(index, count.isNumber() ? count.getDouble() : 1);
            break;
        case kExtensionLiveDataUpdateTypeClear:
            liveArray->clear();
            break;
        default:
            CONSOLE_CFGP(rootConfig) << "Unknown operation for=" << dataRef.name;
            return false;
    }

    return result;
}

bool
ExtensionClient::readExtension(const Context& context, const Object& extension)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    // verify extension schema
    auto schema = propertyAsString(context, extension, "type");
    auto version = propertyAsString(context, extension, "version");
    if (schema != "Schema" || version.compare(MAX_SUPPORTED_SCHEMA_VERSION) > 0) {
        CONSOLE_CFGP(rootConfig) << "Unsupported extension schema version:" << version;
        return false;
    }

    // register extension based on URI
    auto uriObj = propertyAsObject(context, extension, "uri");
    if (!uriObj.isString() || uriObj.empty()) {
        CONSOLE_CFGP(rootConfig).log("Missing or invalid extension URI.");
        return false;
    }
    const auto& uri = uriObj.getString();
    rootConfig->registerExtension(uri);
    mUri = uri;

    auto types = arrayifyPropertyAsObject(context, extension, "types");
    if (!readExtensionTypes(context, types)) {
        return false;
    }

    // register extension commands
    auto commands = arrayifyPropertyAsObject(context, extension, "commands");
    if (!readExtensionCommandDefinitions(context, commands)) {
        return false;
    }

    // register extension event handlers
    auto handlers = arrayifyPropertyAsObject(context, extension, "events");
    if (!readExtensionEventHandlers(context, handlers)) {
        return false;
    }

    // register extension live data
    auto liveData = arrayifyPropertyAsObject(context, extension, "liveData");
    if(!readExtensionLiveData(context, liveData)) {
        return false;
    }

    auto components = arrayifyPropertyAsObject(context, extension, "components");
    if (!readExtensionComponentDefinitions(context, components)) {
        return false;
    }

    return true;
}

bool
ExtensionClient::readExtensionTypes(const Context& context, const Object& types)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    if (!types.isArray()) {
        CONSOLE_CFGP(rootConfig).log("The extension name=%s has a malformed 'commands' block", mUri.c_str());
        return false;
    }

    for (const auto& t : types.getArray()) {
        auto name = propertyAsObject(context, t, "name");
        auto props = propertyAsObject(context, t, "properties");
        if (!name.isString() || !props.isMap()) {
            CONSOLE_CFGP(rootConfig).log("Invalid extension type for extension=%s",
                                    mUri.c_str());
            continue;
        }

        auto properties = std::make_shared<std::map<std::string, ExtensionProperty>>();
        auto extends = propertyAsObject(context, t, "extends");
        if (extends.isString()) {
            auto& extended = extends.getString();
            auto extendedType = mTypes.find(extended);
            if (extendedType != mTypes.end()) {
                properties->insert(extendedType->second->begin(), extendedType->second->end());
            } else {
                CONSOLE_CFGP(rootConfig) << "Unknown type to extend=" << extended
                                    << " for type=" << name.getString()
                                    << " for extension=" << mUri.c_str();
            }
        }

        for (const auto &p : props.getMap()) {
            auto pname = p.first;
            auto ps = p.second;

            Object defValue = Object::NULL_OBJECT();
            BindingType ptype;
            auto preq = true;

            if (ps.isString()) {
                ptype = sBindingMap.get(ps.getString(), kBindingTypeAny);
            } else if (!ps.has("type")) {
                CONSOLE_CFGP(rootConfig).log("Invalid extension property for type=%s extension=%s",
                        name.getString().c_str(), mUri.c_str());
                continue;
            } else {
                defValue = propertyAsObject(context, ps, "default");
                ptype = propertyAsMapped<BindingType>(context, ps, "type", kBindingTypeAny, sBindingMap);
                if (!sBindingMap.has(ptype)) {
                    ptype = kBindingTypeAny;
                }
                preq = propertyAsBoolean(context, ps, "required", false);
            }

            const auto& pfunc = sBindingFunctions.at(ptype);
            auto value = pfunc(context, defValue);

            properties->emplace(pname, ExtensionProperty{ptype, pfunc(context, defValue), preq});
        }
        mTypes.emplace(name.getString(), properties);
    }
    return true;
}

std::vector<ExtensionCommandDefinition>
ExtensionClient::readCommandDefinitionsInternal(const Context& context,const ObjectArray& commands) {
    std::vector<ExtensionCommandDefinition> commandDefs;
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return commandDefs;
    }

    for (const auto& command : commands) {
        // create a command
        auto name = propertyAsObject(context, command, "name");
        if (!name.isString() || name.empty()) {
            CONSOLE_CFGP(rootConfig).log("Invalid extension command for extension=%s", mUri.c_str());
            continue;
        }
        auto commandName = name.asString();
        commandDefs.emplace_back(mUri, commandName);
        auto& commandDef = commandDefs.back();
        // set required response
        auto req = propertyAsBoolean(context, command, "requireResponse", false);
        commandDef.requireResolution(req);
        auto fast = propertyAsBoolean(context, command, "allowFastMode", false);
        commandDef.allowFastMode(fast);

        // add command properties
        if (command.has("payload")) {
            // TODO: Work with references only now, maybe inlines later
            auto payload = command.get("payload");
            std::string type;
            if (payload.isString()) {
                type = payload.getString();
            } else if (payload.isMap()) {
                type = propertyAsString(context, payload, "type");
            }

            if (!mTypes.count(type)) {
                CONSOLE_CFGP(rootConfig).log("The extension name=%s has a malformed `payload` block for command=%s",
                                        mUri.c_str(), commandName.c_str());
                continue;
            }

            auto props = mTypes.at(type);
            for (const auto& p : *props) {
                // add the property
                commandDef.property(p.first, p.second.btype, p.second.defvalue, p.second.required);
            }
        } // properties

        // register the command
        rootConfig->registerExtensionCommand(commandDef);
    }
    return commandDefs;
}

bool
ExtensionClient::readExtensionCommandDefinitions(const Context& context, const Object& commands) {
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    if (!commands.isArray()) {
        CONSOLE_CFGP(rootConfig).log("The extension name=%s has a malformed 'commands' block", mUri.c_str());
        return false;
    }
    readCommandDefinitionsInternal(context, commands.getArray());
    return true;
}

void
ExtensionClient::readExtensionComponentCommandDefinitions(const Context& context, const Object& commands,
                                                          ExtensionComponentDefinition& def) {
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return;
    }

    if (!commands.isArray()) {
        CONSOLE_CFGP(rootConfig).log("The extension component name=%s has a malformed 'commands' block", mUri.c_str());
        return;
    }
    auto commandDefs = readCommandDefinitionsInternal(context, commands.getArray());
    def.commands(commandDefs);
}

bool
ExtensionClient::readExtensionEventHandlers(const Context& context, const Object& handlers)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    if (!handlers.isArray()) {
        CONSOLE_CFGP(rootConfig).log("The extension name=%s has a malformed 'events' block", mUri.c_str());
        return false;
    }

    for (const auto& handler : handlers.getArray()) {
        auto name = propertyAsObject(context, handler, "name");
        if (!name.isString() || name.empty()) {
            CONSOLE_CFGP(rootConfig).log("Invalid extension event handler for extension=%s", mUri.c_str());
            return false;
        } else {
            auto mode = propertyAsMapped<ExtensionEventExecutionMode>(context, handler, "mode",
                    kExtensionEventExecutionModeFast, sExtensionEventExecutionModeBimap);
            mEventModes.emplace(name.asString(), mode);
            rootConfig->registerExtensionEventHandler(ExtensionEventHandler(mUri, name.asString()));
        }
    }

    return true;
}

bool
ExtensionClient::readExtensionLiveData(const Context& context, const Object& liveData)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    if (!liveData.isArray()) {
        CONSOLE_CFGP(rootConfig).log("The extension name=%s has a malformed 'dataBindings' block", mUri.c_str());
        return false;
    }

    for (const auto& binding : liveData.getArray()) {
        auto name = propertyAsObject(context, binding, "name");
        if (!name.isString() || name.empty()) {
            CONSOLE_CFGP(rootConfig).log("Invalid extension data binding for extension=%s", mUri.c_str());
            return false;
        }

        auto typeDef = propertyAsObject(context, binding, "type");
        if (!typeDef.isString()) {
            CONSOLE_CFGP(rootConfig).log("Invalid extension data binding type for extension=%s", mUri.c_str());
            return false;
        }

        auto type = typeDef.getString();
        size_t arrayDefinition = type.find("[]");
        bool isArray = arrayDefinition != std::string::npos;

        if (isArray) {
            type = type.substr(0, arrayDefinition);
        }

        if (!(mTypes.count(type) // Any LiveData may use defined complex types
          || (isArray && sBindingMap.has(type)))) { // Arrays also may use primitive types
            CONSOLE_CFGP(rootConfig).log("Data type=%s, for LiveData=%s is invalid", type.c_str(), name.getString().c_str());
            continue;
        }

        ExtensionLiveDataType ltype;
        PropertyTriggerEvent addEvent;
        PropertyTriggerEvent updateEvent;
        PropertyTriggerEvent removeEvent;
        LiveObjectPtr live;

        if (isArray) {
            ltype = kExtensionLiveDataTypeArray;
            live = LiveArray::create();
        } else {
            ltype = kExtensionLiveDataTypeObject;
            live = LiveMap::create();
        }

        auto events = propertyAsObject(context, binding, "events");
        if (events.isMap()) {
            auto typeProps = mTypes.at(type);
            auto event = propertyAsObject(context, events, "add");
            if (event.isMap()) {
                auto propTriggers = propertyAsObject(context, event, "properties");
                auto triggers = readPropertyTriggers(context, typeProps, propTriggers);
                addEvent = {propertyAsString(context, event, "eventHandler"), triggers};
            }
            event = propertyAsObject(context, events, "update");
            if (event.isMap()) {
                auto propTriggers = propertyAsObject(context, event, "properties");
                auto triggers = readPropertyTriggers(context, typeProps, propTriggers);
                updateEvent = {propertyAsString(context, event, "eventHandler"), triggers};
            }
            event = propertyAsObject(context, events, "set");
            if (event.isMap()) {
                auto propTriggers = propertyAsObject(context, event, "properties");
                auto triggers = readPropertyTriggers(context, typeProps, propTriggers);
                updateEvent = {propertyAsString(context, event, "eventHandler"), triggers};
            }
            event = propertyAsObject(context, events, "remove");
            if (event.isMap()) {
                auto propTriggers = propertyAsObject(context, event, "properties");
                auto triggers = readPropertyTriggers(context, typeProps, propTriggers);
                removeEvent = {propertyAsString(context, event, "eventHandler"), triggers};
            }
        }

        LiveDataRef ldf = {name.getString(), ltype, type, live, addEvent, updateEvent, removeEvent};

        mLiveData.emplace(name.getString(), ldf);
        rootConfig->liveData(name.getString(), live);
        auto liveObjectWatcher = std::shared_ptr<LiveDataObjectWatcher>(shared_from_this());
        rootConfig->liveDataWatcher(name.getString(), liveObjectWatcher);
    }

    return true;
}

std::map<std::string, bool>
ExtensionClient::readPropertyTriggers(const Context& context, const TypePropertiesPtr& type, const Object& triggers)
{
    auto result = std::map<std::string, bool>();

    if (triggers.isNull()) {
        // Include all by default
        for (const auto& p : *type) {
            result.emplace(p.first, true);
        }
        return result;
    }

    auto requested = std::map<std::string, bool>();
    for (const auto& t : triggers.getArray()) {
        auto name = propertyAsString(context, t, "name");
        auto update = propertyAsBoolean(context, t, "update", false);
        auto collapse = propertyAsBoolean(context, t, "collapse", true);

        if (name == "*" && update) {
            // Include all
            for (const auto& p : *type) {
                requested.emplace(p.first, collapse);
            }
        } else if (update) {
            requested.emplace(name, collapse);
        } else {
            requested.erase(name);
        }
    }

    for (const auto& p : *type) {
        auto r = requested.find(p.first);
        if (r != requested.end()) {
            result.emplace(r->first, r->second);
        }
    }

    return result;
}

bool
ExtensionClient::readExtensionComponentEventHandlers(const Context& context,
                                                     const Object& handlers,
                                                     ExtensionComponentDefinition& def)
{
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    if (!handlers.isNull()) {
        if (!handlers.isArray()) {
            CONSOLE_CFGP(rootConfig).log("The extension name=%s has a malformed 'events' block",
                                    mUri.c_str());
            return false;
        }

        for (const auto& handler : handlers.getArray()) {
            auto name = propertyAsObject(context, handler, "name");
            if (!name.isString() || name.empty()) {
                CONSOLE_CFGP(rootConfig).log("Invalid extension event handler for extension=%s",
                                        mUri.c_str());
                return false;
            }
            else {
                auto mode = propertyAsMapped<ExtensionEventExecutionMode>(
                    context, handler, "mode", kExtensionEventExecutionModeFast,
                    sExtensionEventExecutionModeBimap);
                mEventModes.emplace(name.asString(), mode);
                auto eventKey = sComponentPropertyBimap.append(name.asString());
                def.addEventHandler(eventKey, ExtensionEventHandler(mUri, name.asString()));
            }
        }
    }
    return true;
}

bool
ExtensionClient::readExtensionComponentDefinitions(const Context& context, const Object& components) {
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    if (!components.isArray()) {
        CONSOLE_CFGP(rootConfig).log("The extension name=%s has a malformed 'components' block", mUri.c_str());
        return false;
    }

    for (const auto& component : components.getArray()) {
        auto name = propertyAsObject(context, component, "name");
        if (!name.isString() || name.empty()) {
            CONSOLE_CFGP(rootConfig).log("Invalid extension component name for extension=%s", mUri.c_str());
            continue;
        }
        auto componentName = name.asString();
        auto componentDef = ExtensionComponentDefinition(mUri, componentName);

        auto visualContext = propertyAsString(context, component, "context");
        componentDef.visualContextType(visualContext);

        auto commands = propertyAsObject(context, component, "commands");
        if(!commands.isNull()) {
            readExtensionComponentCommandDefinitions(context, commands, componentDef);
        }

        auto events = propertyAsObject(context, component, "events");
        if (!readExtensionComponentEventHandlers(context, events, componentDef)) {
            return false;
        }

        auto props = propertyAsObject(context, component, "properties");
        if (!props.empty() && props.isMap()) {
            auto properties = std::make_shared<std::map<id_type, ExtensionProperty>>();
            for (const auto &p : props.getMap()) {
                auto pname = p.first;
                auto ps = p.second;
                Object defValue = Object::NULL_OBJECT();
                BindingType ptype;
                auto preq = true;

                if (ps.isString()) {
                    ptype = sBindingMap.get(ps.getString(), kBindingTypeAny);
                } else if (!ps.has("type")) {
                    CONSOLE_CFGP(rootConfig).log("Invalid extension property extension=%s", mUri.c_str());
                    continue;
                } else {
                    defValue = propertyAsObject(context, ps, "default");
                    ptype = propertyAsMapped<BindingType>(context, ps, "type", kBindingTypeAny, sBindingMap);
                    if (!sBindingMap.has(ptype)) {
                        ptype = kBindingTypeAny;
                    }
                    preq = propertyAsBoolean(context, ps, "required", false);
                }

                const auto& pfunc = sBindingFunctions.at(ptype);
                auto value = pfunc(context, defValue);

                // returned property key is an incremented integer
                auto propertyKey = sComponentPropertyBimap.append(pname);
                properties->emplace(propertyKey, ExtensionProperty{ptype, std::move(value), preq});
            }
            componentDef.properties(properties);
        }

        rootConfig->registerExtensionComponent(componentDef);
    }

    return true;
}

rapidjson::Value
ExtensionClient::createComponentChange(rapidjson::MemoryPoolAllocator<>& allocator,
                                        ExtensionComponent& component) {
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return rapidjson::Value(rapidjson::kNullType);
    }

    auto extensionURI = component.getUri();
    if (extensionURI != mUri) {
        CONSOLE_CFGP(rootConfig) << "Invalid extension command target for extension=" << mUri;
        return rapidjson::Value(rapidjson::kNullType);
    }

    auto result = std::make_shared<ObjectMap>();
    result->emplace("method", sExtensionMethodBimap.at(kExtensionMethodComponent));
    result->emplace("version", IMPLEMENTED_INTERFACE_VERSION);
    result->emplace("target", extensionURI);
    result->emplace("token", mConnectionToken);
    result->emplace("resourceId", component.getResourceID());
    auto state = static_cast<ExtensionComponentResourceState>(component.getCalculated(kPropertyResourceState).asInt());
    result->emplace("state", sExtensionComponentStateBimap.at(state));

    // State specific message content
    if (state == kResourceReady || state == kResourcePending) {

        // include the viewport
        const auto& contextref = component.getContext()->find("viewport");
        if (!contextref.empty()) {
            result->emplace("viewport", contextref.object().value());
        }

        // Fill the payload with all the dirty dynamic properties that are also kPropOut
        // on first message, this will be all properties
        auto payload = std::make_shared<ObjectMap>();
        auto& corePDS = component.propDefSet().dynamic();
        for (auto& m : component.getDirty()) {
            auto pd = corePDS.find(m);
            if (pd != corePDS.end() && (pd->second.flags & kPropOut) != 0) {
                payload->emplace(pd->second.names[0], component.getCalculated(m));
            }
        }
        result->emplace("payload", payload);
    }

    return Object(result).serialize(allocator);
}


bool
ExtensionClient::processComponentResponse(const Context& context, const Object& response) {
    auto method = propertyAsString(context, response, "method");

    auto componentId = propertyAsString(context, response, "resourceId");
    auto extensionComp = context.extensionManager().findExtensionComponent(componentId);

    if (extensionComp != nullptr) {
        if (method == sExtensionMethodBimap.at(kExtensionMethodComponentFailure)) {
            auto message = propertyAsString(context, response, "message");
            auto code = propertyAsInt(context, response, "code", -1);
            extensionComp->extensionComponentFail(code, message);
        } else if (method == sExtensionMethodBimap.at(kExtensionMethodComponentUpdate)) {
           // TODO apply component properties
        }
    } else {
        auto rootConfig = mRootConfig.lock();
        if (!rootConfig) {
            LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
            return false;
        }
        CONSOLE_CFGP(rootConfig) << "Unable to find component associated with :" << componentId;
    }
    return true;
}

bool ExtensionClient::handleDisconnection(const RootContextPtr& rootContext,
                                          int errorCode,
                                          const std::string& message) {
    auto rootConfig = mRootConfig.lock();
    if (!rootConfig) {
        LOG(LogLevel::kError) << ROOT_CONFIG_MISSING;
        return false;
    }

    const auto& context = rootContext ? rootContext->context() : rootConfig->evaluationContext();

    if (errorCode != 0) {
        for (const auto& componentEntry : context.extensionManager().getExtensionComponents()) {
            auto extensionComponent = componentEntry.second;

            if (mUri == extensionComponent->getUri()) {
                extensionComponent->extensionComponentFail(errorCode, message);
            }
        }
    }
    return true;
}

rapidjson::Value
ExtensionClient::processComponentRequest(rapidjson::MemoryPoolAllocator<>& allocator,
                                         ExtensionComponent& component) {
    return createComponentChange(allocator, component);
}

rapidjson::Value
ExtensionClient::processComponentRelease(rapidjson::MemoryPoolAllocator<>& allocator,
                                         ExtensionComponent& component) {
    return createComponentChange(allocator, component);
}

rapidjson::Value
ExtensionClient::processComponentUpdate(rapidjson::MemoryPoolAllocator<>& allocator,
                                        ExtensionComponent& component) {
    return createComponentChange(allocator, component);
}

} // namespace apl
