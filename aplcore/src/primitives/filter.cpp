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

#include "apl/primitives/filter.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/propdef.h"
#include "apl/utils/session.h"

namespace apl {

Bimap<FilterType , std::string> sFilterTypeBimap = {
    {kFilterTypeBlur,  "Blur"},
    {kFilterTypeNoise, "Noise"}
};

Bimap<int, std::string> sFilterPropertyBimap = {
    {kFilterPropertyUseColor, "useColor"},
    {kFilterPropertyKind,     "kind"},
    {kFilterPropertyRadius,   "radius"},
    {kFilterPropertySigma,    "sigma"},
};

Bimap<int, std::string> sFilterNoiseKindBimap = {
    {kFilterNoiseKindGaussian, "gaussian"},
    {kFilterNoiseKindUniform,  "uniform"},
};

using FilterPropDef = PropDef<FilterProperty, sFilterPropertyBimap>;

static const std::map<FilterType, std::vector<FilterPropDef>>&
getFilterMap() {
    static std::map<FilterType, std::vector<FilterPropDef>> sFilterDefinitions = {
        {kFilterTypeBlur,  {
                               {kFilterPropertyRadius,   Dimension(0),    asNonNegativeAbsoluteDimension}}},
        {kFilterTypeNoise, {
                               {kFilterPropertyUseColor, Object::FALSE_OBJECT(), asBoolean},
                               {kFilterPropertyKind, kFilterNoiseKindGaussian, sFilterNoiseKindBimap},
                               {kFilterPropertySigma, 10, asNonNegativeNumber}}},
    };
    return sFilterDefinitions;
}


Object
Filter::create(const Context& context, const Object& object)
{
    if (object.isFilter())
        return object;

    if (!object.isMap())
        return Object::NULL_OBJECT();

    auto type = propertyAsMapped<FilterType>(context, object, "type", kFilterTypeBlur, sFilterTypeBimap);
    if (type == static_cast<FilterType>(-1)) {
        CONSOLE_CTX(context) << "Unrecognized type field in filter";
        return Object::NULL_OBJECT();
    }

    FilterBag bag;
    for (const auto& property : getFilterMap().at(type)) {
        auto value = property.defvalue;
        if (object.has(property.name))
            value = property.calculate(context, evaluate(context, object.get(property.name)));
        if (value == Object::NULL_OBJECT()) {
            CONSOLE_CTX(context) << "Invalid value for filter property " << property.name;
            return Object::NULL_OBJECT();
        }
        bag.emplace(property.key, value);
    }

    return Object(Filter(type, std::move(bag)));
}

const Object
Filter::getValue(FilterProperty key) const
{
    auto it = mData.find(key);
    if (it == mData.end())
        return Object::NULL_OBJECT();

    return it->second;
}

bool
Filter::operator==(const apl::Filter& rhs) const
{
    if (mType != rhs.mType) return false;

    for (const auto& m : mData) {
        if (m.second != rhs.mData.at(m.first))
            return false;
    }

    return true;
}

rapidjson::Value
Filter::serialize(rapidjson::Document::AllocatorType& allocator) const {
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);
    v.AddMember("type", static_cast<int>(getType()), allocator);

    for (auto& m : mData) {
        v.AddMember( rapidjson::StringRef(sFilterPropertyBimap.at(m.first).c_str()),
            m.second.serialize(allocator),
            allocator);
    }
    return v;
}


} // namespace apl