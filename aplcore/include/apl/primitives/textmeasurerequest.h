#include <utility>

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

#ifndef _APL_TEXT_MEASURE_REQUEST_H
#define _APL_TEXT_MEASURE_REQUEST_H

#include <yoga/YGEnums.h>

#include "apl/utils/hash.h"
#include "apl/utils/streamer.h"

namespace apl {

/**
 * Packaged structure to represent unique text measurement request
 */
struct TextMeasureRequest {
    float width;
    YGMeasureMode widthMode;
    float height;
    YGMeasureMode heightMode;
    std::string paramHash;

    size_t hash() const {
        auto result = std::hash<std::string>{}(paramHash);
        hashCombine<float>(result, width);
        hashCombine<int>(result, widthMode);
        hashCombine<float>(result, height);
        hashCombine<int>(result, heightMode);
        return result;
    }

    bool operator<(const TextMeasureRequest& rhs) const {
        return (width < rhs.width ||
                height < rhs.height ||
                widthMode < rhs.widthMode ||
                heightMode < rhs.heightMode ||
                paramHash < rhs.paramHash);
    }

    bool operator==(const TextMeasureRequest& rhs) const {
        return width == rhs.width &&
               widthMode == rhs.widthMode &&
               height == rhs.height &&
               heightMode == rhs.heightMode &&
               paramHash == rhs.paramHash;
    }

    std::string toString() const {
        std::string result = "TextMeasureRequest<";
        result += "width=" + std::to_string(width) + ",";
        result += "widthMode=" + std::to_string(widthMode) + ",";
        result += "height=" + std::to_string(height) + ",";
        result += "heightMode=" + std::to_string(heightMode) + ",";
        result += "paramHash=" + paramHash + ">";
        return result;
    }
};

} // namespace apl

namespace std {
template<> struct hash<apl::TextMeasureRequest> {
    std::size_t operator()(apl::TextMeasureRequest const& tmr) const noexcept { return tmr.hash(); }
};
} // namespace std

#endif // _APL_TEXT_MEASURE_REQUEST_H
