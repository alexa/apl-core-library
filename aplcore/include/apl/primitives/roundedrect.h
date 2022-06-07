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

#ifndef _APL_ROUNDED_RECT_H
#define _APL_ROUNDED_RECT_H

#include "apl/primitives/rect.h"
#include "apl/primitives/radii.h"

namespace apl {

/**
 * A rectangle with radius values at each corner
 */
class RoundedRect {
public:
    RoundedRect() = default;
    RoundedRect(Rect rect, float radius);
    RoundedRect(Rect rect, Radii radii);

    bool empty() const {
        return mRect.empty();
    }

    /**
     * @return True if this rounded rectangle is actually a rectangle
     */
    bool isRect() const {
        return mRadii.empty();
    }

    /**
     * @return True if this rectangle has the same radius for each corner
     */
    bool isRegular() const {
        return mRadii.isRegular();
    }

    /**
     * @return The bounding rectangle
     */
    const Rect& rect() const { return mRect; }

    /**
     * @return The defined radii
     */
    const Radii& radii() const { return mRadii; }

    /**
     * @return The top-left corner of the bounding rectangle
     */
    Point getTopLeft() const { return mRect.getTopLeft(); }

    /**
     * @return The dimensions of the bounding rectangle
     */
    Size getSize() const { return mRect.getSize(); }

    /**
     * Inset the rounded rectangle by an equal amount in X and Y.  The inset
     * reduces the corner radii by an equal amount.
     * @param inset The amount to inset (may be negative)
     * @return A new rounded rectangle
     */
    RoundedRect inset(float inset) const;

    /**
     * Offset the bounding rectangle by a point.
     * @param p The point to offset by.
     */
    void offset(const Point& p) { mRect.offset(p); }

    friend bool operator==(const RoundedRect& lhs, const RoundedRect& rhs) {
        return lhs.mRect == rhs.mRect && lhs.mRadii == rhs.mRadii;
    }

    friend bool operator!=(const RoundedRect& lhs, const RoundedRect& rhs) {
        return lhs.mRect != rhs.mRect || lhs.mRadii != rhs.mRadii;
    }

    std::string toDebugString() const;

private:
    Rect mRect;
    Radii mRadii;
};

} // namespace apl

#endif // _APL_ROUNDED_RECT_H
