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

#include "apl/engine/binding.h"
#include "apl/primitives/dimension.h"
#include "apl/primitives/object.h"

namespace apl {

const Bimap<BindingType, std::string> sBindingMap = {
    {kBindingTypeAny, "any"},
    {kBindingTypeArray, "array"},
    {kBindingTypeBoolean, "boolean"},
    {kBindingTypeColor, "color"},
    {kBindingTypeComponent, "component"},
    {kBindingTypeDimension, "dimension"},
    {kBindingTypeInteger, "integer"},
    {kBindingTypeMap, "map"},
    {kBindingTypeNumber, "number"},
    {kBindingTypeObject, "object"},
    {kBindingTypeString, "string"},
};

const std::map<BindingType, BindingFunction> sBindingFunctions = {
    {kBindingTypeAny,       [](const Context& c, const Object& v) { return v; }},
    {kBindingTypeArray,     [](const Context& c, const Object& v) { return v; }},
    {kBindingTypeBoolean,   [](const Context& c, const Object& v) { return v.asBoolean(); }},
    {kBindingTypeColor,     [](const Context& c, const Object& v) { return v.asColor(c); }},
    {kBindingTypeComponent, [](const Context& c, const Object& v) { return v; }},
    {kBindingTypeDimension, [](const Context& c, const Object& v) { return v.asDimension(c); }},
    {kBindingTypeInteger,   [](const Context& c, const Object& v) { return std::round(v.asNumber()); }},
    {kBindingTypeMap,       [](const Context& c, const Object& v) { return v; }},
    {kBindingTypeNumber,    [](const Context& c, const Object& v) { return v.asNumber(); }},
    {kBindingTypeObject,    [](const Context& c, const Object& v) { return v; }},
    {kBindingTypeString,    [](const Context& c, const Object& v) { return v.asString(); }},
};


} // namespace apl
