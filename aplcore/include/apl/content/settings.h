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

#ifndef _APL_SETTINGS_H
#define _APL_SETTINGS_H

#include <cmath>

#include "rapidjson/document.h"

#include "apl/content/package.h"
#include "apl/content/rootconfig.h"
#include "apl/primitives/object.h"
#include "apl/utils/deprecated.h"

namespace apl {

/**
 * Definition of document settings. As per specification APL document can define settings that could
 * override device values.
 */
class Settings {
    friend class CoreDocumentContext;
    friend class Content;

public:

    Settings() :
            mJson(nullptr) {}

    explicit Settings(const rapidjson::Value& json) :
            mJson(&json) {}


    int idleTimeout(const RootConfig & config) const {
        Object value = getValue("idleTimeout");
        if (value.isNumber()) {
            auto idle =  value.getInteger();
            if (idle >= 0 ) {
                return idle;
            }
        }
        return config.getDefaultIdleTimeout();
    }

    bool reactiveConditionalInflation(const RootConfig & config) const {
        Object value = getValue("-experimentalIsReactive");
        return value.asBoolean();
    }

    /**
     * @deprecated removed in APL 1.4
     * @return  Recommended time in milliseconds that the document should be kept on the screen
     * before closing due to inactivity.
     */
    APL_DEPRECATED int idleTimeout() const {
        Object value = getValue("idleTimeout");
        if (value.isNumber()) {
            auto idle =  value.getInteger();
            if (idle >= 0 ) {
                return idle;
            }
        }
        return mDefaultIdleTimeout;
    }

    /**
     * Retrieve a value from the settings.
     * @param key the key to retrieve.
     * @return The value or null if it doesn't exist.
     */
    Object getValue(const std::string &key) const {
        if (mJson && !mJson->IsNull()) {
            auto valueIter = mJson->FindMember(key.c_str());
            if (valueIter != mJson->MemberEnd())
                return Object(valueIter->value);
        }

        return Object::NULL_OBJECT();
    }

    /**
     * Adds core defaults to APL document settings if not available and returns the settings json.
     * @param allocator RapidJSON memory allocator
     * @param config    RootConfig
     * @return          The Json Object
     */
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator, const RootConfig & config) {
        rapidjson::Value result(rapidjson::kObjectType);
        if (mJson && mJson->IsObject()) {
            // Document does have settings, so let's copy those in
            result.CopyFrom(*mJson, allocator);
            // Remove the settings that we're opinionated about
            result.RemoveMember("idleTimeout");
            result.RemoveMember("supportsResizing");
        }

        // Add (or re-add) the members with correct types and defaults
        result.AddMember("idleTimeout", idleTimeout(config), allocator);
        result.AddMember("supportsResizing", getValue("supportsResizing").asBoolean(), allocator);
        return result;
    }

    /**
     * Finds the settings section of a Package.
     * @param package The APL Content package.
     * @return json value for Settings, Value.IsNull() is true when not found.
     */
    static const rapidjson::Value& findSettings(Package& package) {
        // Read settings
        const auto& json = package.json();
        auto settingsIter = json.FindMember("settings");

        // NOTE: Backward compatibility for some APL 1.0 users where a runtime allowed "features" instead of "settings"
        if (settingsIter == json.MemberEnd())
            settingsIter = json.FindMember("features");

        if (settingsIter != json.MemberEnd() && settingsIter->value.IsObject()) {
            return settingsIter->value;
        }
        static const auto val = rapidjson::Value();
        return val;
    }

private:

    /**
     * @deprecated use Content->getDocumentSettings()
     */
    APL_DEPRECATED void read(const RootConfig& config) {
        mDefaultIdleTimeout = config.getDefaultIdleTimeout();
    }

private:
    /** @deprecated, removed in APL1.4 **/
    int mDefaultIdleTimeout = 30000;

    const rapidjson::Value *mJson;
};

} // namespace apl

#endif //_APL_SETTINGS_H