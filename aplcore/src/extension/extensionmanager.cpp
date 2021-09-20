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

#include "apl/content/rootconfig.h"
#include "apl/extension/extensionmanager.h"
#include "apl/extension/extensionmediator.h"

namespace apl {

static const bool DEBUG_EXTENSION_MANAGER = false;

ExtensionManager::ExtensionManager(const std::vector<std::pair<std::string, std::string>>& requests,
                                   const RootConfig& rootConfig) {

#ifdef ALEXAEXTENSIONS
    mMediator = rootConfig.getExtensionMediator();
#endif
    
    auto uriToNamespace = std::multimap<std::string, std::string>();
    for (const auto& m : requests) {
        uriToNamespace.emplace(m.second, m.first);
        LOG_IF(DEBUG_EXTENSION_MANAGER) << "URI to Namespace: " << m.second << "->" << m.first;
    }

    // Extensions that define custom commands
    for (const auto& m : rootConfig.getExtensionCommands()) {
        // There may be multiple namespaces for the same extension, so register each of them.
        auto range = uriToNamespace.equal_range(m.getURI());
        for (auto it = range.first; it != range.second; ++it) {
            auto qualifiedName = it->second + ":" + m.getName();
            mExtensionCommands.emplace(qualifiedName, m);
            LOG_IF(DEBUG_EXTENSION_MANAGER) << "extension commands: " << qualifiedName << "->" + m.toDebugString();
        }
    }

    // Extensions that define custom filters
    for (const auto& m : rootConfig.getExtensionFilters()) {
        // There may be multiple namespaces for the same extension, so register each of them
        auto range = uriToNamespace.equal_range(m.getURI());
        for (auto it = range.first ; it != range.second ; ++it) {
            auto qualifiedName = it->second + ":" + m.getName();
            mExtensionFilters.emplace(qualifiedName, m);
            LOG_IF(DEBUG_EXTENSION_MANAGER) << "extension filters: " << qualifiedName << "->" + m.toDebugString();
        }
    }

    // Create a mapping of all possible handlers to a suitable URI/Name
    for (const auto& m : rootConfig.getExtensionEventHandlers()) {
        auto range = uriToNamespace.equal_range(m.getURI());
        for (auto it = range.first; it != range.second; ++it) {
            mQualifiedEventHandlerMap.emplace(it->second + ":" + m.getName(), m);
            LOG_IF(DEBUG_EXTENSION_MANAGER) << "qualified handlers: " << it->second + ":" + m.getName() << "->" << m.toDebugString();
        }
    }

    // Construct the data-binding environmental information for indicating which extensions are installed
    const auto& supported = rootConfig.getSupportedExtensions();
    mEnvironment = std::make_shared<ObjectMap>();

    for (const auto& m : requests) {
        auto it = supported.find(m.second);
        if (it != supported.end()) {
            auto cfg = Object(rootConfig.getExtensionEnvironment(m.second));
            mEnvironment->emplace(m.first, cfg);// Add the NAME.  The URI should already be there.
            LOG_IF(DEBUG_EXTENSION_MANAGER) << "requestedEnvironment: " << m.first << "->" << cfg.toDebugString();
        } else {
            mEnvironment->emplace(m.first, Object::FALSE_OBJECT());
            LOG_IF(DEBUG_EXTENSION_MANAGER) << "requestedEnvironment: " << m.first << "->" << false;
        }
    }

    // Extension defined component types
    for (const auto& m : rootConfig.getExtensionComponentDefinitions()) {
        // There may be multiple namespaces for the same extension, register each of them
        auto range = uriToNamespace.equal_range(m.getURI());
        for (auto it = range.first; it != range.second; ++it) {
            auto qualifiedName = it->second + ":" + m.getName();
            mExtensionComponentDefs.emplace(qualifiedName, m);
            LOG_IF(DEBUG_EXTENSION_MANAGER) << "extension component: " << qualifiedName << "->" + m.toDebugString();
        }
        for (const auto& c : m.getCommands()) {
            auto commandUriName = m.getURI() + ":" + c.getName();
            mComponentCommands.insert(commandUriName);
        }
    }
}

void
ExtensionManager::addEventHandler(const ExtensionEventHandler& handler, Object command) {
    mExtensionEventHandlers[handler] = std::move(command);
}

void
ExtensionManager::addExtensionComponent(const std::string& resourceId, const ExtensionComponentPtr& component) {
    mExtensionComponents.emplace(resourceId, component);
}

void ExtensionManager::removeExtensionComponent(const std::string& resourceId) {
    mExtensionComponents.erase(resourceId);
}

ExtensionCommandDefinition*
ExtensionManager::findCommandDefinition(const std::string& qualifiedName) {
    // If the custom handler doesn't exist
    auto it = mExtensionCommands.find(qualifiedName);
    if (it != mExtensionCommands.end())
        return &it->second;

    return nullptr;
}

bool
ExtensionManager::isComponentCommand(const std::string& qualifiedName) const {
    return mComponentCommands.find(qualifiedName) != mComponentCommands.end();
}

ExtensionComponentDefinition*
ExtensionManager::findComponentDefinition(const std::string& qualifiedName) {
    // If the custom handler doesn't exist
    auto it = mExtensionComponentDefs.find(qualifiedName);
    if (it != mExtensionComponentDefs.end())
        return &it->second;

    return nullptr;
}

ExtensionFilterDefinition*
ExtensionManager::findFilterDefinition(const std::string& qualifiedName) {
    auto it = mExtensionFilters.find(qualifiedName);
    if (it != mExtensionFilters.end())
        return &it->second;

    return nullptr;
}

Object
ExtensionManager::findHandler(const ExtensionEventHandler& handler, std::string resourceId) {
    if (!resourceId.empty()) {
        auto it = mExtensionComponents.find(resourceId);
        if (it != mExtensionComponents.end()) {
            return it->second->findHandler(handler);
        }
    } else {
        auto it = mExtensionEventHandlers.find(handler);
        if (it != mExtensionEventHandlers.end())
            return it->second;
    }
    return Object::NULL_OBJECT();
}

ExtensionComponentPtr
ExtensionManager::findExtensionComponent(const std::string& resourceId) {
    auto it = mExtensionComponents.find(resourceId);
    if (it != mExtensionComponents.end()) {
        return it->second;
    }
    return nullptr;
}

MakeComponentFunc
ExtensionManager::findAndCreateExtensionComponent(const std::string& componentType) {
    auto componentDef = findComponentDefinition(componentType);
    if (componentDef != nullptr) {
        MakeComponentFunc func = [componentDef](const ContextPtr& context, Properties&& properties, const Path& path){
          return ExtensionComponent::create(*componentDef, context, std::move(properties), path);
        };
        return func;
    }
    return nullptr;
}

void
ExtensionManager::notifyComponentUpdate(const ExtensionComponentPtr& component) {
#ifdef ALEXAEXTENSIONS
    if (auto mediator = mMediator.lock())
        mediator->notifyComponentUpdate(*component);
    else
        LOG(LogLevel::kWarn) << "Extension Component updated but not supported.";
#endif
}

} // namespace apl
