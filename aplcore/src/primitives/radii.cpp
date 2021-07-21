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

#include <string>

#include "apl/primitives/radii.h"
#include "apl/utils/streamer.h"

namespace apl {

std::string
Radii::toString() const
{
    std::string result;
    for (int i = 0 ; i < mData.size() ; i++) {
        if (i != 0) result += ", ";
        result += std::to_string(mData[i]);
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
    for (int i = 0 ; i < mData.size() ; i++) {
        if (i != 0) result += ", ";
        result += std::to_string(mData[i]);
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

}  // namespace apl
