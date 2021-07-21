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

#ifndef _APL_FILTER_H
#define _APL_FILTER_H

#include "objectbag.h"

namespace apl {

/**
 * Enumeration of filter types
 */
enum FilterType {
    kFilterTypeBlend,
    kFilterTypeBlur,
    kFilterTypeColor,
    kFilterTypeExtension,
    kFilterTypeGradient,
    kFilterTypeGrayscale,
    kFilterTypeNoise,
    kFilterTypeSaturate,
};

enum FilterProperty {
    /// Amount (used in Grayscale, Saturate)
    kFilterPropertyAmount,
    /// Solid color
    kFilterPropertyColor,
    /// Destination image index
    kFilterPropertyDestination,
    /// Extension properties
    kFilterPropertyExtension,
    /// URI of the extension filter
    kFilterPropertyExtensionURI,
    /// Gradient
    kFilterPropertyGradient,
    /// Noise type enumerated value
    kFilterPropertyKind,
    /// Blend mode
    kFilterPropertyMode,
    /// Name of the extension filter
    kFilterPropertyName,
    /// Blur radius (dimension)
    kFilterPropertyRadius,
    /// Noise standard deviation (number)
    kFilterPropertySigma,
    /// Source image index
    kFilterPropertySource,
    /// Noise use color flag (boolean)
    kFilterPropertyUseColor,
};

enum NoiseFilterKind {
    kFilterNoiseKindUniform,
    kFilterNoiseKindGaussian
};

enum BlendMode {
    kBlendModeNormal,
    kBlendModeMultiply,
    kBlendModeScreen,
    kBlendModeOverlay,
    kBlendModeDarken,
    kBlendModeLighten,
    kBlendModeColorDodge,
    kBlendModeColorBurn,
    kBlendModeHardLight,
    kBlendModeSoftLight,
    kBlendModeDifference,
    kBlendModeExclusion,
    kBlendModeHue,
    kBlendModeSaturation,
    kBlendModeColor,
    kBlendModeLuminosity
};

extern Bimap<FilterType, std::string> sFilterTypeBimap;
extern Bimap<int, std::string> sFilterPropertyBimap;
extern Bimap<int, std::string> sFilterNoiseKindBimap;
extern Bimap<int, std::string> sBlendModeBimap;

/**
 * An generic image-processing filter applied against a bitmap.  Each filter
 * must have a valid type and an optional collection of properties.
 *
 * See notes in <extensionfilterdefinition.h> for how custom filters are defined.
 */
class Filter {
public:
    /**
     * Build a filter from an Object.  The source object may be a filter (in which
     * case it is copied) or it may be a JSON representation of a filter.
     * @param context The data-binding context.
     * @param object The source of the filter.
     * @return An object containing a filter or null
     */
    static Object create(const Context& context, const Object& object);

    /**
     * @return The type of the filter
     */
    FilterType getType() const { return mType; }

    /**
     * @return True if this filter is defined from an extension
     */
    bool isExtensionFilter() const { return mType == kFilterTypeExtension; }

    /**
     * Retrieve a property from a filter.
     * @param key The property to retrieve
     * @return The value or null if it doesn't exist
     */
    Object getValue(FilterProperty key) const;

    /* Standard Object methods */
    bool operator==(const Filter& rhs) const;

    std::string toDebugString() const;

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    bool empty() const { return false; }

    bool truthy() const { return true; }

private:
    Filter(FilterType type, std::map<int, Object>&& data) : mType(type), mData(std::move(data)) {}

private:
    FilterType mType;
    std::map<int, Object> mData;
};

} // namespace apl

#endif //_APL_FILTER_H
