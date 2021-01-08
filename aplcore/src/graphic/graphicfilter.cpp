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

#include "apl/graphic/graphicfilter.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/propdef.h"
#include "apl/utils/session.h"

namespace apl {

Bimap<GraphicFilterType , std::string> sGraphicFilterTypeBimap = {
    {kGraphicFilterTypeDropShadow,     "DropShadow"},
};

Bimap<int, std::string> sGraphicFilterPropertyBimap = {
    {kGraphicPropertyFilterColor,               "color"},
    {kGraphicPropertyFilterHorizontalOffset,    "horizontalOffset"},
    {kGraphicPropertyFilterRadius,              "radius"},
    {kGraphicPropertyFilterVerticalOffset,      "verticalOffset"},
};

using GraphicFilterPropDef = PropDef<GraphicFilterProperty, sGraphicFilterPropertyBimap>;

/**
 * Static definition of all built-in graphic filters
 */
static const std::map<GraphicFilterType, std::vector<GraphicFilterPropDef>>&
getGraphicFilterMap() {
    static std::vector<GraphicFilterPropDef> sDropShadowFilterProperties = {
        {kGraphicPropertyFilterColor,               Color::BLACK,   asColor,             kPropInOut},
        {kGraphicPropertyFilterHorizontalOffset,    0,              asNumber,            kPropInOut},
        {kGraphicPropertyFilterRadius,              0,              asNonNegativeNumber, kPropInOut},
        {kGraphicPropertyFilterVerticalOffset,      0,              asNumber,            kPropInOut},
    };

    static std::map<GraphicFilterType, std::vector<GraphicFilterPropDef>> sGraphicFilterDefinitions = {
        {kGraphicFilterTypeDropShadow,     sDropShadowFilterProperties},
    };

    return sGraphicFilterDefinitions;
}

static Object
calculateNormal(const GraphicFilterPropDef& def,
                const Context& context,
                const Properties& properties)
{
    auto p = properties.find(def.names);
    if (p == properties.end())
        return def.defvalue;

    Object tmp = evaluate(context, p->second);
    if (def.map) {
        int value = def.map->get(tmp.asString(), -1);
        if (value != -1)
            return value;

        CONSOLE_CTX(context) << "Invalid value for graphic filter property " << def.names[0] << ": " << tmp.asString();
        return def.defvalue;
    }

    return def.func(context, tmp);
}

Object
GraphicFilter::create(const Context& context, const Object& object)
{
    if (object.isGraphicFilter())
        return object;

    if (!object.isMap())
        return Object::NULL_OBJECT();

    auto typeName = propertyAsString(context, object, "type");
    if (typeName.empty()) {
        CONSOLE_CTX(context) << "No 'type' property defined for graphic filter";
        return Object::NULL_OBJECT();
    }

    Properties properties(object);

    // Check graphic filter is valid
    if(sGraphicFilterTypeBimap.has(typeName)) {
        auto type = sGraphicFilterTypeBimap.at(typeName);
        std::map<int, Object> data;
        for (const auto& def : getGraphicFilterMap().at(type))
            data.emplace(def.key, calculateNormal(def, context, properties));
        return Object(GraphicFilter(type, std::move(data)));
    }

    CONSOLE_CTX(context) << "Unable to find graphic filter named '" << typeName << "'";
    return Object::NULL_OBJECT();
}

Object
GraphicFilter::getValue(GraphicFilterProperty key) const
{
    auto it = mData.find(key);
    if (it == mData.end())
        return Object::NULL_OBJECT();

    return it->second;
}

bool
GraphicFilter::operator==(const apl::GraphicFilter& rhs) const
{
    return (mType == rhs.mType && mData == rhs.mData);
}

rapidjson::Value
GraphicFilter::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);

    v.AddMember("type", static_cast<int>(mType), allocator);
    for (auto& m : mData)
        v.AddMember(rapidjson::StringRef(sGraphicFilterPropertyBimap.at(m.first).c_str()),
                    m.second.serialize(allocator),
                    allocator);
    return v;
}

std::string
GraphicFilter::toDebugString() const
{
    std::string result = "GraphicFilter<" + sGraphicFilterTypeBimap.at(mType);
    for (const auto& m : mData)
        result += " " + sGraphicFilterPropertyBimap.at(m.first) + ":" + m.second.toDebugString();
    result += ">";
    return result;
}

} // namespace apl
