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

#ifndef _APL_EVALUATE_H
#define _APL_EVALUATE_H

#include "apl/utils/bimap.h"
#include "apl/primitives/object.h"
#include "apl/primitives/dimension.h"

namespace apl {

/**
 * Parse a data-binding string and return the parsed expression. The returned object will
 * be byte code if the string contained at least one data-binding expression and will be
 * a plain string if no data-binding expressions were found
 * @param context  The data-binding context
 * @param value The string value to evaluate
 * @return The byte code or a string if no data-binding was found
 */
Object getDataBinding(const Context& context, const std::string& value);

/**
 * Parse a data-binding string and return the parsed expression.  If the string contains
 * data-binding expressions referring to symbols not defined in the current context
 * or symbols that have been marked as mutable, the returned object will be byte code.
 * If the parsed expression is constant, the returned object will be the appropriate type.
 *
 * @param context The data-binding context
 * @param value The string value to evaluate
 * @return The evaluated object or byte code
 */
Object parseDataBinding(const Context& context, const std::string& value);

/**
 * Parse a data-binding recursively and return the parsed expression tree.  If the object contains
 * any strings with data-binding expressions referring to symbols not defined in the current context
 * or symbols that have been marked as mutable, the returned object will be byte code.
 * @param context The data-binding context
 * @param object The object to evaluate
 * @return The evaluated object or byte code
 */
Object parseDataBindingRecursive(const Context& context, const Object& object);

/**
 * Evaluate an object applying data-binding.  The object or expression will be converted
 * into byte code if necessary, evaluated, and resources will be substituted.
 *
 * @param context The data-binding context.
 * @param object The object to evaluate.
 * @return The result
 */
Object evaluate(const Context& context, const Object& object);

/**
 * Evaluate a string applying data-binding.  The object or expression will be converted
 * into byte code if necessary, evaluated, and resources will be substituted.
 * @param context The data-binding context
 * @param expression The string to evaluate
 * @return The result
 */
Object evaluate(const Context& context, const char *expression);

/**
 * Re-evaluate an equation that is already marked as evaluable.  This method is used
 * when propagating dependency changes.
 * @param context The binding context of the equation
 * @param equation The equation to re-evaluate
 * @return The resultant value
 */
Object reevaluate(const Context& context, const Object& equation);

/**
 * Evaluate an object recursively.  Arrays and maps within the object will also
 * be evaluated for data-binding.
 * @param context The data-binding context.
 * @param object The object to evaluate.
 * @return
 */
Object evaluateRecursive(const Context& context, const Object& object);

std::string propertyAsString(const Context& context, const Object& object, const char *name);
std::string propertyAsString(const Context& context, const Object& object, const char *name, const std::string& defValue);
bool propertyAsBoolean(const Context& context, const Object& object, const char *name, bool defValue);
double propertyAsDouble(const Context& context, const Object& object, const char *name, double defValue);
int propertyAsInt(const Context& context, const Object& object, const char *name, int defValue);
Object propertyAsObject(const Context& context, const Object& object, const char *name);
Object propertyAsRecursive(const Context& context, const Object& object, const char *name);

/**
 * Retrieve a property from an object and do basic data parsing, but allow a node-tree to
 * be returned.
 * @param context The data-binding context
 * @param item The object that contains the named property
 * @param name The name of the property
 * @return The value of the property; may be a Node.
 */
Object propertyAsNode(const Context& context, const Object& item, const char *name);

/**
 * Look up a mapped property.  Return (T) -1 if the property is invalid.  If the property
 * is not specified, return defValue.
 * @tparam T The enumerated type (should map to int).
 * @param context The data-binding context.
 * @param item The host object which contains the property (must be a map).
 * @param name The name of the property.
 * @param defValue The default value to return if the property is not defined.
 * @param bimap The bimap to use to map from valid name to value.
 * @return The property value or -1 if the property is invalid.
 */
template<class T>
T propertyAsMapped(const Context& context,
                   const Object& item,
                   const char *name,
                   T defValue,
                   const Bimap<T, std::string>& bimap)
{
    if (!item.isMap())
        return static_cast<T>(-1);

    if (!item.has(name))
        return defValue;

    auto s = evaluate(context, item.get(name)).asString();
    if (s.empty())
        return defValue;

    return bimap.get(s, static_cast<T>(-1));
}

}  // Namespace apl

#endif // _APL_EVALUATE_H
