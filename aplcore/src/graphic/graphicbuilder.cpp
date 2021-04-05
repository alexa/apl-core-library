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

#include "apl/graphic/graphicbuilder.h"

#include "apl/engine/arrayify.h"
#include "apl/engine/contextdependant.h"

#include "apl/graphic/graphic.h"
#include "apl/graphic/graphicelementcontainer.h"
#include "apl/graphic/graphicelementpath.h"
#include "apl/graphic/graphicelementgroup.h"
#include "apl/graphic/graphicelementtext.h"

#include "apl/utils/session.h"

namespace apl {

static const bool DEBUG_GRAPHIC_BUILDER = false;

using MakeElementFunc = GraphicElementPtr (*)(const GraphicPtr&, const ContextPtr&, const Object& );

static const std::map<std::string, MakeElementFunc> sGraphicElementMap = {
    {"group", GraphicElementGroup::create},
    {"Group", GraphicElementGroup::create},
    {"path",  GraphicElementPath::create},
    {"Path",  GraphicElementPath::create},
    {"text",  GraphicElementText::create},
    {"Text",  GraphicElementText::create},
};


GraphicElementPtr
GraphicBuilder::build(const GraphicPtr& graphic, const Object& json)
{
    GraphicBuilder builder(graphic);

    auto container = GraphicElementContainer::create(graphic, graphic->getContext(), json);
    if (container)
        builder.addChildren(*container, json);
    return container;
}


GraphicElementPtr
GraphicBuilder::build(const Context& context, const Object& json)
{
    GraphicBuilder builder(nullptr);
    auto& mutableContext = const_cast<Context&>(context);
    auto childContext = Context::createFromParent(mutableContext.shared_from_this());
    return builder.createChild(childContext, json);
}


GraphicBuilder::GraphicBuilder(const GraphicPtr& graphic)
    : mGraphic(graphic),
      mMultichildSupport(true)
{
    // Version 1.2 of AVG adds support for the "when" clause and binding multiple children
    // using a "data" array.  This method checks the graphic version.
    if (mGraphic) {
        auto version = graphic->getVersion();
        mMultichildSupport = (version != kGraphicVersion10 && version != kGraphicVersion11);
    }
}


void
GraphicBuilder::addChildren(GraphicElement& element, const Object& json)
{
    LOG_IF(DEBUG_GRAPHIC_BUILDER) << element.toDebugString();

    const auto& context = *element.mContext;
    const auto items = arrayifyProperty(context, json, "item", "items");
    if (items.empty())
        return;

    size_t index = 0;

    // TODO: Add live data object later (maybe).  Right now LiveData isn't quite exposed to AVG.
    if (mMultichildSupport) {
        const auto data = arrayifyPropertyAsObject(context, json, "data");
        const auto dataItems = evaluateRecursive(context, data);
        if (!dataItems.empty()) {
            LOG_IF(DEBUG_GRAPHIC_BUILDER) << "Data child inflation: " << dataItems;
            const auto length = dataItems.size();
            for (size_t dataIndex = 0; dataIndex < length; dataIndex++) {
                const auto& dataItem = dataItems.at(dataIndex);
                auto childContext = Context::createFromParent(element.mContext);
                childContext->putConstant("data", dataItem);
                childContext->putConstant("index", index);
                childContext->putConstant("length", length);
                auto child = createChildFromArray(childContext, items);
                if (child) {
                    element.mChildren.push_back(child);
                    index++;
                }
            }
            return;
        }
    }

    // If we get to this point, we are not doing multi-child inflation
    LOG_IF(DEBUG_GRAPHIC_BUILDER) << "Normal child inflation";
    const auto length = items.size();
    for (size_t i = 0; i < length; i++) {
        const auto& item = items.at(i);
        auto childContext = Context::createFromParent(element.mContext);
        if (mMultichildSupport) {
            childContext->putConstant("index", index);
            childContext->putConstant("length", length);
        }
        auto child = createChildFromArray(childContext, arrayify(context, item));
        if (child) {
            LOG_IF(DEBUG_GRAPHIC_BUILDER) << "child [" << i << "]";
            element.mChildren.push_back(child);
            index++;
        }
    }
}


GraphicElementPtr
GraphicBuilder::createChild(const ContextPtr& context, const Object& json)
{
    LOG_IF(DEBUG_GRAPHIC_BUILDER) << "";

    // Check for a valid child type
    auto type = propertyAsString(*context, json, "type");
    auto it = sGraphicElementMap.find(type);
    if (it == sGraphicElementMap.end()) {
        CONSOLE_CTP(context) << "Invalid graphic child type '" << type << "'";
        return nullptr;
    }

    // Create a new context and apply data binding
    ContextPtr expanded = Context::createFromParent(context);
    auto bindings = arrayifyProperty(*context, json, "bind");
    for (const auto& binding : bindings) {
        auto name = propertyAsString(*expanded, binding, "name");
        if (name.empty() || !binding.has("value"))
            continue;

        // Extract the binding as an optional node tree.
        auto tmp = propertyAsNode(*expanded, binding, "value");
        auto value = evaluateRecursive(*expanded, tmp);
        auto bindingType = propertyAsMapped<BindingType>(*expanded, binding, "type", kBindingTypeAny, sBindingMap);
        auto bindingFunc = sBindingFunctions.at(bindingType);

        // Store the value in the new context.  Binding values are mutable; they can be changed later.
        expanded->putUserWriteable(name, bindingFunc(*expanded, value));

        // If it is a node, we connect up the symbols that it is dependant upon
        if (tmp.isEvaluable())
            ContextDependant::create(expanded, name, tmp, expanded, bindingFunc);
    }

    // Inflate the child
    auto child = it->second(mGraphic, expanded, json);
    if (child && child->hasChildren())
        addChildren(*child, json);
    return child;
}


GraphicElementPtr
GraphicBuilder::createChildFromArray(const ContextPtr& context,
                                     const std::vector<Object>& items)
{
    LOG_IF(DEBUG_GRAPHIC_BUILDER) << items.size();

    const auto length = items.size();
    for (size_t i = 0; i < length; i++) {
        const auto& item = items.at(i);
        if (!item.isMap())
            continue;

        if (!mMultichildSupport || propertyAsBoolean(*context, item, "when", true))
            return createChild(context, items.at(i));
    }

    return nullptr;
}


} // namespace apl
