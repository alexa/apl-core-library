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

#ifndef _ALEXAEXT_EXTENSION_BASE_H
#define _ALEXAEXT_EXTENSION_BASE_H


#include <rapidjson/document.h>
#include <set>
#include <string>

#include "extension.h"

namespace alexaext {

/**
 * Base implementation of an extension.
 */
class ExtensionBase : public Extension {

public:
    ~ExtensionBase() override = default;

    explicit ExtensionBase(const std::string& uri) { mURIs.emplace(uri); }

    explicit ExtensionBase(std::set<std::string>(uris)) : mURIs(std::move(uris)) {}

    const std::set<std::string> &getURIs() override { return mURIs; }

    /**
     * Register a callback for extension generated "Event" messages that are sent from the extension
     * to the document. This callback is registered by the runtime and called by the extension
     * via invokeExtensionEventHandler(...).
     *
     * @deprecated use the activity descriptor variant
     * @param callback The extension event callback.
     */
    void registerEventCallback(EventCallback callback) override { mEventCallback = callback; }

    /**
     * Register a callback for extension generated "Event" messages that are sent from the extension
     * to the document. This callback is registered by the runtime and called by the extension
     * via invokeExtensionEventHandler(...).
     *
     * @param callback The extension event callback.
     */
    void registerEventCallback(EventActivityCallback&& callback) override { mEventActivityCallback = callback; }

    /**
     * Register a callback for extension "LiveDataUpdate" messages that are sent from the extension
     * to the document. This callback is registered by the runtime and called by the extension
     * via invokeLiveDataUpdate(...).
     *
     * @deprecated use the activity descriptor variant
     * @param callback The extension event callback.
    */
    void registerLiveDataUpdateCallback(LiveDataUpdateCallback callback) override { mLiveDataCallback = callback; }

    /**
     * Register a callback for extension "LiveDataUpdate" messages that are sent from the extension
     * to the document. This callback is registered by the runtime and called by the extension
     * via invokeLiveDataUpdate(...).
     *
     * @deprecated use the activity descriptor variant
     * @param callback The extension event callback.
    */
    void registerLiveDataUpdateCallback(LiveDataUpdateActivityCallback&& callback) override { mLiveDataActivityCallback = callback; }

protected:

    /**
     * Invoke an extension event handler in the document.
     *
     * @deprecated Use the activity descriptor variant
     * @param uri The extension URI.
     * @param event The extension generated event.
     * @return true if the event is delivered, false if there is no callback registered.
     */
    bool invokeExtensionEventHandler(const std::string& uri, const rapidjson::Value& event) {
        if (mEventCallback) {
            mEventCallback(uri, event);
            return true;
        }
        return false;
    }

    /**
     * Invoke an extension event handler in the document.
     *
     * @param activity The activity using this extension's functionality.
     * @param event The extension generated event.
     * @return true if the event is delivered, false if there is no callback registered.
     */
    bool invokeExtensionEventHandler(const ActivityDescriptor& activity, const rapidjson::Value& event) {
        if (mEventActivityCallback) {
            mEventActivityCallback(activity, event);
            return true;
        }

        // For backwards compatibility
        if (mEventCallback) {
            mEventCallback(activity.getURI(), event);
            return true;
        }
        return false;
    }

    /**
     * Invoke an live data binding change, or data update handler in the document.
     *
     * @deprecated Use the activity descriptor variant
     * @param uri The extension URI.
     * @param event The extension generated event.
     * @return true if the event is delivered, false if there is no callback registered.
     */
    bool invokeLiveDataUpdate(const std::string& uri, const rapidjson::Value& liveDataUpdate) {
        if (mLiveDataCallback) {
            mLiveDataCallback(uri, liveDataUpdate);
            return true;
        }
        return false;
    }

    /**
     * Invoke an live data binding change, or data update handler in the document.
     *
     * @param activity The activity using this extension's functionality.
     * @param event The extension generated event.
     * @return true if the event is delivered, false if there is no callback registered.
     */
    bool invokeLiveDataUpdate(const ActivityDescriptor& activity, const rapidjson::Value& liveDataUpdate) {
        if (mLiveDataActivityCallback) {
            mLiveDataActivityCallback(activity, liveDataUpdate);
            return true;
        }
        // For backwards compatibility
        if (mLiveDataCallback) {
            mLiveDataCallback(activity.getURI(), liveDataUpdate);
            return true;
        }

        return false;
    }

    /**
     * Component update ignored by default.
     *
     * @param uri The extension URI.
     * @param command The Component message.
     * @return true if the update succeeded.
     */
    bool updateComponent(const std::string &uri, const rapidjson::Value &command) override {
        return false;
    };

    /**
     * Component update ignored by default.
     *
     * @param activity The activity using this extension's functionality.
     * @param command The Component message.
     * @return true if the update succeeded.
     */
    bool updateComponent(const ActivityDescriptor& activity, const rapidjson::Value &command) override {
        return updateComponent(activity.getURI(), command);
    };

private:
    EventCallback mEventCallback; // deprecated
    EventActivityCallback mEventActivityCallback;
    LiveDataUpdateCallback mLiveDataCallback; // deprecated
    LiveDataUpdateActivityCallback mLiveDataActivityCallback;
    std::set<std::string> mURIs;
};

} // namespace alexaext

#endif //_ALEXAEXT_EXTENSION_BASE_H
