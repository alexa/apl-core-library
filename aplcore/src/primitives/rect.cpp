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

#include "apl/primitives/rect.h"
#include <cmath>
#ifdef APL_CORE_UWP
    #include <algorithm>
#endif

namespace apl {

streamer &
operator<<(streamer &os, const Rect &rect) {
    os << rect.toString();
    return os;
}

static inline std::string
floatAsLongString(float value)
{
    auto v = static_cast<long>(value);
    if (value > static_cast<float>(std::numeric_limits<long>::max()))
        v = std::numeric_limits<long>::max();
    else if (value < static_cast<float>(std::numeric_limits<long>::lowest()))
        v = std::numeric_limits<long>::lowest();

    return std::to_string(v);
}

bool
Rect::operator==(const Rect& rhs) const {
    return ((std::isnan(mX) && std::isnan(rhs.mX)) || (mX == rhs.mX))
        && ((std::isnan(mY) && std::isnan(rhs.mY)) || (mY == rhs.mY))
        && ((std::isnan(mWidth) && std::isnan(rhs.mWidth)) || (mWidth == rhs.mWidth))
        && ((std::isnan(mWidth) && std::isnan(rhs.mWidth)) || (mHeight == rhs.mHeight));
}

bool
Rect::operator!=(const Rect& rhs) const {
    return (!(std::isnan(mX) && std::isnan(rhs.mX)) && (mX != rhs.mX))
        || (!(std::isnan(mY) && std::isnan(rhs.mY)) && (mY != rhs.mY))
        || (!(std::isnan(mWidth) && std::isnan(rhs.mWidth)) && (mWidth != rhs.mWidth))
        || (!(std::isnan(mHeight) && std::isnan(rhs.mHeight)) && (mHeight != rhs.mHeight));
    }

std::string
Rect::toString() const {
    return floatAsLongString(mWidth) + "x" + floatAsLongString(mHeight)
            + (static_cast<long>(mX) >= 0 ? "+" : "") + floatAsLongString(mX)
            + (static_cast<long>(mY) >= 0 ? "+" : "") + floatAsLongString(mY);
}

bool
Rect::isEmpty() const {
    return (mWidth == 0 && mHeight == 0) || std::isnan(mWidth) || std::isnan(mHeight);
}

Rect
Rect::intersect(const Rect &other) const {
    if (getLeft() >= other.getRight() || other.getLeft() >= getRight() ||
        getTop() >= other.getBottom() || other.getTop() >= getBottom())
        return {};

    float x = std::max(other.getX(), getX());
    float y = std::max(other.getY(), getY());
    return {x,
            y,
            std::min(other.getRight(), getRight()) - x,
            std::min(other.getBottom(), getBottom()) - y};
}

bool
Rect::contains(const Point& point) const {
    auto x = point.getX();
    auto y = point.getY();

    return !isEmpty() && x >= mX && x <= mX + mWidth && y >= mY && y <= mY + mHeight;
}

float
Rect::distanceTo(const Point& point) const {
    auto x = point.getX() - mX;  // Offset from left side
    auto y = point.getY() - mY;  // Offset from top
    auto dx = x < 0 ? -x : (x > mWidth ? x - mWidth : 0);
    auto dy = y < 0 ? -y : (y > mHeight ? y - mHeight : 0);
    if (dx == 0) return dy;
    if (dy == 0) return dx;
    return std::sqrt(dx*dx + dy*dy);
}

rapidjson::Value
Rect::serialize(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value v(rapidjson::kArrayType);
    v.PushBack(std::isnan(mX) ? 0 : mX, allocator);
    v.PushBack(std::isnan(mY) ? 0 : mY, allocator);
    v.PushBack(std::isnan(mWidth) ? 0 : mWidth, allocator);
    v.PushBack(std::isnan(mHeight) ? 0 : mHeight, allocator);
    return v;
}

std::string
Rect::toDebugString() const{
    return "Rect<" + toString() + ">";
}

}  // namespace apl