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

#include "apl/engine/bindingchange.h"

#include "apl/engine/propdef.h"
#include "apl/engine/typeddependant.h"
#include "apl/utils/identifier.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"
#include "apl/utils/tracing.h"


namespace apl {

bool
isValidBinding(const ContextPtr& context, const Object& binding, const std::string& name)
{
    if (!isValidIdentifier(name)) {
        CONSOLE(context) << "Invalid binding name '" << name << "'";
        return false;
    }

    if (!binding.has("value")) {
        CONSOLE(context) << "Binding '" << name << "' did not specify a value";
        return false;
    }

    if (context->hasLocal(name)) {
        CONSOLE(context) << "Attempted to bind to pre-existing property '" << name << "'";
        return false;
    }
    return true;
}

std::vector<BindingChangePtr>
attachBindings(const ContextPtr& context, const Object& item, std::function<BindingChangePtr(Object&&)> makeBCP)
{
    APL_TRACE_BLOCK("Builder:attachBindings");
    auto bindings = arrayifyProperty(*context, item, "bind");
    std::vector<BindingChangePtr> bindList;

    for (const auto& binding : bindings) {
        auto name = propertyAsString(*context, binding, "name");
        if (!isValidBinding(context, binding, name)) continue;

        // Extract the binding as an optional node tree.
        auto result = parseAndEvaluate(*context, binding.get("value"));
        auto bindingType = optionalMappedProperty<BindingType>(*context, binding, "type",
                                                               kBindingTypeAny, sBindingMap);

        auto bindingFunc = sBindingFunctions.at(bindingType);
        auto value = bindingFunc(*context, result.value);

        BindingChangePtr ptr;
        if (makeBCP) {
            // Construct a BindingChange object if there is a valid "onChange" property on the binding
            auto onChangeCommands = binding.opt("onChange", Object::NULL_OBJECT());
            if (!onChangeCommands.empty()) {
                auto commands = asCommand(*context, onChangeCommands);
                if (!commands.empty()) {
                    ptr = makeBCP(std::move(commands));
                    bindList.push_back(ptr);
                }
            }
        }

        // Store the value in the new context.  Binding values are mutable; they can be changed later.
        context->putUserWriteable(name, value, ptr);

        if (!result.symbols.empty())
            ContextDependant::create(context, name, std::move(result.expression), context,
                                     std::move(bindingFunc), std::move(result.symbols));
    }

    return bindList;
}



} // namespace apl