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

#include <algorithm>

#include "apl/primitives/radii.h"
#include "apl/utils/streamer.h"
#include "apl/utils/stringfunctions.h"

namespace apl {

Radii
Radii::subtract(float value) const
{
    return {
        std::max(0.0f, mData[0] - value),
        std::max(0.0f, mData[1] - value),
        std::max(0.0f, mData[2] - value),
        std::max(0.0f, mData[3] - value)
    };
}

std::string
Radii::toString() const
{
    std::string result;
    for (size_t i = 0 ; i < mData.size() ; i++) {
        if (i != 0) result += ", ";
        result += sutil::to_string(mData[i]);
    }
    return result;
}

streamer&
operator<<(streamer& os, const Radii& radii)
{
    os << radii.toString();
    return os;
}

std::string
Radii::toDebugString() const
{
    std::string result = "Radii<";
    for (size_t i = 0 ; i < mData.size() ; i++) {
        if (i != 0) result += ", ";
        result += sutil::to_string(mData[i]);
    }
    return result + ">";
}

rapidjson::Value
Radii::serialize(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value v(rapidjson::kArrayType);
    for(const auto& val : mData) {
        v.PushBack(val, allocator);
    }
    return v;
}

void
Radii::sanitize()
{
    for (size_t i = 0 ; i < mData.size() ; i++)
        mData[i] = std::max(0.0f, mData[i]);
}

}  // namespace apl
