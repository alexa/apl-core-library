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

#include "alexaext/localextensionproxy.h"

namespace alexaext {

LocalExtensionProxy::LocalExtensionProxy(const ExtensionPtr& extension)
    : mExtension(extension), mFactory(nullptr), mURIs(extension->getURIs())
{}

LocalExtensionProxy::LocalExtensionProxy(const std::string& uri, ExtensionFactory factory)
    : mExtension(nullptr), mFactory(std::move(factory))
{
    mURIs.emplace(uri);
}

/**
* Proxy constructor for an extension using deferred creation. The extension supports
* multiple URI.
*
* @param factory Factory to create the extension.
*/
LocalExtensionProxy::LocalExtensionProxy(std::set<std::string> uri, ExtensionFactory factory)
    : mExtension(nullptr), mFactory(std::move(factory)), mURIs(std::move(uri))
{}

std::set<std::string> LocalExtensionProxy::getURIs() const
{
    return mURIs;
}

bool
LocalExtensionProxy::getRegistration(const std::string& uri,
                                     const rapidjson::Value& registrationRequest,
                                     RegistrationSuccessCallback success,
                                     RegistrationFailureCallback error)
{
    int errorCode = kErrorNone;
    std::string errorMsg;

    // check the URI is supported
    if (mExtension == nullptr || mURIs.find(uri) == mURIs.end()) {
        if (error) {
            rapidjson::Document fail = RegistrationFailure("1.0")
                    .uri(uri)
                    .errorCode(kErrorUnknownURI)
                    .errorMessage(sErrorMessage[kErrorUnknownURI] + uri);
            error(uri, fail);
        }
        return false;
    }

    // request the schema from the extension
    rapidjson::Document registration;
    try {
        registration = mExtension->createRegistration(uri, registrationRequest);
    } catch (const std::exception& e) {
        errorCode = kErrorExtensionException;
        errorMsg = e.what();
    } catch (...) {
        errorCode = kErrorException;
        errorMsg = sErrorMessage[kErrorException];
    }

    // failed schema creation notify failure callback
    if (registration.IsNull() || registration.HasParseError()) {
        if (errorCode == kErrorNone) {
            //  the call was attempted but the extension failed without exception or failure message
            errorCode = kErrorInvalidExtensionSchema;
            errorMsg = sErrorMessage[kErrorInvalidExtensionSchema] + uri;
        }
        if (error) {
            rapidjson::Document fail = RegistrationFailure("1.0")
                    .errorCode(errorCode)
                    .errorMessage(errorMsg)
                    .uri(uri);
            error(uri, fail);
        }
        return false; // registration message failed execution and was not handled by the extension
    }

    // non-success messages are assumed to be failure and forwarded to the error handler
    auto method = RegistrationSuccess::METHOD().Get(registration);
    if (method &&  *method != "RegisterSuccess") {
        if (error) {
            error(uri, registration);
        }
        return true; // registration message was handled handled by the extension
    }

    // notify success callback
    if (success) {
        success(uri, registration);
    }
    return true;
}

bool
LocalExtensionProxy::initializeExtension(const std::string& uri)
{
    if (!mExtension && mFactory) {
        // create the extension
        mExtension = mFactory(uri);
    }

    if (!mExtension || mInitialized.count(uri)) return false;

    std::weak_ptr<LocalExtensionProxy> weakSelf = shared_from_this();
    mExtension->registerEventCallback([weakSelf](const std::string& uri, const rapidjson::Value &event) {
        if (auto self = weakSelf.lock()) {
            for (const auto &callback : self->mEventCallbacks) {
                callback(uri, event);
            }
        }
    });

    mExtension->registerLiveDataUpdateCallback(
    [weakSelf](const std::string& uri, const rapidjson::Value &liveDataUpdate) {
        if (auto self = weakSelf.lock()) {
            for (const auto &callback : self->mLiveDataCallbacks) {
                callback(uri, liveDataUpdate);
            }
        }
    });

    mInitialized.emplace(uri);

    return true;
}

bool
LocalExtensionProxy::isInitialized(const std::string& uri) const
{
    return mURIs.count(uri) && mExtension && mInitialized.count(uri);
}

bool
LocalExtensionProxy::invokeCommand(const std::string& uri,
                                   const rapidjson::Value& command,
                                   CommandSuccessCallback success,
                                   CommandFailureCallback error)
{
    // verify the command has an ID
    const rapidjson::Value* commandValue = Command::ID().Get(command);
    if (!commandValue || !commandValue->IsNumber()) {
        if (error) {
            rapidjson::Document fail = CommandFailure("1.0")
                    .uri(uri)
                    .errorCode(kErrorInvalidMessage)
                    .errorMessage(sErrorMessage[kErrorInvalidMessage]);
            error(uri, fail);
        }
        return false;
    }
    int commandID = (int) commandValue->GetDouble();

    // check the URI is supported and the command has ID
    if (mExtension == nullptr || mURIs.find(uri) == mURIs.end()) {
        if (error) {
            rapidjson::Document fail = CommandFailure("1.0")
                    .uri(uri)
                    .id(commandID)
                    .errorCode(kErrorUnknownURI)
                    .errorMessage(sErrorMessage[kErrorUnknownURI] + uri);
            error(uri, fail);
        }
        return false;
    }

    // invoke the extension command
    int errorCode = kErrorNone;
    std::string errorMsg;
    bool result = false;
    try {
        result = mExtension->invokeCommand(uri, command);
    } catch (const std::exception& e) {
        errorCode = kErrorExtensionException;
        errorMsg = e.what();
    } catch (...) {
        errorCode = kErrorException;
        errorMsg = sErrorMessage[kErrorException];
    }

    // failed command invocation
    if (!result) {
        if (error) {
            if (errorCode == kErrorNone) {
                //  the call was attempted but the extension failed without exception
                errorCode = kErrorFailedCommand;
                errorMsg = sErrorMessage[kErrorFailedCommand] + std::to_string(commandID);
            }
            rapidjson::Document fail = CommandFailure("1.0")
                    .uri(uri)
                    .id(commandID)
                    .errorCode(errorCode)
                    .errorMessage(errorMsg);
            error(uri, fail);
        }
        return false;
    }

    // notify success callback
    if (success) {
        rapidjson::Document win = CommandSuccess("1.0")
                .uri(uri)
                .id(commandID);
        success(uri, win);
    }
    return true;
}

void
LocalExtensionProxy::registerEventCallback(Extension::EventCallback callback)
{
    if (callback) mEventCallbacks.emplace_back(std::move(callback));
}

void
LocalExtensionProxy::registerLiveDataUpdateCallback(Extension::LiveDataUpdateCallback callback)
{
    if (callback) mLiveDataCallbacks.emplace_back(std::move(callback));
}

void
LocalExtensionProxy::onRegistered(const std::string &uri, const std::string &token)
{
    if (mExtension) mExtension->onRegistered(uri, token);
}

void
LocalExtensionProxy::onUnregistered(const std::string &uri, const std::string &token)
{
    if (mExtension) mExtension->onUnregistered(uri, token);
}

} // namespace alexaext
