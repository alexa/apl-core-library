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

#ifndef _APL_SHADOW_H
#define _APL_SHADOW_H

#include "apl/primitives/color.h"
#include "apl/primitives/point.h"
#include "apl/utils/noncopyable.h"

namespace apl {
namespace sg {

class Shadow : public NonCopyable {
public:
    virtual ~Shadow() {}

    bool visible() const {
        return (mOffset.getX() != 0 || mOffset.getY() != 0 || mRadius != 0) &&
               !mColor.transparent();
    }

    void setColor(Color color) { mColor = color; }
    Color getColor() const { return mColor; }

    void setOffset(Point point) { mOffset = point; }
    Point getOffset() const { return mOffset; }

    void setRadius(float radius) { mRadius = radius; }
    float getRadius() const { return mRadius; }

    friend bool operator==(const Shadow& lhs, const Shadow& rhs){
        return lhs.mColor == rhs.mColor && lhs.mOffset == rhs.mOffset && lhs.mRadius == rhs.mRadius;
    }

    friend bool operator!=(const Shadow& lhs, const Shadow& rhs){
        return lhs.mColor != rhs.mColor || lhs.mOffset != rhs.mOffset || lhs.mRadius != rhs.mRadius;
    }

    std::string toDebugString() const {
        return "Shadow " + mColor.asString() + " " + mOffset.toString() + " " + std::to_string(mRadius);
    }

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const {
        rapidjson::Value out(rapidjson::kObjectType);
        out.AddMember("color", mColor.serialize(allocator), allocator);
        out.AddMember("offset", mOffset.serialize(allocator), allocator);
        out.AddMember("radius", mRadius, allocator);
        return out;
    }

private:
    Color mColor = Color::TRANSPARENT;
    Point mOffset;
    float mRadius = 0.0f;
};

} // namespace sg
} // namespace apl

#endif // _APL_SHADOW_H
