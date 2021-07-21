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

#ifndef _ALEXAEXT_EXTENSION_H
#define _ALEXAEXT_EXTENSION_H

#include <functional>
#include <memory>
#include <set>
#include <string>

#include <rapidjson/document.h>

#include "extensionmessage.h"

namespace alexaext {

/**
 * The Extension interface defines the contract exposed from the Extension to the
 * document. It supports invoking an extension command from the document, and sending
 * events from the extension to the document.
 */
class Extension {

public:
    virtual ~Extension() = default;

    /**
     * Get the URIs supported by the extension.
     *
     * @return The extension URIs.
     */
    virtual const std::set<std::string>& getURIs() = 0;

    /**
     * Create a registration for the extension. The registration is returned in a "RegistrationSuccess" or
     * "RegistrationFailure" message. The extension is defined by a unique token per registration, an environment of
     * static properties, and the extension schema.
     *
     * The schema defines the extension api, including commands, events and live data.  The "RegistrationRequest"
     * parameter contains a schema version, which matches the schema versions supported by the runtime, and extension
     * settings defined by the requesting document.
     *
     * std::exception or ExtensionException thrown from this method are converted to "RegistrationFailure"
     * messages and returned to the caller.
     *
     * This method is called by the extension framework when the extension is requested by a document.
     *
     * @param uri The extension URI.
     * @param registrationRequest A "RegistrationRequest" message, includes extension settings.
     * @return A extension "RegistrationSuccess" or "RegistrationFailure"  message.
     */
    virtual rapidjson::Document createRegistration(const std::string &uri,
            const rapidjson::Value& registrationRequest) = 0;

    /**
     * Callback definition for extension "Event" messages. The extension will call back to
     * invoke an extension event handler in the document.
     *
     * @param uri The URI of the extension.
     * @param event A extension "Event" message.
     */
    using EventCallback =
    std::function<void(const std::string& uri, const rapidjson::Value &event)>;

    /**
     * Callback registration for extension "Event" messages. Guaranteed to be called before the document is mounted.
     * The callback forwards events to the document event handlers.
     *
     * @param callback The callback for events generating from the extension.
     */
    virtual void registerEventCallback(EventCallback callback) = 0;

    /**
     * Callback definition for extension "LiveDataUpdate" messages. The extension will call back to
     * update the data binding or invoke a lived data handler in the document.
     *
     * @param uri The URI of the extension.
     * @param liveDataUpdate The "LiveDataUpdate" message.
     */
    using LiveDataUpdateCallback =
    std::function<void(const std::string &uri, const rapidjson::Value &liveDataUpdate)>;

    /**
     * Callback for extension "LiveDataUpdate" messages. Guaranteed to be called before the document is mounted.
     * The callback forwards live data changes to the document data binding and live data handlers.
     *
     * @param callback The callback for live data updates generating from the extension.
     */
    virtual void registerLiveDataUpdateCallback(LiveDataUpdateCallback callback) = 0;

    /**
     * Execute a Command that was initiated by the document.
     *
     * std::exception or ExtensionException thrown from this method are converted to "CommandFailure"
     * messages and returned to the caller.
     *
     * @param uri The extension URI.
     * @param command The requested Command message.
     * @return true if the command succeeded.
     */
    virtual bool invokeCommand(const std::string& uri, const rapidjson::Value &command) = 0;

    /**
     * Invoked after registration has been completed successfully. This is useful for
     * stateful extensions that require initializing session data upfront.
     *
     * @param uri The extension URI used during registration.
     * @param token The client token issued during registration.
     */
    virtual void onRegistered(const std::string &uri, const std::string &token) {}
};

using ExtensionPtr = std::shared_ptr<Extension>;

} // namespace alexaext

#endif //_ALEXAEXT_EXTENSION_H
