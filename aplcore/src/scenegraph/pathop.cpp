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

#include "apl/scenegraph/pathop.h"

namespace apl {
namespace sg {

rapidjson::Value
PathOp::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = rapidjson::Value(rapidjson::kObjectType);
    result.AddMember("paint", paint->serialize(allocator), allocator);
    if (nextSibling)
        result.AddMember("op", nextSibling->serialize(allocator), allocator);
    return result;
}


std::string
StrokePathOp::toDebugString() const
{
    auto d = std::string();
    auto len = dashes.size();
    if (len)
        d += std::to_string(dashes[0]);
    for (int i = 1 ; i < len ; i++)
        d += "," + std::to_string(dashes[i]);

    return "Stroke width=" + std::to_string(strokeWidth) +
        " miterLimit=" + std::to_string(miterLimit) +
        " pathLen=" + std::to_string(pathLength) +
        " dashOffset=" + std::to_string(dashOffset) +
        " lineCap=" + sGraphicLineCapBimap.at(lineCap) +
        " lineJoin=" + sGraphicLineJoinBimap.at(lineJoin) +
        " dashes=[" + d + "]";
}

rapidjson::Value
StrokePathOp::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = PathOp::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("stroke"), allocator);
    result.AddMember("width", strokeWidth, allocator);
    result.AddMember("miterLimit", miterLimit, allocator);
    result.AddMember("pathLength", pathLength, allocator);
    result.AddMember("dashOffset", dashOffset, allocator);
    result.AddMember("lineCap", rapidjson::Value(sGraphicLineCapBimap.at(lineCap).c_str(), allocator), allocator);
    result.AddMember("lineJoin", rapidjson::Value(sGraphicLineJoinBimap.at(lineJoin).c_str(), allocator), allocator);
    if (!dashes.empty()) {
        auto dashArray = rapidjson::Value(rapidjson::kArrayType);
        for (const auto& dash : dashes)
            dashArray.PushBack(dash, allocator);
        result.AddMember("dashes", dashArray, allocator);
    }
    return result;
}

std::string
FillPathOp::toDebugString() const
{
    // TODO: Add fill type
    return "Fill";
}

rapidjson::Value
FillPathOp::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = PathOp::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("fill"), allocator);
    result.AddMember("fillType",
                     rapidjson::StringRef(fillType == kFillTypeEvenOdd ? "even-odd" : "winding"),
                     allocator);
    return result;
}

} // namespace sg
} // namespace apl
