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

Object
Gradient::create(const Context& context, const Object& object)
{
    if (object.isGradient())
        return object;

    if (!object.isMap())
        return Object::NULL_OBJECT();

    // Extract and evaluate the color range
    auto colorRange = arrayifyProperty(context, object, "colorRange");
    auto length = colorRange.size();
    if (length < 2) {
        CONSOLE_CTX(context) << "Gradient does not have suitable color range";
        return Object::NULL_OBJECT();
    }

    std::vector<Color> colors;
    for (const auto& m : colorRange)
        colors.push_back(evaluate(context, m).asColor(context));

    // Extract and evaluate the input range
    auto inputRange = arrayifyProperty(context, object, "inputRange");
    auto inputRangeLength = inputRange.size();
    if (inputRangeLength != 0 && inputRangeLength != length) {
        CONSOLE_CTX(context) << "Gradient input range must match the color range length";
        return Object::NULL_OBJECT();
    }

    std::vector<double> inputs;
    if (inputRange.size() > 0) {
        double last = 0;
        for (const auto& m : inputRange) {
            double value = evaluate(context, m).asNumber();
            if (value < last || value > 1) {
                CONSOLE_CTX(context) << "Gradient input range not in ascending order within range [0,1]";
                return Object::NULL_OBJECT();
            }
            inputs.push_back(value);
            last = value;
        }
    } else {
        for (int i = 0 ; i < length ; i++)
            inputs.push_back(static_cast<double>(i) / (length - 1));
    }

    GradientType type = propertyAsMapped<GradientType>(context, object, "type", LINEAR, sGradientTypeMap);
    if (type < 0) {
        CONSOLE_CTX(context) << "Unrecognized type field in gradient";
        return Object::NULL_OBJECT();
    }

    double angle = propertyAsDouble(context, object, "angle", 0.0);
    return Object(Gradient(type, angle, std::move(colors), std::move(inputs)));
}

rapidjson::Value
Gradient::serialize(rapidjson::Document::AllocatorType& allocator) const {
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);
    v.AddMember("angle", getAngle(), allocator);
    Value colorRange(rapidjson::kArrayType);
    for(auto color : getColorRange()) {
        colorRange.PushBack(Value(color.asString().c_str(), allocator), allocator);
    }
    v.AddMember("colorRange", colorRange, allocator);
    Value inputRange(rapidjson::kArrayType);
    for(double input : getInputRange()) {
        inputRange.PushBack(input, allocator);
    }
    v.AddMember("inputRange", inputRange, allocator);
    v.AddMember("type", static_cast<int>(getType()), allocator);
    return v;
}


}  // namespace apl