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

#ifndef _APL_EXTENSION_COMPONENT_DEFINITION_H
#define _APL_EXTENSION_COMPONENT_DEFINITION_H

#include "apl/component/componentproperties.h"
#include "apl/content/extensioncommanddefinition.h"
#include "apl/content/extensioneventhandler.h"

namespace apl {

/**
 * To create a custom extension component we first define it with a @ExtensionComponentDefinition and
 * then to create one of those components we pass the created @ExtensionComponentDefinition to
 * @ExtensionComponent
 */
class ExtensionComponentDefinition {
public:
    /**
     * Standard constructor
     * @param uri The URI of the extension
     * @param name The name of the component.
     */
    ExtensionComponentDefinition(std::string uri, std::string name)
        : mURI(std::move(uri)),
          mName(std::move(name)),
          mExtensionComponentProperties(std::make_shared<std::map<id_type, ExtensionProperty>>()){}

    /**
     * Sets the visual context type for the component definition.
     * @param visualContextType visual context type
     * @return reference to current ExtensionComponentDefinition
     */
    ExtensionComponentDefinition& visualContextType(const std::string& visualContextType) {
        mVisualContextType = visualContextType;
        return *this;
    }

    /**
     * Sets the commands associated with the extension component.
     * @param commands The parsed commands associated with the extension component
     * @return reference to current ExtensionComponentDefinition
     */
    ExtensionComponentDefinition& commands(const std::vector<ExtensionCommandDefinition>& commands) {
        mCommands = commands;
        return *this;
    }

    /**
     * @return The URI of the extension
     */
    std::string getURI() const { return mURI; }

    /**
     * @return The name of the component
     */
    std::string getName() const { return mName; }

    /**
     * @return Visual context type of extension component
     */
    std::string getVisualContextType() const { return mVisualContextType; }

    /**
     * @return The commands associated with the extension component
     */
    const std::vector<ExtensionCommandDefinition>& getCommands() const { return mCommands; }

    /**
     * @return EventHandlers associated with ExtensionComponentDefinition
     */
    const std::map<id_type, ExtensionEventHandler>& getEventHandlers() const { return mEventHandlers; }

    /**
     * @return string for debugging.
     */
    std::string toDebugString() const {
        return "ExtensionComponentDefinition< uri:" + mURI + ",name:" + mName + ">";
    };

    /**
     * Adds an eventHandler to an internal list.
     * @param eventHandler Extension Event Handler
     */
    void addEventHandler(const id_type& key, ExtensionEventHandler eventHandler) {
        mEventHandlers.emplace(key, std::move(eventHandler));
    }

    void properties(ExtensionPropertiesPtr extensionComponentProperties) {
        mExtensionComponentProperties = std::move(extensionComponentProperties);
    }

    const ExtensionPropertiesPtr& getExtensionComponentProperties() const { return mExtensionComponentProperties; }

private:
    std::string mURI;
    std::string mName;
    std::string mVisualContextType;
    std::vector<ExtensionCommandDefinition> mCommands;
    std::map<id_type, ExtensionEventHandler> mEventHandlers;
    ExtensionPropertiesPtr mExtensionComponentProperties;
};

} // namespace apl

#endif // _APL_EXTENSION_COMPONENT_DEFINITION_H
