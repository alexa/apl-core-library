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

#ifndef _ALEXAEXT_LOCALEXTENSIONPROXY_H
#define _ALEXAEXT_LOCALEXTENSIONPROXY_H


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
class LocalExtensionProxy final : public ExtensionProxy {

public:
    /**
     * Factory method provided by the extension author to create the extension described
     * by this definition and the uri.
     *
     * @param The extension URI.
     * @return The extension instance.
     */
    using ExtensionFactory = std::function<ExtensionPtr(const std::string &uri)>;

    /**
     * Proxy constructor for a local extension.
     *
     * @param extension The extension.
     */
    explicit LocalExtensionProxy(const ExtensionPtr &extension)
            : mExtension(extension), mFactory(nullptr), mURIs(extension->getURIs()) {}

    /**
     * Proxy constructor for an extension using deferred creation. The extension
     * supports a single URI.
     *
     * @param factory Factory to create the extension.
     */
    explicit LocalExtensionProxy(const std::string &uri, ExtensionFactory factory)
            : mExtension(nullptr), mFactory(std::move(factory)) {
        mURIs.emplace(uri);
    }

    /**
     * Proxy constructor for an extension using deferred creation. The extension supports
     * multiple URI.
     *
     * @param factory Factory to create the extension.
     */
    explicit LocalExtensionProxy(std::set<std::string> uri, ExtensionFactory factory)
            : mExtension(nullptr), mFactory(std::move(factory)), mURIs(std::move(uri)) {}

    std::set<std::string> getURIs() override { return mURIs; };

    bool getRegistration(const std::string &uri, const rapidjson::Value &registrationRequest,
                         RegistrationSuccessCallback success,
                         RegistrationFailureCallback error) override {

        int errorCode = kErrorNone;
        std::string errorMsg;

        // check the URI is supported
        if (mExtension == nullptr || mURIs.find(uri) == mURIs.end()) {
            if (error) {
                rapidjson::Document fail = RegistrationFailure(uri, kErrorUnknownURI,
                                                    sErrorMessage[kErrorUnknownURI] + uri);
                error(uri, fail);
            }
            return false;
        }

        // request the schema from the extension
        rapidjson::Document registration;
        try {
            registration = mExtension->createRegistration(uri, registrationRequest);
        }
        catch (const std::exception &e) {
            errorCode = kErrorExtensionException;
            errorMsg = e.what();
        }
        catch (...) {
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
                rapidjson::Document fail = RegistrationFailure(uri, errorCode, errorMsg);
                error(uri, fail);
            }
            return false; // registration message failed execution and was not handled by the extension
        } else if (BaseMessage::getMethod(registration) != sExtensionMethods[kExtensionMethodRegisterSuccess] ) {
            // non-success messages are assumed to be failure and forwarded to the error handler
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

    bool initializeExtension(const std::string &uri) override {
        if (!mExtension && mFactory) {
            // create the extension
            mExtension = mFactory(uri);
        }
        return mExtension != nullptr;
    }

    bool invokeCommand(const std::string &uri, const rapidjson::Value &command,
                       CommandSuccessCallback success, CommandFailureCallback error) override {

        // verify the command has an ID
        int commandID = Command::getId(command);
        if (commandID <= 0) {
            if (error) {
                rapidjson::Document fail = CommandFailure(uri, commandID, kErrorInvalidMessage, sErrorMessage[kErrorInvalidMessage]);
                error(uri, fail);
            }
            return false;
        }

        // check the URI is supported and the command has ID
        if (mExtension == nullptr || mURIs.find(uri) == mURIs.end()) {
            if (error) {
                rapidjson::Document fail = CommandFailure(uri, commandID, kErrorUnknownURI, sErrorMessage[kErrorUnknownURI] + uri);
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
        }
        catch (const std::exception &e) {
            errorCode = kErrorExtensionException;
            errorMsg = e.what();
        }
        catch (...) {
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
                rapidjson::Document fail = CommandFailure(uri, commandID, errorCode, errorMsg);
                error(uri, fail);
            }
            return false;
        }

        // notify success callback
        if (success) {
            rapidjson::Document win = CommandSuccess(uri, commandID);
            success(uri, win);
        }
        return true;
    }

    void registerEventCallback(Extension::EventCallback callback) override {
        if (mExtension)
            mExtension->registerEventCallback(callback);
    }

    void registerLiveDataUpdateCallback(Extension::LiveDataUpdateCallback callback) override {
        if (mExtension)
            mExtension->registerLiveDataUpdateCallback(callback);
    }

private:
    ExtensionPtr mExtension;
    ExtensionFactory mFactory;
    std::set<std::string> mURIs;
};

using LocalExtensionProxyPtr = std::shared_ptr<LocalExtensionProxy>;

} // namespace alexaext

#endif //_ALEXAEXT_LOCALEXTENSIONPROXY_H
