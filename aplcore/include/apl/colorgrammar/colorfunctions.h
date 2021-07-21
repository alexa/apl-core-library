/*
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
 *
 * Pre-defined color functions
 */

#ifndef _COLORGRAMMAR_COLOR_FUNCTIONS_H
#define _COLORGRAMMAR_COLOR_FUNCTIONS_H

#include <cstdio>
#include <algorithm>
#include <cmath>

#include "apl/utils/log.h"

namespace apl {
namespace colorgrammar {
static const bool DEBUG_COLOR_FUNC = false;

inline double clampPercent(double percent) {
    if (percent < 0) return 0;
    if (percent > 1) return 1;
    return percent;
}

inline uint32_t clamp255(double v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<uint32_t>(std::round(v));
}

inline double clampHue(double hue) {
    if (hue < 0) return 0;
    if (hue > 360) return 360;
    return hue;
}

inline uint32_t applyAlpha(uint32_t color, double percent) {
    double alpha = clampPercent(percent) * (color & 0x000000ff);
    uint32_t result = (color & 0xffffff00) | static_cast<uint32_t>(alpha);
    LOGF_IF(DEBUG_COLOR_FUNC, "rgb(%08x, %lf) -> %08x", color, percent, result);
    return result;
}

inline uint32_t colorFromRGB(double red, double green, double blue) {
    uint32_t color = 0x000000ff | (clamp255(red) << 24) | (clamp255(green) << 16) | clamp255(blue) << 8;
    LOGF_IF(DEBUG_COLOR_FUNC, "rgb(%lf,%lf,%lf) -> %08x", red, green, blue, color);
    return color;
}

inline uint32_t colorFromRGBA(double red, double green, double blue, double alpha) {
    uint32_t color = clamp255(alpha * 0xff) | (clamp255(red) << 24) | (clamp255(green) << 16) | clamp255(blue) << 8;
    LOGF_IF(DEBUG_COLOR_FUNC, "rgba(%lf,%lf,%lf,%lf) -> %08x", red, green, blue, alpha, color);
    return color;
}

inline double hueToRgb(double p, double q, double t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1./6) return p + (q-p) * 6 * t;
    if (t < 0.5) return q;
    if (t < 2./3) return p + (q-p) * 6 * (2./3 - t);
    return p;
}

inline uint32_t colorFromHSLA(double hue, double sat, double light, double alpha) {
    double h = clampHue(hue);
    double s = clampPercent(sat);
    double l = clampPercent(light);
    double a = clampPercent(alpha);

    if (s == 0)
        return colorFromRGBA(l*255, l*255, l*255, a);
    h /= 360.0;

    double q = l < 0.5 ? l * (1+s) : l + s - l*s;
    double p = 2 * l - q;

    return colorFromRGBA(
        hueToRgb(p, q, h + 1./3) * 255,
        hueToRgb(p, q, h) * 255,
        hueToRgb(p, q, h - 1./3) * 255,
        a);
}

inline uint32_t colorFromHSL(double hue, double sat, double light) {
    return colorFromHSLA(hue, sat, light, 1.0);
}

/**
 * Calculate a color value from a hexidecimal string
 * @param hex
 * @param color
 * @return
 */
inline bool colorFromHex(std::string hex, uint32_t& color) {
    switch (hex.length()) {
        case 5:  // #RGBA
            color = (17 * stoul(hex.substr(1, 1), nullptr, 16)) << 24 |
                    (17 * stoul(hex.substr(2, 1), nullptr, 16)) << 16 |
                    (17 * stoul(hex.substr(3, 1), nullptr, 16)) << 8 |
                    (17 * stoul(hex.substr(4, 1), nullptr, 16));
            return true;

        case 4:  // #RGB
            color = 0x000000ff |
                    (17 * stoul(hex.substr(1, 1), nullptr, 16)) << 24 |
                    (17 * stoul(hex.substr(2, 1), nullptr, 16)) << 16 |
                    (17 * stoul(hex.substr(3, 1), nullptr, 16)) << 8;
            return true;

        case 9:  // #RRGGBBAA
            color = stoul(hex.substr(1), nullptr, 16);
            return true;

        case 7:  // #RRGGBB
            color = 0x000000ff | stoul(hex.substr(1), nullptr, 16) << 8;
            return true;
    }

    return false;
}

}  // namespace colorgrammar
}  // namespace apl

#endif // _COLORGRAMMAR_COLOR_FUNCTIONS_H