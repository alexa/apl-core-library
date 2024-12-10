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

#include "apl/component/textmeasurement.h"
#include "apl/utils/hash.h"
#include "apl/utils/streamer.h"
#include "apl/utils/stringfunctions.h"

namespace apl {

/**
 * Packaged structure to represent unique text measurement request
 */
struct TextMeasureRequest {
    float width;
    MeasureMode widthMode;
    float height;
    MeasureMode heightMode;
    size_t paramHash;

    std::size_t hash() const {
        auto result = paramHash;
        hashCombine<float>(result, width);
        hashCombine<int>(result, widthMode);
        hashCombine<float>(result, height);
        hashCombine<int>(result, heightMode);
        return result;
    }

    bool operator<(const TextMeasureRequest& rhs) const {
        return hash() < rhs.hash();
    }

    bool operator==(const TextMeasureRequest& rhs) const {
        return widthMode == rhs.widthMode &&
               heightMode == rhs.heightMode &&
               paramHash == rhs.paramHash &&
               (widthMode == MeasureMode::Undefined ||
                    (std::isnan(width) && std::isnan(rhs.width)) ||
                    (width == rhs.width)) &&
               (heightMode == MeasureMode::Undefined ||
                    (std::isnan(height) && std::isnan(rhs.height)) ||
                    (height == rhs.height));
    }

    std::string toString() const {
        std::string result = "TextMeasureRequest<";
        result += "width=" + sutil::to_string(width) + ",";
        result += "widthMode=" + std::to_string(widthMode) + ",";
        result += "height=" + sutil::to_string(height) + ",";
        result += "heightMode=" + std::to_string(heightMode) + ",";
        result += "paramHash=" + std::to_string(paramHash) + ">";
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
