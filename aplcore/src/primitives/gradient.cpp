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

#include "apl/engine/evaluate.h"
#include "apl/engine/arrayify.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"
#include "apl/primitives/object.h"
#include "apl/primitives/gradient.h"

namespace apl {

const Bimap<Gradient::GradientType, std::string> sGradientTypeMap = {
    {Gradient::LINEAR, "linear"},
    {Gradient::RADIAL, "radial"}
};

const Bimap<Gradient::GradientSpreadMethod, std::string> sGradientSpreadMethodMap = {
    {Gradient::PAD,     "pad"},
    {Gradient::REFLECT, "reflect"},
    {Gradient::REPEAT,  "repeat"},
};

const Bimap<Gradient::GradientUnits, std::string> sGradientUnitsMap = {
    {Gradient::kGradientUnitsBoundingBox, "boundingBox"},
    {Gradient::kGradientUnitsUserSpace,   "userSpace"},
};

const Bimap<int, std::string> sGradientPropertiesMap = {
    {kGradientPropertyType,         "type"},
    {kGradientPropertyColorRange,   "colorRange"},
    {kGradientPropertyInputRange,   "inputRange"},
    {kGradientPropertyAngle,        "angle"},
    {kGradientPropertySpreadMethod, "spreadMethod"},
    {kGradientPropertyX1,           "x1"},
    {kGradientPropertyX2,           "x2"},
    {kGradientPropertyY1,           "y1"},
    {kGradientPropertyY2,           "y2"},
    {kGradientPropertyCenterX,      "centerX"},
    {kGradientPropertyCenterY,      "centerY"},
    {kGradientPropertyRadius,       "radius"},
    {kGradientPropertyUnits,        "units"},
};


inline void convertAngleToCoordinates(double angle, double& x1, double& x2, double& y1, double& y2) {
    // Normalise to 0-360
    angle = angle - std::floor(angle / 360) * 360;
    double angleRads = angle * M_PI / 180;

    double st = sin(angleRads);
    double ct = cos(angleRads);

    // Piecewise functions.
    // https://www.desmos.com/calculator/34s7fnfsjf
    if (angle >= 0 && angle < 90) {
        x2 = (ct + st) * st / 2;
        y2 = (ct + st) * ct / 2;
    } else if (angle >= 90 && angle < 180) {
        x2 = (- ct + st) * st / 2;
        y2 = (- ct + st) * ct / 2;
    } else if (angle >= 180 && angle < 270) {
        x2 = - (ct + st) * st / 2;
        y2 = - (ct + st) * ct / 2;
    } else {
        x2 = (ct - st) * st / 2;
        y2 = (ct - st) * ct / 2;
    }

    // Shift to our target coordinates
    x2 += 0.5;
    y2 += 0.5;

    // Start is opposite
    x1 = 1 - x2;
    y1 = 1 - y2;
}

Object
Gradient::create(const Context& context, const Object& object, bool avg)
{
    if (object.isGradient())
        return object;

    if (!object.isMap())
        return Object::NULL_OBJECT();

    if (avg && !object.has("type")) {
        CONSOLE_CTX(context) << "Type field is required in AVG gradient";
        return Object::NULL_OBJECT();
    }

    auto type = propertyAsMapped<GradientType>(context, object, "type", LINEAR, sGradientTypeMap);
    if (type < 0) {
        CONSOLE_CTX(context) << "Unrecognized type field in gradient";
        return Object::NULL_OBJECT();
    }

    // Extract and evaluate the color range
    auto colorRange = arrayifyPropertyAsObject(context, object, "colorRange");
    auto length = colorRange.size();
    if (length < 2) {
        CONSOLE_CTX(context) << "Gradient does not have suitable color range";
        return Object::NULL_OBJECT();
    }

    // Extract and evaluate the input range
    auto inputRange = arrayifyPropertyAsObject(context, object, "inputRange");
    auto inputRangeLength = inputRange.size();
    if (inputRangeLength != 0 && inputRangeLength != length) {
        CONSOLE_CTX(context) << "Gradient input range must match the color range length";
        return Object::NULL_OBJECT();
    }

    std::map<GradientProperty, Object> properties;

    colorRange = evaluateRecursive(context, colorRange);

    std::vector<Object> colors;
    for (const auto& m : colorRange.getArray())
        colors.emplace_back(m.asColor(context));

    inputRange = evaluateRecursive(context, inputRange);

    std::vector<Object> inputs;
    if (!inputRange.empty()) {
        double last = 0;
        for (const auto& m : inputRange.getArray()) {
            double value = m.asNumber();
            if (value < last || value > 1) {
                CONSOLE_CTX(context) << "Gradient input range not in ascending order within range [0,1]";
                return Object::NULL_OBJECT();
            }

            inputs.emplace_back(value);
            last = value;
        }
    } else {
        for (int i = 0 ; i < length ; i++) {
            inputs.emplace_back(static_cast<double>(i) / (length - 1));
        }
    }

    properties.emplace(kGradientPropertyType, type);
    properties.emplace(kGradientPropertyColorRange, std::move(colors));
    properties.emplace(kGradientPropertyInputRange, std::move(inputs));

    auto units = propertyAsMapped<GradientUnits>(context, object, "units",
                                                 kGradientUnitsBoundingBox, sGradientUnitsMap);
    properties.emplace(kGradientPropertyUnits, units);

    // AVG specific handling
    if (type == LINEAR) {
        double angle = propertyAsDouble(context, object, "angle", 0.0);
        GradientSpreadMethod spreadMethod = PAD;
        double x1 = 0.0;
        double x2 = 1.0;
        double y1 = 0.0;
        double y2 = 1.0;

        if (avg) {
            spreadMethod = propertyAsMapped<GradientSpreadMethod>(context, object, "spreadMethod", PAD,
                                                                  sGradientSpreadMethodMap);
            x1 = propertyAsDouble(context, object, "x1", 0.0);
            x2 = propertyAsDouble(context, object, "x2", 1.0);
            y1 = propertyAsDouble(context, object, "y1", 0.0);
            y2 = propertyAsDouble(context, object, "y2", 1.0);
        } else {
            // Convert angle to coordinates.
            properties.emplace(kGradientPropertyAngle, angle);
            convertAngleToCoordinates(angle, x1, x2, y1, y2);
        }

        properties.emplace(kGradientPropertySpreadMethod, spreadMethod);
        properties.emplace(kGradientPropertyX1, x1);
        properties.emplace(kGradientPropertyX2, x2);
        properties.emplace(kGradientPropertyY1, y1);
        properties.emplace(kGradientPropertyY2, y2);
    } else if (type == RADIAL) {
        double centerX = 0.5;
        double centerY = 0.5;
        double radius = 0.7071;

        if (avg) {
            centerX = propertyAsDouble(context, object, "centerX", 0.5);
            centerY = propertyAsDouble(context, object, "centerY", 0.5);
            radius = propertyAsDouble(context, object, "radius", 0.7071);
        }

        properties.emplace(kGradientPropertyCenterX, centerX);
        properties.emplace(kGradientPropertyCenterY, centerY);
        properties.emplace(kGradientPropertyRadius, radius);
    }

    return Object(Gradient(std::move(properties)));
}

std::string
Gradient::toDebugString() const {
    std::string result = "Gradient<";

    for (auto& p : mProperties) {
        result += sGradientPropertiesMap.at(p.first) + "=" + p.second.toDebugString() + " ";
    }

    result += ">";
    return result;
}

rapidjson::Value
Gradient::serialize(rapidjson::Document::AllocatorType& allocator) const {
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);
    for (auto& p : mProperties) {
        v.AddMember(rapidjson::Value(
                sGradientPropertiesMap.at(p.first).c_str(), allocator),
                p.second.serialize(allocator).Move(), allocator);
    }

    return v;
}

bool
Gradient::operator==(const apl::Gradient &other) const {
    return mProperties == other.mProperties;
}

Gradient::Gradient(std::map<GradientProperty, Object>&& properties) :
    mProperties(std::move(properties)) {
    for(auto& m : mProperties.at(kGradientPropertyInputRange).getArray())
        mInputRange.emplace_back(m.asNumber());
    for(auto& m : mProperties.at(kGradientPropertyColorRange).getArray())
        mColorRange.emplace_back(m.asColor());
}

}  // namespace apl
