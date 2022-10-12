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

#ifndef _APL_SG_PAINT_H
#define _APL_SG_PAINT_H

#include "apl/scenegraph/common.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/userdata.h"
#include "apl/primitives/color.h"
#include "apl/primitives/gradient.h"

namespace apl {
namespace sg {

class Paint : public Counter<Paint>,
              public UserData<Paint>,
              public NonCopyable,
              public std::enable_shared_from_this<Paint> {
public:
    enum Type {
        kColor,
        kLinearGradient,
        kRadialGradient,
        kPattern
    };

    /**
     * @return The type of this paint.
     */
    Type type() const { return mType; }

    /**
     * @return True if the paint has been modified.  Calling this clears the flag
     */
    bool modified() {
        auto result = mModified;
        mModified = false;
        return result;
    }

    bool setOpacity(float opacity);
    float getOpacity() const { return mOpacity; }

    bool setTransform(const Transform2D& transform2D);
    const Transform2D& getTransform() const { return mTransform; }

    /**
     * @return True if this paint is visible on the screen (not transparent)
     */
    virtual bool visible() const = 0;

    /**
     * @return A human-readable debugging string
     */
    virtual std::string toDebugString() const = 0;

    virtual rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

protected:
    explicit Paint(Type type) : mType(type) {}

    const Type mType;
    bool mModified = false;
    float mOpacity = 1.0f;
    Transform2D mTransform;  // TODO: Use this for AVG Path/Text drawing...
};


#define PAINT_SUBCLASS(TYPE_NAME, PARENT_TYPE, TYPE_ENUM)                 \
  public:                                                   \
    TYPE_NAME() : PARENT_TYPE(TYPE_ENUM) {}                        \
    static const TYPE_NAME *cast(const Paint *paint) {                    \
        assert(paint != nullptr && paint->type() == TYPE_ENUM); \
        return reinterpret_cast<const TYPE_NAME*>(paint);          \
    }                                                       \
    static TYPE_NAME *cast(Paint *paint) {                    \
        assert(paint != nullptr && paint->type() == TYPE_ENUM); \
        return reinterpret_cast<TYPE_NAME*>(paint);          \
    }                                                       \
    static TYPE_NAME *cast(const PaintPtr& paint) {           \
        return cast(paint.get());                            \
    }                                                       \
    static bool is_type(const Paint *paint) { return paint && paint->type() == TYPE_ENUM; } \
    static bool is_type(const PaintPtr& paint) { return is_type(paint.get()); }          \
    std::string toDebugString() const override;                           \
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override


class ColorPaint : public Paint {
    PAINT_SUBCLASS(ColorPaint, Paint, kColor);
    bool visible() const override { return mOpacity > 0 && mColor.alpha() > 0; }

    bool setColor(Color color);
    Color getColor() const { return mColor; }

private:
    Color mColor = Color::TRANSPARENT;
};

class GradientPaint : public Paint {
public:
    explicit GradientPaint(Type type) : Paint(type) {}
    bool visible() const override {
        if (mOpacity <= 0)
            return false;

        for (const auto& color : mColors)
            if (color.alpha() > 0)
                return true;
        return false;
    }

    bool setPoints(const std::vector<double>& points);
    const std::vector<double>& getPoints() const { return mPoints; }

    bool setColors(const std::vector<Color>& colors);
    const std::vector<Color>& getColors() const { return mColors; }

    bool setSpreadMethod(Gradient::GradientSpreadMethod spreadMethod);
    Gradient::GradientSpreadMethod getSpreadMethod() const { return mSpreadMethod; }

    bool setUseBoundingBox(bool useBoundingBox);
    bool getUseBoundingBox() const { return mUseBoundingBox; }

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override;

private:
    std::vector<double> mPoints;
    std::vector<Color> mColors;
    Gradient::GradientSpreadMethod mSpreadMethod = Gradient::GradientSpreadMethod::PAD;
    bool mUseBoundingBox = true;
};

class LinearGradientPaint : public GradientPaint {
    PAINT_SUBCLASS(LinearGradientPaint, GradientPaint, kLinearGradient);

    bool setStart(const Point& start);
    Point getStart() const { return mStart; }

    bool setEnd(const Point& end);
    Point getEnd() const { return mEnd; }

private:
    Point mStart;
    Point mEnd;
};

class RadialGradientPaint : public GradientPaint {
    PAINT_SUBCLASS(RadialGradientPaint, GradientPaint, kRadialGradient);

    bool setCenter(const Point& center);
    Point getCenter() const { return mCenter; }

    bool setRadius(float radius);
    float getRadius() const { return mRadius; }

private:
    Point mCenter;
    float mRadius = 1.0f;
};

class PatternPaint : public Paint {
public:
    PAINT_SUBCLASS(PatternPaint, Paint, kPattern);
    bool visible() const override { return mOpacity > 0; }  // TODO: Fix this

    bool setSize(const Size& size);
    Size getSize() const { return mSize; }

    bool setNode(const NodePtr& node);
    NodePtr getNode() const { return mNode; }

private:
    Size mSize;
    NodePtr mNode;
};

} // namespace sg
} // namespace apl

#endif // _APL_SG_PAINT_H
