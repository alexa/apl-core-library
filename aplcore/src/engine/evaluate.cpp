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

#include "apl/engine/evaluate.h"
#include "apl/datagrammar/bytecodeassembler.h"
#include "apl/datagrammar/bytecodeoptimizer.h"
#include "apl/engine/context.h"
#include "apl/utils/session.h"

namespace apl {

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
parseDataBinding(const Context& context, const std::string& value, bool optimize)
{
    auto result = datagrammar::ByteCodeAssembler::parse(context, value);
    if (optimize && result.is<datagrammar::ByteCode>())
        result.get<datagrammar::ByteCode>()->optimize();

    return result;
}

Object
parseDataBindingNested(const Context& context, const Object& object, bool optimize) // NOLINT(misc-no-recursion)
{
    if (object.isString()) {
        return parseDataBinding(context, object.getString(), optimize);
    }
    else if (object.isTrueMap()) {
        auto result = std::make_shared<std::map<std::string, Object>>();
        for (const auto &m : object.getMap())
            result->emplace(m.first, parseDataBindingNested(context, m.second, optimize));
        return { result };
    } else if (object.isArray()) {
        auto v = std::make_shared<std::vector<Object>>();
        for (auto index = 0; index < object.size(); index++)
            v->push_back(parseDataBindingNested(context, object.at(index), optimize));
        return { v };
    }

    return object;
}

Object
applyDataBindingNested(const Context& context, // NOLINT(misc-no-recursion)
                       const Object& object,
                       BoundSymbolSet *symbols,
                       int depth)
{
    if (object.is<datagrammar::ByteCode>())
        return resourceLookup(context, object.get<datagrammar::ByteCode>()->evaluate(symbols, depth));

    if (object.isTrueMap()) {
        auto result = std::make_shared<std::map<std::string, Object>>();
        for (const auto& m : object.getMap())
            result->emplace(m.first, applyDataBindingNested(context, m.second, symbols, depth));
        return { result };
    }

    if (object.isArray()) {  // Embedded data-bound strings are inserted in-line: E.g., [ 1, "${b}" ]
        std::vector<Object> v;
        for (auto index = 0 ; index < object.size() ; index++) {
            auto item = object.at(index);
            auto itemEvaluated = applyDataBindingNested(context, item, symbols, depth);
            if (item.is<datagrammar::ByteCode>() && itemEvaluated.isArray()) {  // Insert the results into the array
                for (const auto& n : itemEvaluated.getArray())
                    v.push_back(n);
            } else {
                v.push_back(itemEvaluated);
            }
        }
        return { std::move(v) };
    }

    return resourceLookup(context, object);
}

ApplyResult
applyDataBinding(const Context& context,
                 const Object& object,
                 const BindingFunction& bindingFunction)
{
    BoundSymbolSet symbols;
    auto result = applyDataBindingNested(context, object, &symbols, 0);
    if (bindingFunction)
        result = bindingFunction(context, result);
    return { result, std::move(symbols) };
}

Object
evaluate(const Context& context, const Object& object)
{
    // If it is a string, we check for data-binding
    auto result = object.isString() ? parseDataBinding(context, object.getString(), false) : object;

    // Expressions and bound symbols are evaluated
    if (result.isEvaluable())
        result = result.eval();

    // Strings get a resource check
    return resourceLookup(context, result);
}

Object
evaluate(const Context& context, const char *expression)
{
    return evaluate(context, Object(expression));
}

Object
evaluateNested(const Context& context, const Object& object, BoundSymbolSet *symbolSet)
{
    auto result = parseDataBindingNested(context, object, false);
    return applyDataBindingNested(context, result, symbolSet, 0);
}

Object
evaluateInternal(const Context& context, const Object& object, BoundSymbolSet *symbolSet, int depth)
{
    auto result = parseDataBindingNested(context, object, false);
    return applyDataBindingNested(context, result, symbolSet, depth);
}

ParseResult
parseAndEvaluate(const Context& context,
                 const Object& object,
                 bool optimize)
{
    BoundSymbolSet symbols;
    auto parsed = parseDataBindingNested(context, object, optimize);
    auto evaluated = applyDataBindingNested(context, parsed, &symbols, 0);
    return { std::move(evaluated), std::move(parsed), std::move(symbols) };
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

    auto result = evaluate(context, item.get(name)).asNumber();
    // asNumber() returns NaN if the object wasn't a number.  But when
    // loading properties with a default value, we generally don't want to store NaN.
    // Instead, we return the default value.
    return std::isnan(result) ? defValue : result;
}

int
propertyAsInt(const Context& context,
              const Object& item,
              const char *name,
              int defValue )
{
    if (!item.isMap() || !item.has(name))
        return defValue;

    auto result = evaluate(context, item.get(name)).asValidInt();
    return result.second ? result.first : defValue;
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

    return evaluateNested(context, item.get(name));
}

} // namespace apl
