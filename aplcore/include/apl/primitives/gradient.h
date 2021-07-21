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

#ifndef _APL_GRADIENT_H
#define _APL_GRADIENT_H

#include <vector>

#include "color.h"

namespace apl {

class Object;
class Context;

enum GradientProperty {
    kGradientPropertyType,
    kGradientPropertyColorRange,
    kGradientPropertyInputRange,
    kGradientPropertyAngle,
    kGradientPropertySpreadMethod,
    kGradientPropertyX1,
    kGradientPropertyY1,
    kGradientPropertyX2,
    kGradientPropertyY2,
    kGradientPropertyCenterX,
    kGradientPropertyCenterY,
    kGradientPropertyRadius,
    kGradientPropertyUnits
};

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

    /** Possible linear gradient spread methods. */
    enum GradientSpreadMethod {
        PAD,
        REFLECT,
        REPEAT
    };

    /** Gradient units */
    enum GradientUnits {
        kGradientUnitsBoundingBox,
        kGradientUnitsUserSpace
    };

    /**
     * Build a gradient from an Object. The source object may be a gradient (in
     * which case it is copied) or may be a JSON representation of a gradient.
     * @param context The data-binding context.
     * @param object The source of the gradient.
     * @return An object containing a gradient or null.
     */
    static Object create(const Context& context, const Object& object) { return create(context, object, false); }

    /**
     * Build a AVG gradient from an Object. The source object may be a gradient (in
     * which case it is copied) or may be a JSON representation of a gradient.
     * @param context The data-binding context.
     * @param object The source of the gradient.
     * @return An object containing a gradient or null.
     */
    static Object createAVG(const Context& context, const Object& object) { return create(context, object, true); }

    /**
     * @return The type of the gradient.
     */
    GradientType getType() const { return static_cast<GradientType>(mProperties.at(kGradientPropertyType).getInteger()); }

    /**
     * @deprecated use getProperty(kGradientPropertyAngle) instead.
     * @return The angle of the gradient, expressed in degrees.  Only applies
     *         to linear gradients.  0 is up, 90 is to the right, 180 is down
     *         and 270 is to the left.
     */
    double getAngle() const {
        if (getType() != LINEAR) {
            return 0;
        }
        return mProperties.at(kGradientPropertyAngle).getDouble();
    }

    /**
     * @deprecated use getProperty(kGradientPropertyColorRange) instead.
     * @return The vector of color stops.
     */
    const std::vector<Color> getColorRange() const { return mColorRange; }

    /**
     * @deprecated use getProperty(kGradientPropertyInputRange) instead.
     * @return The vector of input stops.  These are the values of the color
     *         stops. They are guaranteed to be in ascending numerical order in
     *         the range [0,1].
     */
    const std::vector<double> getInputRange() const { return mInputRange; }

    /**
     * @param key property key
     * @return gradient property.
     */
    Object getProperty(GradientProperty key) const {
        const auto& prop = mProperties.find(key);
        if (prop != mProperties.end()) {
            return prop->second;
        }
        return Object::NULL_OBJECT();
    }

    /* Standard Object methods */
    bool operator==(const Gradient& other) const;

    std::string toDebugString() const;
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    bool empty() const { return false; }
    bool truthy() const { return true; }

private:
    Gradient(std::map<GradientProperty, Object>&& properties);

    static Object create(const Context& context, const Object& object, bool avg);

private:
    std::vector<Color> mColorRange;
    std::vector<double> mInputRange;
    std::map<GradientProperty, Object> mProperties;
};

} // namespace apl

#endif //_APL_GRADIENT_H
