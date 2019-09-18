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
 */

#include <cmath>
#include <functional>

#include "apl/engine/properties.h"
#include "apl/engine/evaluate.h"
#include "apl/primitives/dimension.h"

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
        [](unsigned char c) { return !std::isalnum(c) && (c != '_') && (c != '-'); }), result.end());
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

Dimension
Properties::asAbsoluteDimension(const Context& context, const char *name, double defvalue)
{
    auto s = mProperties.find(name);
    if (s == mProperties.end())
        return Dimension(DimensionType::Absolute, defvalue);

    return evaluate(context, s->second).asAbsoluteDimension(context);
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

Object
Properties::forParameter(const Context& context, const Parameter& parameter )
{
    auto it = mProperties.find(parameter.name);
    if (it == mProperties.end())
        return evaluate(context, parameter.defvalue);

    auto result = evaluate(context, it->second);
    mProperties.erase(it);   // Remove the property from the list

    if (result.isNull())
        return evaluate(context, parameter.defvalue);

    return sBindingFunctions.at(parameter.type)(context, result);
}

}  // namespace apl
