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

#include "apl/scenegraph/paint.h"
#include "apl/scenegraph/node.h"

namespace apl {

extern Bimap<Gradient::GradientSpreadMethod, std::string> sGradientSpreadMethodMap;

namespace sg {

static std::string tail(const Paint& paint)
{
    return std::string(" opacity=") + std::to_string(paint.getOpacity()) +
        " transform=" + paint.getTransform().toDebugString();
}

bool
Paint::setOpacity(float opacity)
{
    if (opacity == mOpacity)
        return false;

    mOpacity = opacity;
    mModified = true;
    return true;
}

bool
Paint::setTransform(const Transform2D& transform2D)
{
    if (mTransform == transform2D)
        return false;

    mTransform = std::move(transform2D);
    mModified = true;
    return true;
}

rapidjson::Value
Paint::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = rapidjson::Value(rapidjson::kObjectType);
    result.AddMember("opacity", mOpacity, allocator);
    if (!mTransform.isIdentity())
        result.AddMember("transform", mTransform.serialize(allocator), allocator);
    return result;
}

std::string
ColorPaint::toDebugString() const
{
    return "ColorPaint color=" + mColor.asString() + tail(*this);
}

bool
ColorPaint::setColor(Color color)
{
    if (color == mColor)
        return false;

    mColor = color;
    mModified = true;
    return true;
}

rapidjson::Value
ColorPaint::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Paint::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("colorPaint"), allocator);
    result.AddMember("color", mColor.serialize(allocator), allocator);
    return result;
}


static std::string
gradToString(const GradientPaint& grad) {
    auto result = std::string("points=[");

    auto len = grad.getPoints().size();
    if (len)
        result += std::to_string(grad.getPoints().at(0));
    for (auto i = 1 ; i < len ; i++)
        result += "," + std::to_string(grad.getPoints().at(i));

    result += "] colors=[";
    len = grad.getColors().size();
    if (len)
        result += grad.getColors().at(0).asString();
    for (auto i = 1 ; i < len ; i++)
        result += "," + grad.getColors().at(i).asString();

    result += std::string("] spread=") + std::to_string(grad.getSpreadMethod()) +
              " bb=" + (grad.getUseBoundingBox() ? "yes" : "no");
    return result + tail(grad);
}

bool
GradientPaint::setPoints(const std::vector<double>& points)
{
    if (mPoints == points)
        return false;

    mPoints = std::move(points);
    mModified = true;
    return true;
}

bool
GradientPaint::setColors(const std::vector<Color>& colors)
{
    if (mColors == colors)
        return false;

    mColors = std::move(colors);
    mModified = true;
    return true;
}

bool
GradientPaint::setSpreadMethod(Gradient::GradientSpreadMethod spreadMethod)
{
    if (mSpreadMethod == spreadMethod)
        return false;

    mSpreadMethod = spreadMethod;
    mModified = true;
    return true;
}

bool
GradientPaint::setUseBoundingBox(bool useBoundingBox)
{
    if (mUseBoundingBox == useBoundingBox)
        return false;

    mUseBoundingBox = useBoundingBox;
    mModified = true;
    return true;
}

rapidjson::Value
GradientPaint::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Paint::serialize(allocator);

    auto pointArray = rapidjson::Value(rapidjson::kArrayType);
    for (const auto& m : mPoints)
        pointArray.PushBack(m, allocator);
    result.AddMember("points", pointArray, allocator);

    auto colorArray = rapidjson::Value(rapidjson::kArrayType);
    for (const auto& m : mColors)
        colorArray.PushBack(m.serialize(allocator), allocator);
    result.AddMember("colors", colorArray, allocator);

    result.AddMember(
        "spreadMethod",
        rapidjson::Value(sGradientSpreadMethodMap.at(mSpreadMethod).c_str(), allocator), allocator);
    result.AddMember("usingBoundingBox", mUseBoundingBox, allocator);

    return result;
}


std::string
LinearGradientPaint::toDebugString() const
{
    return std::string("LinearGradientPaint ") + gradToString(*this) +
           " start=" + mStart.toString() + " end=" + mEnd.toString();
}

bool
LinearGradientPaint::setStart(const Point& start)
{
    if (mStart == start)
        return false;

    mStart = start;
    mModified = true;
    return true;
}

bool
LinearGradientPaint::setEnd(const Point& end)
{
    if (mEnd == end)
        return false;

    mEnd = end;
    mModified = true;
    return true;
}

rapidjson::Value
LinearGradientPaint::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = GradientPaint::serialize(allocator);

    result.AddMember("type", "linearGradient", allocator);
    result.AddMember("start", mStart.serialize(allocator), allocator);
    result.AddMember("end", mEnd.serialize(allocator), allocator);

    return result;
}

std::string
RadialGradientPaint::toDebugString() const
{
    return std::string("RadialGradientPaint ") + gradToString(*this) +
           " center=" + mCenter.toString() + " radius=" + std::to_string(mRadius);
}

bool
RadialGradientPaint::setCenter(const Point& center)
{
    if (mCenter == center)
        return false;

    mCenter = center;
    mModified = true;
    return true;
}

bool
RadialGradientPaint::setRadius(float radius)
{
    if (mRadius == radius)
        return false;

    mRadius = radius;
    mModified = true;
    return true;
}

rapidjson::Value
RadialGradientPaint::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = GradientPaint::serialize(allocator);

    result.AddMember("type", "radialGradient", allocator);
    result.AddMember("center", mCenter.serialize(allocator), allocator);
    result.AddMember("radius", mRadius, allocator);

    return result;
}


std::string
PatternPaint::toDebugString() const
{
    return "PatternPaint size=" + mSize.toString() + tail(*this);
}

bool
PatternPaint::setSize(const Size& size)
{
    if (mSize == size)
        return false;

    mSize = size;
    mModified = true;
    return true;
}

bool
PatternPaint::setNode(const NodePtr& node)
{
    if (mNode == node)
        return false;

    mNode = node;
    mModified = true;
    return true;
}

rapidjson::Value
PatternPaint::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Paint::serialize(allocator);

    result.AddMember("type", "patternPaint", allocator);
    result.AddMember("size", mSize.serialize(allocator), allocator);
    result.AddMember("node", mNode->serialize(allocator), allocator);  // TODO: Do we need an array of node here?

    return result;
}

} // namespace sg
} // namespace apl