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
 * Given the cubic polynomial f(t) = a*(1-t)^3 + 3*b*t*(1-t)^2 + 3*c*t^2*(1-t) + d*t^3,
 * calculate the values of t where f'(t) = 0 and 0 < t < 1.  Return the number of roots
 * found and store the values of those roots in the provided array.
 * @param a The starting point, a = f(0)
 * @param b The first control point
 * @param c The second control point
 * @param d The ending point, d = f(1)
 * @param root An reference to an array with at least two elements.  Return the value of t where
 *             f'(t) = 0 if it exists and is in the interval (0,1)
 * @return The number of valid roots found.  May be 0, 1, or 2.
 */
int findCubicZeros(float a, float b, float c, float d, float roots[])
{
    // Reduce f'(t) down to e*t^2 + 2*f*t + g
    auto e = -a + 3*b - 3*c + d;
    auto f = a - 2*b + c;
    auto g = b - a;

    if (std::abs(e) < 1e-12) {  // If e is zero we have a simple linear equation
        if (std::abs(f) < 1e-12)  // If f is also zero we have straight line
            return 0;

        auto t = -g/(2*f);
        if (t < 0.0f || t >= 1.0f)
            return 0;
        roots[0] = t;
        return 1;
    }

    auto discriminant = f*f - e*g;
    if (discriminant < 0.0f)
        return 0;  // Imaginary roots
    auto h = sqrt(discriminant);
    auto t1 = (-f + h) / e;
    auto t2 = (-f - h) / e;

    int i = 0;
    if (t1 > 0 && t1 < 1.0f)
        roots[i++] = t1;
    if (t2 > 0 && t2 < 1.0f)
        roots[i++] = t2;
    return i;
}

/**
 * Given the quadratic polynomial f(t) = a*(1-t)^2 + 2*b*t*(1-t) + c*t^2, calculate
 * the values of t where f'(t) = 0 and 0 < t < 1.
 * @param a The starting point, a = f(0)
 * @param b The control point.
 * @param c The ending point, c = f(1)
 * @param root Return the value of t where f'(t) = 0 if it exists and is in the interval (0,1)
 * @return True if a root was found.
 */
bool findQuadraticZero(float a, float b, float c, float& root)
{
    auto d = a - 2 * b + c;
    if (std::abs(d) < 1e-12)
        return 0;

    root = (a-b)/d;
    return root > 0 && root < 1.0f;
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
