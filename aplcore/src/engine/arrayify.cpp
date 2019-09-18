/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/engine/arrayify.h"

namespace apl {

ConstJSONArray
arrayify(const rapidjson::Value& value)
{
    return ConstJSONArray(&value);
}

/*
 * Given a JSON object, look up a property by name or names and return the resulting
 * object.  Do not do any data-binding expansion.
 */
ConstJSONArray
arrayifyProperty(const rapidjson::Value& value)
{
    return ConstJSONArray();
}

std::vector<Object>
arrayifyProperty(const Context& context, const Object& value)
{
    return std::vector<Object>();
}

std::vector<Object>
arrayify(const Context& context, const Object& value)
{
    // A top-level string may be a data-bound expression that expands into an array
    if (value.isString()) {
        Object v = evaluate(context, value);
        if (v.isArray())
            return v.getArray();

        return std::vector<Object>({v});
    }

    if (value.isArray()) {
        std::vector<Object> result;
        for (const auto& m : value.getArray()) {
            if (m.isString()) {
                Object v = evaluate(context, m);
                if (v.isArray()) {
                    for (const auto& n : v.getArray())
                        result.push_back(n);
                }
                else
                    result.push_back(v);
            }
            else {
                result.push_back(m);
            }
        }
        return result;
    }

    // Any other type of object gets returned as [ value ]
    return std::vector<Object>({value});
}


std::vector<Object>
deepArrayifyProperty(const Context& context, const Object& value)
{
    return std::vector<Object>();
}

std::vector<Object>
deepArrayify(const Context& context, const Object& value)
{
    // A top-level string may be a data-bound expression that expands into an array
    if (value.isString()) {
        Object v = evaluate(context, value);
        if (v.isArray())
            return v.getArray();

        return std::vector<Object>({v});
    }

    if (value.isArray()) {
        std::vector<Object> result;
        for (const auto& m : value.getArray()) {
            if (m.isString()) {
                Object v = evaluate(context, m);
                if (v.isArray()) {
                    for (const auto& n : v.getArray())
                        result.push_back(n);
                }
                else
                    result.push_back(v);
            }
            else {
                result.push_back(evaluateRecursive(context, m));
            }
        }
        return result;
    }

    // Any other type of object gets evaluated recursively
    return std::vector<Object>({evaluateRecursive(context, value)});
}



} // namespace apl