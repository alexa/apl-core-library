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

#include "apl/extension/extensionmanager.h"

#include "apl/content/rootconfig.h"
#include "apl/extension/extensionmediator.h"
#include "apl/extension/extensionclient.h"

namespace apl {

static const bool DEBUG_EXTENSION_MANAGER = false;

ExtensionManager::ExtensionManager(const std::vector<ExtensionRequest>& requests,
                                   const RootConfig& rootConfig,
                                   const SessionPtr& session) {
    auto uriToNamespace = std::multimap<std::string, std::string>();
    for (const auto& m : requests) {
        uriToNamespace.emplace(m.uri, m.name);
        LOG_IF(DEBUG_EXTENSION_MANAGER).session(session) << "URI to Namespace: " << m.uri << "->" << m.name;
    }

    // The following map contains keys representing the extension URIs and
    // values representing the environment that those extensions registered
    ObjectMap supported = rootConfig.getSupportedExtensions();

    auto clients = rootConfig.getLegacyExtensionClients();

#ifdef ALEXAEXTENSIONS
    auto mediator = rootConfig.getExtensionMediator();
    if (mediator) {
        const auto& mediatorClients = mediator->getClients();
        clients.insert(mediatorClients.begin(), mediatorClients.end());
    }

    // Save weak mediator for later
    mMediator = mediator;
#endif

    for (auto& client : clients) {
        if (!client.second->registered()) continue;

        const auto& schema = client.second->extensionSchema();

        // Mark extension as supported and save environment
        supported[client.first] = schema.environment;

        // There may be multiple namespaces for the same extension, so register each of them.
        auto range = uriToNamespace.equal_range(client.first);
        for (auto it = range.first; it != range.second; ++it) {
            for (const auto& m : schema.commandDefinitions) {
                auto qualifiedName = it->second + ":" + m.getName();
                mCommandDefinitions.emplace(qualifiedName, m);
                LOG_IF(DEBUG_EXTENSION_MANAGER).session(session)
                    << "extension commands: " << qualifiedName << "->" + m.toDebugString();
            }

            for (const auto& m : schema.filterDefinitions) {
                auto qualifiedName = it->second + ":" + m.getName();
                mFilterDefinitions.emplace(qualifiedName, m);
                LOG_IF(DEBUG_EXTENSION_MANAGER).session(session)
                    << "extension filters: " << qualifiedName << "->" + m.toDebugString();
            }

            for (const auto& m : schema.eventHandlers) {
                auto qualifiedName = it->second + ":" + m.getName();
                mEventHandlers.emplace(it->second + ":" + m.getName(), m);
                LOG_IF(DEBUG_EXTENSION_MANAGER).session(session)
                    << "qualified handlers: " << it->second + ":" + m.getName() << "->"
                    << m.toDebugString();
            }

            for (const auto& m : schema.componentDefinitions) {
                auto qualifiedName = it->second + ":" + m.getName();
                mComponentDefinitions.emplace(qualifiedName, m);
                LOG_IF(DEBUG_EXTENSION_MANAGER).session(session)
                    << "extension component: " << qualifiedName << "->" + m.toDebugString();
            }
        }
    }

    // Extensions that define custom commands
    for (const auto& m : rootConfig.getExtensionCommands()) {
        // There may be multiple namespaces for the same extension, so register each of them.
        auto range = uriToNamespace.equal_range(m.getURI());
        for (auto it = range.first; it != range.second; ++it) {
            auto qualifiedName = it->second + ":" + m.getName();
            mCommandDefinitions.emplace(qualifiedName, m);
            LOG_IF(DEBUG_EXTENSION_MANAGER).session(session) << "extension commands: " << qualifiedName
                << "->" + m.toDebugString();
        }
    }

    // Extensions that define custom filters
    for (const auto& m : rootConfig.getExtensionFilters()) {
        // There may be multiple namespaces for the same extension, so register each of them
        auto range = uriToNamespace.equal_range(m.getURI());
        for (auto it = range.first ; it != range.second ; ++it) {
            auto qualifiedName = it->second + ":" + m.getName();
            mFilterDefinitions.emplace(qualifiedName, m);
            LOG_IF(DEBUG_EXTENSION_MANAGER).session(session) << "extension filters: " << qualifiedName
                << "->" + m.toDebugString();
        }
    }

    // Create a mapping of all possible handlers to a suitable URI/Name
    for (const auto& m : rootConfig.getExtensionEventHandlers()) {
        auto range = uriToNamespace.equal_range(m.getURI());
        for (auto it = range.first; it != range.second; ++it) {
            mEventHandlers.emplace(it->second + ":" + m.getName(), m);
            LOG_IF(DEBUG_EXTENSION_MANAGER).session(session) << "qualified handlers: "
                << it->second + ":" + m.getName() << "->" << m.toDebugString();
        }
    }
 
    mEnvironment = std::make_shared<ObjectMap>();

    for (const auto& m : requests) {
        auto it = supported.find(m.uri);
        if (it != supported.end()) {
            mEnvironment->emplace(m.name, it->second);// Add the NAME.  The URI should already be there.
            LOG_IF(DEBUG_EXTENSION_MANAGER).session(session) << "requestedEnvironment: " << m.name
                 << "->" << it->second.toDebugString();
        } else {
            mEnvironment->emplace(m.name, Object::FALSE_OBJECT());
            LOG_IF(DEBUG_EXTENSION_MANAGER).session(session) << "requestedEnvironment: " << m.name
                << "->" << false;
        }
    }

    // Extension defined component types
    for (const auto& m : rootConfig.getExtensionComponentDefinitions()) {
        // There may be multiple namespaces for the same extension, register each of them
        auto range = uriToNamespace.equal_range(m.getURI());
        for (auto it = range.first; it != range.second; ++it) {
            auto qualifiedName = it->second + ":" + m.getName();
            mComponentDefinitions.emplace(qualifiedName, m);
            LOG_IF(DEBUG_EXTENSION_MANAGER).session(session) << "extension component: " << qualifiedName
                << "->" + m.toDebugString();
        }
    }
}

void
ExtensionManager::addEventHandler(const ExtensionEventHandler& handler, Object command) {
    mEventHandlerCommandMap[handler] = std::move(command);
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
    auto it = mCommandDefinitions.find(qualifiedName);
    if (it != mCommandDefinitions.end())
        return &it->second;

    return nullptr;
}

ExtensionComponentDefinition*
ExtensionManager::findComponentDefinition(const std::string& qualifiedName) {
    // If the custom handler doesn't exist
    auto it = mComponentDefinitions.find(qualifiedName);
    if (it != mComponentDefinitions.end())
        return &it->second;

    return nullptr;
}

ExtensionFilterDefinition*
ExtensionManager::findFilterDefinition(const std::string& qualifiedName) {
    auto it = mFilterDefinitions.find(qualifiedName);
    if (it != mFilterDefinitions.end())
        return &it->second;

    return nullptr;
}

Object
ExtensionManager::findHandler(const ExtensionEventHandler& handler) {

    auto it = mEventHandlerCommandMap.find(handler);
    if (it != mEventHandlerCommandMap.end())
        return it->second;

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
        MakeComponentFunc func = [this, componentDef](const ContextPtr& context, Properties&& properties, const Path& path){
          auto component = ExtensionComponent::create(*componentDef, context, std::move(properties), path);
          auto extComponent = ExtensionComponent::cast(component);
          this->addExtensionComponent(extComponent->getResourceID(), extComponent);
          return component;
        };
        return func;
    }
    return nullptr;
}

void
ExtensionManager::notifyComponentUpdate(const ExtensionComponentPtr& component, bool resourceNeeded) {
#ifdef ALEXAEXTENSIONS
    if (auto mediator = mMediator.lock()) {
        mediator->notifyComponentUpdate(component, resourceNeeded);
    } else {
        LOG(LogLevel::kWarn) << "Extension Component updated but not supported.";
    }
#endif
}

} // namespace apl
