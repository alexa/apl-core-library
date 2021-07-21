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

#include "apl/extension/extensionmanager.h"
#include "apl/primitives/filter.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/propdef.h"
#include "apl/utils/session.h"

namespace apl {

Bimap<FilterType , std::string> sFilterTypeBimap = {
    {kFilterTypeBlend,     "Blend"},
    {kFilterTypeBlur,      "Blur"},
    {kFilterTypeColor,     "Color"},
    {kFilterTypeExtension, "Extension"},
    {kFilterTypeGradient,  "Gradient"},
    {kFilterTypeGrayscale, "Grayscale"},
    {kFilterTypeNoise,     "Noise"},
    {kFilterTypeSaturate,  "Saturate"},
};

Bimap<int, std::string> sFilterPropertyBimap = {
    {kFilterPropertyAmount,       "amount"},
    {kFilterPropertyColor,        "color"},
    {kFilterPropertyDestination,  "destination"},
    {kFilterPropertyExtension,    "extension"},
    {kFilterPropertyExtensionURI, "extensionURI"},
    {kFilterPropertyGradient,     "gradient"},
    {kFilterPropertyKind,         "kind"},
    {kFilterPropertyMode,         "mode"},
    {kFilterPropertyName,         "name"},
    {kFilterPropertyRadius,       "radius"},
    {kFilterPropertySigma,        "sigma"},
    {kFilterPropertySource,       "source"},
    {kFilterPropertyUseColor,     "useColor"},
};

Bimap<int, std::string> sFilterNoiseKindBimap = {
    {kFilterNoiseKindGaussian, "gaussian"},
    {kFilterNoiseKindUniform,  "uniform"},
};

Bimap<int, std::string> sBlendModeBimap = {
    {kBlendModeNormal,     "normal"},
    {kBlendModeMultiply,   "multiply"},
    {kBlendModeScreen,     "screen"},
    {kBlendModeOverlay,    "overlay"},
    {kBlendModeDarken,     "darken"},
    {kBlendModeLighten,    "lighten"},
    {kBlendModeColorDodge, "color-dodge"},
    {kBlendModeColorDodge, "colorDodge"},
    {kBlendModeColorBurn,  "color-burn"},
    {kBlendModeColorBurn,  "colorBurn"},
    {kBlendModeHardLight,  "hard-light"},
    {kBlendModeHardLight,  "hardLight"},
    {kBlendModeSoftLight,  "soft-light"},
    {kBlendModeSoftLight,  "softLight"},
    {kBlendModeDifference, "difference"},
    {kBlendModeExclusion,  "exclusion"},
    {kBlendModeHue,        "hue"},
    {kBlendModeSaturation, "saturation"},
    {kBlendModeColor,      "color"},
    {kBlendModeLuminosity, "luminosity"},
};

using FilterPropDef = PropDef<FilterProperty, sFilterPropertyBimap>;

/**
 * Static definition of all built-in filters
 */
static const std::map<FilterType, std::vector<FilterPropDef>>&
getFilterMap() {
    static std::vector<FilterPropDef> sBlendFilterProperties = {
        {kFilterPropertyDestination, -2,               asInteger},
        {kFilterPropertyMode,        kBlendModeNormal, sBlendModeBimap},
        {kFilterPropertySource,      -1,               asInteger}
    };

    static std::vector<FilterPropDef> sBlurFilterProperties = {
        {kFilterPropertyRadius, Dimension(0), asNonNegativeAbsoluteDimension},
        {kFilterPropertySource, -1,           asInteger},
    };

    static std::vector<FilterPropDef> sColorFilterProperties = {
        {kFilterPropertyColor, Color(), asColor},
    };

    static std::vector<FilterPropDef> sGradientFilterProperties = {
        {kFilterPropertyGradient, Object::NULL_OBJECT(), asGradient},
    };

    static std::vector<FilterPropDef> sGrayscaleFilterProperties = {
        {kFilterPropertyAmount, 0, asOpacity},  // Note: Opacity bounds in range [0, 1]
        {kFilterPropertySource, -1,  asInteger},
    };

    static std::vector<FilterPropDef> sNoiseFilterProperties = {
        {kFilterPropertyKind,     kFilterNoiseKindGaussian, sFilterNoiseKindBimap},
        {kFilterPropertySigma,    10,                       asNonNegativeNumber},
        {kFilterPropertySource,   -1,                       asInteger},
        {kFilterPropertyUseColor, Object::FALSE_OBJECT(),   asBoolean},
    };

    static std::vector<FilterPropDef> sSaturateFilterProperties = {
        {kFilterPropertyAmount, 1.0, asNonNegativeNumber},
        {kFilterPropertySource, -1,  asInteger},
    };

    static std::map<FilterType, std::vector<FilterPropDef>> sFilterDefinitions = {
        {kFilterTypeBlend,     sBlendFilterProperties},
        {kFilterTypeBlur,      sBlurFilterProperties},
        {kFilterTypeColor,     sColorFilterProperties},
        {kFilterTypeGradient,  sGradientFilterProperties},
        {kFilterTypeGrayscale, sGrayscaleFilterProperties},
        {kFilterTypeNoise,     sNoiseFilterProperties},
        {kFilterTypeSaturate,  sSaturateFilterProperties},
    };

    return sFilterDefinitions;
}

static Object
calculateNormal(const FilterPropDef& def,
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

        CONSOLE_CTX(context) << "Invalid value for filter property " << def.names[0] << ": " << tmp.asString();
        return def.defvalue;
    }

    return def.func(context, tmp);
}

static Object
calculateExtended(const std::string& name,
                  const ExtensionFilterDefinition::Property& def,
                  const Context& context,
                  const Properties& properties)
{
    auto p = properties.find(name);
    if (p == properties.end())
        return def.defaultValue;

    const auto& bfunc = sBindingFunctions.at(def.bindingType);
    return bfunc(context, evaluate(context, p->second));
}

Object
Filter::create(const Context& context, const Object& object)
{
    if (object.isFilter())
        return object;

    if (!object.isMap())
        return Object::NULL_OBJECT();

    auto typeName = propertyAsString(context, object, "type");
    if (typeName.empty()) {
        CONSOLE_CTX(context) << "No 'type' property defined for filter";
        return Object::NULL_OBJECT();
    }

    Properties properties(object);

    // Check for a built-in filter
    auto type = sFilterTypeBimap.get(typeName, kFilterTypeExtension);
    if (type != kFilterTypeExtension) {
        std::map<int, Object> data;
        for (const auto& def : getFilterMap().at(type))
            data.emplace(def.key, calculateNormal(def, context, properties));
        return Object(Filter(type, std::move(data)));
    }

    // Check for a custom filter (this returns a pointer to an ExtensionFilterDefinition)
    auto efdp = context.extensionManager().findFilterDefinition(typeName);
    if (efdp) {
        // Put all custom extension properties into a local map under the kFilterPropertyExtension index
        auto extensionData = std::make_shared<ObjectMap>();
        for (const auto& def : efdp->getPropertyMap())
            extensionData->emplace(def.first, calculateExtended(def.first, def.second, context, properties));

        std::map<int, Object> data = {
            {kFilterPropertyExtension,    extensionData},
            {kFilterPropertyExtensionURI, efdp->getURI()},
            {kFilterPropertyName,         efdp->getName()},
        };

        // If there is one or two images, add a source property
        if (efdp->getImageCount() != ExtensionFilterDefinition::ZERO) {
            auto source = propertyAsInt(context, object, "source", -1);
            data.emplace(kFilterPropertySource, source);
        }

        // If there are two images, add a destination property
        if (efdp->getImageCount() == ExtensionFilterDefinition::TWO) {
            auto destination = propertyAsInt(context, object, "destination", -2);
            data.emplace(kFilterPropertyDestination, destination);
        }

        return Object(Filter(kFilterTypeExtension, std::move(data)));
    }

    CONSOLE_CTX(context) << "Unable to find filter named '" << typeName << "'";
    return Object::NULL_OBJECT();
}

Object
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
    return (mType == rhs.mType && mData == rhs.mData);
}

rapidjson::Value
Filter::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);

    v.AddMember("type", static_cast<int>(mType), allocator);
    for (auto& m : mData)
        v.AddMember(rapidjson::StringRef(sFilterPropertyBimap.at(m.first).c_str()),
                    m.second.serialize(allocator),
                    allocator);
    return v;
}

std::string
Filter::toDebugString() const
{
    std::string result = "Filter<" + sFilterTypeBimap.at(mType);
    for (const auto& m : mData)
        result += " " + sFilterPropertyBimap.at(m.first) + ":" + m.second.toDebugString();
    result += ">";
    return result;
}


} // namespace apl
