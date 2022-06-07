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

#include <apl/primitives/radii.h>
#include "apl/primitives/roundedrect.h"

namespace apl {

// Sum two radii.  If they are larger than a maximum value, scale them down so they fit.
inline void scaleRadii(float& r1, float& r2, float sumMax)
{
    if (r1 + r2 > sumMax) {
        float scale = sumMax / (r1 + r2);
        r1 *= scale;
        r2 *= scale;
    }
}

// Clip a radius value to between 0 and a maximum value;
inline float clipRadius(float r, float maxRadius)
{
    if (r < 0) return 0;
    return r < maxRadius ? r : maxRadius;
}

RoundedRect::RoundedRect(Rect rect, float radius)
    : RoundedRect(rect, Radii{radius})
{}

RoundedRect::RoundedRect(Rect rect, Radii radii)
    : mRect(rect)
{
    auto w = mRect.getWidth();
    auto h = mRect.getHeight();
    auto side = std::min(w, h);

    // Clip all radii to fit within the side length (and check for negative values)
    auto tl = clipRadius(radii.topLeft(), side);
    auto tr = clipRadius(radii.topRight(), side);
    auto bl = clipRadius(radii.bottomLeft(), side);
    auto br = clipRadius(radii.bottomRight(), side);

    // If the sum of the radii on any given side is greater than the side length, they need to be scaled down
    scaleRadii(tl, bl, h);
    scaleRadii(tr, br, h);

    // Repeat the scaling, this time for top and bottom
    scaleRadii(tl, tr, w);
    scaleRadii(bl, br, w);

    mRadii = {tl, tr, bl, br};
}

RoundedRect
RoundedRect::inset(float inset) const
{
    return {
        mRect.inset(inset),
        mRadii.subtract(inset)
    };
}

std::string
RoundedRect::toDebugString() const
{
    return mRect.toDebugString() + ":" + mRadii.toDebugString();
}

} // namespace apl