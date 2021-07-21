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
 *
 * Array-ification
 *
 * Use these functions to extract a named array from a RapidJSON object
 * and iterate over that array.
 *
 * Simple, in-line:  'items': [ A,B,C ]
 * Data-bound:       'items': "${myItems}"
 * Inline inflate:   'items': [ A,${b},C ]    where b = B
 * Inline array:     'items': [ A,${b},C ]    where b = [B1, B2]
 *
 * If the extracted object is a string, evaluate it.
 * If the extract object is an array, march the list of elements and inflate each string.
 */

#ifndef _APL_ARRAYIFY_H
#define _APL_ARRAYIFY_H

#include "rapidjson/document.h"
#include "apl/engine/context.h"
#include "apl/engine/evaluate.h"

namespace apl {

/*
 * Convenience class for iterating over array-ified JSON arrays without constructing
 * a complete std::vector<Object> map.  This class holds a raw pointer to the internal
 * rapidjson value, so it is important to not release the original rapidjson object
 * while this class is in use.
 */
class ConstJSONArray
{
public:
    using value_type = rapidjson::GenericValue<rapidjson::UTF8<> >;

    ConstJSONArray() : mValue(nullptr) {}
    explicit ConstJSONArray(const rapidjson::Value *value) : mValue(value) {}

    const value_type * begin() const {
        if (!mValue)
            return nullptr;

        if (mValue->IsArray())
            return mValue->Begin();

        return mValue;
    }

    const value_type * end() const {
        if (!mValue)
            return nullptr;

        if (mValue->IsArray())
            return mValue->End();

        return mValue + 1;
    }

    size_t size() const {
        if (!mValue)
            return 0;

        if (mValue->IsArray())
            return mValue->Size();

        return 1;
    }

    bool empty() const {
        return size() == 0;
    }

    const value_type & operator[](size_t index) const {
        if (mValue->IsArray())
            return (*mValue)[index];

        return *mValue;
    }

private:
    const rapidjson::Value *mValue;
};

/**
 * Convert a single rapidjson Value into an array of values.  If the value is
 * an array, it returns that array.  If the value is not an array, it creates
 * a virtual array of that single value.
 * @param value The value to arrayify.
 * @return An array-ified object.
 */
inline ConstJSONArray
arrayify(const rapidjson::Value& value)
{
    return ConstJSONArray(&value);
}

/**
 * Termination method for the templated version of this function.  Always returns
 * an empty array.
 * @param value The object.
 * @return An empty array.
 */
inline ConstJSONArray
arrayifyProperty(const rapidjson::Value& value)
{
    return ConstJSONArray();
}

/**
 * Extract a named property from the rapidjson object and return it as an array.
 * This method is templated so you can pass in any number of names.  The first name
 * that matches a property in the rapidjson object will be returned as an array.  If
 * no names match, we return an empty array.
 *
 * Typically you will call this as a simple function.  For example, to extract an
 * the "item" or "items" child of an object::
 *
 *     auto result = arrayifyProperty( jsonObject, "item", "items" );
 *
 * This function does NOT perform any data-binding.
 *
 * @tparam T
 * @tparam Types
 * @param value The object to extract the named property from.
 * @param name The name of the first child property to look for
 * @param other The names of the other child properties to look or
 * @return An arrayified object.
 */
template <typename T, typename... Types> ConstJSONArray
arrayifyProperty(const rapidjson::Value& value, T name, Types... other)
{
    if (!value.IsObject())
        return {};

    auto it = value.FindMember(name);
    if (it != value.MemberEnd())
        return arrayify(it->value);

    return arrayifyProperty(value, other...);
}


/**
 * Arrayify a given value and return it as an array of values, having performed data-binding
 * on the top-level values.
 *
 * If the passed value is string this function will run data-binding on that string.  If the
 * result of data-binding is an array, that array is returned immediately.  If the result of
 * data-binding is any other type, then an array of length 1 is returned containing that result.
 *
 * If the passed value is an array, this function will iterate over each element in the array.
 * Any string element in the array will get a data-binding pass.  If the result of the data-binding
 * is an array, then the new array is inserted into the top-level array; otherwise, the result
 * of the data-binding is included in the top-level array.
 *
 * If the passed value is any other type, this function will return an array of length 1 containing
 * that element.
 *
 * Assume that the following bindings are in place:
 *
 *     "a" = "fuzzy duck"
 *     "b" = [ "a", "b" ]
 *     "c" = "This is a ${a}"
 *
 * Then the following expressions will evaluate as follows:
 *
 *     23                   -> [ 23 ]
 *     "random string"      -> [ "random string" ]
 *     "${a}"               -> [ "fuzzy duck" ]
 *     "${b}"               -> [ "a", "b" ]
 *     "${c}"               -> [ "This is a ${a}" ]
 *     [ 1, 2, "${a}" ]     -> [ 1, 2, "fuzzy duck" ]
 *     [ "${b}", "${b}" ]   -> [ "a", "b", "a", "b" ]
 *     { "name": "${a}" }   -> [ {"name": "${a}"} ]
 *
 * @param context The data-binding context to evaluate in.
 * @param value The object to array-ify.
 * @return An array of objects.
 */
extern std::vector<Object> arrayify(const Context& context, const Object& value);

/**
 * Termination function for the templated version of this function.
 */
inline std::vector<Object>
arrayifyProperty(const Context& context, const Object& value)
{
    return std::vector<Object>();
}

/**
 * Extract a named property from an object and return it as an array.
 * This method is templated so you can pass in any number of names.  The first name
 * that matches a property in the object will be returned as an array.  If
 * no names match, we return an empty array.
 *
 * Typically you will call this as a simple function.  For example, to extract an
 * the "item" or "items" child of an object:
 *
 *     auto result = arrayifyProperty( context, myObject, "item", "items" );
 *
 * This function performs data-binding and interpolates arrays as per the function
 * arrayify(const Context&, const Object&).
 *
 * @tparam T
 * @tparam Types
 * @param context The data-binding context
 * @param value The object to extract the named property from.
 * @param name The name of the property to extract and array-ify.
 * @param other Alternative names for that property.
 * @return A vector of objects.
 */
template <typename T, typename... Types> std::vector<Object>
arrayifyProperty(const Context& context, const Object& value, T name, Types... other)
{
    if (value.isMap() && value.has(name))
        return arrayify(context, value.get(name));

    return arrayifyProperty(context, value, other...);
}

/**
 * These routines do arrayification, but they return the result as a single Object that contains an array.
 * @param context
 * @param value
 * @return
 */

extern Object arrayifyAsObject(const Context& context, const Object& value);

inline Object
arrayifyPropertyAsObject(const Context& context, const Object& value)
{
    return Object(std::vector<Object>{});
}

template <typename T, typename... Types> Object
arrayifyPropertyAsObject(const Context& context, const Object& value, T name, Types... other)
{
    if (value.isMap() && value.has(name))
        return arrayifyAsObject(context, value.get(name));

    return arrayifyPropertyAsObject(context, value, other...);
}

}  // namespace apl

#endif // _APL_ARRAYIFY_H
