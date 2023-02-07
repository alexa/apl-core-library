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
 */

#include "apl/scenegraph/pathbounds.h"

namespace apl {
namespace sg {

/**
 * A support class to hold the bounding rectangle.  As points are added to the
 * class, the rectangle expands to hold the new point.
 */
class ExpandingRect {
public:
    void add(float x, float y) {
        if (!mInitialized) {
            mMinX = x;
            mMinY = y;
            mMaxX = x;
            mMaxY = y;
            mInitialized = true;
        }
        else {
            mMinX = std::min(x, mMinX);
            mMaxX = std::max(x, mMaxX);
            mMinY = std::min(y, mMinY);
            mMaxY = std::max(y, mMaxY);
        }
    }

    Rect rect() const {
        if (mInitialized)
            return {mMinX, mMinY, mMaxX - mMinX, mMaxY - mMinY};
        else
            return {};
    }

private:
    float mMinX = 0.0f;
    float mMaxX = 0.0f;
    float mMinY = 0.0f;
    float mMaxY = 0.0f;
    bool mInitialized = false;
};

/**
 * Given the cubic polynomial f(t) = p1*(1-t)^3 + 3*p2*t*(1-t)^2 + 3*p3*t^2*(1-t) + p4*t^3,
 * calculate the values of t where f'(t) = 0 and 0 < t < 1.  Return the number of roots
 * found and store the values of those roots in the provided array.
 * @param p1 The starting point, p1 = f(0)
 * @param p2 The first control point
 * @param p3 The second control point
 * @param p4 The ending point, p4 = f(1)
 * @param root An reference to an array with at least two elements.  Return the value of t where
 *             f'(t) = 0 if it exists and is in the interval (0,1)
 * @return The number of valid roots found.  May be 0, 1, or 2.
 */
int findCubicZeros(float p1, float p2, float p3, float p4, float roots[])
{
    // Reduce b'(t) down to a*t^2 + b*t + c
    auto a = -p1 + 3*p2 - 3*p3 + p4;
    auto b = 2 * (p1 - 2*p2 + p3);
    auto c = p2 - p1;

    // Following Numerical Recipes in C (section 5.6)
    auto discriminant = b*b - 4*a*c;
    if (discriminant < 0.0f)
        return 0;  // Imaginary roots
    auto sqrt_d = std::sqrt(discriminant);
    if (b < 0.0)
        sqrt_d = -sqrt_d;
    auto q = -0.5 * (b + sqrt_d);

    int i = 0;

    // The roots are q/a and c/q.
    if (std::abs(a) > std::abs(q)) {
        auto t = q / a;
        if (t > 0 && t < 1.0f)
            roots[i++] = static_cast<float>(t);
    }

    if (std::abs(q) > std::abs(c)) {
        auto t = c / q;
        if (t > 0 && t < 1.0f)
            roots[i++] = static_cast<float>(t);
    }

    return i;
}

/**
 * Given the quadratic polynomial f(t) = p1*(1-t)^2 + 2*p2*t*(1-t) + p3*t^2, calculate
 * the values of t where f'(t) = 0 and 0 < t < 1.
 * @param p1 The starting point, p1 = f(0)
 * @param p2 The control point.
 * @param p3 The ending point, p3 = f(1)
 * @param root Return the value of t where f'(t) = 0 if it exists and is in the interval (0,1)
 * @return True if p1 root was found.
 */
bool findQuadraticZero(float p1, float p2, float p3, float& root)
{
    // f'(t) = -a + b*t  [or (p2 - p1) + (p1 - 2*p2 + p3)t ]
    // Root is a / b
    auto a = p1 - p2;
    auto b = p1 - 2 * p2 + p3;

    if (std::abs(b) > std::abs(a)) {
        root = a / b;
        if (root > 0 && root < 1.0f)
            return true;
    }

    return false;
}

/**
 * A support class for calculating the bounds of a general path.
 */
class BoundingBox {
public:
    /**
     * Update the bounding box with a move command.  Moving to a new location doesn't
     * actually change the bounding box unless it is followed by command that draws a segment.
     * @param p An array of the pair x and y.
     */
    void addMove(const float p[]) {
        mLastAdded = false;
        mLastX = p[0];
        mLastY = p[1];
    }

    /**
     * Add a line segment to the bounding box.
     * @param p An array of the pair x and y
     */
    void addLine(const float p[]) {
        if (!mLastAdded) {
            mRect.add(mLastX, mLastY);
            mLastAdded = true;
        }

        mRect.add(p[0], p[1]);
        mLastX = p[0];
        mLastY = p[1];
    }

    /**
     * Add a quadratic Bézier curve to the bounding box.  This method adds the starting point and
     * ending point.  It checks the curve for minimums and maximums of x(t) and y(t) and adds those
     * points as well.
     * @param p An array of four values: cx1, cy2, x, and y.
     */
    void addQuadratic(const float p[]) {
        if (!mLastAdded) {
            mRect.add(mLastX, mLastY);
            mLastAdded = true;
        }

        float root;
        // Find the minimums and maximums of x(t)
        if (findQuadraticZero(mLastX, p[0], p[2], root))
            addQuadraticPoint(p, root);

        // Minimums and maximums of y(t)
        if (findQuadraticZero(mLastY, p[1], p[3], root))
            addQuadraticPoint(p, root);

        mRect.add(p[2], p[3]);
        mLastX = p[2];
        mLastY = p[3];
    }

    /**
     * Add a cubic Bézier curve to the bounding box.  This method adds the starting point and
     * ending point.  It checks the curve for minimums and maximums of x(t) and y(t) and adds those
     * points as well.
     * @param p An array of six values: cx1, cy1, cx2, cy2, x, and y.
     */
    void addCubic(const float p[]) {
        if (!mLastAdded) {
            mRect.add(mLastX, mLastY);
            mLastAdded = true;
        }

        float roots[2];
        // Find valid minimums and maximums of x(t)
        auto count = findCubicZeros(mLastX, p[0], p[2], p[4], roots);
        for (int i = 0 ; i < count ; i++)
            addCubicPoint(p, roots[i]);

        // Minimums and maximums of y(t)
        count = findCubicZeros(mLastY, p[1], p[3], p[5], roots);
        for (int i = 0 ; i < count ; i++)
            addCubicPoint(p, roots[i]);

        mRect.add(p[4], p[5]);
        mLastX = p[4];
        mLastY = p[5];
    }

    /**
     * @return The bounding rectangle.
     */
    Rect getRect() const {
        return mRect.rect();
    }

private:
    void addQuadraticPoint(const float p[], float t) {
        const auto mt = 1 - t;
        const auto t1 = mt * mt;
        const auto t2 = 2 * t * mt;
        const auto t3 = t * t;
        mRect.add(mLastX * t1 + p[0] * t2 + p[2] * t3,
                  mLastY * t1 + p[1] * t2 + p[3] * t3);
    }

    void addCubicPoint(const float p[], float t) {
        const auto mt = 1 - t;
        const auto t1 = mt * mt * mt;
        const auto t2 = 3 * t * mt * mt;
        const auto t3 = 3 * t * t * mt;
        const auto t4 = t * t * t;
        mRect.add(mLastX * t1 + p[0] * t2 + p[2] * t3 + p[4] * t4,
                  mLastY * t1 + p[1] * t2 + p[3] * t3 + p[5] * t4);
    }

private:
    ExpandingRect mRect;
    float mLastX = 0.0f;
    float mLastY = 0.0f;
    bool mLastAdded = false;
};

Rect
calculatePathBounds(const std::string& commands, const std::vector<float>& points)
{
    auto *ptr = points.data();
    BoundingBox bb;

    for (const auto& m : commands) {
        switch (m) {
            case 'M':
                bb.addMove(ptr);
                ptr += 2;
                break;
            case 'L':
                bb.addLine(ptr);
                ptr += 2;
                break;
            case 'Q':
                bb.addQuadratic(ptr);
                ptr += 4;
                break;
            case 'C':
                bb.addCubic(ptr);
                ptr += 6;
                break;
            default:
                // No action required
                break;
        }
    }

    return bb.getRect();
}

Rect
calculatePathBounds(const Transform2D& transform,
                    const std::string& commands,
                    const std::vector<float>& points)
{
    return calculatePathBounds(commands, transform * points);
}

} // namespace sg
} // namespace apl
