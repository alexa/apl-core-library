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

#include "apl/primitives/transform2d.h"

#include "apl/primitives/point.h"
#include "apl/primitives/transformgrammar.h"

namespace apl {

streamer&
operator<<(streamer& os, const Transform2D& transform)
{
    for (int i = 0 ; i < 6 ; i++) {
        os << transform.mData[i];
        if (i != 5) os << ", ";
    }
    return os;
}

std::string
Transform2D::toDebugString() const
{
    auto result = "Transform2D<" + sutil::to_string(mData[0]);
    for (int i = 1 ; i < 6 ; i++)
        result += ", " + sutil::to_string(mData[i]);
    return result + ">";
}

Transform2D
Transform2D::parse(const SessionPtr& session, const std::string& transform)
{
    class CombinedTransformationAccumulator : public TransformationAccumulator {
    public:
        void add(const apl::TransformPtr& t) override {
            transform *= t->evaluate();
        }

        Transform2D transform;
    };

    CombinedTransformationAccumulator accum;

    if (t2grammar::parse(session, transform, accum))
        return accum.transform;
    else
        return {};
}

Rect
Transform2D::calculateAxisAlignedBoundingBox(const Rect& rect) const {

    if (isIdentity())
        return rect;

    // add translation to the offset of the rect
    Transform2D t2D = *this * Transform2D::translate(rect.getTopLeft());

    // apply the transform to the 4 corners
    auto p1 = t2D * Point(0,0);
    auto p2 = t2D * Point(rect.getWidth(),0);
    auto p3 = t2D * Point(rect.getWidth(),rect.getHeight());
    auto p4 = t2D * Point(0, rect.getHeight());

    // find the min/max x and y for bounding box
    auto minX = std::min(p1.getX(), std::min(p2.getX(), (std::min(p3.getX(), p4.getX()))));
    auto maxX = std::max(p1.getX(), std::max(p2.getX(), (std::max(p3.getX(), p4.getX()))));
    auto minY = std::min(p1.getY(), std::min(p2.getY(), (std::min(p3.getY(), p4.getY()))));
    auto maxY = std::max(p1.getY(), std::max(p2.getY(), (std::max(p3.getY(), p4.getY()))));
    auto result = Rect(minX, minY, maxX-minX, maxY-minY);

    return result;
}

}  // namespace apl
