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

#include "apl/engine/properties.h"

#include <functional>

#include "apl/content/rootconfig.h"
#include "apl/datasource/datasource.h"
#include "apl/engine/context.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/typeddependant.h"
#include "apl/primitives/dimension.h"
#include "apl/utils/identifier.h"
#include "apl/utils/session.h"
#include "apl/utils/stringfunctions.h"

namespace apl {

/**
 * Extract a valid component ID.  The valid characters in an ID are [_a-zA-Z0-9]+
 */
std::string
Properties::asLabel(const Context& context, const char *name)
{
    auto s = mProperties.find(name);
    if (s == mProperties.end())
        return "";

    auto result = evaluate(context, s->second).asString();
    result.erase(std::remove_if(result.begin(), result.end(),
        [](unsigned char c) {
            return !sutil::isalnum(c) && (c != '_') && (c != '-');
        }), result.end());
    return result;
}

std::string
Properties::asString(const Context& context, const char *name, const char *defvalue)
{
    auto s = mProperties.find(name);
    if (s == mProperties.end())
        return defvalue;

    return evaluate(context, s->second).asString();
}

bool
Properties::asBoolean(const Context& context, const char *name, bool defvalue)
{
    auto s = mProperties.find(name);
    if (s == mProperties.end())
        return defvalue;

    return evaluate(context, s->second).asBoolean();
}

double
Properties::asNumber(const Context& context, const char *name, double defvalue)
{
    auto s = mProperties.find(name);
    if (s == mProperties.end())
        return defvalue;

    return evaluate(context, s->second).asNumber();
}

void
Properties::emplace(const Object& item)
{
    if (!item.isMap())
        return;

    for (const auto& kv : item.getMap()) {
        const std::string name = kv.first;
        if (name == "type" || name == "when")
            continue;
        // Note that this doesn't override an existing one (deliberately!)
        mProperties.emplace(name, kv.second);
    }
}

void
Properties::addToContext(const ContextPtr &context, const Parameter &parameter, bool userWriteable) {
    auto it = mProperties.find(parameter.name);

    // Parameter names are only added to the data-binding context if they are valid identifiers.
    if (!isValidIdentifier(parameter.name)) {
        CONSOLE(context) << "Unable to add parameter '" << parameter.name
                         << "' to context. Invalid identifier.";
        if (it != mProperties.end()) mProperties.erase(it); // Remove it just in case
        return;
    }

    auto bindingFunc = sBindingFunctions.at(parameter.type);
    Object result = bindingFunc(*context, evaluate(*context, parameter.defvalue));
    if (it != mProperties.end()) {
        Object value = it->second;
        mProperties.erase(it); // Remove the property from the list

        if (value.isString()) {
            auto v = parseAndEvaluate(*context, value);
            if (!v.symbols.empty()) {
                ContextDependant::create(context, parameter.name, std::move(v.expression), context,
                                         bindingFunc, std::move(v.symbols));
            }

            // We allow any (not only direct) binding to be "enriched".
            value = v.value;
        }

        if (!value.isNull()) {
            result = bindingFunc(*context, value);
            // If type explicitly specified we may want to "enrich" the object.
            if (value.isMap() && value.has("type")) {
                std::string type = value.get("type").getString();
                if (sBindingMap.has(type)) {
                    bindingFunc = sBindingFunctions.at(sBindingMap.at(type));
                    result = bindingFunc(*context, value);
                }
                else if (auto provider = context->getRootConfig().getDataSourceProvider(type)) {
                    result = DataSource::create(context, provider, value, parameter.name);
                }
            }
        }
    }

    if (userWriteable) {
        context->putUserWriteable(parameter.name, result);
    }
    else {
        context->putConstant(parameter.name, result);
    }
}

}  // namespace apl
