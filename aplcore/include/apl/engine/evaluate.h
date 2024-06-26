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

#include <utility>

#include "apl/engine/binding.h"
#include "apl/primitives/boundsymbolset.h"
#include "apl/primitives/object.h"
#include "apl/utils/bimap.h"

namespace apl {

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
 * Evaluate an object recursively.  Arrays and maps within the object will also
 * be evaluated for data-binding.
 * @param context The data-binding context.
 * @param object The object to evaluate.
 * @param symbolSet An optional symbol set to populate with the results of evaluation
 * @return The result of recursive evaluation.
 */
Object
evaluateNested(const Context& context, const Object& object, BoundSymbolSet *symbolSet=nullptr);

/**
 * This method is only used by the byte code evaluator for the "eval(x)" built-in function.
 * It is is basically the same as the evaluateNested() method, but it tracks evaluation depth.
 * @param context The data-binding context
 * @param object The object to evaluate
 * @param symbolSet The symbol set to populate with the results of the evaluation (may be nullptr)
 * @param depth The current evaluation depth
 * @return The result of the evaluation
 */
Object
evaluateInternal(const Context& context, const Object& object, BoundSymbolSet *symbolSet, int depth);

/**
 * The structure returned by parseAndEvaluate().
 */
struct ParseResult {
    Object value;       /// The calculated value after all data-binding expressions are evaluated
    Object expression;  /// The expanded object processed for data-binding expressions
    BoundSymbolSet symbols;  /// The bound symbols used when calculating "value" from "expression".
};

/**
 * The structure returned by applyDataBinding().
 */
struct ApplyResult {
    Object value;  /// The calculated value after all data-binding expressions are evaluated
    BoundSymbolSet symbols;  /// The bound symbols used when calculating the value.
};


/**
 * Recursively parse an object for data-binding and return (a) the result of evaluating the
 * object in the assigned context, (b) the same object with byte-code in the places that will
 * need to be re-evaluated, and (c) the symbols used to evaluated the nested expression.
 *
 * Hint: if there are no returned symbols, the object is constant and you can ignore the
 * second half of the returned pair.
 *
 * @param context The data-binding context to use when evaluating.
 * @param object The object to recursively parse and evaluate.
 * @param optimize If true, optimize the byte code.  Defaults to true.
 * @return A ParseResult object
 */
ParseResult
parseAndEvaluate(const Context& context,
                 const Object& object,
                 bool optimize=true);

/**
 * Apply data-binding to an object that was previously parsed using parseAndEvaluate().
 * The same context that was used for parsing must be used to re-apply the data binding.
 * If a SymbolSet pointer is passed, it will be populated with the list of symbols referenced
 * when the object is re-evaluated.
 * @param context The data-binding context.
 * @param object The object to re-evaluate.
 * @param bindingFunction A binding function to apply to the evaluated object.
 * @return An ApplyResult object containing the calculated value and the symbols referenced.
 */
ApplyResult
applyDataBinding(const Context& context,
                 const Object& object,
                 const BindingFunction& bindingFunction);


std::string propertyAsString(const Context& context, const Object& object, const char *name);
std::string propertyAsString(const Context& context, const Object& object, const char *name, const std::string& defValue);
bool propertyAsBoolean(const Context& context, const Object& object, const char *name, bool defValue);
double propertyAsDouble(const Context& context, const Object& object, const char *name, double defValue);
int propertyAsInt(const Context& context, const Object& object, const char *name, int defValue);
Object propertyAsObject(const Context& context, const Object& object, const char *name);
Object propertyAsRecursive(const Context& context, const Object& object, const char *name);


/**
 * Look up a mapped property.  Return a pair of { T, bool } of the calculated value and a boolean
 * indicating if the calculated value is valid.  The calculated value is only valid if (a) the
 * property existed in the item and (b) the property value maps correctly to one of the defined
 * values in the bimap.
 *
 * @tparam T The enumerated type (should map to int).
 * @param context The data-binding context.
 * @param item The host object which contains the property (must be a map).
 * @param name The name of the property.
 * @param bimap The bimap to use to map from valid name to value.
 * @return std::pair{ value, bool } where the bool is true only if the property was valid.
 */
template<class T>
std::pair<T, bool>
requiredMappedProperty(const Context& context,
                       const Object& item,
                       const char *name,
                       const Bimap<T, std::string>& bimap)
{
    if (!item.isMap() || !item.has(name))
        return { static_cast<T>(-1), false };

    auto s = evaluate(context, item.get(name)).asString();
    if (s.empty())
        return { static_cast<T>(-1), false };

    auto it = bimap.find(s);
    if (it == bimap.endBtoA())
        return { static_cast<T>(-1), false };

    return { it->second, true };
}

/**
 * Look up a mapped property.  Return the value of the mapped property if it is found and valid.
 * Otherwise return the default value.
 *
 * @tparam T The enumerated type (should map to int).
 * @param context The data-binding context.
 * @param item The host object which contains the property (must be a map).
 * @param name The name of the property.
 * @param defValue The default value.
 * @param bimap The bimap to use to map from valid name to value.
 * @return The mapped value or the default value.
 */
template<class T>
T
optionalMappedProperty(const Context& context,
                       const Object& item,
                       const char *name,
                       T defValue,
                       const Bimap<T, std::string>& bimap)
{
    if (!item.isMap() || !item.has(name))
        return defValue;

    auto s = evaluate(context, item.get(name)).asString();
    if (s.empty())
        return defValue;

    return bimap.get(s, defValue);
}


/**
 * Look up a mapped property.  If the property isn't specified, return the default value.
 * If it is specified, but invalid, return an error.
 *
 * @tparam T The enumerated type (should map to int).
 * @param context The data-binding context.
 * @param item The host object which contains the property (must be a map).
 * @param name The name of the property.
 * @param defValue The default value.
 * @param bimap The bimap to use to map from valid name to value.
 * @return A pair of (value, boolean), where the boolean is true if the value is valid.
 */
template<class T>
std::pair<T, bool>
optionalStrictMappedProperty(const Context& context,
                             const Object& item,
                             const char *name,
                             T defValue,
                             const Bimap<T, std::string>& bimap)
{
    if (!item.isMap() || !item.has(name))
        return { defValue, true };

    auto s = evaluate(context, item.get(name)).asString();
    if (s.empty())
        return { defValue, false };

    auto it = bimap.find(s);
    if (it == bimap.endBtoA())
        return { defValue, false };

    return { it->second, true };
}



}  // Namespace apl

#endif // _APL_EVALUATE_H
