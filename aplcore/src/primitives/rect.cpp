/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
    if (value > std::numeric_limits<long>::max())
        v = std::numeric_limits<long>::max();
    else if (value < std::numeric_limits<long>::lowest())
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

const std::string
Rect::toString() const {
    return floatAsLongString(mWidth) + "x" + floatAsLongString(mHeight)
            + (mX >= 0 ? "+" : "") + floatAsLongString(mX)
            + (mY >= 0 ? "+" : "") + floatAsLongString(mY);
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

rapidjson::Value
Rect::serialize(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value v(rapidjson::kArrayType);
    v.PushBack(std::isnan(mX) ? 0 : mX, allocator);
    v.PushBack(std::isnan(mY) ? 0 : mY, allocator);
    v.PushBack(std::isnan(mWidth) ? 0 : mWidth, allocator);
    v.PushBack(std::isnan(mHeight) ? 0 : mHeight, allocator);
    return v;
}

}  // namespace apl