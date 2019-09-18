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

#include "apl/engine/evaluate.h"
#include "apl/datagrammar/databindingrules.h"
#include "apl/engine/context.h"
#include "apl/primitives/dimension.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"

namespace apl {

const bool DEBUG_DATA_BINDING = false;

/**
 * Evaluation stages we need:
 *
 *  1. If a string, apply data binding to expand the string.
 *  2. If it is still a string, expand resources and repeat data-binding step
 *  3. Convert the object to the correct internal type.  It is still an object.
 */
const Object
parseDataBinding(const Context& context, const std::string& value)
{
    try {
        pegtl::data_parser parser(value, "parseDataBinding");
        datagrammar::Stacks stacks(context);
        parser.parse<datagrammar::grammar, datagrammar::action>(stacks);
        Object result = stacks.finish();
        LOG_IF(DEBUG_DATA_BINDING) << "Parse data binding " << value << "=" << result;
        return result;
    }
    catch (pegtl::parse_error e) {
        CONSOLE_CTX(context) << "Parse error in '" << value << "' - " << e.what();
    }

    return value;
}

const Object
applyDataBinding(const Context& context, const std::string& value)
{
    Object parsed = parseDataBinding(context, value);
    if (parsed.isNode())
        return parsed.eval(context);

    return parsed;
}

const Object
evaluatePostDataBinding(const Context& context, const Object& object)
{
    auto result = object.isNode() ? object.eval(context) : object;

    // Strings get a resource check
    if (result.isString()) {
        std::string s = result.getString();
        if (!s.empty() && s[0] == '@' && context.has(s))
            return context.opt(s);    // This isn't efficient because we do a has() and a get().
    }

    return result;
}

const Object
evaluate(const Context& context, const Object& object)
{
    // If it is a string, we check for data-binding
    auto result = object.isString() ? parseDataBinding(context, object.getString()) : object;

    // Nodes get evaluated
    if (result.isNode())
        result = result.eval(context);

    // Strings get a resource check
    if (result.isString()) {
        std::string s = result.getString();
        if (!s.empty() && s[0] == '@' && context.has(s))
            return context.opt(s);    // This isn't efficient because we do a has() and a get().
    }

    return result;
}

const Object
evaluate(const Context& context, const char *expression)
{
    return evaluate(context, Object(expression));
}

const Object
evaluateRecursive(const Context& context, const Object& object)
{
    if (object.isString()) {
        auto result = applyDataBinding(context, object.getString());

        // Check for resources
        if (result.isString()) {
            std::string s = result.getString();
            if (!s.empty() && s[0] == '@' && context.has(s))
                return context.opt(s);    // This isn't efficient because we do a has() and a get().
        }

        return result;
    }
    else if (object.isMap()) {
        auto result = std::make_shared<std::map<std::string, Object>>();
        for (const auto& m : object.getMap())
            result->emplace(m.first, evaluateRecursive(context, m.second));
        return Object(result);
    }
    else if (object.isArray()) {
        auto v = std::make_shared<std::vector<Object>>();
        for (auto index = 0 ; index < object.size() ; index++)
            v->push_back(evaluateRecursive(context, object.at(index)));
        return Object(v);
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

Dimension
propertyAsDimension(const Context& context,
                    const Object& item,
                    const char *name)
{
    if (!item.isMap() || !item.has(name))
        return Dimension();  // Returns "auto"

    return evaluate(context, item.get(name)).asDimension(context);
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
