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

const std::string
Rect::toString() const {
    return floatAsLongString(mWidth) + "x" + floatAsLongString(mHeight)
            + (mX >= 0 ? "+" : "") + floatAsLongString(mX)
            + (mY >= 0 ? "+" : "") + floatAsLongString(mY);
}

Rect Rect::intersect(const Rect &other) const {
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

}  // namespace apl