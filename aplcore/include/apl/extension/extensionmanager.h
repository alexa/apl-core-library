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
#include "apl/content/extensionfilterdefinition.h"
#include "apl/content/extensioneventhandler.h"
#include "apl/primitives/object.h"

namespace apl {

/**
 * The extension manager maintains the list of custom events and custom commands
 * registered by extensions and appearing in the document.
 */
class ExtensionManager {
public:
    ExtensionManager(const std::vector<std::pair<std::string, std::string>>& requests, const RootConfig& rootConfig);

    /**
     * @return A map of qualified name to the extension event handler definition.
     */
    const std::map<std::string, ExtensionEventHandler>& qualifiedHandlerMap() const { return mQualifiedEventHandlerMap; }

    /**
     * Add a document or package-level event handler by name.  These are added as
     * the packages and document are scanned.
     * @param handler The extension event handler
     * @param command The command to execute
     */
    void addEventHandler(const ExtensionEventHandler& handler, Object command);

    /**
     * Search the custom commands for one with the given name.
     * @param qualifiedName The name of the custom command in the form EXT_NAME:CMD_NAME
     * @return The command definition or nullptr if it is not found
     */
    ExtensionCommandDefinition* findCommandDefinition(const std::string& qualifiedName);

    /**
     * Search the custom filters for one with the given name.
     * @param qualifiedName The name of the custom filter in the form EXT_NAME:FILTER_NAME
     * @return The filter definition or nullptr if it is not found
     */
    ExtensionFilterDefinition* findFilterDefinition(const std::string& qualifiedName);

    /**
     * Finds an appropriate custom handler to invoke.  Returns NULL if no such handler exists
     * @param handler The extension event handler to invoke
     * @return The Object attached to this handler or NULL.
     */
    Object findHandler(const ExtensionEventHandler& handler);

    /**
     * @return A mapping of URI or NAME to boolean values suitable for including in the
     *         data-binding context under "environment.extensions.XXXX"
     */
    ObjectMapPtr getEnvironment() const { return mEnvironment; }

private:
    std::map<std::string, ExtensionEventHandler> mQualifiedEventHandlerMap;  // Qualified name to extension event handler
    std::map<std::string, ExtensionCommandDefinition> mExtensionCommands;  // Qualified name to extension command definition
    std::map<std::string, ExtensionFilterDefinition> mExtensionFilters;  // Qualified name to extension filter definition

    std::map<ExtensionEventHandler, Object> mExtensionEventHandlers;
    ObjectMapPtr mEnvironment;
};

} // namespace apl

#endif // _APL_EXTENSION_MANAGER_H
