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
     * Set the resource type of the component.  The resource type specifies how the system
     * resources are allocated to render the component.  An example resource type may be "Surface".
     * Execution environments define the the supported resource types.  The execution environment
     * uses a default when no value is set.  Unsupported resourceTypes result in undefined behavior.
     * See the execution environment documentation for supported types.
     *
     * @param resourceType The rendering resource type.
     * @return reference to current ExtensionComponentDefinition
     */
    ExtensionComponentDefinition& resourceType(const std::string& resourceType) {
        mResourceType = resourceType;
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
     * @return The type of resource used for rendering the component.
     */
    std::string getResourceType() const { return mResourceType; }

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
    std::string mResourceType;
    std::map<id_type, ExtensionEventHandler> mEventHandlers;
    ExtensionPropertiesPtr mExtensionComponentProperties;
};

} // namespace apl

#endif // _APL_EXTENSION_COMPONENT_DEFINITION_H
