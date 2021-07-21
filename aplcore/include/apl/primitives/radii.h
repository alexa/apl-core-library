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

#ifndef _APL_RADII_H
#define _APL_RADII_H

#include <array>
#include "rapidjson/document.h"

namespace apl {

class streamer;

/**
 * Store corner radii for borders.  These are normally stored in display-independent pixels or DP.
 */
class Radii {
public:
    enum Corner {
        kTopLeft = 0,
        kTopRight = 1,
        kBottomLeft = 2,
        kBottomRight = 3
    };

    /**
     * Assign a zero radius to each corner
     */
    Radii() : mData{0,0,0,0} {}

    /**
     * Assign the same radius to each corner
     * @param radius The radius to assign
     */
    Radii(float radius) : mData{radius, radius, radius, radius} {}

    /**
     * Define specific values for each corner
     * @param topLeft The top-left radius
     * @param topRight The top-right radius
     * @param bottomLeft The bottom-left radius
     * @param bottomRight The bottom-right radius
     */
    Radii(float topLeft, float topRight, float bottomLeft, float bottomRight)
        : mData{topLeft, topRight, bottomLeft, bottomRight}
    {}

    /**
     * Construct from a fixed set of values.  The order is top-left, top-right,
     * bottom-left, bottom-right.
     * @param values The radius values to assign.
     */
    Radii(std::array<float,4>&& values) : mData(std::move(values)) {}

    Radii(const Radii& rhs) = default;
    Radii& operator=(const Radii& rhs) = default;

    /**
     * @return The top-left radius
     */
    float topLeft() const { return mData[kTopLeft]; }

    /**
     * @return The top-right radius
     */
    float topRight() const { return mData[kTopRight]; }

    /**
     * @return The bottom-left radius
     */
    float bottomLeft() const { return mData[kBottomLeft]; }

    /**
     * @return The bottom-right radius
     */
    float bottomRight() const { return mData[kBottomRight]; }

    /**
     * Return a specific radius by index
     * @param corner The corner to return
     * @return The radius
     */
    float radius(Corner corner) { return mData[corner]; }

    /**
     * Compare two sets of radii for inequality
     * @param rhs The other radii
     * @return True if they are not equal
     */
    bool operator!=(const Radii& rhs) const {
        return mData != rhs.mData;
    }

    /**
     * @deprecated Use "empty()" instead
     * @return True if all of the corners have been set to zero
     */
    bool isEmpty() const { return empty(); }

    /**
     * @return The array of radii.  These are guaranteed to be in the order
     *         top-left, top-right, bottom-left, bottom-right.
     */
    std::array<float, 4> get() const { return mData; }

    /**
     * @return A human-readable interpretation of the radii
     */
    std::string toString() const;

    friend streamer& operator<<(streamer& os, const Radii& radii);

    /* Standard Object methods */
    bool operator==(const Radii& rhs) const { return mData == rhs.mData; }

    std::string toDebugString() const;
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    bool empty() const { return mData[0] == 0 && mData[1] == 0 && mData[2] == 0 && mData[3] == 0; }
    bool truthy() const { return !empty(); }

private:
    std::array<float, 4> mData;
};

} // apl

#endif //_APL_RADII_H
