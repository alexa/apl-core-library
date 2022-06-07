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

#ifndef _ALEXAEXT_EXTENSION_PROXY_H
#define _ALEXAEXT_EXTENSION_PROXY_H


#include <functional>
#include <rapidjson/document.h>
#include <set>
#include <string>

#include "extension.h"

namespace alexaext {

/**
 * Extension proxy provides access to a single extension.  It is responsible for
 * providing the execution environment access to the ExtensionDescriptor before the extension
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
    virtual std::set<std::string> getURIs() const = 0;

    /**
     * Initialize the extension. This extension should load resources or configure state associated
     * with the given uri.
     *
     * @param uri The extension URI.
     * @return true if the extension is initialized successfully.
     */
    virtual bool initializeExtension(const std::string &uri) = 0;

    /**
     * Check if extension was initialized.
     *
     * @param uri The extension URI.
     * @return true if the extension initialized, false otherwise.
     */
    virtual bool isInitialized(const std::string &uri) const = 0;

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
     * @deprecated Use the activity descriptor variant
     * @param uri The URI used to identify the extension.
     * @param registrationRequest A "RegistrationRequest" message, includes extension settings.
     * @param success The callback for success, provides the extension schema.
     * @param error The callback for failure, identifies the error code and message.
     * @return true, if the request for a schema can be processed.
     */
    virtual bool getRegistration(const std::string &uri,
                                 const rapidjson::Value &registrationRequest,
                                 RegistrationSuccessCallback success,
                                 RegistrationFailureCallback error) { return false; }

    /**
     * Callback supplied by the runtime for successful execution of getRegistration(...). This
     * callback supports asynchronous response.
     *
     * @param activity The extension activity.
     * @param registrationSuccess A "RegistrationSuccess" message, containing the extension schema.
     */
    using RegistrationSuccessActivityCallback =
        std::function<void(const ActivityDescriptor& activity, const rapidjson::Value& registrationSuccess)>;

    /**
     * Callback supplied by the runtime for failed execution of getRegistration(...). This
     * callback supports asynchronous response.
     *
     * @param activity The extension activity.
     * @param registrationFailure A "RegistrationFailure" message containing the error codee and message.
     */
    using RegistrationFailureActivityCallback =
        std::function<void(const ActivityDescriptor& activity, const rapidjson::Value& registrationFailure)>;

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
     * @param activity The activity requesting this extension's functionality.
     * @param registrationRequest A "RegistrationRequest" message, includes extension settings.
     * @param success The callback for success, provides the extension schema.
     * @param error The callback for failure, identifies the error code and message.
     * @return true, if the request for a schema can be processed.
     */
    virtual bool getRegistration(const ActivityDescriptor& activity,
                                 const rapidjson::Value& registrationRequest,
                                 RegistrationSuccessActivityCallback&& success,
                                 RegistrationFailureActivityCallback&& error) {
        return getRegistration(activity.getURI(), registrationRequest,
                [activity, success](const std::string &uri, const rapidjson::Value &registrationSuccess) {
                    success(activity, registrationSuccess);
                },
                [activity, error](const std::string &uri, const rapidjson::Value &registrationFailure) {
                    error(activity, registrationFailure);
                });
    }

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
     * @deprecated Use the activity descriptor variant
     * @param uri The extension URI.
     * @param command A "Command" message.
     * @param success The callback for success, provides the command results.
     * @param error The callback for failure, identifies the command error.
     * @return true, if the request can be processed.
     */
    virtual bool invokeCommand(const std::string &uri,
                               const rapidjson::Value &command,
                               CommandSuccessCallback success,
                               CommandFailureCallback error) { return false; }

    /**
     * Callback for successful execution of invokeCommand(...).
     *
     * @param activity The extension activity.
     * @param successResponse The "CommandSuccess" message containing the result.
     */
    using CommandSuccessActivityCallback =
        std::function<void(const ActivityDescriptor& activity, const rapidjson::Value &commandSuccess)>;

    /**
     * Callback for failed execution of invokeCommand(...).
     *
     * @param activity The extension activity.
     * @param commandResponse The the "CommandFailure" message containing failure the reason.
     */
    using CommandFailureActivityCallback =
        std::function<void(const ActivityDescriptor& activity, const rapidjson::Value &commandFailure)>;

    /**
     * Forwards a command invocation to the extension. The command is initiated by the document.
     *
     * Implementors of the callback may enforce a timeout.
     *
     * @param activity The activity requesting this extension's functionality.
     * @param command A "Command" message.
     * @param success The callback for success, provides the command results.
     * @param error The callback for failure, identifies the command error.
     * @return true, if the request can be processed.
     */
    virtual bool invokeCommand(const ActivityDescriptor& activity,
                               const rapidjson::Value& command,
                               CommandSuccessActivityCallback&& success,
                               CommandFailureActivityCallback&& error) {
        return invokeCommand(activity.getURI(), command,
                [activity, success](const std::string &uri, const rapidjson::Value &commandSuccess) {
                    success(activity, commandSuccess);
                },
                [activity, error](const std::string &uri, const rapidjson::Value &commandFailure) {
                    error(activity, commandFailure);
                });
    }

    /**
     * Forward a component message to the extension. May be initiated by the document or core.
     *
     * @deprecated Use @c sendComponentMessage
     * @param uri The extension URI.
     * @param message Extension message.
     * @return true, if the request can be processed.
     */
    virtual bool sendMessage(const std::string &uri, const rapidjson::Value &message) { return false; }

    /**
     * Forward a component message to the extension. May be initiated by the document or core.
     *
     * @deprecated Use the activity descriptor variant
     * @param uri The extension URI.
     * @param message Extension message.
     * @return true, if the request can be processed.
     */
    virtual bool sendComponentMessage(const std::string &uri, const rapidjson::Value &message) { return sendMessage(uri, message); }

    /**
     * Forward a component message to the extension. May be initiated by the document or core.
     *
     * @param activity The activity requesting this extension's functionality.
     * @param message Extension message.
     * @return true, if the request can be processed.
     */
    virtual bool sendComponentMessage(const ActivityDescriptor& activity, const rapidjson::Value& message) {
        return sendComponentMessage(activity.getURI(), message);
    }

    /**
     * Register a callback for extension generated "Event" messages that are sent from the extension
     * to the document. This callback is registered by the runtime and called by ExtensionBase
     * via invokeExtensionEventHandler(...).
     *
     * This method can be called multiple times to register multiple callbacks.
     *
     * @deprecated Use the activity descriptor variant
     * @param callback The extension event callback.
     */
    virtual void registerEventCallback(Extension::EventCallback callback) {}

    /**
     * Register a callback for extension generated "Event" messages that are sent from the extension
     * to the document. This callback is registered by the execution environment and called by ExtensionBase
     * via invokeExtensionEventHandler(...).
     *
     * This method can be called multiple times to register multiple callbacks.
     *
     * @param activity The extension activity for the specified callback
     * @param callback The extension event callback.
     */
    virtual void registerEventCallback(const alexaext::ActivityDescriptor& activity,
                                       Extension::EventActivityCallback&& callback) {}

    /**
     * Register a callback for extension generated "LiveDataUpdate" messages that are sent from
     * the extension to the document. This callback is registered by the runtime and called by
     * ExtensionBase via invokeLiveDataUpdate(...).
     *
     * This method can be called multiple times to register multiple callbacks.
     *
     * @deprecated Use the activity descriptor variant
     * @param callback The live data update callback.
     */
    virtual void registerLiveDataUpdateCallback(Extension::LiveDataUpdateCallback callback) {}

    /**
     * Register a callback for extension generated "LiveDataUpdate" messages that are sent from
     * the extension to the document. This callback is registered by the runtime and called by
     * ExtensionBase via invokeLiveDataUpdate(...).
     *
     * This method can be called multiple times to register multiple callbacks.
     *
     * @param activity The extension activity for the specified callback
     * @param callback The live data update callback.
     */
    virtual void registerLiveDataUpdateCallback(const alexaext::ActivityDescriptor& activity,
                                                Extension::LiveDataUpdateActivityCallback&& callback) {}

    /**
     * Invoked when an extension behind this proxy is successfully registered.
     *
     * @deprecated Use the activity descriptor variant
     * @param uri The extension URI
     * @param token The activity token issued during registration
     */
    virtual void onRegistered(const std::string &uri, const std::string &token) {}

    /**
     * Invoked when an extension behind this proxy is successfully registered.
     *
     * @param activity The activity requesting this extension's functionality.
     */
    virtual void onRegistered(const ActivityDescriptor& activity) {
        onRegistered(activity.getURI(), activity.getId());
    }

    /**
     * Invoked when an extension is unregistered. Session represented by the provided token is no longer valid.
     *
     * @deprecated Use the activity descriptor variant
     * @param uri The extension URI
     * @param token The activity token issued during registration
     */
    virtual void onUnregistered(const std::string &uri, const std::string &token) {}

    /**
     * Invoked when an extension is unregistered. Session represented by the provided token is no longer valid.
     *
     * @param activity The activity requesting this extension's functionality.
     */
    virtual void onUnregistered(const ActivityDescriptor& activity) {
        onUnregistered(activity.getURI(), activity.getId());
    }

    /**
     * Invoked when a system rendering resource, such as display surface, is ready for use. This
     * method will be called after the execution environment receives a "Component" message with a
     * resource state of "Ready".  Not all execution environments support shared rendering resources.
     *
     * @deprecated Use the activity descriptor variant
     * @param uri The extension URI.
     * @param resourceHolder Access to the rendering resource.
     */
    virtual void onResourceReady(const std::string &uri, const ResourceHolderPtr& resourceHolder) {}

    /**
     * Invoked when a system rendering resource, such as display surface, is ready for use. This
     * method will be called after the execution environment receives a "Component" message with a
     * resource state of "Ready".  Not all execution environments support shared rendering resources.
     *
     * @param activity The activity requesting this extension's functionality.
     * @param resourceHolder Access to the rendering resource.
     */
    virtual void onResourceReady(const ActivityDescriptor& activity, const ResourceHolderPtr& resourceHolder) {
        onResourceReady(activity.getURI(), resourceHolder);
    }

    /**
     * @see Extension::onSessionStarted
     */
    virtual void onSessionStarted(const SessionDescriptor& session) {}

    /**
     * @see Extension::onSessionEnded
     */
    virtual void onSessionEnded(const SessionDescriptor& session) {}

    /**
     * @see Extension::onForeground
     */
    virtual void onForeground(const ActivityDescriptor& activity) {}

    /**
     * @see Extension::onBackground
     */
    virtual void onBackground(const ActivityDescriptor& activity) {}

    /**
     * @see Extension::onHidden
     */
    virtual void onHidden(const ActivityDescriptor& activity) {}
};

using ExtensionProxyPtr = std::shared_ptr<ExtensionProxy>;

} // namespace alexaext

#endif //_ALEXAEXT_EXTENSION_PROXY_H
