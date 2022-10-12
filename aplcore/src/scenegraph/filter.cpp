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

#include "apl/scenegraph/filter.h"

namespace apl {
namespace sg {

std::string
BlendFilter::toDebugString() const
{
    return "Blend mode=" + sBlendModeBimap.at(blendMode);
}

rapidjson::Value
BlendFilter::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto out = rapidjson::Value(rapidjson::kObjectType);
    out.AddMember("type", "blendFilter", allocator);
    if (back)
        out.AddMember("back", back->serialize(allocator), allocator);
    if (front)
        out.AddMember("front", front->serialize(allocator), allocator);
    out.AddMember("mode", rapidjson::Value(sBlendModeBimap.at(blendMode).c_str(), allocator), allocator);
    return out;
}

std::string
BlurFilter::toDebugString() const
{
    return "Blur radius=" + std::to_string(radius);
}

rapidjson::Value
BlurFilter::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto out = rapidjson::Value(rapidjson::kObjectType);
    out.AddMember("type", "blurFilter", allocator);
    if (filter)
        out.AddMember("filter", filter->serialize(allocator), allocator);
    out.AddMember("radius", radius, allocator);
    return out;
}


std::string
GrayscaleFilter::toDebugString() const
{
    return "Grayscale amount=" + std::to_string(amount);
}

rapidjson::Value
GrayscaleFilter::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto out = rapidjson::Value(rapidjson::kObjectType);
    out.AddMember("type", "grayscaleFilter", allocator);
    if (filter)
        out.AddMember("filter", filter->serialize(allocator), allocator);
    out.AddMember("amount", amount, allocator);
    return out;
}


std::string
MediaObjectFilter::toDebugString() const
{
    auto result = std::string("MediaObject");
    if (mediaObject)
        result += " url=" + mediaObject->url();
    return result;
}

rapidjson::Value
MediaObjectFilter::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto out = rapidjson::Value(rapidjson::kObjectType);
    out.AddMember("type", "mediaObjectFilter", allocator);
    if (mediaObject)
        out.AddMember("mediaObject", mediaObject->serialize(allocator), allocator);
    return out;
}


std::string
NoiseFilter::toDebugString() const
{
    return "Noise kind=" + sFilterNoiseKindBimap.at(kind) +
           " useColor=" + (useColor ? "yes" : "no") +
           " sigma=" + std::to_string(sigma);
}

rapidjson::Value
NoiseFilter::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto out = rapidjson::Value(rapidjson::kObjectType);
    out.AddMember("type", "noiseFilter", allocator);
    if (filter)
        out.AddMember("filter", filter->serialize(allocator), allocator);
    out.AddMember("kind", rapidjson::Value(sFilterNoiseKindBimap.at(kind).c_str(), allocator), allocator);
    out.AddMember("useColor", useColor, allocator);
    out.AddMember("sigma", sigma, allocator);
    return out;
}


std::string
SaturateFilter::toDebugString() const
{
    return "Saturate amount=" + std::to_string(amount);
}

rapidjson::Value
SaturateFilter::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto out = rapidjson::Value(rapidjson::kObjectType);
    out.AddMember("type", "saturateFilter", allocator);
    if (filter)
        out.AddMember("filter", filter->serialize(allocator), allocator);
    out.AddMember("amount", amount, allocator);
    return out;
}


std::string
SolidFilter::toDebugString() const
{
    return "Solid";
}

rapidjson::Value
SolidFilter::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto out = rapidjson::Value(rapidjson::kObjectType);
    out.AddMember("type", "solidFilter", allocator);
    out.AddMember("paint", paint->serialize(allocator), allocator);
    return out;
}

} // namespace sg
} // namespace apl