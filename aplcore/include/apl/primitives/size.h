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

#ifndef _APL_SIZE_H
#define _APL_SIZE_H

#include "apl/utils/streamer.h"

namespace apl {

/**
 * Simple class to represent a size (width and height).  Values are allowed to be negative
 */
template<typename T>
class tSize
{
public:
    /**
     * Default constructor initializes the size to (0,0)
     */
    tSize() : mWidth(0), mHeight(0) {}

    /**
     * Common constructor
     * @param width The width-value
     * @param height The height-value
     */
    tSize(T width, T height) : mWidth(width), mHeight(height) {}

    /**
     * @return The width
     */
    T getWidth() const { return mWidth; }

    /**
     * @return The height
     */
    T getHeight() const { return mHeight; }

    bool empty() const { return mWidth <= 0 && mHeight <= 0; }

    bool operator==(const tSize& other) const { return mWidth == other.mWidth && mHeight == other.mHeight; }
    bool operator!=(const tSize& other) const { return mWidth != other.mWidth || mHeight != other.mHeight; }

    friend tSize operator*(const tSize& lhs, float scale) { return tSize(lhs.mWidth * scale, lhs.mHeight * scale); }
    friend tSize operator*(float scale, const tSize& rhs) { return tSize(rhs.mWidth * scale, rhs.mHeight * scale); }

    friend streamer& operator<<(streamer& os, const tSize& s) {
        os << s.toString();
        return os;
    }

    std::string toString() const {
        return std::to_string(mWidth) + "x" + std::to_string(mHeight);
    }

private:
    T mWidth;
    T mHeight;
};

using Size = tSize<float>;
using DSize = tSize<double>;

} // namespace apl

#endif // _APL_SIZE_H
