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
#include <map>
#include <rapidjson/document.h>

namespace alexaext {

/**
 * Extension schema version.
 */
enum ExtensionSchemaVersion : std::uint32_t {
    kAlexaExtVersion10 = 0x1
};

static std::map<ExtensionSchemaVersion, std::string> sExtensionSchemaVersion = {
        {kAlexaExtVersion10, "1.0"}
};

/**
 * Message RPC version.
 */
enum ExtensionRPCVersion : std::uint32_t {
    kAlexaExtRPC10 = 0x1
};

static std::map<ExtensionRPCVersion, std::string> sExtensionRPCVersion = {
        {kAlexaExtRPC10, "1.0"}
};


enum ExtensionMethod {
    kExtensionMethodRegister,
    kExtensionMethodRegisterSuccess,
    kExtensionMethodRegisterFailure,
    kExtensionMethodCommand,
    kExtensionMethodCommandSuccess,
    kExtensionMethodCommandFailure,
    kExtensionMethodEvent,
    kExtensionMethodLiveDataUpdate,
};

static std::map<ExtensionMethod, std::string> sExtensionMethods = {
    {kExtensionMethodRegister,        "Register"},
    {kExtensionMethodRegisterSuccess, "RegisterSuccess"},
    {kExtensionMethodRegisterFailure, "RegisterFailure"},
    {kExtensionMethodCommand,         "Command"},
    {kExtensionMethodCommandSuccess,  "CommandSuccess"},
    {kExtensionMethodCommandFailure,  "CommandFailure"},
    {kExtensionMethodEvent,           "Event"},
    {kExtensionMethodLiveDataUpdate,  "LiveDataUpdate"}
};

static const rapidjson::Value sEmpty;
static std::atomic<int> sCmdId(64);

/**
 * Base message holds a document smart pointer to allow for chaining in builders.
 *
 * TODO this is an experimental class, the API is expected to change significantly.
 */
class BaseMessage {
public:

    BaseMessage& put(const std::string& key, const rapidjson::Value& value) {
        auto& alloc = mMessage.GetAllocator();
        rapidjson::Value copy;
        copy.CopyFrom(value, alloc);
        mMessage.AddMember(rapidjson::Value(key.c_str(), alloc).Move(), copy, alloc);
        return *this;
    }

    BaseMessage& put(const std::string& key, const std::string& value) {
        auto& alloc = mMessage.GetAllocator();
        mMessage.AddMember(rapidjson::Value(key.c_str(), alloc).Move(),
                           rapidjson::Value(value.c_str(), alloc).Move(), alloc);
        return *this;
    }

    rapidjson::Document& getDocument() {
        return mMessage;
    }

    operator rapidjson::Document&&() {
        return std::move(mMessage);
    }

    static std::string getTarget(const rapidjson::Value& message) {
        if (message.HasMember("target")) {
            return message["target"].GetString();
        }
        return "";
    }

    static std::string getVersion(const rapidjson::Value& message) {
        if (message.HasMember("version")) {
            return message["version"].GetString();
        }
        return "";
    }

    static std::string getMethod(const rapidjson::Value& message) {
        if (message.HasMember("method")) {
            return message["method"].GetString();
        }
        return "";
    }

    static const rapidjson::Value& getValue(const char* key, const rapidjson::Value& message) {
        if (message.HasMember(key))
            return message[key];
        return sEmpty;
    }

protected:
    explicit BaseMessage() {
        mMessage.SetObject();
    }

    explicit BaseMessage(const std::string& uri, ExtensionMethod method)
            : BaseMessage(sExtensionMethods[method], uri) {}

    explicit BaseMessage(const std::string& uri, const std::string& method) {
        mMessage.SetObject();
        put("target", uri);
        put("uri", uri);
        put("version", sExtensionRPCVersion[kAlexaExtRPC10]);
        put("method", method);
    }

    rapidjson::Document mMessage;
};


/**
 * Registration Request builder.
 */
class RegistrationRequest : public BaseMessage {
public:
    explicit RegistrationRequest(const std::string& uri)
            : BaseMessage(uri, sExtensionMethods[kExtensionMethodRegister]) {}

    RegistrationRequest& settings(const rapidjson::Value& settings) {
        put("settings", settings);
        return *this;
    }

    static const rapidjson::Value& getSettings(const rapidjson::Value& registrationRequest) {
        if (registrationRequest.HasMember("settings"))
            return registrationRequest["settings"];
        return sEmpty;
    }
};


class BaseFailure : public BaseMessage {
public:

    static std::string getMessage(const rapidjson::Value& message) {
        if (message.HasMember("message")) {
            // intentional narrowing conversion for systems that store numbers as double
            return message["message"].GetString();
        }
        return "";
    }

    static int getCode(const rapidjson::Value& message) {
        if (message.HasMember("code")) {
            // intentional narrowing conversion for systems that store numbers as double
            return (int) message["code"].GetDouble();
        }
        return -1;
    }

protected:
    explicit BaseFailure(const std::string& uri, const std::string& method, int errorCode,
                         const std::string& errorMessage)
            : BaseMessage(uri, method) {
        auto& alloc = mMessage.GetAllocator();
        mMessage.AddMember("code", errorCode, alloc);
        mMessage.AddMember("message", rapidjson::Value(errorMessage.c_str(), alloc).Move(), alloc);
    }
};


/**
* Registration Failure builder.
*/
class RegistrationFailure : public BaseFailure {
public:
    explicit RegistrationFailure(const std::string& uri, int errorCode, const std::string& errorMessage)
            : BaseFailure(uri, sExtensionMethods[kExtensionMethodRegisterFailure], errorCode, errorMessage) {}
};

/**
* Registration Success builder.
*/
class RegistrationSuccess : public BaseMessage {
public:
    explicit RegistrationSuccess(const std::string& uri, const std::string& token)
            : BaseMessage(uri, sExtensionMethods[kExtensionMethodRegisterSuccess]) {
        put("token", token);
    }

    RegistrationSuccess& environment(const rapidjson::Value& environment) {
        put("environment", environment);
        return *this;
    }

    RegistrationSuccess& schema(const rapidjson::Value& schema) {
        put("schema", schema);
        return *this;
    }

    static const rapidjson::Value& getEnvironment(const rapidjson::Value& message) {
        return BaseMessage::getValue("environment", message);
    }

    static const rapidjson::Value& getSchema(const rapidjson::Value& message) {
        return BaseMessage::getValue("schema", message);
    }
};

/**
 * Extension Schema builder.
 */
class ExtensionSchema : public BaseMessage {
public:
    explicit ExtensionSchema(const std::string& uri)
            : BaseMessage() {
        auto& alloc = mMessage.GetAllocator();
        mMessage.AddMember(rapidjson::Value("events", alloc).Move(),
                           rapidjson::Value(rapidjson::kArrayType), alloc);
        mMessage.AddMember(rapidjson::Value("types", alloc).Move(),
                           rapidjson::Value(rapidjson::kArrayType), alloc);
        mMessage.AddMember(rapidjson::Value("commands", alloc).Move(),
                           rapidjson::Value(rapidjson::kArrayType), alloc);
        mMessage.AddMember(rapidjson::Value("liveData", alloc).Move(),
                           rapidjson::Value(rapidjson::kArrayType), alloc);
    }

    ExtensionSchema& event(const std::string& eventName) {
        return event(eventName, true);
    }

    ExtensionSchema& event(const std::string& eventName, bool fastMode) {
        auto events = mMessage["events"].GetArray();
        auto& alloc = mMessage.GetAllocator();
        rapidjson::Value event(rapidjson::kObjectType);
        event.AddMember(rapidjson::Value("name", alloc).Move(),
                        rapidjson::Value(eventName.c_str(), alloc).Move(), alloc);
        event.AddMember(rapidjson::Value("fastMode", alloc).Move(),
                        fastMode, alloc);
        events.PushBack(event.Move(), alloc);
        return *this;
    }

};


/**
  * Command Request builder.
  */
class Command : public BaseMessage {
public:
    explicit Command(const std::string& uri, const std::string& commandName)
            : BaseMessage(uri, sExtensionMethods[kExtensionMethodCommand]) {
        auto& alloc = mMessage.GetAllocator();
        mMessage.AddMember("id", sCmdId++, alloc);
        mMessage.AddMember("payload", rapidjson::Value(rapidjson::kObjectType).Move(), alloc);
        mMessage.AddMember("name", rapidjson::Value(commandName.c_str(), alloc).Move(), alloc);
    }

    Command& property(const std::string& key, rapidjson::Value& value) {
        auto& alloc = mMessage.GetAllocator();
        auto payload = mMessage["payload"].GetObject();
        rapidjson::Value copy;
        copy.CopyFrom(value,mMessage.GetAllocator());
        payload.AddMember(rapidjson::Value(key.c_str(), alloc).Move(), copy, alloc);
        return *this;
    }

    static int getId(const rapidjson::Value& command) {
        if (command.HasMember("id")) {
            // intentional narrowing conversion for systems that store numbers as double
            return (int) command["id"].GetDouble();
        }
        return -1;
    }

    static std::string getName(const rapidjson::Value& command) {
        if (command.HasMember("name")) {
            // intentional narrowing conversion for systems that store numbers as double
            return command["name"].GetString();
        }
        return "";
    }

    static const rapidjson::Value& getPayload(const rapidjson::Value& message) {
        return BaseMessage::getValue("payload", message);
    }
};


/**
* Registration Success builder.
*/
class CommandSuccess : public BaseMessage {
public:
    explicit CommandSuccess(const std::string& uri, int commandId)
            : BaseMessage(uri, sExtensionMethods[kExtensionMethodCommandSuccess]) {
        auto& alloc = mMessage.GetAllocator();
        mMessage.AddMember("id", commandId, alloc);
    }

    CommandSuccess& result(const rapidjson::Value& result) {
        put("result", result);
        return *this;
    }
};

/**
* Command Failure builder.
*/
class CommandFailure : public BaseFailure {
public:
    explicit CommandFailure(const std::string& uri, int commandId, int errorCode,
                            const std::string& errorMessage)
            : BaseFailure(uri, sExtensionMethods[kExtensionMethodCommandFailure], errorCode, errorMessage) {
        auto& alloc = mMessage.GetAllocator();
        mMessage.AddMember("id", commandId, alloc);
    }

    static int getId(const rapidjson::Value& command) {
        if (command.HasMember("id")) {
            // intentional narrowing conversion for systems that store numbers as double
            return (int) command["id"].GetDouble();
        }
        return -1;
    }
};


/**
 * Event builder.
 */
class Event : public BaseMessage {
public:
    explicit Event(const std::string& uri, const std::string& eventName)
            : BaseMessage(uri, sExtensionMethods[kExtensionMethodEvent]) {
        auto& alloc = mMessage.GetAllocator();
        mMessage.AddMember("name", rapidjson::Value(eventName.c_str(), alloc).Move(), alloc);
        mMessage.AddMember("payload", rapidjson::Value(rapidjson::kObjectType).Move(), alloc);
    }

    Event& property(const std::string& key, rapidjson::Value& value) {
        auto& alloc = mMessage.GetAllocator();
        auto payload = mMessage["payload"].GetObject();
        rapidjson::Value copy;
        copy.CopyFrom(value,mMessage.GetAllocator());
        payload.AddMember(rapidjson::Value(key.c_str(), alloc).Move(), value, alloc);
        return *this;
    }

    static std::string getName(const rapidjson::Value& event) {
        if (event.HasMember("name")) {
            // intentional narrowing conversion for systems that store numbers as double
            return event["name"].GetString();
        }
        return "";
    }
};


/**
 * LiveDataUpdate builder.
 */
class LiveDataUpdate : public BaseMessage {
public:
    explicit LiveDataUpdate(const std::string& uri, const std::string& objectName)
            : BaseMessage(uri, sExtensionMethods[kExtensionMethodLiveDataUpdate]) {
        auto& alloc = mMessage.GetAllocator();
        mMessage.AddMember("name", rapidjson::Value(objectName.c_str(), alloc).Move(), alloc);
        mMessage.AddMember("operations", rapidjson::Value(rapidjson::kArrayType).Move(), alloc);
    }

    LiveDataUpdate& operation(const rapidjson::Value& operation) {
        auto& alloc = mMessage.GetAllocator();
        auto op = mMessage["operation"].GetArray();
        rapidjson::Value copy;
        copy.CopyFrom(operation,mMessage.GetAllocator());
        op.PushBack(copy, alloc);
        return *this;
    }

    // TODO Operation builders for different operation types

    static std::string getName(const rapidjson::Value& liveDataUpdate) {
        if (liveDataUpdate.HasMember("name")) {
            return liveDataUpdate["name"].GetString();
        }
        return "";
    }
};

} // namespace alexaext

#endif // ALEXAEXT_EXTENSIONMESSAGE_H
