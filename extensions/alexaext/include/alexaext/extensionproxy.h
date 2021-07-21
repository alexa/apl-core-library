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

#ifndef _ALEXAEXT_EXTENSIONPROXY_H
#define _ALEXAEXT_EXTENSIONPROXY_H


#include <functional>
#include <rapidjson/document.h>
#include <set>
#include <string>


#include "extension.h"

namespace alexaext {

/**
 * Extension proxy provides access to a single extension.  It is responsible for
 * providing the runtime access to the ExtensionDescriptor before the extension
 * is in use, and instantiating the extension when requested.
 */
class ExtensionProxy {

public:
    virtual ~ExtensionProxy() = default;

    /**
     * Get the URIs described by the extension.
     *
     * @return The extension URIs.
     */
    virtual std::set<std::string> getURIs() = 0;

    /**
     * Initialize the extension. This extension should load resources or configure state associated
     * with the given uri.
     *
     * @return true if the extension is initialized successfully.
     */
    virtual bool initializeExtension(const std::string &uri) = 0;

    /**
     * Callback supplied by the runtime for successful execution of getRegistration(...). This
     * callback supports asynchronous response.
     *
     * @param uri The extension URI.
     * @param registrationSuccess A "RegistrationSuccess" message, containing the extension schema.
     */
    using RegistrationSuccessCallback =
    std::function<void(const std::string &uri, const rapidjson::Value &registrationSuccess)>;

    /**
     * Callback supplied by the runtime for failed execution of getRegistration(...). This
     * callback supports asynchronous response.
     *
     * @param uri The extension URI.
     * @param registrationFailure A "RegistrationFailure" message containing the error codee and message.
     */
    using RegistrationFailureCallback =
    std::function<void(const std::string &uri, const rapidjson::Value &registrationFailure)>;

    /**
     * Called by the runtime to get the extension schema for the URI. This call may be
     * responded to asynchronously via callback. The method should return true if success
     * is expected, and false if the request cannot be handled.  An invalid URI is
     * an example of an immediate return of false.
     *
     * Successful execution of the request will call the SuccessCallback with a "RegistrationSuccess" message
     * and return true.
     *
     * The extension may process the registration request and respond with "RegistrationFailure". This method
     * will return true because the message was processed.  An example of "RegistrationFailure" would be the document
     * missing a required extension setting.
     *
     * Failure during execution of the request will call the FailureCallback with "RegistrationFailure" and
     * return false. Reasons for execution  failure may include unavailable system resources that prevent communication
     * with the extension, or exceptions thrown by the extension.
     *
     * Implementors of the callback may enforce a timeout.
     *
     * @param uri The URI used to identify the extension.
     * @param registrationRequest A "RegistrationRequest" message, includes extension settings.
     * @param success The callback for success, provides the extension schema.
     * @param error The callback for failure, identifies the error code and message.
     * @return true, if the request for a schema can be processed.
     */
    virtual bool getRegistration(const std::string &uri, const rapidjson::Value &registrationRequest,
                                 RegistrationSuccessCallback success, RegistrationFailureCallback error) = 0;

    /**
     * Callback for successful execution of invokeCommand(...).
     *
     * @param uri The extension URI.
     * @param successResponse The "CommandSuccess" message containing the result.
     */
    using CommandSuccessCallback =
    std::function<void(const std::string &uri, const rapidjson::Value &commandSuccess)>;

    /**
     * Callback for failed execution of invokeCommand(...).
     *
     * @param uri The  extension URI.
     * @param commandResponse The the "CommandFailure" message containing failure the reason.
     */
    using CommandFailureCallback =
    std::function<void(const std::string &uri, const rapidjson::Value &commandFailure)>;

    /**
     * Forwards a command invocation to the extension. The command is initiated by the document.
     *
     * Implementors of the callback may enforce a timeout.
     *
     * @param uri The extension URI.
     * @param command A "Command" message.
     * @param success The callback for success, provides the command results.
     * @param error The callback for failure, identifies the command error.
     * @return true, if the request can be processed.
     */
    virtual bool invokeCommand(const std::string &uri, const rapidjson::Value &command,
                               CommandSuccessCallback success, CommandFailureCallback error) = 0;

    /**
     * Register a callback for extension generated "Event" messages that are sent from the extension
     * to the document. This callback is registered by the runtime and called by ExtensionBase
     * via invokeExtensionEventHandler(...).
     *
     * This method can be called multiple times to register multiple callbacks.
     *
     * @param callback The extension event callback.
     */
    virtual void registerEventCallback(Extension::EventCallback callback) = 0;

    /**
     * Register a callback for extension generated "LiveDataUpdate" messages that are sent from
     * the extension to the document. This callback is registered by the runtime and called by
     * ExtensionBase via invokeLiveDataUpdate(...).
     *
     * This method can be called multiple times to register multiple callbacks.
     *
     * @param callback The live data update callback.
     */
    virtual void registerLiveDataUpdateCallback(Extension::LiveDataUpdateCallback callback) = 0;

    /**
     * Invoked when an extension behind this proxy is successfully registered.
     *
     * @param uri The extension URI
     * @param token The client token issued during registration
     */
     virtual void onRegistered(const std::string &uri, const std::string &token) = 0;
};

using ExtensionProxyPtr = std::shared_ptr<ExtensionProxy>;

} // namespace alexaext

#endif //_ALEXAEXT_EXTENSIONPROXY_H
