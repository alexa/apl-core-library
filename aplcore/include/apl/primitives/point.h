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

#ifndef _APL_POINT_H
#define _APL_POINT_H

#include "apl/utils/streamer.h"

#include <algorithm>
#include <cmath>

namespace apl {

/**
 * Simple class to represent a point in space.
 */
template<typename T>
class tPoint
{
public:
    /**
     * Default constructor initializes the point to (0,0)
     */
    tPoint() : mX(0), mY(0) {}

    /**
     * Common constructor
     * @param x The x-value
     * @param y The y-value
     */
    tPoint(T x, T y) : mX(x), mY(y) {}

    /**
     * @return The x-value
     */
    T getX() const { return mX; }

    /**
     * @return The y-value
     */
    T getY() const { return mY; }

    /**
     * Add a second point to this point.
     * @param other The second point.
     * @return This point.
     */
    tPoint& operator+=(const tPoint& other) {
        mX += other.mX; mY += other.mY; return *this;
    }

    tPoint& operator-=(const tPoint& other) {
        mX -= other.mX; mY -= other.mY; return *this;
    }

    bool operator==(const tPoint& other) const { return mX == other.mX && mY == other.mY; }
    bool operator!=(const tPoint& other) const { return mX != other.mX || mY != other.mY; }

    friend tPoint operator+(const tPoint& lhs, const tPoint& rhs) { return tPoint(lhs.mX + rhs.mX, lhs.mY + rhs.mY); }
    friend tPoint operator-(const tPoint& lhs, const tPoint& rhs) { return tPoint(lhs.mX - rhs.mX, lhs.mY - rhs.mY); }

    tPoint operator-() const { return tPoint(-mX, -mY); }

    friend streamer& operator<<(streamer& os, const tPoint& p) {
        os << p.mX << "," << p.mY;
        return os;
    }

    std::string toString() const {
        return std::to_string(mX) + "," + std::to_string(mY);
    }

    bool isFinite() { return std::isfinite(mX) && std::isfinite(mY); }

    /**
     * Get bottom right bounding position for two provided points.
     * @param p1 first point.
     * @param p2 second point.
     * @return Point which bounds both positions to the bottom and right.
     */
    static tPoint bottomRightBound(const tPoint& p1, const tPoint& p2) {
        return {std::max(p1.getX(), p2.getX()), std::max(p1.getY(), p2.getY())};
    }

    /**
     * Get top left bounding position for two provided points.
     * @param p1 first point.
     * @param p2 second point.
     * @return Point which bounds both points to the top and left.
     */
    static tPoint topLeftBound(const tPoint& p1, const tPoint& p2) {
        return {std::min(p1.getX(), p2.getX()), std::min(p1.getY(), p2.getY())};
    }

private:
    T mX, mY;
};

using Point = tPoint<float>;
using DPoint = tPoint<double>;

} // namespace apl

#endif //_APL_POINT_H
