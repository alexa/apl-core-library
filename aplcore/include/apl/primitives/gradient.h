/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef _APL_GRADIENT_H
#define _APL_GRADIENT_H

#include <vector>

#include "color.h"

namespace apl {

class Object;
class Context;

/**
 * Represent a linear or radial gradient. Normally used in the Image for the
 * overlayGradient. Because gradients may be defined in a resource, we treat
 * them as a primitive type and place them inside of Objects.
 */
class Gradient {
public:
    enum GradientType {
        /** Linear gradient */
        LINEAR,
        /** Radial gradient, centered about the center of an object */
        RADIAL
    };

    /**
     * Build a gradient from an Object. The source object may be a gradient (in
     * which case it is copied) or may be a JSON representation of a gradient.
     * @param context The data-binding context.
     * @param object The source of the gradient.
     * @return An object containing a gradient or null.
     */
    static Object create(const Context& context, const Object& object);

    /**
     * @return The type of the gradient.
     */
    GradientType getType() const { return mType; }

    /**
     * @return The angle of the gradient, expressed in degrees.  Only applies
     *         to linear gradients.  0 is up, 90 is to the right, 180 is down
     *         and 270 is to the left.
     */
    double getAngle() const { return mAngle; }

    /**
     * @return The vector of color stops.
     */
    const std::vector<Color> getColorRange() const { return mColorRange; }

    /**
     * @return The vector of input stops.  These are the values of the color
     *         stops. They are guaranteed to be in ascending numerical order in
     *         the range [0,1].
     */
    const std::vector<double> getInputRange() const { return mInputRange; }

    std::string asString() const {
        return "gradient<>";
    }

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

private:
    Gradient(GradientType type, double angle,
                std::vector<Color>&& colorRange, std::vector<double>&& inputRange) :
        mType(std::move(type)),
        mAngle(std::move(angle)),
        mColorRange(std::move(colorRange)),
        mInputRange(std::move(inputRange))
    {}

private:
    GradientType mType;
    double mAngle;
    std::vector<Color> mColorRange;
    std::vector<double> mInputRange;
};

} // namespace apl

#endif //_APL_GRADIENT_H
