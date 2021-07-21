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

#ifndef _APL_RECT_H
#define _APL_RECT_H

#include "rapidjson/document.h"

#include "apl/primitives/point.h"
#include "apl/primitives/size.h"

namespace apl {

/**
 * A simple rectangle class.  A rectangle has a left, top, width, and height.  The width and height
 * will always be non-negative.
 */
class Rect
{
public:
    /**
     * Initialize an empty rectangle offset.  We give it a "bad" location to avoid accidental hits.
     */
    Rect() : mX(-1000000), mY(-1000000), mWidth(0), mHeight(0) {}

    /**
     * Initialize a rectangle.
     * @param x The x-value of the top-left corner.
     * @param y The y-value of the top-left corner.
     * @param width The width.
     * @param height The height.
     */
    Rect(float x, float y, float width, float height) {
        if (width < 0) {
            mX = x + width;
            mWidth = -width;
        } else {
            mX = x;
            mWidth = width;
        }

        if (height < 0) {
            mY = y + height;
            mHeight = -height;
        } else {
            mY = y;
            mHeight = height;
        }
    }

    /**
     * Compare two rectangles for equality.
     * @param rhs The other rectangle.
     * @return True if they are equal.  Two invalid rectangles are always equal.
     */
    bool operator==(const Rect& rhs) const;

    /**
     * Compare two rectangles for inequality
     * @param rhs The other rectangle.
     * @return True if they are not equal
     */
    bool operator!=(const Rect& rhs) const;

    /**
     * Record to string stream.
     * @return Stream.
     */
    friend streamer& operator<<(streamer&, const Rect&);

    /**
     * @deprecated Remove this in favor of "empty()"
     * @return True if this rectangle has zero or undefined width and height.
     */
    bool isEmpty() const;

    /**
     * @return True if this rectangle has zero or undefined width and height.
     */
    bool empty() const { return isEmpty(); }

    /**
     * @return True if this rectangle is not empty
     */
    bool truthy() const { return !isEmpty(); }

    /**
     * @return The x-value of the top-left corner
     */
    float getX() const { return mX; }

    /**
     * @return The x-value of the left side.  Assumes that the width is non-negative.
     */
    float getLeft() const { return mX; }

    /**
     * @return The x-value of the right side.  Assumes that the width is non-negative.
     */
    float getRight() const { return mX + mWidth; }

    /**
     * @return The y-value of the top-left corner.
     */
    float getY() const { return mY; }

    /**
     * @return The y-value of the top.  Assumes that the height is non-negative.
     */
    float getTop() const { return mY; }

    /**
     * @return The y-value of the bottom.  Assumes that the height is non-negative.
     */
    float getBottom() const { return mY + mHeight; }

    /**
     * @return The height
     */
    float getHeight() const { return mHeight; }

    /**
     * @return The width
     */
    float getWidth() const { return mWidth; }

    /**
     * @return The size (width/height).
     */
    Size getSize() const { return {mWidth, mHeight}; }

    /**
     * @return The top-left corner as a point.
     */
    Point getTopLeft() const { return {mX, mY}; }

    /**
     * @return The top-right corner as a point.
     */
    Point getTopRight() const { return {mX + mWidth, mY}; }

    /**
     * @return The bottom-left corner as a point
     */
    Point getBottomLeft() const { return {mX, mY + mHeight}; }

    /**
     * @return The bottom-right corner as a point
     */
    Point getBottomRight() const { return {mX + mWidth, mY + mHeight}; }

    /**
     * @return The center of the rectangle in the X direction
     */
    float getCenterX() const { return mX + mWidth / 2; }

    /**
     * @return The center of the rectangle in the Y direction
     */
    float getCenterY() const { return mY + mHeight / 2; }

    /**
     * @return The center of this rectangle
     */
    Point getCenter() const { return {getCenterX(), getCenterY()}; }

    /**
     * Offset this rectangle by a distance specified by a point.
     * @param p The distance to offset by.
     */
    void offset(const Point& p) { mX += p.getX(); mY += p.getY(); }

    /**
     * Get rect intersection with other rect.
     * @param other rect to intersect with.
     * @return intersection Rect.
     */
    Rect intersect(const Rect& other) const;

    /**
     * Check whether point is within this rectangle.  The point must in the region [left, right] and [top, bottom].
     * @param point The point to check
     * @return True if the point is within the rectangle.
     */
    bool contains(const Point& point) const;

    /**
     * Calculate the distance between this rectangle and a point.  If the point
     * falls in the interior of the rectangle, the distance is zero.
     * @param point The point
     * @return The Euclidean distance from the rectangle to the point
     */
    float distanceTo(const Point& point) const;

    /**
     * Get rect area value.
     * @return rect area.
     */
    float area() const { return mWidth * mHeight; }

    /**
     * Serialize into a string.
     *
     * For historical reasons this method ensures that the reported sizes are integral
     * values so they can be reported in the visual context.
     *
     * @return The serialized rectangle
     */
    std::string toString() const;

    /**
     * Serialize into JSON format
     * @param allocator
     * @return The serialized rectangle
     */
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    std::string toDebugString() const;

private:
    float mX;
    float mY;
    float mWidth;
    float mHeight;
};


} // namespace apl

#endif //_APL_RECT_H
