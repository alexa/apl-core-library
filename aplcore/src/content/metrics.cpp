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

#include "apl/content/metrics.h"

namespace apl {

Bimap<int, std::string> sScreenShapeBimap = {
    { RECTANGLE, "rectangle" },
    { ROUND, "round" },
};

Bimap<int, std::string> sViewportModeBimap = {
    { kViewportModeAuto, "auto" },
    { kViewportModeHub, "hub" },
    { kViewportModeMobile, "mobile" },
    { kViewportModePC, "pc" },
    { kViewportModeTV, "tv" }
};

std::string Metrics::toDebugString() const {
    return  "Metrics<"
            "theme=" + mTheme + ", "
            "size=" + std::to_string(static_cast<int>(mPixelWidth)) + "x"
                    + std::to_string(static_cast<int>(mPixelHeight)) + ", "
            "dpi=" + std::to_string(static_cast<int>(mDpi)) + ", "
            "shape=" + (mShape == ROUND ? "round" : "rectangle") + ", "
            "mode=" + sViewportModeBimap.at(mMode) + ">";
}

}