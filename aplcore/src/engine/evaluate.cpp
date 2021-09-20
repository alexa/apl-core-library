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

#include <type_traits>

#include "apl/engine/evaluate.h"
#include "apl/datagrammar/bytecodeassembler.h"
#include "apl/engine/context.h"
#include "apl/primitives/dimension.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"

namespace apl {

Object
getDataBinding(const Context& context, const std::string& value)
{
    return datagrammar::ByteCodeAssembler::parse(context, value);
}

Object
parseDataBinding(const Context& context, const std::string& value)
{
    auto result = datagrammar::ByteCodeAssembler::parse(context, value);
    if (result.isEvaluable())
        return result.getByteCode()->simplify();

    return result;
}

Object
parseDataBindingRecursive(const Context& context, const Object& object)
{
    if (object.isString()) {
        return parseDataBinding(context, object.getString());
    }
    else if (object.isTrueMap()) {
        auto result = std::make_shared<std::map<std::string, Object>>();
        for (const auto &m : object.getMap())
            result->emplace(m.first, parseDataBindingRecursive(context, m.second));
        return Object(result);
    } else if (object.isArray()) {
        auto v = std::make_shared<std::vector<Object>>();
        for (auto index = 0; index < object.size(); index++)
            v->push_back(parseDataBindingRecursive(context, object.at(index)));
        return Object(v);
    }

    return object;
}

Object
applyDataBinding(const Context& context, const std::string& value)
{
    Object parsed = getDataBinding(context, value);
    if (parsed.isEvaluable())
        return parsed.eval();

    return parsed;
}

Object
evaluate(const Context& context, const Object& object)
{
    // If it is a string, we check for data-binding
    auto result = object.isString() ? getDataBinding(context, object.getString()) : object;

    // Nodes get evaluated
    if (result.isEvaluable())
        result = result.eval();

    // Strings get a resource check
    if (result.isString()) {
        std::string s = result.getString();
        if (!s.empty() && s[0] == '@' && context.has(s))
            return context.opt(s);    // This isn't efficient because we do a has() and a get().
    }

    return result;
}

Object
evaluate(const Context& context, const char *expression)
{
    return evaluate(context, Object(expression));
}

Object
reevaluate(const Context& context, const Object& equation)
{
    // An evaluable equation is re-calculated.  Otherwise, we try a recursive evaluation.
    auto result = equation.isEvaluable() ? equation.eval() : evaluateRecursive(context, equation);

    // Strings get a resource check
    if (result.isString()) {
        std::string s = result.getString();
        if (!s.empty() && s[0] == '@' && context.has(s))
            return context.opt(s);    // This isn't efficient because we do a has() and a get().
    }

    return result;
}

/**
 * Resource lookup on object if it is of string type. Otherwise original object will be returned.
 */
Object
resourceLookup(const Context& context, const Object& result)
{
    if (result.isString()) {
        std::string s = result.getString();
        if (!s.empty() && s[0] == '@' && context.has(s))
            return context.opt(s);    // This isn't efficient because we do a has() and a get().
    }

    return result;
}

Object
evaluateRecursive(const Context& context, const Object& object)
{
    if (object.isString()) {
        auto result = applyDataBinding(context, object.getString());
        return resourceLookup(context, result);
    }
    else if (object.isTrueMap()) {
        auto result = std::make_shared<std::map<std::string, Object>>();
        for (const auto& m : object.getMap())
            result->emplace(m.first, evaluateRecursive(context, m.second));
        return Object(result);
    }
    else if (object.isArray()) {  // Embedded data-bound strings are inserted in-line: E.g., [ 1, "${b}" ]
        std::vector<Object> v;
        for (auto index = 0 ; index < object.size() ; index++) {
            auto item = object.at(index);
            auto itemEvaluated = evaluateRecursive(context, item);
            if (item.isString() && itemEvaluated.isArray()) {  // Insert the results into the array
                for (const auto& n : itemEvaluated.getArray())
                    v.push_back(n);
            } else {
                v.push_back(itemEvaluated);
            }
        }
        return Object(std::move(v));
    }
    else if (object.isEvaluable()) {
        auto result = object.eval();
        return resourceLookup(context, result);
    }

    return object;
}

bool
propertyAsBoolean(const Context& context,
                  const Object& item,
                  const char *name,
                  bool defValue )
{
    if (!item.isMap() || !item.has(name))
        return defValue;

    return evaluate(context, item.get(name)).asBoolean();
}

std::string
propertyAsString(const Context& context,
                 const Object& item,
                 const char *name)
{
    if (!item.isMap() || !item.has(name))
        return "";

    return evaluate(context, item.get(name)).asString();
}

std::string
propertyAsString(const Context& context,
                 const Object& item,
                 const char *name,
                 const std::string& defValue)
{
    if (!item.isMap() || !item.has(name))
        return defValue;

    return evaluate(context, item.get(name)).asString();
}

double
propertyAsDouble(const Context& context,
                  const Object& item,
                  const char *name,
                  double defValue )
{
    if (!item.isMap() || !item.has(name))
        return defValue;

    return evaluate(context, item.get(name)).asNumber();
}

int
propertyAsInt(const Context& context,
                 const Object& item,
                 const char *name,
                 int defValue )
{
    if (!item.isMap() || !item.has(name))
        return defValue;

    return evaluate(context, item.get(name)).asInt();
}

Object
propertyAsObject(const Context& context,
                 const Object& item,
                 const char *name)
{
    if (!item.isMap() || !item.has(name))
        return Object::NULL_OBJECT();

    return evaluate(context, item.get(name));
}

Object
propertyAsRecursive(const Context& context,
                 const Object& item,
                 const char *name)
{
    if (!item.isMap() || !item.has(name))
        return Object::NULL_OBJECT();

    return evaluateRecursive(context, item.get(name));
}

Object
propertyAsNode(const Context& context,
               const Object& item,
               const char *name)
{
    if (!item.isMap() || !item.has(name))
        return Object::NULL_OBJECT();

    auto object = item.get(name);
    return object.isString() ? parseDataBinding(context, object.getString()) : object;
}

} // namespace apl
