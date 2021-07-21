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

#include <functional>

#include "apl/primitives/transform.h"
#include "apl/utils/session.h"
#include "apl/engine/evaluate.h"
#include "apl/primitives/dimension.h"

namespace apl {

// Evaluate this object as a dimension.  If relative, use the side length to convert to a DP value.
inline float
evalDim(const Dimension& object, float side)
{
    return object.isRelative() ? side * object.getValue() / 100 : object.getValue();
}

using ScalarFunction = std::function<Transform2D(float)>;

class ScalarTransform : public Transform {
public:
    ScalarTransform(ScalarFunction function, Type type, float value) : mFunction(function), mType(type), mValue(value) {};

    Transform2D evaluate(float, float) const override {
        return mFunction(mValue);
    }

    bool canInterpolate(Transform& other) const override {
        auto o = dynamic_cast<ScalarTransform*>(&other);
        if (o == nullptr)
            return false;

        return &mFunction == &o->mFunction;
    }

    Transform2D interpolate(Transform& other, float alpha, float, float) const override {
        float v2 = static_cast<ScalarTransform&>(other).mValue;
        float value = mValue * (1-alpha) + v2 * alpha;
        return mFunction(value);
    }

    Type getType() const override { return mType; }

private:
    ScalarFunction mFunction;
    Type mType;
    float mValue;
};

class ScaleTransform : public Transform {
public:
    ScaleTransform(float x, float y) : mX(x), mY(y) {};

    Transform2D evaluate(float, float) const override {
        return Transform2D::scale(mX, mY);
    }

    bool canInterpolate(Transform& other) const override {
        return dynamic_cast<ScaleTransform*>(&other) != nullptr;
    }

    Transform2D interpolate(Transform& other, float alpha, float, float) const override {
        auto o = dynamic_cast<ScaleTransform*>(&other);
        float x = mX * (1-alpha) + o->mX * alpha;
        float y = mY * (1-alpha) + o->mY * alpha;
        return Transform2D::scale(x, y);
    }

    Type getType() const override { return SCALE; }

private:
    float mX;
    float mY;
};

class TranslateTransform : public Transform {
public:
    TranslateTransform(const Dimension& x, const Dimension& y) : mX(x), mY(y) {}

    Transform2D evaluate(float width, float height) const override {
        return Transform2D::translate(evalDim(mX, width), evalDim(mY, height));
    }

    bool canInterpolate(Transform& other) const override {
        return dynamic_cast<TranslateTransform*>(&other) != nullptr;
    }

    Transform2D interpolate(Transform& other, float alpha, float width, float height) const override {
        auto o = dynamic_cast<TranslateTransform*>(&other);
        float x = evalDim(mX, width) * (1-alpha) + evalDim(o->mX, width) * alpha;
        float y = evalDim(mY, height) * (1-alpha) + evalDim(o->mY, height) * alpha;
        return Transform2D::translate(x, y);
    }

    Type getType() const override { return TRANSLATE; }

private:
    Dimension mX;
    Dimension mY;
};

/**
 * Convert from an object to a transform.  The objects are of the form { "rotate": VALUE } or
 * { "scale": 2, "scaleY": 3 }.
 * @param context The data-binding context to evaluate within.
 * @param element The object to be converted into a transform.
 * @return The transform.
 */
static std::unique_ptr<Transform> transformFromElement(const Context& context, const Object& element)
{
    if (!element.isMap()) {
        CONSOLE_CTX(context) << "Illegal transform element " << element;
        return nullptr;
    }

    auto rotate = propertyAsObject(context, element, "rotate");
    if (rotate != Object::NULL_OBJECT())
        return std::unique_ptr<Transform>(new ScalarTransform(Transform2D::rotate, Transform::ROTATE, rotate.asNumber()));

    auto scaleX = propertyAsObject(context, element, "scaleX");
    auto scaleY = propertyAsObject(context, element, "scaleY");
    auto scale = propertyAsObject(context, element, "scale");
    if (scale != Object::NULL_OBJECT() || scaleX != Object::NULL_OBJECT() || scaleY != Object::NULL_OBJECT()) {
        float sx = 1;
        float sy = 1;
        if (scale != Object::NULL_OBJECT())
            sx = sy = scale.asNumber();
        if (scaleX != Object::NULL_OBJECT())
            sx = scaleX.asNumber();
        if (scaleY != Object::NULL_OBJECT())
            sy = scaleY.asNumber();
        return std::unique_ptr<Transform>(new ScaleTransform(sx, sy));
    }

    auto skewX = propertyAsObject(context, element, "skewX");
    if (skewX != Object::NULL_OBJECT())
        return std::unique_ptr<Transform>(new ScalarTransform(Transform2D::skewX, Transform::SKEW_X, skewX.asNumber()));

    auto skewY = propertyAsObject(context, element, "skewY");
    if (skewY != Object::NULL_OBJECT())
        return std::unique_ptr<Transform>(new ScalarTransform(Transform2D::skewY, Transform::SKEW_Y, skewY.asNumber()));

    auto translateX = propertyAsObject(context, element, "translateX");
    auto translateY = propertyAsObject(context, element, "translateY");
    if (translateX != Object::NULL_OBJECT() || translateY != Object::NULL_OBJECT()) {
        Dimension tx = 0;
        Dimension ty = 0;
        if (translateX != Object::NULL_OBJECT())
            tx = translateX.asNonAutoDimension(context);
        if (translateY != Object::NULL_OBJECT())
            ty = translateY.asNonAutoDimension(context);
        return std::unique_ptr<Transform>(new TranslateTransform(tx, ty));
    }

    CONSOLE_CTX(context) << "Transform element doesn't have a valid property" << element;
    return nullptr;
}

/******************************************************/

class TransformationImpl : public Transformation {
public:
    TransformationImpl(const Context& context, const std::vector<Object>& array)
        : mLastWidth(-1),
          mLastHeight(-1),
          mNeedsCentering(false)
    {
        for (auto& item : array) {
            auto transform = transformFromElement(context, item);
            if (transform) {
                if (transform->getType() != Transform::TRANSLATE)
                    mNeedsCentering = true;
                mArray.push_back(std::move(transform));
            }
        }
    }

    Transform2D get(float width, float height) override
    {
        if (width == mLastWidth && height == mLastHeight)
            return mTransform2D;

        mLastWidth = width;
        mLastHeight = height;

        mTransform2D = Transform2D();
        if (mArray.size() == 0)
            return mTransform2D;

        // Rotation happens about the origin
        if (mNeedsCentering)
            mTransform2D = Transform2D::translate(width/2, height/2);

        for (auto& item : mArray)
            mTransform2D *= item->evaluate(width, height);

        if (mNeedsCentering)
            mTransform2D *= Transform2D::translate(-width/2, -height/2);

        return mTransform2D;
    }

private:
    std::vector<std::unique_ptr<Transform>> mArray;

    // Minor optimizations to avoid recalculating the matrix.
    Transform2D mTransform2D;
    float mLastWidth;
    float mLastHeight;
    bool mNeedsCentering;   // Minor optimization - skip centering if we only are dealing with translation.
};

/******************************************************/

class InterpolatedTransformationImpl : public InterpolatedTransformation {
public:
    InterpolatedTransformationImpl(const Context& context, const std::vector<Object>& from, const std::vector<Object>& to)
        : mAlpha(0),
          mLastWidth(-1),
          mLastHeight(-1),
          mNeedsCentering(false)
    {
        auto len = std::min(from.size(), to.size());
        if (len != from.size() || len != to.size())
            CONSOLE_CTX(context) << "Mismatched transformation lengths";

        for (int i = 0 ; i < len ; i++) {
            auto fromTransform = transformFromElement(context, from.at(i));
            if (fromTransform == nullptr)
                continue;

            auto toTransform = transformFromElement(context, to.at(i));
            if (toTransform == nullptr)
                continue;

            auto fromType = fromTransform->getType();
            auto toType = toTransform->getType();
            if (fromType != toType) {
                CONSOLE_CTX(context) << "Type mismatch between animation elements " << i << " from:" << fromType << " to:" << toType;
                continue;
            }

            mFrom.push_back(std::move(fromTransform));
            mTo.push_back(std::move(toTransform));

            if (fromType != Transform::TRANSLATE)
                mNeedsCentering = true;
        }
    }

    Transform2D get(float width, float height) override
    {
        if (width == mLastWidth && height == mLastHeight)
            return mTransform2D;

        mLastWidth = width;
        mLastHeight = height;

        mTransform2D = Transform2D();
        if (mFrom.size() == 0)
            return mTransform2D;

        // Rotation happens about the origin
        if (mNeedsCentering)
            mTransform2D = Transform2D::translate(width/2, height/2);

        for (int i = 0 ; i < mFrom.size() ; i++) {
            auto from = mFrom.at(i).get();
            auto to = mTo.at(i).get();
            mTransform2D *= from->interpolate(*to, mAlpha, width, height);
        }

        if (mNeedsCentering)
            mTransform2D *= Transform2D::translate(-width/2, -height/2);

        return mTransform2D;
    }

    bool interpolate(float alpha) override 
    {
        if (alpha == mAlpha)
            return false;

        mAlpha = alpha;
        mLastWidth = -1;   // Force recalculation
        return true;
    }

private:
    std::vector<std::unique_ptr<Transform>> mFrom;
    std::vector<std::unique_ptr<Transform>> mTo;

    // Minor optimizations to avoid recalculating the matrix.
    Transform2D mTransform2D;
    float mAlpha;
    float mLastWidth;
    float mLastHeight;
    bool mNeedsCentering;   // Minor optimization - skip centering if we only are dealing with translation.
};



/******************************************************/

std::shared_ptr<Transformation>
Transformation::create(const Context& context, const std::vector<Object>& array)
{
    return std::make_shared<TransformationImpl>(context, array);
}

std::shared_ptr<InterpolatedTransformation>
InterpolatedTransformation::create(const Context& context,
                                   const std::vector<Object>& from,
                                   const std::vector<Object>& to)
{
    return std::make_shared<InterpolatedTransformationImpl>(context, from, to);
}


} // namespace apl
