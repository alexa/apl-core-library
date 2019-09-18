/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/primitives/object.h"
#include "apl/content/rootconfig.h"

namespace apl {

/**
 * Definition of document settings. As per specification APL document can define some settings that could
 * override device values.
 */
class Settings {
    friend class RootContext;

public:
    Settings(const RootConfig& config) :
        mIdleTimeout(config.getDefaultIdleTimeout()),
        mJson(nullptr)
    {}

    /**
     * @return  Recommended time in milliseconds that the document should be kept on the screen
     * before closing due to inactivity.
     */
    int idleTimeout() const { return mIdleTimeout; }

    /**
     * Retrieve a value from the settings.
     * @param key the key to retrieve.
     * @return The value or null if it doesn't exist.
     */
    const Object getValue(const std::string &key) const {
        if (mJson) {
            auto valueIter = mJson->FindMember(key.c_str());
            if (valueIter != mJson->MemberEnd())
                return Object(valueIter->value);
        }

        return Object::NULL_OBJECT();
    }

private:

    void read(const rapidjson::Value& json) {
        mJson = &json;
        Object value = getValue("idleTimeout");
        if (value.isNumber()) {
            mIdleTimeout = value.getInteger();
        }
    }

private:
    int mIdleTimeout;
    const rapidjson::Value *mJson;
};

} // namespace apl

#endif //_APL_SETTINGS_H