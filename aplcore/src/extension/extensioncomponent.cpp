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

#include "apl/extension/extensioncomponent.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/yogaproperties.h"
#include "apl/extension/extensionmanager.h"
#include "apl/time/sequencer.h"
#include "apl/utils/random.h"

namespace apl {

static std::map<PropertyKey, std::string> sPropertyHandlers = {
    {kPropertyResourceOnFatalError, "FatalError"}
};

CoreComponentPtr
ExtensionComponent::create(const ExtensionComponentDefinition& definition,
                           const ContextPtr& context, Properties&& properties,
                           const Path& path)
{
    auto component = std::make_shared<ExtensionComponent>(definition, context, std::move(properties), path);
    component->initialize();
    context->extensionManager().addExtensionComponent(component->getResourceID(), component);

    return component;
}

ExtensionComponent::ExtensionComponent(const ExtensionComponentDefinition& definition,
                                       const ContextPtr& context, Properties&& properties,
                                       const Path& path)
    : CoreComponent(context, std::move(properties), path),
      mDefinition(definition),
      mPropDefSet(CoreComponent::propDefSet())
{
    std::vector<ComponentPropDef> customPropDefs;
    for (const auto& entry: *mDefinition.getExtensionComponentProperties()) {
        customPropDefs.emplace_back(static_cast<PropertyKey>(entry.first),
                                    Object(entry.second.defvalue),
                                    sBindingFunctions.at(entry.second.btype),
                                    kPropInOut|kPropDynamic);
    }

    // Add extension event handlers into propdefset.
    for (const auto& entry: mDefinition.getEventHandlers()) {
        customPropDefs.emplace_back(static_cast<PropertyKey>(entry.first),
                                    Object::NULL_OBJECT(),
                                    asCommand,
                                    kPropIn);
    }

    mPropDefSet.add({
        {kPropertyResourceId,           "",                    asString,  kPropOut},
        {kPropertyResourceOnFatalError, Object::EMPTY_ARRAY(), asCommand, kPropIn},
        {kPropertyResourceState,        kResourcePending, asInteger, kPropRuntimeState},
    }).add(customPropDefs);
}

std::string
ExtensionComponent::name() const
{
    return mDefinition.getName();
}

const EventPropertyMap &
ExtensionComponent::eventPropertyMap() const
{
    static auto getType = [](const CoreComponent *c) {
        auto extensionComp = dynamic_cast<const ExtensionComponent*>(c);
        if (extensionComp != nullptr) {
            return extensionComp->mDefinition.getName();
        }
        return std::string();
    };

    static auto getResourceId = [](const CoreComponent *c) {
      auto extensionComp = dynamic_cast<const ExtensionComponent*>(c);
      if (extensionComp != nullptr) {
          return extensionComp->getResourceID();
      }
      return std::string();
    };

    static EventPropertyMap sExtensionComponentEventProperties = EventPropertyMap({
            {"type", getType },
            {"resourceId", getResourceId }
    });
    return sExtensionComponentEventProperties;
}

void
ExtensionComponent::release()
{
    CoreComponent::release();
    getContext()->extensionManager().removeExtensionComponent(getResourceID());
}

void
ExtensionComponent::initialize()
{
    CoreComponent::initialize();
    mResourceID = Random::generateToken(getUri());
    setCalculated(kPropertyResourceId, mResourceID);
}

std::string
ExtensionComponent::getVisualContextType() const
{
    std::string visualContextType = mDefinition.getVisualContextType();
    return visualContextType.empty() ? VISUAL_CONTEXT_TYPE_EMPTY : visualContextType;
}

Object
ExtensionComponent::findHandler(const ExtensionEventHandler& handler)
{
    return getCalculated(
        static_cast<PropertyKey>(sComponentPropertyBimap.get(handler.getName(), -1)));
}

void
ExtensionComponent::onFatalError(int errorCode, const std::string& message)
{
    // Trigger onFatalError event
    auto& commands = getCalculated(kPropertyResourceOnFatalError);
    auto properties = std::make_shared<ObjectMap>();

    properties->emplace("errorCode", errorCode);
    properties->emplace("error", message);

    // onFatalError is always executed in fast mode.
    mContext->sequencer().executeCommands(
        commands,
        createEventContext(sPropertyHandlers.at(kPropertyResourceOnFatalError), properties),
        shared_from_corecomponent(),
        true);
}

void
ExtensionComponent::extensionComponentFail(int errorCode, const std::string& message) {
    setCalculated(kPropertyResourceState, kResourceError);
    setDirty(kPropertyResourceState);
    onFatalError(errorCode, message);
}

void
ExtensionComponent::updateResourceState(const ExtensionComponentResourceState& state)
{
    setCalculated(kPropertyResourceState, state);
    // TODO allow for viewhost error/errorCode update
    if (state == kResourceError)
        onFatalError(1, "Resource not available");
    notifyExtension();
}

void
ExtensionComponent::handlePropertyChange(const ComponentPropDef& def, const Object& value)
{
    // default behavior
    CoreComponent::handlePropertyChange(def, value);
    notifyExtension();
}

void
ExtensionComponent::notifyExtension() {
    // Notify the extension the component has changed
    auto extManager = getContext()->extensionManager();
    extManager.notifyComponentUpdate(std::static_pointer_cast<ExtensionComponent>(shared_from_this()));
}


} // namespace apl