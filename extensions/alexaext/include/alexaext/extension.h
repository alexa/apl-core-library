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

#include "activitydescriptor.h"
#include "extensionmessage.h"
#include "extensionresourceholder.h"
#include "sessiondescriptor.h"
#include "types.h"

namespace alexaext {

/**
 * The Extension interface defines the contract exposed from the extension to a activity (e.g. a
 * typical activity for an APL extension is a rendering task for an APL document). Extensions are
 * typically lazily instantiated by an execution environment (e.g. APL or Alexa Web for Games) in
 * response to the extension being requested by an activity.
 *
 * The extension contract also defines the lifecycle of an extension. The lifecycle of an extension
 * starts with an activity requesting it. Each activity belongs to exactly one session for the
 * entire duration of the activity.
 *
 * During a activity interaction, an extension will receive a well-defined sequence of calls. For
 * example, consider a common extension use case: a single, standalone APL document requests an
 * extension to render its contents, and then gets finished (i.e. taken off screen) after a short
 * interaction. In this example, the activity corresponds to the rendering task for the APL document,
 * the session corresponds to the skill session.
 *
 * The extension would, for this example, receive the following sequence of calls as
 * follows:
 *
 * - @c onSessionStarted is called with the document's session descriptor
 * - @c createRegistration is called for the document
 * - @c onActivityRegistered is called when the registration succeeds
 * - @c onForeground is called to indicate that the activity is being rendered in the foreground
 * - the activity can then send commands and receive events with the extension
 * - the document is finished by the APL execution environment after user interactions are done
 * - @c onActivityUnregistered is called to indicate that the document is no longer active
 * - @c onSessionEnded is called (could be delayed)
 *
 * Consider the more complex case of an extension being requested by a set of related APL documents
 * interacting with each other via the APL backstack. For example, this could be a menu flow
 * implemented as a series of distinct documents. For a multi-document session, a typical flow
 * would instead be as follows:
 * - @c onSessionStarted is called with the first document's session descriptor
 * - @c createRegistration is called for the first document
 * - @c onActivityRegistered is called when the registration succeeds
 * - @c onForeground is called to indicate that the activity is being rendered in the foreground
 * - the activity can then send commands and receive events with the extension
 * - a new document is rendered in the same session, and the current one is pushed to the backstack
 * - @c createRegistration is called for the second document
 * - @c onActivityRegistered is called when the registration succeeds
 * - @c onHidden is called for the first activity to indicate it is now hidden
 * - @c onForeground is called to indicate that the activity is in the foreground
 * - the second document can now interact with the extension
 * - the second document restores the first document from the backstack
 * - @c onActivityUnregistered is called to indicate that the second document is no longer active
 * - @c onForeground is called to indicate that the first document is now again in the foreground
 * - the first document is finished
 * - @c onActivityUnregistered is called to indicate that the first document is no longer active
 * - @c onSessionEnded is called (could be delayed)
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
     * Create a registration for the extension. The registration is returned in a
     * "RegistrationSuccess" or "RegistrationFailure" message. The extension is defined by a unique
     * token per registration, an environment of static properties, and the extension schema. This
     * method is called by the extension framework when the extension is requested by an
     * activity.
     *
     * The schema defines the extension api, including commands, events and live data.  The
     * "RegistrationRequest" parameter contains a schema version, which matches the schema versions
     * supported by the execution environment, and extension settings defined by the requesting
     * activity.
     *
     * std::exception or ExtensionException thrown from this method are converted to
     * "RegistrationFailure" messages and returned to the caller.
     *
     * @deprecated Use the @c ActivityDescriptor variant
     * @param uri The extension URI.
     * @param registrationRequest A "RegistrationRequest" message, includes extension settings.
     * @return A extension "RegistrationSuccess" or "RegistrationFailure"  message.
     */
    virtual rapidjson::Document createRegistration(const std::string& uri,
                                                   const rapidjson::Value& registrationRequest) {
        return RegistrationFailure::forException(uri, "Not implemented");
    };

    /**
     * Create a registration for the extension. The registration is returned in a
     * "RegistrationSuccess" or "RegistrationFailure" message. The extension is defined by a unique
     * token per registration, an environment of static properties, and the extension schema. This
     * method is called by the extension framework when the extension is requested by an
     * activity.
     *
     * The schema defines the extension api, including commands, events and live data.  The
     * "RegistrationRequest" parameter contains a schema version, which matches the schema versions
     * supported by the execution environment, and extension settings defined by the requesting
     * activity.
     *
     * std::exception or ExtensionException thrown from this method are converted to
     * "RegistrationFailure" messages and returned to the caller.
     *
     * The activity descriptor has a pre-populated activity identifier. If an extension chooses to
     * use this identifier, it can simply return a response that uses "<AUTO_TOKEN>" as the activity
     * token. If an extension chooses to provide a new token instead, it will be used as the
     * activity identifier for all subsequent calls.
     *
     * @param activity The activity using this extension.
     * @param registrationRequest A "RegistrationRequest" message, includes extension settings.
     * @return A extension "RegistrationSuccess" or "RegistrationFailure"  message.
     */
    virtual rapidjson::Document createRegistration(const ActivityDescriptor& activity,
                                                   const rapidjson::Value& registrationRequest) {
        return createRegistration(activity.getURI(), registrationRequest);
    }

    /**
     * Callback definition for extension "Event" messages. The extension will call back to
     * invoke an extension event handler in the activity.
     *
     * @param uri The URI of the extension.
     * @param event A extension "Event" message.
     */
    using EventCallback =
        std::function<void(const std::string& uri, const rapidjson::Value& event)>;

    /**
     * Callback definition for extension "Event" messages. The extension will call back to
     * invoke an extension event handler in the activity.
     *
     * @param activity The activity using this extension.
     * @param event A extension "Event" message.
     */
    using EventActivityCallback =
    std::function<void(const ActivityDescriptor& activity, const rapidjson::Value& event)>;

    /**
     * Callback registration for extension "Event" messages. When the activity corresponds to an APL
     * document rendering task, this method is guaranteed to be called before the document is
     * mounted. The callback forwards events to the activity event handlers.
     *
     * @deprecated Use the @c ActivityDescriptor variant
     * @param callback The callback for events generating from the extension.
     */
    virtual void registerEventCallback(EventCallback callback) { }

    /**
     * Callback registration for extension "Event" messages. When the activity corresponds to an APL
     * document rendering task, this method is guaranteed to be called before the document is
     * mounted. The callback forwards events to the document event handlers.
     *
     * @param callback The callback for events generating from the extension.
     */
    virtual void registerEventCallback(EventActivityCallback&& callback) { }

    /**
     * Callback definition for extension "LiveDataUpdate" messages. The extension will call back to
     * update the data binding or invoke a lived data handler in the activity.
     *
     * @deprecated Use the ActivityDescriptor variant
     * @param uri The URI of the extension.
     * @param liveDataUpdate The "LiveDataUpdate" message.
     */
    using LiveDataUpdateCallback =
        std::function<void(const std::string& uri, const rapidjson::Value& liveDataUpdate)>;

    /**
     * Callback definition for extension "LiveDataUpdate" messages. The extension will call back to
     * update the data binding or invoke a lived data handler in the activity.
     *
     * @param activity The activity using this extension.
     * @param liveDataUpdate The "LiveDataUpdate" message.
     */
    using LiveDataUpdateActivityCallback =
    std::function<void(const ActivityDescriptor& activity, const rapidjson::Value& liveDataUpdate)>;

    /**
     * Callback for extension "LiveDataUpdate" messages. When the activity corresponds to an APL
     * document rendering task, this method is guaranteed to be called before the document is
     * mounted. The callback forwards live data changes to the activity and live data handlers.
     * In the case of APL activities, this will update the document data binding.
     *
     * @param callback The callback for live data updates generating from the extension.
     */
    virtual void registerLiveDataUpdateCallback(LiveDataUpdateCallback callback) { }

    /**
     * Callback for extension "LiveDataUpdate" messages. When the activity corresponds to an APL
     * document rendering task, this method is guaranteed to be called before the document is
     * mounted. The callback forwards live data changes to the activity and live data handlers.
     * In the case of APL activities, this will update the document data binding.
     *
     * @param callback The callback for live data updates generating from the extension.
     */
    virtual void registerLiveDataUpdateCallback(LiveDataUpdateActivityCallback&& callback) { }

    /**
     * Execute a Command that was initiated by the activity.
     *
     * std::exception or ExtensionException thrown from this method are converted to
     * "CommandFailure" messages and returned to the caller.
     *
     * @deprecated Use the @c ActivityDescriptor variant
     * @param uri The extension URI.
     * @param command The requested Command message.
     * @return true if the command succeeded.
     */
    virtual bool invokeCommand(const std::string& uri, const rapidjson::Value& command) {
        return false;
    }

    /**
     * Execute a Command that was initiated by the activity.
     *
     * std::exception or ExtensionException thrown from this method are converted to
     * "CommandFailure" messages and returned to the caller.
     *
     * @param activity The activity using this extension.
     * @param command The requested Command message.
     * @return true if the command succeeded.
     */
    virtual bool invokeCommand(const ActivityDescriptor& activity, const rapidjson::Value& command) {
        return invokeCommand(activity.getURI(), command);
    }

    /**
     * Invoked after registration has been completed successfully. This is useful for
     * stateful extensions that require initializing activity data upfront.
     *
     * @deprecated Use the @c ActivityDescriptor variant
     * @param uri The extension URI used during registration.
     * @param token The activity token issued during registration.
     */
    virtual void onRegistered(const std::string& uri, const std::string& token) {}

    /**
     * Invoked after registration has been completed successfully. This is useful for
     * stateful extensions that require initializing activity data upfront.
     *
     * @param activity The activity using this extension.
     */
    virtual void onActivityRegistered(const ActivityDescriptor& activity) {
        onRegistered(activity.getURI(), activity.getId());
    }

    /**
     * Invoked after extension unregistered. This is useful for stateful extensions that require
     * cleaning up activity data.
     *
     * @deprecated Use the @c ActivityDescriptor variant
     * @param uri The extension URI used during registration.
     * @param token The activity token issued during registration.
     */
    virtual void onUnregistered(const std::string& uri, const std::string& token) {}

    /**
     * Invoked after extension unregistered. This is useful for stateful extensions that require
     * cleaning up activity data.
     *
     * @param activity The activity using this extension.
     */
    virtual void onActivityUnregistered(const ActivityDescriptor& activity) {
        onUnregistered(activity.getURI(), activity.getId());
    }

    /**
     * Update an Extension Component.  A "Component" message is received when the extension
     * component changes state, or has a property updated.
     *
     * @deprecated Use the @c ActivityDescriptor variant
     * @param uri The extension URI.
     * @param command The Component message.
     * @return true if the update succeeded.
     */
    virtual bool updateComponent(const std::string& uri, const rapidjson::Value& command) {
        return false;
    }

    /**
     * Update an Extension Component.  A "Component" message is received when the extension
     * component changes state, or has a property updated.
     *
     * @param activity The activity using this extension.
     * @param command The Component message.
     * @return true if the update succeeded.
     */
    virtual bool updateComponent(const ActivityDescriptor& activity, const rapidjson::Value& command) {
        return updateComponent(activity.getURI(), command);
    }

    /**
     * Invoked when a system resource, such as display surface, is ready for use. This method
     * will be called after the extension receives a message indicating the resource is "Ready".
     * Messages supporting shared resources: "Component"
     * Not all execution environments support shared resources.
     *
     * @deprecated Use the @c ActivityDescriptor variant
     * @param uri The extension URI.
     * @param resourceHolder Access to the resource.
     */
    virtual void onResourceReady(const std::string& uri, const ResourceHolderPtr& resourceHolder) {}

    /**
     * Invoked when a system resource, such as display surface, is ready for use. This method
     * will be called after the extension receives a message indicating the resource is "Ready".
     * Messages supporting shared resources: "Component"
     * Not all execution environments support shared resources.
     *
     * @param activity The activity using this extension.
     * @param resourceHolder Access to the resource.
     */
    virtual void onResourceReady(const ActivityDescriptor& activity, const ResourceHolderPtr& resourceHolder) {
        onResourceReady(activity.getURI(), resourceHolder);
    }

    /**
     * Called whenever a new session that requires this extension is started. This is guaranteed
     * to be called before @c onActivityRegistered for any activity that belongs to the specified
     * session.
     *
     * No guarantees are made regarding the time at which this is invoked, only that if
     * @c onActivityRegistered is invoked, this call will have happened prior to it. For example, a
     * typical implementation will withhold the call until extension registration is triggered in
     * order to avoid spurious notifications about contexts being created / destroyed that do not
     * require this extension.
     *
     * This call is guaranteed to be made only once for a given session and extension pair.
     *
     * @param session The session being started.
     */
    virtual void onSessionStarted(const SessionDescriptor& session) {}

    /**
     * Invoked when a previously started session has ended. This is only called when
     * @c onSessionStarted was previously called for the same session.
     *
     * This call is guaranteed to be made only once for a given session and extension pair.
     *
     * @param session The session that ended.
     */
    virtual void onSessionEnded(const SessionDescriptor& session) {}

    /**
     * Invoked when a visual activity becomes in the foreground. If an activity does not
     * have any associated visual presentation, this method is never called for it. If a
     * visual activity starts in the foreground, this method will be called right after
     * a successful registration.
     *
     * @param activity The activity using this extension.
     */
    virtual void onForeground(const ActivityDescriptor& activity) {}

    /**
     * Invoked when a visual activity becomes in the background, i.e. it is still completely or
     * partially visible, but is no longer the active visual presentation. If an activity does not
     * have any associated visual presentation, this method is never called for it. If a
     * visual activity starts in the background, this method will be called right after
     * a successful registration.
     *
     * Extensions are encouraged to avoid publishing updates to backgrounded activities as
     * they may not be able to process them.
     *
     * @param activity The activity using this extension.
     */
    virtual void onBackground(const ActivityDescriptor& activity) {}

    /**
     * Invoked when a visual activity becomes in hidden, i.e. it is no longer visible (e.g. it was
     * pushed to the backstack, or was temporarily replaced by another presentation activity). If an
     * activity does not have any associated visual presentation, this method is never called for
     * it. If a visual activity starts in the background, this method will be called right after
     * a successful registration.
     *
     * This method is not called when an activity leaves the screen because it ended.
     *
     * Extensions are encouraged to avoid publishing updates to hidden activities as
     * they are typically not able to process them.
     *
     * @param activity The activity using this extension.
     */
    virtual void onHidden(const ActivityDescriptor& activity) {}
};

using ExtensionPtr = std::shared_ptr<Extension>;

} // namespace alexaext

#endif //_ALEXAEXT_EXTENSION_H
