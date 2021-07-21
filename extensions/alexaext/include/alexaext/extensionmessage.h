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
#ifndef ALEXAEXT_EXTENSIONMESSAGE_H
#define ALEXAEXT_EXTENSIONMESSAGE_H


#include <atomic>
#include <cstdint>
#include <iostream>
#include <map>

#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "alexaext/extensionexception.h"
#include "alexaext/extensionschema.h"

namespace alexaext {


/**
 * The extension message schema defines json based messages used by the extension
 * framework for communication between the the extension and the execution environment.
 * The schema supports messages for extension registration, commands, events, and data binding.
 * The payload of extension messages is defined by the Extension Schema.
 *
 * Registration: Handshake between runtime and extension when the extension is requested by a document.
 *      RegistrationRequest: (runtime -> extension) a request to use the extension
 *      RegistrationSuccess: (extension -> runtime) success response to RegistrationRequest
 *      RegistrationFailure: (extension -> runtime) failure response to RegistrationRequest
 *
 * Commands: Discrete messages sent to the extension from the document.
 *      Command: (runtime -> extension) invoke an extension command
 *      CommandSuccess: (extension -> runtime) a successful execution of a requested Command
 *      CommandFailure: (extension -> runtime) a failure to execute requested Command
 *
 * Events: Discrete messages sent by the extension, and received by the document.
 *     Event: (extension -> runtime) notifies the runtime an event was generated within the Extension
 *
 * Data Binding: Dynamic state information streamed from the extension and made available to the document
 * in the data binding context.
 *     LiveDataUpdate: (extension -> runtime) notifies the runtime extension generated data has changed
 *
 * Extension message are json objects and may be created with using rapidjson, or using the builders from
 * this class.
 *
 * An example message may look as follows:
 *
 *     auto event = Event("FunEvent").uri("alexaext:myextension:10")
 *           .name("myEvent")
 *           .property("key1", 1)
 *           .property("key2", true)
 *           .property("key3", "three");
 *
 *
 * Message builders may be assigned to rapidjson::Document objects.  Move semantics are used
 * in the assignment, making the memory resources of the builder invalid.
 *              rapidjson::Document eventDoc = event;
 *                                OR
 * rapidjson::Document eventDoc = Event("FunEvent").uri("alexaext:myextension:10")
 *
 * Message values can be extracted using the rapidjson:Pointers defined in the builders.  For example:
 *
 *              auto uri = Command::URI().Get(rawMessage)->GetString();
 *
 *  Rapidjson tutorial: https://rapidjson.org/md_doc_tutorial.html
 *  Rapidjson Pointers: https://rapidjson.org/md_doc_pointer.html
 *
 */

static std::string DEFAULT_SCHEMA_VERSION = "1.0";

/**
 * Base message provides properties common to all messages and supports assignment of
 * a message builder to a rapidjson::Document.
 */
template<typename Message>
class BaseMessage {
public:
    virtual ~BaseMessage() = default;

    rapidjson::Document& getDocument() {
        return *mMessage;
    }

    operator rapidjson::Document&&() {
        return std::move(*mMessage);
    }

    // deprecated, use uri
    Message& target(const std::string& target) {
        uri(target);
        return static_cast<Message&>(*this);
    }

    // deprecated, use URI
    static const rapidjson::Pointer& TARGET() {
        static const rapidjson::Pointer ptr("/target");
        return ptr;
    }

    Message& uri(const std::string& uri) {
        URI().Set(*mMessage, uri.c_str());
        TARGET().Set(*mMessage, uri.c_str());
        return static_cast<Message&>(*this);
    }

    static const rapidjson::Pointer& URI() {
        static const rapidjson::Pointer ptr("/uri");
        return ptr;
    }

    static const rapidjson::Pointer& VERSION() {
        static const rapidjson::Pointer ptr("/version");
        return ptr;
    }

    static const rapidjson::Pointer& METHOD() {
        static const rapidjson::Pointer ptr("/method");
        return ptr;
    }

protected:

    explicit BaseMessage(const std::string& method, const std::string& version)
            : mMessage(std::make_shared<rapidjson::Document>()) {
        VERSION().Set(*mMessage, version.c_str());
        METHOD().Set(*mMessage, method.c_str());
    }

    std::shared_ptr<rapidjson::Document> mMessage;
};

/**
 * Base failure message contains error and error codes and error messages
 */
template<typename Failure>
class BaseFailure {
public:
    virtual ~BaseFailure() = default;

    Failure& errorCode(int errorCode) {
        CODE().Set(*mFailMessage, errorCode);
        return static_cast<Failure&>(*this);
    }

    static const rapidjson::Pointer& CODE() {
        static const rapidjson::Pointer ptr("/code");
        return ptr;
    }

    Failure& errorMessage(const std::string errorMessage) {
        MESSAGE().Set(*mFailMessage, errorMessage.c_str());
        return static_cast<Failure&>(*this);
    }

    static const rapidjson::Pointer& MESSAGE() {
        static const rapidjson::Pointer ptr("/message");
        return ptr;
    }


protected:
    explicit BaseFailure(const std::shared_ptr<rapidjson::Document>& failMessage)
            : mFailMessage(failMessage) {}

    std::shared_ptr<rapidjson::Document> mFailMessage;
};


/**
 * Trait for messages that carry a property payload.
 * This class creates the payload container object, only when a value is set.
 */
template<typename Message>
class Payload {
public:

    // Set a complex property with move semantics.
    Message& property(const std::string& key, rapidjson::Value& value) {
        auto& alloc =  mPayloadMessage->GetAllocator();
        getPath()->AddMember(rapidjson::Value().SetString(key.c_str(), alloc),
                        value.Move(),
                        alloc);
        return static_cast<Message&>(*this);
    }

    // Set a complex property with copy semantics.
    Message& property(const std::string& key, const rapidjson::Value& value) {
        auto& alloc =  mPayloadMessage->GetAllocator();
        getPath()->AddMember(rapidjson::Value().SetString(key.c_str(), alloc),
                rapidjson::Value().CopyFrom(value, alloc),
                       alloc);
        return static_cast<Message&>(*this);
    }

    // Set a property of type string
    Message& property(const std::string& key, const std::string& value) {
        return property(key, value.c_str());
    }

    Message& property(const std::string& key, const char *value) {
        auto& alloc =  mPayloadMessage->GetAllocator();
        getPath()->AddMember(rapidjson::Value().SetString(key.c_str(), alloc ),
                             rapidjson::Value().SetString(value, alloc), alloc);
        return static_cast<Message&>(*this);
    }

    /**
     * Set a property of type T. Supports primitive types:
    * tparam T Either bool, int, unsigned, int64_t, uint64_t, double, float
    */
    template<typename T,
             typename std::enable_if<std::is_arithmetic<T>::value, bool>::type = true>
    Message& property(const std::string& key, const T value) {
        auto& alloc =  mPayloadMessage->GetAllocator();
        getPath()->AddMember(rapidjson::Value().SetString(key.c_str(), alloc ),
                rapidjson::Value().Set(value), alloc);
        return static_cast<Message&>(*this);
    }

protected:
    explicit Payload(const std::shared_ptr<rapidjson::Document>& payloadMessage, const rapidjson::Pointer& path)
            : mPayloadMessage(payloadMessage), mPath(path) {}

    rapidjson::Value * getPath() {
        rapidjson::Value *container = mPath.Get(*mPayloadMessage);
        if (!container) {
            container = &mPath.Set(*mPayloadMessage,rapidjson::Value(rapidjson::kObjectType));
        }
        return container;
    }

protected:
    std::shared_ptr<rapidjson::Document> mPayloadMessage;
    rapidjson::Pointer mPath;
};

/**
 * Get a value of type T, or return the provided default. Supports primitive types:
 * tparam T Either bool, int, unsigned, int64_t, uint64_t, double, float
*/
template<typename T>
T GetWithDefault(const rapidjson::Pointer& path, const rapidjson::Value& root, T defaultValue) {
    const rapidjson::Value* value = path.Get(root);
    if (!value)
        return defaultValue;
    if (value->Is<T>()) {
        return value->Get<T>();
    }
    if (value->IsNumber()) {
        return (T) value->GetDouble();
    }
    return defaultValue;
}

template<>
const char* GetWithDefault<const char*>(const rapidjson::Pointer& path,
                                         const rapidjson::Value& root,
                                         const char* defaultValue);

template<>
std::string GetWithDefault<std::string>(const rapidjson::Pointer& path,
                                        const rapidjson::Value& root,
                                        std::string defaultValue);

/**
 * Get a value of type T, or return the provided default. Supports primitive types:
 * tparam T Either bool, int, unsigned, int64_t, uint64_t, double, float, const char*
*/
template<typename T>
T GetWithDefault(const std::string& path, const rapidjson::Value& root, T defaultValue) {
    auto localPath = "/" + path;
    rapidjson::Pointer ptr(localPath.c_str());
    return GetWithDefault(ptr, root, defaultValue);
}

/**
 * Get a value of type T, or return the provided default. Supports primitive types:
 * tparam T Either bool, int, unsigned, int64_t, uint64_t, double, float, const char*
*/
template<typename T>
T GetWithDefault(const std::string& path, const rapidjson::Value* root, T defaultValue) {
    if (!root) return defaultValue;
    return GetWithDefault<T>(path, *root, defaultValue);
}

/**
 * Create a "pretty" string from a Value.
 */
inline std::string AsPrettyString(const rapidjson::Value& value) {
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return std::string(buffer.GetString());
}

/**
 * Create a string from a Value.
 */
inline std::string AsString(const rapidjson::Value& value) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return std::string(buffer.GetString());
}

/**
 * Registration Request builder. A RegistrationRequest represents a document request
 * to use an extension.
 */
class RegistrationRequest : public BaseMessage<RegistrationRequest> {
public:

    explicit RegistrationRequest(const std::string& version)
            : BaseMessage("Register", version) {}

    // uses Copy semantics
    RegistrationRequest& settings(const rapidjson::Value& settings) {
        SETTINGS().Set(*mMessage, settings);
        return *this;
    }

    // uses Move semantics
    RegistrationRequest& settings(rapidjson::Value& settings) {
        SETTINGS().Set(*mMessage, settings);
        return *this;
    }

    static const rapidjson::Pointer& SETTINGS() {
        static const rapidjson::Pointer ptr("/settings");
        return ptr;
    }
};


/**
* Registration Failure builder. A response to RegistrationRequest that indicates failure.
*/
class RegistrationFailure :
        public BaseMessage<RegistrationFailure>,
        public BaseFailure<RegistrationFailure> {
public:
    explicit RegistrationFailure(const std::string& version)
            : BaseMessage("RegisterFailure", version),
              BaseFailure(mMessage) {}

    /**
     * Creates a new registration failure message indicating that the specified URI is unknown.
     *
     * @param uri The unknown URI
     * @return The new registration failure message
     */
    static RegistrationFailure forUnknownURI(const std::string &uri) {
        return RegistrationFailure(DEFAULT_SCHEMA_VERSION)
            .uri(uri)
            .errorCode(kErrorUnknownURI)
            .errorMessage(sErrorMessage[kErrorUnknownURI] + uri);
    }

    /**
     * Creates a new registration failure message indicating that an invalid message was
     * received.
     *
     * @param uri The URI of the extension
     * @return The new registration failure message
     */
    static RegistrationFailure forInvalidMessage(const std::string &uri) {
        return RegistrationFailure(DEFAULT_SCHEMA_VERSION)
            .uri(uri)
            .errorCode(kErrorInvalidMessage)
            .errorMessage(sErrorMessage[kErrorInvalidMessage]);
    }

    /**
     * Creates a new registration failure message indicating that an unknown exception has
     * been encountered.
     *
     * @param uri The URI of the extension
     * @return The new registration failure message
     */
    static RegistrationFailure forUnknownException(const std::string &uri) {
        return RegistrationFailure(DEFAULT_SCHEMA_VERSION)
            .uri(uri)
            .errorCode(kErrorException)
            .errorMessage(sErrorMessage[kErrorException]);
    }

    /**
     * Creates a new registration failure message indicating that an extension encountered
     * an exception.
     *
     * @param uri The URI of the extension
     * @param message A human-readable message describing the exception
     * @return The new registration failure message
     */
    static RegistrationFailure forException(const std::string &uri, const std::string &message) {
        std::string errorMessage = sErrorMessage[kErrorExtensionException];
        auto pos = errorMessage.find("%s");
        if (pos != std::string::npos) {
            errorMessage.replace(pos, 2 /* length of %s */, uri);
        }

        pos = errorMessage.find("%s");
        if (pos != std::string::npos) {
            errorMessage.replace(pos, 2 /* length of %s */, message);
        }

        return RegistrationFailure(DEFAULT_SCHEMA_VERSION)
            .uri(uri)
            .errorCode(kErrorExtensionException)
            .errorMessage(errorMessage);
    }

    /**
     * Creates a new registration failure message indicating that the specified command ID failed
     * to execute.
     *
     * @param uri The URI of the extension
     * @param commandId The ID of the failed command
     * @return The new registration failure message
     */
    static RegistrationFailure forFailedCommand(const std::string &uri, const std::string &commandId) {
        return RegistrationFailure(DEFAULT_SCHEMA_VERSION)
            .uri(uri)
            .errorCode(kErrorFailedCommand)
            .errorMessage(sErrorMessage[kErrorFailedCommand] + commandId);
    }

    /**
     * Creates a new registration failure message indicating that the extension schema
     * for the specified URI was invalid.
     *
     * @param uri The extension URI
     * @return The new registration failure message
     */
    static RegistrationFailure forInvalidExtensionSchema(const std::string &uri) {
        return RegistrationFailure(DEFAULT_SCHEMA_VERSION)
            .uri(uri)
            .errorCode(kErrorInvalidExtensionSchema)
            .errorMessage(sErrorMessage[kErrorInvalidExtensionSchema] + uri);
    }
};

/**
 * Builder for environment properties.
 */
class Environment : public Payload<Environment> {
public:
    explicit Environment(const std::shared_ptr<rapidjson::Document>& payloadMessage, const rapidjson::Pointer& path)
            : Payload(payloadMessage, path) {}

    Environment& version(const std::string& value) {
        VERSION().Set(*getPath(), value.c_str(), mPayloadMessage->GetAllocator());
        return *this;
    }

    static const rapidjson::Pointer& VERSION() {
        static const rapidjson::Pointer ptr("/version");
        return ptr;
    }
};

/**
* Registration Success builder. A response to RegistrationRequest the indicates success.
*/
class RegistrationSuccess :
        public BaseMessage<RegistrationSuccess> {
public:
    explicit RegistrationSuccess(const std::string& version)
            : BaseMessage("RegisterSuccess", version) {}

    RegistrationSuccess& token(const std::string& token) {
        TOKEN().Set(*mMessage, token.c_str());
        return *this;
    }

    static const rapidjson::Pointer& TOKEN() {
        static const rapidjson::Pointer ptr("/token");
        return ptr;
    }

    RegistrationSuccess& environment(const std::function<void(Environment&)>& f) {
        Environment env(mMessage, ENVIRONMENT());
        f(env);
        return *this;
    }

    // uses copy semantics
    RegistrationSuccess& environment(const rapidjson::Value& environment) {
        ENVIRONMENT().Set(*mMessage, environment);
        return *this;
    }

    static const rapidjson::Pointer& ENVIRONMENT() {
        static const rapidjson::Pointer ptr("/environment");
        return ptr;
    }

    RegistrationSuccess&
    schema(const std::string& schemaVersion, const std::function<void(ExtensionSchema&)>& f) {
        ExtensionSchema builder(&mMessage->GetAllocator(), schemaVersion);
        f(builder);
        SCHEMA().Set(*mMessage, builder);
        return *this;
    }

    // uses copy semantics
    RegistrationSuccess& schema(const rapidjson::Value& schema) {
        SCHEMA().Set(*mMessage, schema);
        return *this;
    }

    static const rapidjson::Pointer& SCHEMA() {
        static const rapidjson::Pointer ptr("/schema");
        return ptr;
    }
};


/**
  * Command Request builder. Discrete messages send to the extension from the document.
  */
class Command : public BaseMessage<Command>, public Payload<Command> {
public:
    explicit Command(const std::string& version)
            : BaseMessage("Command", version), Payload<Command>(mMessage, PAYLOAD()) {}

    Command& id(int id) {
        ID().Set(*mMessage, id);
        return *this;
    }

    Command& name(const std::string& name) {
        NAME().Set(*mMessage, name.c_str());
        return *this;
    }

    static const rapidjson::Pointer& ID() {
        static const rapidjson::Pointer ptr("/id");
        return ptr;
    }

    static const rapidjson::Pointer& NAME() {
        static const rapidjson::Pointer ptr("/name");
        return ptr;
    }

    static const rapidjson::Pointer& PAYLOAD() {
        static const rapidjson::Pointer ptr("/payload");
        return ptr;
    }

};

/**
* Command Success builder. A response to Command messages indicating successful execution.
*/
class CommandSuccess : public BaseMessage<CommandSuccess> {
public:
    explicit CommandSuccess(const std::string& version)
            : BaseMessage("CommandSuccess", version) {}

    CommandSuccess& id(int id) {
        ID().Set(*mMessage, id);
        return *this;
    }

    // uses copy semantics
    CommandSuccess& result(const rapidjson::Value& result) {
        RESULT().Set(*mMessage, result);
        return *this;
    }

    static const rapidjson::Pointer& RESULT() {
        static const rapidjson::Pointer ptr("/result");
        return ptr;
    }

    static const rapidjson::Pointer& ID() {
        static const rapidjson::Pointer ptr("/id");
        return ptr;
    }
};

/**
* Command Failure builder. A respose to Command messages indicating execution failure.
*/
class CommandFailure :
        public BaseMessage<CommandFailure>,
        public BaseFailure<CommandFailure> {
public:
    explicit CommandFailure(const std::string& version)
            : BaseMessage("CommandFailure", version),
              BaseFailure(mMessage) {}

    CommandFailure& id(int id) {
        ID().Set(*mMessage, id);
        return *this;
    }

    static const rapidjson::Pointer& ID() {
        static const rapidjson::Pointer ptr("/id");
        return ptr;
    }
};

/**
 * Event builder. Discrete messages sent by the extension, and received by the document.
 */
class Event : public BaseMessage<Event>, public Payload<Event> {
public:
    explicit Event(const std::string& version)
            : BaseMessage("Event", version), Payload(mMessage, PAYLOAD()) {}


    Event& name(const std::string& name) {
        NAME().Set(*mMessage, name.c_str());
        return *this;
    }

    static const rapidjson::Pointer& NAME() {
        static const rapidjson::Pointer ptr("/name");
        return ptr;
    }

    static const rapidjson::Pointer& PAYLOAD() {
        static const rapidjson::Pointer ptr("/payload");
        return ptr;
    }

};

// forward declare LiveDataUpdate builder dependencies for file readability
class LiveDataMapOperation;
class LiveDataArrayOperation;

/**
 * LiveDataUpdate builder. Dynamic state information streamed from the extension and made
 * available to the document in the data binding context.
 */
class LiveDataUpdate
        : public BaseMessage<LiveDataUpdate>,
          public SchemaBuilder {
public:
    explicit LiveDataUpdate(const std::string& version)
            : BaseMessage("LiveDataUpdate", version),
              SchemaBuilder(mMessage) {
        OPERATIONS().Set(*mMessage, rapidjson::Value(rapidjson::kArrayType));
    }

    LiveDataUpdate& objectName(const std::string& name) {
        OBJECT_NAME().Set(*mMessage, name.c_str());
        return *this;
    }

    static const rapidjson::Pointer& OBJECT_NAME() {
        static const rapidjson::Pointer ptr("/name");
        return ptr;
    }

    LiveDataUpdate&
    liveDataArrayUpdate(const std::function<void(LiveDataArrayOperation &)> &builder) {
        factoryPush<LiveDataArrayOperation>(OPERATIONS(), builder);
        return *this;
    }

    LiveDataUpdate&
    liveDataMapUpdate(const std::function<void(LiveDataMapOperation&)>& builder) {
        factoryPush<LiveDataMapOperation>(OPERATIONS(), builder);
        return *this;
    }

    static const rapidjson::Pointer& OPERATIONS() {
        static const rapidjson::Pointer ptr("/operations");
        return ptr;
    }
};

/**
 * Base builder for a live data operation.
 */
template<typename Operation>
class LiveDataOperation {
public:

    virtual Operation& type(const std::string& type) {
        TYPE().Set(*mValue, type.c_str(), *mAllocator);
        return static_cast<Operation&>(*this);
    }

    static const rapidjson::Pointer& TYPE() {
        static const rapidjson::Pointer ptr("/type");
        return ptr;
    }

    /**
    * Add an Item of type T. Supports primitive types:
    * tparam T Either bool, int, unsigned, int64_t, uint64_t, double, float, const char*
    */
    template<typename T>
    Operation& item(const T value) {
        ITEM().Set(*mValue, value, *mAllocator);
        return static_cast<Operation&>(*this);
    }

    // Add a complex item with move semantics.
    Operation& item(rapidjson::Value& value) {
        ITEM().Set(*mValue, value, *mAllocator);
        return static_cast<Operation&>(*this);
    }

    // Add a item string value.
    Operation& item(const std::string& value) {
        ITEM().Set(*mValue, rapidjson::Value().Set(value.c_str(), *mAllocator), *mAllocator);
        return static_cast<Operation&>(*this);
    }

    // Add a complex item with copy semantics.
    Operation& item(const rapidjson::Value& value) {
        ITEM().Set(*mValue, rapidjson::Value().CopyFrom(value, *mAllocator), *mAllocator);
        return static_cast<Operation&>(*this);
    }

    static const rapidjson::Pointer& ITEM() {
        static const rapidjson::Pointer ptr("/item");
        return ptr;
    }

    operator rapidjson::Value&&() {
        return std::move(*mValue);
    }

protected:

    explicit LiveDataOperation(rapidjson::MemoryPoolAllocator<>* allocator)
            : mAllocator(allocator),
              mValue(std::make_shared<rapidjson::Value>()) {}

    rapidjson::Document::AllocatorType* mAllocator;
    std::shared_ptr<rapidjson::Value> mValue;
};

/**
 * Live data operation update for a data map.
 */
class LiveDataMapOperation
        : public LiveDataOperation<LiveDataMapOperation> {
public:

    explicit LiveDataMapOperation(rapidjson::Document::AllocatorType* allocator)
            : LiveDataOperation(allocator) {
    }

    // Map operation name: Set, Remove
    LiveDataMapOperation& type(const std::string& type) override {
        return LiveDataOperation::type(type);
    }

    static const rapidjson::Pointer& TYPE() {
        return LiveDataOperation::TYPE();
    }

    static const rapidjson::Pointer& ITEM() {
        return LiveDataOperation::ITEM();
    }

    // Array index.
    LiveDataMapOperation& key(const std::string& key) {
        KEY().Set(*mValue, key.c_str(), *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& KEY() {
        static const rapidjson::Pointer ptr("/key");
        return ptr;
    }
};

/**
 * Live data operation update for array data.
 */
class LiveDataArrayOperation
        : public LiveDataOperation<LiveDataArrayOperation> {
public:

    explicit LiveDataArrayOperation(rapidjson::Document::AllocatorType* allocator)
            : LiveDataOperation(allocator) {
    }

    // Array operation name: Insert, Update, Set, Remove, Clear
    LiveDataArrayOperation& type(const std::string& type) override {
        return LiveDataOperation::type(type);
    }

    static const rapidjson::Pointer& TYPE() {
        static const rapidjson::Pointer ptr("/type");
        return ptr;
    }

    // Array index.
    LiveDataArrayOperation& index(int index) {
        INDEX().Set(*mValue, index, *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& INDEX() {
        static const rapidjson::Pointer ptr("/index");
        return ptr;
    }

    // Remove Only, Number of items to remove.
    LiveDataArrayOperation& count(const int count) {
        COUNT().Set(*mValue, count, *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& COUNT() {
        static const rapidjson::Pointer ptr("/count");
        return ptr;
    }
};

} // namespace alexaext

#endif // ALEXAEXT_EXTENSIONMESSAGE_H


