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

#include "apl/engine/parameterarray.h"
#include "apl/primitives/dimension.h"
#include "apl/engine/arrayify.h"

namespace apl {

// Convert a named property to a valid item from a map
template<class T> T
extractMapped(const rapidjson::Value& object,
              const char *name,
              T defValue)
{
    assert(object.IsObject());

    auto it = object.FindMember(name);
    if (it == object.MemberEnd() || !it->value.IsString())
        return defValue;

    return sBindingMap.get(it->value.GetString(), defValue);
}


ParameterArray::ParameterArray(const rapidjson::Value& layout)
{
    for (const auto& param : arrayifyProperty(layout, "parameters")) {
        if (param.IsString()) {
            mArray.emplace_back(param.GetString(), kBindingTypeAny, Object::NULL_OBJECT());
        }
        else if (param.IsObject()) {
            auto it = param.FindMember("name");
            if (it != param.MemberEnd() && it->value.IsString()) {
                BindingType type = extractMapped(param, "type", kBindingTypeAny);
                Object defvalue = Object::NULL_OBJECT();
                auto dv = param.FindMember("default");
                if (dv != param.MemberEnd())
                    defvalue = Object(dv->value);
                mArray.emplace_back(it->value.GetString(), type, std::move(defvalue));
            }
        }
    }
}


} // namespace apl