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

#include "apl/scenegraph/path.h"
#include "apl/scenegraph/pathbounds.h"

namespace apl {
namespace sg {

bool
operator==(const PathPtr& lhs, const PathPtr& rhs) {
    if (lhs == nullptr && rhs == nullptr)
        return true;
    if (lhs == nullptr || rhs == nullptr || lhs->type() != rhs->type())
        return false;

    switch (lhs->type()) {
        case Path::kRect: {
            auto left = RectPath::cast(lhs);
            auto right = RectPath::cast(rhs);
            return left->getRect() == right->getRect();
        }
        case Path::kRoundedRect: {
            auto left = RoundedRectPath::cast(lhs);
            auto right = RoundedRectPath::cast(rhs);
            return left->getRoundedRect() == right->getRoundedRect();
        }
        case Path::kGeneral: {
            auto left = GeneralPath::cast(lhs);
            auto right = GeneralPath::cast(rhs);
            return left->getValue() == right->getValue() && left->getPoints() == right->getPoints();
        }
        case Path::kFrame: {
            auto left = FramePath::cast(lhs);
            auto right = FramePath::cast(rhs);
            return left->getRoundedRect() == right->getRoundedRect() && left->getInset() == right->getInset();
        }
    }
    return false;
}

bool
RectPath::empty() const
{
    // A rectangular path always has segments that can be stroked.
    return false;
}

std::string
RectPath::toDebugString() const
{
    return std::string("RectPath ") + mRect.toDebugString();
}

bool
RectPath::setRect(Rect rect)
{
    if (rect == mRect)
        return false;

    mRect = rect;
    mModified = true;
    return true;
}

Rect
RectPath::boundingBox(const Transform2D& transform) const
{
    return mRect.boundingBox(transform);
}

rapidjson::Value
RectPath::serialize(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("type", rapidjson::StringRef("rectPath"), allocator);
    result.AddMember("rect", mRect.serialize(allocator), allocator);
    return result;
}

bool
RoundedRectPath::empty() const
{
    // A rounded rectangular path always has segments that can be stroked.
    return false;
}

std::string
RoundedRectPath::toDebugString() const
{
    return std::string("RoundedRectPath ") + mRoundedRect.toDebugString();
}

bool
RoundedRectPath::setRoundedRect(const RoundedRect& roundedRect)
{
    if (mRoundedRect == roundedRect)
        return false;

    mRoundedRect = roundedRect;
    mModified = true;
    return true;
}

Rect
RoundedRectPath::boundingBox(const Transform2D& transform) const
{
    // Note: this bounding box could be made tighter if there are large radii and 45 degree rotation
    return mRoundedRect.rect().boundingBox(transform);
}

rapidjson::Value
RoundedRectPath::serialize(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("type", rapidjson::StringRef("roundedRectPath"), allocator);
    result.AddMember("rect", mRoundedRect.rect().serialize(allocator), allocator);
    result.AddMember("radii", mRoundedRect.radii().serialize(allocator), allocator);
    return result;
}

bool
FramePath::empty() const
{
    // A frame path always has segments that can be stroked.
    return false;
}

std::string
FramePath::toDebugString() const
{
    return std::string("FramePath ") + mRoundedRect.toDebugString() +
           " inset=" + sutil::to_string(mInset);
}

bool
FramePath::setRoundedRect(const RoundedRect& roundedRect)
{
    if (mRoundedRect == roundedRect)
        return false;

    mRoundedRect = roundedRect;
    mModified = true;
    return true;
}

bool
FramePath::setInset(float inset)
{
    if (mInset == inset)
        return false;

    mInset = inset;
    mModified = true;
    return true;
}

Rect
FramePath::boundingBox(const Transform2D& transform) const
{
    // Note: this bounding box could be made tighter if there are large radii and 45 degree rotation
    return mRoundedRect.rect().boundingBox(transform);
}

rapidjson::Value
FramePath::serialize(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("type", rapidjson::StringRef("framePath"), allocator);
    result.AddMember("rect", mRoundedRect.rect().serialize(allocator), allocator);
    result.AddMember("radii", mRoundedRect.radii().serialize(allocator), allocator);
    result.AddMember("inset", mInset, allocator);
    return result;
}

bool
GeneralPath::empty() const
{
    for (const auto& m : mValue)
        if (m != 'M' && m != 'Z')
            return false;

    return true;
}

std::string
GeneralPath::toDebugString() const
{
    auto result = std::string("GeneralPath ") + mValue + " [";
    auto len = mPoints.size();
    if (len)
        result += sutil::to_string(mPoints.at(0));
    for (int i = 1 ; i < len ; i++)
        result += "," + sutil::to_string(mPoints.at(i));
    return result + "]";
}

Rect
GeneralPath::boundingBox(const Transform2D& transform) const
{
    return calculatePathBounds(transform, mValue, mPoints);
}

rapidjson::Value
GeneralPath::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("type", rapidjson::StringRef("generalPath"), allocator);
    result.AddMember("values", rapidjson::Value(mValue.c_str(), allocator), allocator);
    rapidjson::Value points(rapidjson::kArrayType);
    for (const auto& m : mPoints)
        points.PushBack(m, allocator);
    result.AddMember("points", points, allocator);
    return result;
}


} // namespace sg
} // namespace apl