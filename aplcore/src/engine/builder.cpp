/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <utility>
#include <cmath>

#include "apl/engine/binding.h"
#include "apl/engine/builder.h"
#include "apl/engine/context.h"
#include "apl/engine/contextdependant.h"
#include "apl/engine/arrayify.h"
#include "apl/engine/evaluate.h"
#include "apl/component/corecomponent.h"
#include "apl/component/containercomponent.h"
#include "apl/component/framecomponent.h"
#include "apl/component/imagecomponent.h"
#include "apl/component/pagercomponent.h"
#include "apl/component/scrollviewcomponent.h"
#include "apl/component/sequencecomponent.h"
#include "apl/component/textcomponent.h"
#include "apl/component/touchwrappercomponent.h"
#include "apl/component/vectorgraphiccomponent.h"
#include "apl/component/videocomponent.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/properties.h"
#include "apl/engine/parameterarray.h"
#include "apl/utils/log.h"
#include "apl/utils/path.h"
#include "apl/utils/session.h"
#include "apl/primitives/object.h"

namespace apl {

const bool DEBUG_BUILDER = false;


using MakeComponentFunc = CoreComponentPtr (*)(const ContextPtr&, Properties&&, const std::string&);

static const std::map<std::string, MakeComponentFunc> sComponentMap = {
    {"Container",     ContainerComponent::create},
    {"Text",          TextComponent::create},
    {"Image",         ImageComponent::create},
    {"ScrollView",    ScrollViewComponent::create},
    {"Frame",         FrameComponent::create},
    {"Sequence",      SequenceComponent::create},
    {"TouchWrapper",  TouchWrapperComponent::create},
    {"Pager",         PagerComponent::create},
    {"VectorGraphic", VectorGraphicComponent::create},
    {"Video",         VideoComponent::create}
};

void
Builder::populateSingleChildLayout(const ContextPtr& context,
                                   const Object& item,
                                   const CoreComponentPtr& layout,
                                   const Path& path)
{
    LOG_IF(DEBUG_BUILDER) << "call";

    Properties props;
    auto child = expandSingleComponentFromArray(context,
                                                arrayifyProperty(context, item, "item", "items"),
                                                props,
                                                layout,
                                                path.addProperty(item, "item", "items"));
    layout->appendChild(child, false);
}

void
Builder::populateLayoutComponent(const ContextPtr& context,
                                 const Object& item,
                                 const CoreComponentPtr& layout,
                                 const Path& path)
{
    LOG_IF(DEBUG_BUILDER) << path;

    Properties firstProps;

    auto child = expandSingleComponentFromArray(context,
                                                arrayifyProperty(context, item, "firstItem"),
                                                firstProps,
                                                layout,
                                                path.addProperty(item, "firstItem"));
    if (child && child->isValid())
        layout->appendChild(child, false);

    bool numbered = layout->getCalculated(kPropertyNumbered).asBoolean();
    int ordinal = 1;
    int index = 0;

    const auto items = arrayifyProperty(context, item, "item", "items");
    if (!items.empty()) {
        auto childPath = path.addProperty(item, "item", "items");
        auto dataItems = evaluateRecursive(context, arrayifyProperty(context, item, "data"));

        if (!dataItems.empty()) {
            LOG_IF(DEBUG_BUILDER) << "data size=" << dataItems.size();
            auto length = dataItems.size();
            for (size_t dataIndex = 0 ; dataIndex < length ; dataIndex++) {
                const auto& data = dataItems.at(dataIndex);
                auto childContext = Context::create(context);
                childContext->putConstant("data", data);
                childContext->putConstant("index", index);
                childContext->putConstant("length", length);
                if (numbered)
                    childContext->putConstant("ordinal", ordinal);

                Properties childProps;
                child = expandSingleComponentFromArray(childContext,
                                                       items,
                                                       childProps,
                                                       layout, childPath);
                if (child && child->isValid()) {
                    layout->appendChild(child, false);
                    index++;

                    if (numbered) {
                        int numbering = child->getCalculated(kPropertyNumbering).getInteger();
                        if (numbering == kNumberingNormal) ordinal++;
                        else if (numbering == kNumberingReset) ordinal = 1;
                    }
                }
            }
        }
            // TODO: A list of children.  Ignore the data object???  Or add to context??
        else {
            LOG_IF(DEBUG_BUILDER) << "items size=" << items.size();
            auto length = items.size();
            for (int i = 0; i < length; i++) {
                const auto& element = items.at(i);
                auto childContext = Context::create(context);
                childContext->putConstant("index", index);
                childContext->putConstant("length", length);
                if (numbered)
                    childContext->putConstant("ordinal", ordinal);

                // TODO: Numbered, spacing, ordinal changes
                Properties childProps;
                child = expandSingleComponentFromArray(childContext,
                                                       arrayify(context, element),
                                                       childProps,
                                                       layout,
                                                       childPath.addIndex(i));
                if (child && child->isValid()) {
                    layout->appendChild(child, false);
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

    Properties lastProps;
    child = expandSingleComponentFromArray(context,
                                           arrayifyProperty(context, item, "lastItem"),
                                           lastProps,
                                           layout,
                                           path.addProperty(item, "lastItem"));

    if (child && child->isValid())
        layout->appendChild(child, false);
}

/**
 * Expand a single component or layout by type.
 *
 * @param context Current data-binding context
 * @param item The JSON-definition of the item to expand.
 * @param properties The user-specified properties for this component
 * @param parent The parent component of this component
 * @param path The path of this component
 */
CoreComponentPtr
Builder::expandSingleComponent(const ContextPtr& context,
                               const Object& item,
                               Properties& properties,
                               const CoreComponentPtr& parent,
                               const Path& path)
{
    LOG_IF(DEBUG_BUILDER) << path.toString();

    std::string type = propertyAsString(*context, item, "type");
    if (type.empty()) {
        CONSOLE_CTP(context) << "Invalid type in component";
        return nullptr;
    }

    auto method = sComponentMap.find(type);
    if (method != sComponentMap.end()) {
        LOG_IF(DEBUG_BUILDER) << "Expanding primitive " << type;

        // Copy the items into the properties map.
        properties.emplace(item);

        // Create a new context and fill out the binding
        ContextPtr expanded = Context::create(context);
        auto bindings = arrayifyProperty(context, item, "bind");
        for (const auto& binding : bindings) {
            auto name = propertyAsString(*expanded, binding, "name");
            if (name.empty() || !binding.has("value"))
                continue;

            // Extract the binding as an optional node tree.
            auto tmp = propertyAsNode(*expanded, binding, "value");
            auto value = evaluate(*expanded, tmp);
            auto bindingType = propertyAsMapped<BindingType>(*expanded, binding, "type", kBindingTypeAny, sBindingMap);
            auto bindingFunc = sBindingFunctions.at(bindingType);

            // Store the value in the new context.  Binding values are mutable; they can be changed later.
            expanded->putUserWriteable(name, bindingFunc(*expanded, value));

            // If it is a node, we connect up the symbols that it is dependant upon
            if (tmp.isNode()) {
                std::set<std::string> symbols;
                tmp.symbols(symbols);
                for (const auto& symbol : symbols) {
                    auto c = expanded->findContextContaining(symbol);
                    if (c != nullptr)
                        ContextDependant::create(c, symbol, expanded, name, expanded, tmp, bindingFunc);
                }
            }
        }

        // Construct the component
        CoreComponentPtr component = CoreComponentPtr(method->second(expanded, std::move(properties), path.toString()));
        if (!component) {
            CONSOLE_CTP(context) << "Unable to inflate component";
            return nullptr;
        }

        if (component->singleChild()) {
            populateSingleChildLayout(expanded, item, component, path);
        } else if (component->multiChild()) {
            populateLayoutComponent(expanded, item, component, path);
        }

        LOG_IF(DEBUG_BUILDER) << "Returning component " << *component;
        return component;
    }

    LOG_IF(DEBUG_BUILDER) << "Expanding layout '" << type << "'";
    auto resource = context->getLayout(type);
    if (!resource.empty()) {
        properties.emplace(item);
        return expandLayout(context, properties, resource.json(), parent, resource.path());
    }

    CONSOLE_CTP(context) << "Unable to find layout or component '" << type << "'";
    return nullptr;
}

/**
 * Expand a single component from a "when" list of possible components
 *
 * @param context Current data-binding context
 * @param items A vector of the components to expand
 * @param properties The user-specified properties for this component
 * @param parent The parent component of this component
 * @param path The path description of this component
 */
CoreComponentPtr
Builder::expandSingleComponentFromArray(const ContextPtr& context,
                                        const std::vector<Object>& items,
                                        Properties& properties,
                                        const CoreComponentPtr& parent,
                                        const Path& path)
{
    LOG_IF(DEBUG_BUILDER) << path;
    for (int index = 0; index < items.size(); index++) {
        const auto& item = items.at(index);
        if (!item.isMap())
            continue;

        if (propertyAsBoolean(*context, item, "when", true))
            return expandSingleComponent(context, item, properties, parent, path.addIndex(index));
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
 */
CoreComponentPtr
Builder::expandLayout(const ContextPtr& context,
                      Properties& properties,
                      const rapidjson::Value& layout,
                      const CoreComponentPtr& parent,
                      const Path& path)
{
    LOG_IF(DEBUG_BUILDER) << path;
    assert(layout.IsObject());

    // Build a new context for this layout.
    ContextPtr cptr = Context::create(context);

    // Add each parameter to the context.  It's either going to come from
    // a property or its default value.  This will remove the matching property from
    // the property map.
    ParameterArray params(layout);
    for (const auto& param : params) {
        LOG_IF(DEBUG_BUILDER) << "Parsing parameter: " << param.name;
        cptr->putUserWriteable(param.name, properties.forParameter(*cptr, param));
    }

    if (DEBUG_BUILDER) {
        for (std::shared_ptr<const Context> p = cptr; p; p = p->parent()) {
            for (const auto& m : *p)
                LOG(LogLevel::DEBUG) << m.first << ": " << m.second;
        }
    }
    return expandSingleComponentFromArray(cptr,
                                          arrayifyProperty(cptr, layout, "item", "items"),
                                          properties,
                                          parent,
                                          path.addProperty(layout, "item", "items"));
}



CoreComponentPtr
Builder::inflate(const ContextPtr& context,
                 Properties& mainProperties,
                 const rapidjson::Value& mainDocument)
{
    return expandLayout(context, mainProperties, mainDocument, nullptr,
        Path(context->getRootConfig().getTrackProvenance() ? std::string(Path::MAIN) + "/mainTemplate" : ""));
}

CoreComponentPtr
Builder::inflate(const ContextPtr& context,
                 const rapidjson::Value& component)
{
    assert(component.IsObject());

    Properties p;
    return expandSingleComponent(context, Object(component), p, nullptr,
        Path(context->getRootConfig().getTrackProvenance() ? "_virtual" : ""));
}

} // namespace apl
