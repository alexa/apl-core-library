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
 */

#ifndef _APL_DIMENSION_H
#define _APL_DIMENSION_H

#include <string>
#include "apl/utils/streamer.h"

namespace apl {

class Context;

/**
 * The type of the dimension.
 */
enum class DimensionType {
    /// An absolute, measurable unit.  Stored internally as dp.
    Absolute,
    /// A size, stored as a percentage of the parent component.
    Relative,
    /// Automatically size to fit the contents.
    Auto
};

/**
 * A dimension may be absolute, relative, or auto.
 */
class Dimension
{
public:
    /**
     * Creates an auto-sized dimension.
     */
    Dimension() : mType(DimensionType::Auto), mValue(0) {}

    /**
     * Creates an absolute dimension of a specific size.
     * @param size Size, in display-independent pixels.
     */
    Dimension(double size) : mType(DimensionType::Absolute), mValue(size) {}

    /**
     * General constructor for building any type of dimension.
     * @param type The dimension type.
     * @param value The dimension size.  Ignored for auto.  Relative sizes should be in the range [0, 1].
     */
    Dimension(DimensionType type, double value) : mType(type), mValue(value) {}

    /**
     * Construct a dimension by parsing a string.  Accepts units such as "22px", "3dp", "auto", and "10%".
     * @param context The data-binding context.
     * @param str The string to parse.
     */
    Dimension(const Context& context, const std::string& str, bool preferRelative=false);

    // Copy, move, and assignment
    Dimension(const Dimension& rhs) = default;
    Dimension(Dimension&& rhs) = default;
    Dimension& operator=(const Dimension& rhs) = default;

    // equality
    bool operator==(const Dimension& rhs) const {
        return rhs.getType() == getType() && rhs.getValue() == getValue();
    }
    bool operator!=(const Dimension& rhs) const {
        return rhs.getType() != getType() || rhs.getValue() != getValue();
    }

    /**
     * @return True if this is an "auto" dimension.
     */
    bool isAuto() const { return mType == DimensionType::Auto; }

    /**
     * @return True if this is a relative dimension.
     */
    bool isRelative() const { return mType == DimensionType::Relative; }

    /**
     * @return True if this is an absolute dimension
     */
    bool isAbsolute() const { return mType == DimensionType::Absolute; }

    /**
     * @return The internal value of the dimension.  Undefined for auto dimensions.
     */
    double getValue() const { return mValue; }

    /**
     * @return The type of the dimension (auto, relative, absolute).
     */
    DimensionType getType() const { return mType; }

    friend streamer& operator<<(streamer&, const Dimension&);

private:
    DimensionType mType;
    double mValue;
};

}  // namespace apl

#endif // _APL_DIMENSION_H
