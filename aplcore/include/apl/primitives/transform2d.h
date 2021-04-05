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

#ifndef _APL_TRANSFORM_2D_H
#define _APL_TRANSFORM_2D_H

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#include <array>
#include <cmath>

#include "apl/common.h"

#include "apl/primitives/point.h"
#include "apl/primitives/rect.h"
#include "rapidjson/document.h"

#include "apl/utils/bimap.h"
#include "apl/utils/log.h"

namespace apl {

enum TransformType {
    kTransformTypeRotate,
    kTransformTypeScaleX,
    kTransformTypeScaleY,
    kTransformTypeScale,
    kTransformTypeSkewX,
    kTransformTypeSkewY,
    kTransformTypeTranslateX,
    kTransformTypeTranslateY,
};

extern Bimap<TransformType, std::string> sTransformTypeMap;

const static bool DEBUG_TRANSFORM = false;

/**
 * Represent a 2D transformation matrix.
 *
 * The basic structure is
 *
 *    a c e
 *    b d f
 *    0 0 1
 *
 * where a-f are the elements in the internal array.
 *
 * We're following the definition in CSS-transforms, section 9.1
 */
class Transform2D {
public:
    /**
     * Translation in the x-direction
     * @param tx Distance to translate
     * @return The transform
     */
    static Transform2D translateX(float tx) {
        LOG_IF(DEBUG_TRANSFORM) << "translateX " << tx;
        return Transform2D({1,0,0,1,tx,0});
    }

    /**
     * Translation in the y-direction
     * @param ty Distance to translate
     * @return The transform
     */
    static Transform2D translateY(float ty) {
        LOG_IF(DEBUG_TRANSFORM) << "translateY " << ty;
        return Transform2D({1,0,0,1,0,ty});
    }

    /**
     * Translation transformation.
     * @param tx Distance to translate in the x-direction
     * @param ty Distance to translate in the y-direction
     * @return The transform
     */
    static Transform2D translate(float tx, float ty) {
        LOG_IF(DEBUG_TRANSFORM) << "translate " << tx << "," << ty;
        return Transform2D({1,0,0,1,tx,ty});
    }

    /**
     * Translation transformation.
     * @param point The point from which to extract x & y distances
     * @return The transform
     */
    static Transform2D translate(const Point &point) {
        return translate(point.getX(), point.getY());
    }

    /**
     * Scale in the x-direction.
     * @param sx Scaling factor.  Should satisfy sx > 0
     * @return The transform
     */
    static Transform2D scaleX(float sx) {
        LOG_IF(DEBUG_TRANSFORM) << "scaleX " << sx;
        return Transform2D({sx,0,0,1,0,0});
    }

    /**
     * Scale in the y-direction
     * @param sy Scaling factor.  Should satisfy sy > 0
     * @return The transform
     */
    static Transform2D scaleY(float sy) {
        LOG_IF(DEBUG_TRANSFORM) << "scaleY " << sy;
        return Transform2D({1,0,0,sy,0,0});
    }

    /**
     * Scale in both the x- and y-direction
     * @param s Scaling factor. Should satisfy s > 0
     * @return The transform
     */
    static Transform2D scale(float s) {
        LOG_IF(DEBUG_TRANSFORM) << "scale " << s;
        return Transform2D({s,0,0,s,0,0});
    }

    /**
     * Scale in both the x- and y-direction
     * @param s Scaling factor. Should satisfy s > 0
     * @return The transform
     */
    static Transform2D scale(float sx, float sy) {
        LOG_IF(DEBUG_TRANSFORM) << "scale " << sx << ", " << sy;
        return Transform2D({sx,0,0,sy,0,0});
    }

    /**
     * Rotation clockwise about the origin.
     * @param angle Rotation angle in degrees.
     * @return The transform
     */
    static Transform2D rotate(float angle) {
        LOG_IF(DEBUG_TRANSFORM) << "rotate " << angle;
        double radians = angle * M_PI / 180;
        auto c = static_cast<float>(cos(radians));
        auto s = static_cast<float>(sin(radians));
        return Transform2D({c,s,-s,c,0,0});
    }

    /**
     * Skew transformation along the x-axis by the given angle.
     * @param angle Skew angle in degrees
     * @return The transform
     */
    static Transform2D skewX(float angle) {
        LOG_IF(DEBUG_TRANSFORM) << "skewX " << angle;
        double radians = angle * M_PI / 180;
        return Transform2D({1,0,static_cast<float>(tan(radians)),1,0,0});
    }

    /**
     * Skew transformation along the y-axis by the given angle.
     * @param angle Skew angle in degrees
     * @return The transform
     */
    static Transform2D skewY(float angle) {
        LOG_IF(DEBUG_TRANSFORM) << "skewY " << angle;
        double radians = angle * M_PI / 180;
        return Transform2D({1,static_cast<float>(tan(radians)),0,1,0,0});
    }

    /**
     * Parse a string representing a transform
     * @param string The string to parse
     * @return A 2D transform.  This will be the identify transform if the string fails to parse.
     */
    static Transform2D parse(const SessionPtr& session, const std::string& string);

    /**
     * Default constructor creates the identity matrix.
     */
    Transform2D() : mData{1,0,0,1,0,0} {}

    /**
     * Constructor from arbitrary elements
     * @param values Array of values (length 6)
     */
    explicit Transform2D(std::array<float,6>&& values)
        : mData(values) {}

    Transform2D(const Transform2D&) = default;
    Transform2D& operator=(const Transform2D&) = default;

    /**
     * @return the scaling factor in the X direction due to this transform.
     */
    float getXScaling() const { return std::sqrt(mData[0]*mData[0] + mData[1]*mData[1]); }

    /**
     * @return the scaling factor in the Y direction due to this transform.
     */
    float getYScaling() const { return std::sqrt(mData[2]*mData[2] + mData[3]*mData[3]); }

    /**
    * Calculate the axis aligned bounding box of a transformed rectangle. This
    * method transforms the 4 corners of the rectangle, and returns the minimum,
    * axis aligned, rectangle that encompasses the transformed corners.
    *
    * @return A new rectangle representing the axis aligned bounding box.
    */
    Rect calculateAxisAlignedBoundingBox(const Rect& rect);

    /**
     * Product of two transforms
     * @param lhs left transform
     * @param rhs right transform
     * @return The new transform
     */
    friend Transform2D operator*(const Transform2D& lhs, const Transform2D& rhs) {
        Transform2D result(lhs);
        result *= rhs;
        return result;
    }

    /**
     * Merge this transform with another transform, right-associated.
     * @param rhs The right transform
     * @return The updated transform
     */
    Transform2D& operator*=(const Transform2D& rhs) {
        float a = mData[0]*rhs.mData[0] + mData[2]*rhs.mData[1];
        float b = mData[1]*rhs.mData[0] + mData[3]*rhs.mData[1];
        float c = mData[0]*rhs.mData[2] + mData[2]*rhs.mData[3];
        float d = mData[1]*rhs.mData[2] + mData[3]*rhs.mData[3];
        float e = mData[0]*rhs.mData[4] + mData[2]*rhs.mData[5] + mData[4];
        float f = mData[1]*rhs.mData[4] + mData[3]*rhs.mData[5] + mData[5];
        mData = {a,b,c,d,e,f};
        return *this;
    }

    /**
     * Calculate how a point moves based on this transform.
     * @param lhs The transform
     * @param rhs The point
     * @return The resultant point.
     */
    friend Point operator*(const Transform2D& lhs, const Point& rhs) {
        return {lhs.mData[0] * rhs.getX() + lhs.mData[2] * rhs.getY() + lhs.mData[4],
                lhs.mData[1] * rhs.getX() + lhs.mData[3] * rhs.getY() + lhs.mData[5]};
    }

    /**
     * @return True if this transformation is singular
     */
    bool singular() const {
        float d1 = mData[0] * mData[3];
        float d2 = mData[1] * mData[2];
        return !std::isfinite(d1) || !std::isfinite(d2) || !std::isfinite(mData[4]) || !std::isfinite(mData[5])
                || d1 == d2;
    }

    /**
     * Calculate the inverse transform.  Note: this will produce a matrix of NaN if the transform
     * is singular.
     * @return The inverse transform.
     */
     Transform2D inverse() const {
        if (empty())
            return Transform2D();

        float d = mData[0] * mData[3] - mData[1] * mData[2];
        return Transform2D({mData[3] / d, -mData[1] / d, -mData[2] / d, mData[0] / d,
                            (mData[2] * mData[5] - mData[3] * mData[4]) / d,
                            (mData[1] * mData[4] - mData[0] * mData[5]) / d});
    }

    /**
     * Compare two transforms for equality
     * @param lhs The first transform
     * @param rhs The second transform
     * @return True if the transforms are exactly equal
     */
    friend bool operator==(const Transform2D& lhs, const Transform2D& rhs) {
        return lhs.mData == rhs.mData;
    }

    /**
     * Compare two transforms for inequality.
     * @param lhs The first transform
     * @param rhs The second transform
     * @return True if the transforms differ
     */
    friend bool operator!=(const Transform2D& lhs, const Transform2D& rhs) {
        return lhs.mData != rhs.mData;
    }

    /**
     * @return The array of transform elements.
     */
    std::array<float, 6> get() const { return mData; }

    friend streamer& operator<<(streamer& os, const Transform2D& transform);

    /**
      * Serialize this transform into a 6 element array.
      * @param allocator
      * @return
      */
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const {
        rapidjson::Value v(rapidjson::kArrayType);
        if (singular()) {
            // Avoid serializing NaN that could be present in transforms by
            // serializing the singular transform [0, 0, 0, 0, 0, 0] instead
            // TODO: serialize the original transform instead, but using a different JSON encoding
            for (int i = 0; i < mData.size(); i++) {
                v.PushBack(0.0f, allocator);
            }
        } else {
            for (const auto& p : mData)
                v.PushBack(p, allocator);
        }

        return v;
    }

    /**
     * @return True if the transform is the identity.
     */
    bool isIdentity() const {
        return empty();
    }

    /**
     * @return True if this transform is empty - that is, if the transform is the identity.
     */
    bool empty() const {
        return mData[0] == 1 && mData[1] == 0 && mData[2] == 0 &&
               mData[3] == 1 && mData[4] == 0 && mData[5] == 0;
    }

    /**
     * @return Always return true.
     */
    bool truthy() const {
        return true;
    }

    std::string toDebugString() const;

private:
    std::array<float, 6> mData;
};

} // namespace apl

#endif // _APL_TRANSFORM_2D_H
