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

#ifndef _APL_EXTENSION_MANAGER_H
#define _APL_EXTENSION_MANAGER_H

#include <string>
#include <set>

#include "apl/content/extensioncommanddefinition.h"
#include "apl/content/extensioncomponentdefinition.h"
#include "apl/content/extensioneventhandler.h"
#include "apl/content/extensionfilterdefinition.h"
#include "apl/content/extensionrequest.h"
#include "apl/engine/builder.h"
#include "apl/extension/extensioncomponent.h"
#include "apl/primitives/object.h"

namespace apl {

class ExtensionMediator;

/**
 * The extension manager maintains the list of custom events and custom commands
 * registered by extensions and appearing in the document.
 */
class ExtensionManager {
public:
    ExtensionManager(
        const std::vector<ExtensionRequest>& requests,
        const RootConfig& rootConfig,
        const SessionPtr& session);

    /**
     * @return A map of qualified names to event handler definitions.
     */
    const std::map<std::string, ExtensionEventHandler>& getEventHandlerDefinitions() const { return mEventHandlers; }

    /**
     * @return A map of qualified names to command definitions.
     */
    const std::map<std::string, ExtensionCommandDefinition>& getCommandDefinitions() const { return mCommandDefinitions; }

    /**
     * @return A map of qualified names to component definitions.
     */
    const std::map<std::string, ExtensionComponentDefinition>& getComponentDefinitions() const { return mComponentDefinitions; }

    /**
     * @return A map of qualified names to filter definitions.
     */
    const std::map<std::string, ExtensionFilterDefinition>& getFilterDefinitions() const { return mFilterDefinitions; }

    /**
     * Add a document or package-level event handler by name.  These are added as
     * the packages and document are scanned.
     * @param handler The extension event handler
     * @param command The command to execute
     */
    void addEventHandler(const ExtensionEventHandler& handler, Object command);

    /**
     * Add an extension component by unique resource ID.
     * @param resourceId Unique resource ID associated with the component.
     * @param component Extension Component
     */
    void addExtensionComponent(const std::string& resourceId, const ExtensionComponentPtr& component);

    /**
     * Remove extension component associated with the resource ID
     * @param resourceId Unique resource ID associated with the component
     */
    void removeExtensionComponent(const std::string& resourceId);

    /**
     * Search the custom commands for one with the given name.
     * @param qualifiedName The name of the custom command in the form EXT_NAME:CMD_NAME
     * @return The command definition or nullptr if it is not found
     */
    ExtensionCommandDefinition* findCommandDefinition(const std::string& qualifiedName);

    /**
     * Search the extension component definitions for one with the given name.
     * @param qualifiedName The name of the custom component in the form EXT_NAME:COMPONENT_NAME
     * @return The component definition or nullptr if it is not found
     */
    ExtensionComponentDefinition* findComponentDefinition(const std::string& qualifiedName);

    /**
     * Search the custom filters for one with the given name.
     * @param qualifiedName The name of the custom filter in the form EXT_NAME:FILTER_NAME
     * @return The filter definition or nullptr if it is not found
     */
    ExtensionFilterDefinition* findFilterDefinition(const std::string& qualifiedName);

    /**
     * Search the extension components for unique resource ID
     * @param resourceId The name of extension component. This is the unique resource ID associated
     *                      with the component.
     * @return The extension component or nullptr if it is not found
     */
    ExtensionComponentPtr findExtensionComponent(const std::string& resourceId);

    /**
     * Finds an appropriate custom handler to invoke. Returns NULL if no such handler exists
     * @param handler The extension event handler to invoke
     * @return The Object attached to this handler or NULL.
     */
    Object findHandler(const ExtensionEventHandler& handler);

    /**
     * @return A mapping of URI or NAME to boolean values suitable for including in the
     *         data-binding context under "environment.extensions.<extensionName>"
     */
    ObjectMapPtr getEnvironment() const { return mEnvironment; }

    /**
     * Returns the extension components maintained by the manager
     * @return map of extension components.
     */
    const std::map<std::string, ExtensionComponentPtr>& getExtensionComponents() const { return mExtensionComponents; };

    /**
     * Finds and creates extension component specified by the component type
     * @param componentType Component type specified in APL document
     * @return Function which creates extension component
     */
    MakeComponentFunc findAndCreateExtensionComponent(const std::string& componentType);

    /**
     * Returns the map of extension component definitions maintained by the manager.
     * @return Map of extension component definitions.
     */
    const std::map<std::string, ExtensionComponentDefinition>& getExtensionComponentDefinitions() const { return mComponentDefinitions; }

    /**
     * Notify extensions that the component has changed state or has a property update.
     *
    * @param component ExtensionComponent reference.
    * @param resourceNeeded The component needs a rendering resource.
    */
    void notifyComponentUpdate(const ExtensionComponentPtr& component, bool resourceNeeded);

private:
    std::map<std::string, ExtensionEventHandler> mEventHandlers;  // Qualified name to extension event handler
    std::map<std::string, ExtensionCommandDefinition> mCommandDefinitions;  // Qualified name to extension command definition
    std::map<std::string, ExtensionComponentDefinition> mComponentDefinitions; // Qualified name to extension component definition
    std::map<std::string, ExtensionFilterDefinition> mFilterDefinitions;  // Qualified name to extension filter definition
    std::map<ExtensionEventHandler, Object> mEventHandlerCommandMap;
    std::map<std::string, ExtensionComponentPtr> mExtensionComponents; // ResourceId to extension component
    ObjectMapPtr mEnvironment;
    // mediator processes extension messages
    std::weak_ptr<ExtensionMediator> mMediator;
};

} // namespace apl

#endif // _APL_EXTENSION_MANAGER_H
