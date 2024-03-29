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
#include "apl/primitives/transform2d.h"

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
Rect::empty() const {
    return (mWidth == 0 && mHeight == 0) || std::isnan(mWidth) || std::isnan(mHeight);
}

Rect
Rect::intersect(const Rect &other) const {
    if (getLeft() >= other.getRight() || other.getLeft() >= getRight() ||
        getTop() >= other.getBottom() || other.getTop() >= getBottom())
        return {};

    auto left = std::max(getLeft(), other.getLeft());
    auto top = std::max(getTop(), other.getTop());
    auto right = std::min(getRight(), other.getRight());
    auto bottom = std::min(getBottom(), other.getBottom());

    return { left, top, right - left, bottom - top };
}

Rect
Rect::extend(const Rect& other) const
{
    if (empty()) return other;
    if (other.empty()) return *this;

    auto left = std::min(getLeft(), other.getLeft());
    auto top = std::min(getTop(), other.getTop());
    auto right = std::max(getRight(), other.getRight());
    auto bottom = std::max(getBottom(), other.getBottom());

    return { left, top, right - left, bottom - top };
}

bool
Rect::contains(const Point& point) const {
    auto x = point.getX();
    auto y = point.getY();

    return !empty() && x >= mX && x <= mX + mWidth && y >= mY && y <= mY + mHeight;
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

Rect
Rect::inset(float dx, float dy) const {
    auto w = std::max(0.0f, mWidth - 2 * dx);
    auto h = std::max(0.0f, mHeight - 2 * dy);
    auto x = w <= 0 ? mX + mWidth / 2 : mX + dx;
    auto y = h <= 0 ? mY + mHeight / 2 : mY + dy;
    return {x, y, w, h};
}

Rect
Rect::boundingBox(const apl::Transform2D& transform) const
{
    if (empty())
        return {};

    auto p1 = transform * getTopLeft();
    auto p2 = transform * getTopRight();
    auto p3 = transform * getBottomLeft();
    auto p4 = transform * getBottomRight();

    auto left = std::min({p1.getX(), p2.getX(), p3.getX(), p4.getX()});
    auto top = std::min({p1.getY(), p2.getY(), p3.getY(), p4.getY()});
    auto right =  std::max({p1.getX(), p2.getX(), p3.getX(), p4.getX()});
    auto bottom = std::max({p1.getY(), p2.getY(), p3.getY(), p4.getY()});

    return { left, top, right - left, bottom - top };
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
    return "Rect<" + std::to_string(mWidth) + "x" + std::to_string(mHeight)
           + (mX >= 0 ? "+" : "") + std::to_string(mX)
           + (mY >= 0 ? "+" : "") + std::to_string(mY) + ">";
}

}  // namespace apl
