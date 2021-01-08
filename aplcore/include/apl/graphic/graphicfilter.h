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

#ifndef _APL_GRAPHIC_FILTER_H
#define _APL_GRAPHIC_FILTER_H

#include "apl/primitives/object.h"
#include "apl/utils/bimap.h"

namespace apl {

/**
 * Enumeration of graphic filter types
 */
enum GraphicFilterType {
    kGraphicFilterTypeDropShadow,
};

enum GraphicFilterProperty {
    /// Color of the shadow
    kGraphicPropertyFilterColor,
    /// Horizontal offset of the shadow
    kGraphicPropertyFilterHorizontalOffset,
    /// Blur radius of the shadow
    kGraphicPropertyFilterRadius,
    /// Vertical offset of the shadow
    kGraphicPropertyFilterVerticalOffset,
};

extern Bimap<GraphicFilterType, std::string> sGraphicFilterTypeBimap;
extern Bimap<int, std::string> sGraphicFilterPropertyBimap;

/**
 * Vector Graphic specific filter applied against a bitmap. Each filter
 * must have a valid type and an optional collection of properties.
 */
class GraphicFilter {
public:
    /**
     * Build a graphic filter from an Object.  The source object may be a graphic filter (in which
     * case it is copied) or it may be a JSON representation of a filter.
     * @param context The data-binding context.
     * @param object The source of the graphic filter.
     * @return An object containing a graphic filter or null
     */
    static Object create(const Context& context, const Object& object);

    /**
     * @return The type of the graphic filter
     */
    GraphicFilterType getType() const { return mType; }

    /**
     * Retrieve a property from a graphic filter.
     * @param key The property to retrieve
     * @return The value or null if it doesn't exist
     */
    Object getValue(GraphicFilterProperty key) const;

    /* Standard Object methods */
    bool operator==(const GraphicFilter& rhs) const;

    std::string toDebugString() const;

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    bool empty() const { return false; }

    bool truthy() const { return true; }

private:
    GraphicFilter(GraphicFilterType type, std::map<int, Object>&& data) : mType(type), mData(std::move(data)) {}

private:
    GraphicFilterType mType;
    std::map<int, Object> mData;
};

} // namespace apl

#endif //_APL_GRAPHIC_FILTER_H
