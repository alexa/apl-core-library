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
 *
 * Construct a virtual DOM hierarchy
 */


#include <stdexcept>
#include <cmath>

#include "apl/component/containercomponent.h"
#include "apl/component/edittextcomponent.h"
#include "apl/component/framecomponent.h"
#include "apl/component/gridsequencecomponent.h"
#include "apl/component/imagecomponent.h"
#include "apl/component/pagercomponent.h"
#include "apl/component/scrollviewcomponent.h"
#include "apl/component/sequencecomponent.h"
#include "apl/component/textcomponent.h"
#include "apl/component/touchwrappercomponent.h"
#include "apl/component/vectorgraphiccomponent.h"
#include "apl/component/videocomponent.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/arrayify.h"
#include "apl/engine/binding.h"
#include "apl/engine/builder.h"
#include "apl/engine/context.h"
#include "apl/engine/contextdependant.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/parameterarray.h"
#include "apl/livedata/livearray.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/engine/properties.h"
#include "apl/extension/extensioncomponent.h"
#include "apl/extension/extensionmanager.h"
#include "apl/livedata/layoutrebuilder.h"
#include "apl/primitives/object.h"
#include "apl/utils/log.h"
#include "apl/utils/path.h"
#include "apl/utils/session.h"
#include "apl/utils/tracing.h"

namespace apl {

const bool DEBUG_BUILDER = false;

static std::map<std::string, MakeComponentFunc> sComponentMap = {
    {"Container",     ContainerComponent::create},
    {"Text",          TextComponent::create},
    {"Image",         ImageComponent::create},
    {"ScrollView",    ScrollViewComponent::create},
    {"EditText",      EditTextComponent::create},
    {"Frame",         FrameComponent::create},
    {"Sequence",      SequenceComponent::create},
    {"GridSequence",  GridSequenceComponent::create},
    {"TouchWrapper",  TouchWrapperComponent::create},
    {"Pager",         PagerComponent::create},
    {"VectorGraphic", VectorGraphicComponent::create},
    {"Video",         VideoComponent::create}
};

void
Builder::populateSingleChildLayout(const ContextPtr& context,
                                   const Object& item,
                                   const CoreComponentPtr& layout,
                                   const Path& path,
                                   bool fullBuild,
                                   bool useDirtyFlag)
{
    LOG_IF(DEBUG_BUILDER) << "call";
    APL_TRACE_BLOCK("Builder:populateSingleChildLayout");
    auto child = expandSingleComponentFromArray(context,
                                                arrayifyProperty(*context, item, "item", "items"),
                                                Properties(),
                                                layout,
                                                path.addProperty(item, "item", "items"),
                                                fullBuild,
                                                useDirtyFlag);
    layout->appendChild(child, useDirtyFlag);
}

void
Builder::populateLayoutComponent(const ContextPtr& context,
                                 const Object& item,
                                 const CoreComponentPtr& layout,
                                 const Path& path,
                                 bool fullBuild,
                                 bool useDirtyFlag)
{
    LOG_IF(DEBUG_BUILDER) << path;
    APL_TRACE_BLOCK("Builder:populateLayoutComponent");

    auto child = expandSingleComponentFromArray(context,
                                                arrayifyProperty(*context, item, "firstItem"),
                                                Properties(),
                                                layout,
                                                path.addProperty(item, "firstItem"),
                                                fullBuild,
                                                useDirtyFlag);
    bool hasFirstItem = false;
    if (child && child->isValid()) {
        hasFirstItem = true;
        layout->appendChild(child, useDirtyFlag);
    }

    bool numbered = layout->getCalculated(kPropertyNumbered).asBoolean();
    int ordinal = 1;
    int index = 0;

    std::shared_ptr<LayoutRebuilder> layoutBuilder = nullptr;  // Reserve space for now.  In the future, move all logic in

    const auto items = arrayifyProperty(*context, item, "item", "items");
    if (!items.empty()) {
        auto childPath = path.addProperty(item, "item", "items");
        auto data = arrayifyPropertyAsObject(*context, item, "data");

        auto liveData = data.getLiveDataObject();
        if (liveData && liveData->asArray()) {
            layoutBuilder = LayoutRebuilder::create(context, layout, mOld, liveData->asArray(), items, childPath, numbered);
            layoutBuilder->build(useDirtyFlag);
        }
        else {
            auto dataItems = evaluateRecursive(*context, data);
            if (!dataItems.empty()) {
                LOG_IF(DEBUG_BUILDER) << "data size=" << dataItems.size();

                // Transform data into LiveData and use rebuilder to have more control over its content.
                auto rawArray = ObjectArray();
                for (const auto& dataItem : dataItems.getArray()) {
                    rawArray.emplace_back(dataItem);
                }
                liveData = LiveDataObject::create(
                        LiveArray::create(std::move(rawArray)),
                        context,
                        "__data" + layout->getUniqueId());

                layoutBuilder = LayoutRebuilder::create(context, layout, mOld, liveData->asArray(), items, childPath, numbered);
                layoutBuilder->build(useDirtyFlag);
            }
            else {
                LOG_IF(DEBUG_BUILDER) << "items size=" << items.size();
                auto length = items.size();
                for (int i = 0; i < length; i++) {
                    const auto& element = items.at(i);
                    auto childContext = Context::createFromParent(context);
                    childContext->putConstant("index", index);
                    childContext->putConstant("length", length);
                    if (numbered)
                        childContext->putConstant("ordinal", ordinal);

                    // TODO: Numbered, spacing, ordinal changes
                    child = expandSingleComponentFromArray(childContext,
                                                           arrayify(*context, element),
                                                           Properties(),
                                                           layout,
                                                           childPath.addIndex(i),
                                                           fullBuild,
                                                           useDirtyFlag);
                    // TODO: Full or not full here?
                    if (child && child->isValid()) {
                        layout->appendChild(child, useDirtyFlag);
                        index++;

                        if (numbered) {
                            int numbering = child->getCalculated(kPropertyNumbering).getInteger();
                            if (numbering == kNumberingNormal) ordinal++;
                            else if (numbering == kNumberingReset) ordinal = 1;
                        }
                    }
                }
            }
        }
    }

    child = expandSingleComponentFromArray(context,
                                           arrayifyProperty(*context, item, "lastItem"),
                                           Properties(),
                                           layout,
                                           path.addProperty(item, "lastItem"),
                                           fullBuild,
                                           useDirtyFlag);

    bool hasLastItem = false;
    if (child && child->isValid()) {
        hasLastItem = true;
        layout->appendChild(child, useDirtyFlag);
    }

    // Chance to get final child dependent set-up before actual layout happened.
    layout->finalizePopulate();

    if (layoutBuilder)
        layoutBuilder->setFirstLast(hasFirstItem, hasLastItem);
}

MakeComponentFunc
Builder::findComponentBuilderFunc(const ContextPtr& context, const std::string &type) {
    auto method = sComponentMap.find(type);
    if (method != sComponentMap.end()) {
        return method->second;
    }

    return context->extensionManager().findAndCreateExtensionComponent(type);
}

/**
 * Expand a single component or layout by type.
 *
 * @param context Current data-binding context
 * @param item The JSON-definition of the item to expand.
 * @param properties The user-specified properties for this component
 * @param parent The parent component of this component
 * @param path The path of this component
 * @param fullBuild Build full tree.
 * @param useDirtyFlag true to notify runtime about changes with dirty properties
 */
CoreComponentPtr
Builder::expandSingleComponent(const ContextPtr& context,
                               const Object& item,
                               Properties&& properties,
                               const CoreComponentPtr& parent,
                               const Path& path,
                               bool fullBuild,
                               bool useDirtyFlag)
{
    LOG_IF(DEBUG_BUILDER) << path.toString();
    APL_TRACE_BLOCK("Builder:expandSingleComponent");

    std::string type = propertyAsString(*context, item, "type");
    if (type.empty()) {
        CONSOLE_CTP(context) << "Invalid type in component";
        return nullptr;
    }

    if (auto method = findComponentBuilderFunc(context, type)) {
        LOG_IF(DEBUG_BUILDER) << "Expanding primitive " << type;

        // Copy the items into the properties map.
        properties.emplace(item);

        // Create a new context and fill out the binding
        ContextPtr expanded = Context::createFromParent(context);
        attachBindings(expanded, item);

        // Construct the component
        CoreComponentPtr component = CoreComponentPtr(method(expanded, std::move(properties), path));
        if (!component || !component->isValid()) {
            CONSOLE_CTP(context) << "Unable to inflate component";
            if (component)
                component->release();
            return nullptr;
        }

        CoreComponentPtr oldComponent;
        if(mOld) {
            oldComponent = std::dynamic_pointer_cast<CoreComponent>(
                mOld->findComponentById(component->getId()));
            copyPreservedBindings(component, oldComponent);
        }

        if (fullBuild) {
            if (component->singleChild()) {
                populateSingleChildLayout(expanded, item, component, path, true, useDirtyFlag);
            } else if (component->multiChild()) {
                populateLayoutComponent(expanded, item, component, path, true, useDirtyFlag);
            }
        } else {
            expanded->putConstant("_item", item);
        }

        if(oldComponent) {
            // Copy preserved properties other than local bindings
            copyPreservedProperties(component, oldComponent);
        }

        LOG_IF(DEBUG_BUILDER) << "Returning component " << *component;
        return component;
    }

    LOG_IF(DEBUG_BUILDER) << "Expanding layout '" << type << "'";
    auto resource = context->getLayout(type);
    if (!resource.empty()) {
        properties.emplace(item);
        return expandLayout(context, properties, resource.json(), parent, resource.path(), fullBuild, useDirtyFlag);
    }

    CONSOLE_CTP(context) << "Unable to find layout or component '" << type << "'";
    return nullptr;
}

/**
 * Process data bindings
 * @param context
 * @param item
 */
void
Builder::attachBindings(const apl::ContextPtr& context, const apl::Object& item)
{
    APL_TRACE_BLOCK("Builder:attachBindings");
    auto bindings = arrayifyProperty(*context, item, "bind");
    for (const auto& binding : bindings) {
        auto name = propertyAsString(*context, binding, "name");
        if (name.empty() || !binding.has("value"))
            continue;

        if (context->hasLocal(name)) {
            CONSOLE_CTP(context) << "Attempted to bind to pre-existing property '" << name << "'";
            continue;
        }

        // Extract the binding as an optional node tree.
        auto tmp = propertyAsNode(*context, binding, "value");
        auto value = evaluateRecursive(*context, tmp);
        auto bindingType = propertyAsMapped<BindingType>(*context, binding, "type", kBindingTypeAny, sBindingMap);
        auto bindingFunc = sBindingFunctions.at(bindingType);

        // Store the value in the new context.  Binding values are mutable; they can be changed later.
        context->putUserWriteable(name, bindingFunc(*context, value));

        // If it is a node, we connect up the symbols that it is dependant upon
        if (tmp.isEvaluable())
            ContextDependant::create(context, name, tmp, context, bindingFunc);
    }
}

/**
 * Expand a single component from a "when" list of possible components
 *
 * @param context Current data-binding context
 * @param items A vector of the components to expand
 * @param properties The user-specified properties for this component
 * @param parent The parent component of this component
 * @param path The path description of this component
 * @param fullBuild Build full tree.
 * @param useDirtyFlag true to notify runtime about changes with dirty properties
 */
CoreComponentPtr
Builder::expandSingleComponentFromArray(const ContextPtr& context,
                                        const std::vector<Object>& items,
                                        Properties&& properties,
                                        const CoreComponentPtr& parent,
                                        const Path& path,
                                        bool fullBuild,
                                        bool useDirtyFlag)
{
    LOG_IF(DEBUG_BUILDER) << path;
    for (int index = 0; index < items.size(); index++) {
        const auto& item = items.at(index);
        if (!item.isMap())
            continue;

        if (propertyAsBoolean(*context, item, "when", true)) {
            return expandSingleComponent(context, item, std::move(properties), parent, path.addIndex(index), fullBuild, useDirtyFlag);
        }
    }

    return nullptr;
}

/**
 * Expand a layout defined either as the "mainTemplate" or a named layout
 * from the "layouts" section of a package.
 *
 * @param context Current data-binding context.
 * @param properties The user-specified properties for this layout.
 * @param layout The JSON definition of the layout object.
 * @param parent The parent component of this layout.
 * @param fullBuild Build full tree.
 * @param useDirtyFlag true to notify runtime about changes with dirty properties
 */
CoreComponentPtr
Builder::expandLayout(const ContextPtr& context,
                      Properties& properties,
                      const rapidjson::Value& layout,
                      const CoreComponentPtr& parent,
                      const Path& path,
                      bool fullBuild,
                      bool useDirtyFlag)
{
    LOG_IF(DEBUG_BUILDER) << path;
    if (!layout.IsObject()) {
        std::string errorMessage = "Layout inflation for one of the components failed. Path: " + path.toString();
        CONSOLE_CTP(context) << errorMessage;
        return nullptr;
    }
    APL_TRACE_BLOCK("Builder:expandLayout");

    // Build a new context for this layout.
    ContextPtr cptr = Context::createFromParent(context);

    // Add each parameter to the context.  It's either going to come from
    // a property or its default value.  This will remove the matching property from
    // the property map.
    ParameterArray params(layout);
    for (const auto& param : params) {
        LOG_IF(DEBUG_BUILDER) << "Parsing parameter: " << param.name;
        properties.addToContext(cptr, param, true);
    }

    Builder::attachBindings(cptr, layout);

    if (DEBUG_BUILDER) {
        for (std::shared_ptr<const Context> p = cptr; p; p = p->parent()) {
            for (const auto& m : *p)
                LOG(LogLevel::kDebug) << m.first << ": " << m.second;
        }
    }
    return expandSingleComponentFromArray(cptr,
                                          arrayifyProperty(*cptr, layout, "item", "items"),
                                          std::move(properties),
                                          parent,
                                          path.addProperty(layout, "item", "items"),
                                          fullBuild,
                                          useDirtyFlag);
}

/**
 * Copy preserved properties and local bindings from one component to another
 *
 * @param newComponent The component to copy properties to
 * @param originalComponent The component to copy properties from
 * @param copyLocalBindings if true, copy local bindings, true by default
 * @param copyComponentProperties if true, copy local bindings, true by default
 */
static inline void copyPreserved(const CoreComponentPtr& newComponent, const CoreComponentPtr& originalComponent,
                                 bool copyLocalBindings, bool copyComponentProperties)
{
    if(!(copyLocalBindings || copyComponentProperties))
        return;

    if (!originalComponent || !newComponent)
        return;

    const auto& preserved = newComponent->getCalculated(kPropertyPreserve).getArray();
    if (preserved.empty())
        return;

    // We have an original component and we have at least one property that should be restored
    auto originalContext = originalComponent->getContext();
    for (const auto& m : preserved) {
        auto property = m.asString();
        bool isPropLocal = originalContext->hasLocal(property);
        if(!isPropLocal && copyComponentProperties) {
            newComponent->setProperty(property, originalComponent->getProperty(property));
        } else if(copyLocalBindings && isPropLocal) {
            auto oldRef = originalContext->find(property);
            if(!oldRef.empty())
                newComponent->setProperty(property, oldRef.object().value());
        }
    }
}

/**
 * Copy preserved local bindings from one component to other.
 *
 * @param newComponent The component to copy bindings to.
 * @param originalComponent The component to copy bindings from.
 */
void
Builder::copyPreservedBindings(const CoreComponentPtr& newComponent, const CoreComponentPtr& originalComponent)
{
    APL_TRACE_BLOCK("Builder:copyPreservedBindings");
    LOG_IF(DEBUG_BUILDER) << newComponent << " old=" << mOld;
    copyPreserved(newComponent, originalComponent, true, false);
}

/**
 * Copy preserved component properties from one component to other.
 *
 * @param newComponent The component to copy bindings to.
 * @param originalComponent The component to copy bindings from.
 */
void
Builder::copyPreservedProperties(const CoreComponentPtr& newComponent, const CoreComponentPtr& originalComponent)
{
    APL_TRACE_BLOCK("Builder:copyPreservedProperties");
    LOG_IF(DEBUG_BUILDER) << newComponent << " old=" << mOld;
    copyPreserved(newComponent, originalComponent, false, true);
}

CoreComponentPtr
Builder::inflate(const ContextPtr& context,
                 Properties& mainProperties,
                 const rapidjson::Value& mainDocument)
{
    APL_TRACE_BLOCK("Builder:inflate");
    return expandLayout(context, mainProperties, mainDocument, nullptr,
        Path(context->getRootConfig().getTrackProvenance() ? std::string(Path::MAIN) + "/mainTemplate" : ""), true, false);
}

CoreComponentPtr
Builder::inflate(const ContextPtr& context,
                 const Object& component)
{
    assert(component.isMap() || component.isArray());
    if (component.isMap())
        return expandSingleComponent(context, component, Properties(), nullptr,
            Path(context->getRootConfig().getTrackProvenance() ? "_virtual" : ""), true, false);
    else if (component.isArray())
        return expandSingleComponentFromArray(context, component.getArray(), Properties(), nullptr,
            Path(context->getRootConfig().getTrackProvenance() ? "_virtual" : ""), true, false);
    else
        return nullptr;
}

} // namespace apl
