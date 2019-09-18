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

#ifndef _APL_FILTER_H
#define _APL_FILTER_H

#include "objectbag.h"

namespace apl {

/**
 * Enumeration of filter types
 */
enum FilterType {
    /// Gaussian Blur
    kFilterTypeBlur,
    /// Generated Noise
    kFilterTypeNoise,
};

enum FilterProperty {
    /// Noise color flag (boolean)
    kFilterPropertyUseColor,
    /// Noise type enumerated value
    kFilterPropertyKind,
    /// Blur radius (dimension)
    kFilterPropertyRadius,
    /// Noise standard deviation (number)
    kFilterPropertySigma
};

enum NoiseFilterKind {
    kFilterNoiseKindUniform,
    kFilterNoiseKindGaussian
};

extern Bimap<FilterType, std::string> sFilterTypeBimap;
extern Bimap<int, std::string> sFilterPropertyBimap;
extern Bimap<int, std::string> sFilterNoiseKindBimap;

using FilterBag = ObjectBag<sFilterPropertyBimap>;

/**
 * An generic image-processing filter applied against a bitmap.  Each filter
 * must have a valid type and an optional collection of properties.  Currently
 * we only support the "Blur" filter, which is a Gaussian blur with a specified
 * radius as a dimension.
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
     * Retrieve a property from a filter.
     * @param key The property to retrieve
     * @return The value or null if it doesn't exist
     */
    const Object getValue(FilterProperty key) const;

    /**
     * Serialize this into JSON format
     * @param allocator
     * @return
     */
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    bool operator==(const Filter& rhs) const;

private:
    Filter(FilterType type, FilterBag&& bag) : mType(type), mData(std::move(bag)) {}

    FilterType mType;
    FilterBag mData;
};

} // namespace apl

#endif //_APL_FILTER_H
