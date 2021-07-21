/*
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
 * ParameterArray
 *
 * The list of parameters that can be bound to values in a layout is a parameter array.
 * Each item in the list may be a single string (indicating a property name) or a
 * object with a name, type, and default value.  This data structure holds the parsed
 * version of the parameter array.
 */

#ifndef _APL_PARAMETER_ARRAY
#define _APL_PARAMETER_ARRAY

#include <vector>

#include "apl/engine/binding.h"
#include "apl/primitives/object.h"

namespace apl {

struct Parameter {
    Parameter(const std::string& name, BindingType type, const Object& defvalue)
        : name(name),
          type(type),
          defvalue(defvalue) {}

    std::string name;
    BindingType type;
    Object defvalue;
};

class ParameterArray {
public:
    ParameterArray(const rapidjson::Value& layout);

    size_t size() const { return mArray.size(); }
    const Parameter& at(size_t index) const { return mArray.at(index); }

    std::vector<Parameter>::iterator begin() { return mArray.begin(); }
    std::vector<Parameter>::iterator end() { return mArray.end(); }

private:
    std::vector<Parameter> mArray;
};

} // namespace apl

#endif // _APL_PARAMETER_ARRAY