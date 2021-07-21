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

#include "apl/scaling/metricstransform.h"
#include "apl/scaling/scalingcalculator.h"
#include <string>

namespace apl {

float
MetricsTransform::toViewhost(float value) const {
    return value * getScaleToViewhost() * getDpi() / CORE_DPI;
}

float
MetricsTransform::toCore(float value) const {
    return value * getScaleToCore() * CORE_DPI / getDpi();
}

float
MetricsTransform::getViewhostWidth() const {
    return toViewhost(getWidth());
}

float
MetricsTransform::getViewhostHeight() const {
    return toViewhost(getHeight());
}

void
MetricsTransform::init() {
    auto result = scaling::calculate(mMetrics, mOptions);
    mScaleToViewhost = static_cast<float>(std::get<0>(result));
    mMetrics = std::get<1>(result);
    mChosenSpec = std::get<2>(result);
    mScaleToCore = 1 / mScaleToViewhost;
    mDpi = mMetrics.getDpi();
    mWidth = mMetrics.getWidth();
    mHeight = mMetrics.getHeight();
    mMode = mMetrics.getViewportMode();
}

std::string ViewportSpecification::toDebugString() const {
    return  "ViewportSpecification<"
            "mode=" + sViewportModeBimap.at(mode) + ", "
            "shape=" + std::string(isRound ? "ROUND" : "RECT") + ", "
            "wmin=" + std::to_string(static_cast<int>(wmin)) + ", "
            "wmax=" + std::to_string(static_cast<int>(wmax)) + ", "
            "hmin=" + std::to_string(static_cast<int>(hmin)) + ", "
            "hmax=" + std::to_string(static_cast<int>(hmax)) + ">";
}

} // namespace apl