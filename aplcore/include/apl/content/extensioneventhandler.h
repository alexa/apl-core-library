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

#ifndef _APL_EXTENSION_EVENT_HANDLER_H
#define _APL_EXTENSION_EVENT_HANDLER_H

#include <string>

namespace apl {

/**
 * Store information about a custom extension event handler.
 */
class ExtensionEventHandler {
public:
    /**
     * Define an extension event handler.
     * @param uri The URI of the extension.
     * @param name The name of the event handler.
     */
    ExtensionEventHandler(const std::string& uri, const std::string& name) : mURI(uri), mName(name) {}

    /**
     * @return The extension URI associated with this event handler
     */
    std::string getURI() const { return mURI; }

    /**
     * @return The name of this event handler
     */
    std::string getName() const { return mName; }

    bool operator<(const ExtensionEventHandler& rhs) const {
        if (mURI < rhs.mURI) return true;
        if (mURI > rhs.mURI) return false;
        return mName < rhs.mName;
    }

    bool operator==(const ExtensionEventHandler& rhs) const {
        return mURI == rhs.mURI && mName == rhs.mName;
    }

    /**
    * @return string for debugging.
    */
    std::string toDebugString() const {
        return "ExtensionEventHandler< uri:" + mURI + " name:" + mName + ">";
    }

private:
    std::string mURI;
    std::string mName;
};
} // namespace apl

#endif // _APL_EXTENSION_EVENT_HANDLER_H
