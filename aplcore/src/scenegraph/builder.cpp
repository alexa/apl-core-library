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

#include "apl/scenegraph/layer.h"

#include "apl/component/corecomponent.h"
#include "apl/graphic/graphicelement.h"
#include "apl/graphic/graphicpattern.h"
#include "apl/primitives/accessibilityaction.h"
#include "apl/scenegraph/accessibility.h"
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/graphicfragment.h"
#include "apl/scenegraph/pathparser.h"
#include "apl/scenegraph/scenegraphupdates.h"

namespace apl {
namespace sg {

LayerPtr
layer(const std::string& name, Rect bounds, float opacity, Transform2D transform)
{
    return std::make_shared<Layer>(name, bounds, opacity, std::move(transform));
}

NodePtr
transform(Transform2D transform, const NodePtr& child)
{
    auto node = std::make_shared<TransformNode>();
    node->setTransform(transform);
    node->setChild(child);
    return node;
}

NodePtr
transform(Point offset, const NodePtr& child)
{
    return transform(Transform2D::translate(offset), child);
}

NodePtr
transform(const Object& object, const NodePtr& child)
{
    return transform(object.get<Transform2D>(), child);
}

NodePtr
transform()
{
    return std::make_shared<TransformNode>();
}

NodePtr
clip(PathPtr path, const NodePtr& child)
{
    auto node = std::make_shared<ClipNode>();
    node->setPath(std::move(path));
    node->setChild(child);
    return node;
}


NodePtr
opacity(float opacity, const NodePtr& child)
{
    auto node = std::make_shared<OpacityNode>();
    node->setOpacity(opacity);
    node->setChild(child);
    return node;
}

NodePtr
opacity(const Object& object, const NodePtr& child)
{
    return opacity(static_cast<float>(object.asNumber()), child);
}

NodePtr
image(FilterPtr image, Rect target, Rect source)
{
    // Note that the image may be null
    auto node = std::make_shared<ImageNode>();
    node->setImage(std::move(image));
    node->setTarget(std::move(target));
    node->setSource(std::move(source));
    return node;
}

NodePtr
video(MediaPlayerPtr player, Rect target, VideoScale scale)
{
    assert(player);
    auto node = std::make_shared<VideoNode>();
    node->setMediaPlayer(std::move(player));
    node->setTarget(std::move(target));
    node->setScale(scale);
    return node;
}

NodePtr
shadowNode(ShadowPtr shadow, const NodePtr& child)
{
    // No assert. nullptr shadow is a valid case.
    auto node = std::make_shared<ShadowNode>();
    if (shadow) node->setShadow(std::move(shadow));
    node->setChild(child);
    return node;
}

NodePtr
draw(PathPtr path, PathOpPtr op)
{
    assert(path);
    assert(op);
    auto node = std::make_shared<DrawNode>();
    node->setPath(std::move(path));
    node->setOp(std::move(op));
    return node;
}

NodePtr
text(TextLayoutPtr textLayout, PathOpPtr op)
{
    assert(textLayout);
    assert(op);
    auto node = std::make_shared<TextNode>();
    node->setTextLayout(std::move(textLayout));
    node->setOp(std::move(op));
    return node;
}

NodePtr
text(TextLayoutPtr textLayout, PathOpPtr op, Range range)
{
    assert(textLayout);
    assert(op);
    auto node = std::make_shared<TextNode>();
    node->setTextLayout(std::move(textLayout));
    node->setOp(std::move(op));
    node->setRange(range);
    return node;
}

NodePtr
editText(EditTextPtr editText, EditTextBoxPtr editTextBox,
         EditTextConfigPtr editTextConfig, const std::string& text)
{
    assert(editText);
    auto node = std::make_shared<EditTextNode>();
    node->setEditText(std::move(editText));
    node->setEditTextBox(std::move(editTextBox));
    node->setEditTextConfig(std::move(editTextConfig));
    node->setText(text);
    return node;
}

PathPtr
path(apl::Rect rect)
{
    auto path = std::make_shared<RectPath>();
    path->setRect(rect);
    return path;
}

PathPtr
path(apl::Rect rect, float radius)
{
    auto path = std::make_shared<RoundedRectPath>();
    path->setRoundedRect(RoundedRect{rect, radius});
    return path;
}


PathPtr
path(Rect rect, Radii radii)
{
    auto path = std::make_shared<RoundedRectPath>();
    path->setRoundedRect(RoundedRect{rect, radii});
    return path;
}

PathPtr
path(RoundedRect roundedRect)
{
    auto path = std::make_shared<RoundedRectPath>();
    path->setRoundedRect(roundedRect);
    return path;
}

PathPtr
path(RoundedRect roundedRect, float inset)
{
    auto path = std::make_shared<FramePath>();
    path->setRoundedRect(std::move(roundedRect));
    path->setInset(inset);
    return path;
}

PathPtr
path(const std::string& path)
{
    return parsePathString(path);
}

PaintPtr
paint(Color color, float opacity)
{
    auto paint = std::make_shared<ColorPaint>();
    paint->setColor(color);
    paint->setOpacity(opacity);
    return paint;
}

PaintPtr
paint(const Gradient& gradient, float opacity, Transform2D transform)
{
    auto spreadMethod = static_cast<Gradient::GradientSpreadMethod>(gradient.getProperty(
        kGradientPropertySpreadMethod).asInt());
    auto useBoundingBox = gradient.getProperty(kGradientPropertyUnits).asInt() == Gradient::kGradientUnitsBoundingBox;

    switch (gradient.getType()) {
        case Gradient::LINEAR: {
            auto paint = std::make_shared<LinearGradientPaint>();
            paint->setOpacity(opacity);
            paint->setTransform(std::move(transform));
            paint->setPoints(gradient.getInputRange());
            paint->setColors(gradient.getColorRange());
            paint->setStart(Point {
                static_cast<float>(gradient.getProperty(kGradientPropertyX1).asNumber()),
                static_cast<float>(gradient.getProperty(kGradientPropertyY1).asNumber())
            });
            paint->setEnd(Point {
                static_cast<float>(gradient.getProperty(kGradientPropertyX2).asNumber()),
                static_cast<float>(gradient.getProperty(kGradientPropertyY2).asNumber())
            });
            paint->setSpreadMethod(spreadMethod);
            paint->setUseBoundingBox(useBoundingBox);
            return paint;
        }

        case Gradient::RADIAL: {
            auto paint = std::make_shared<RadialGradientPaint>();
            paint->setOpacity(opacity);
            paint->setTransform(transform);
            paint->setPoints(gradient.getInputRange());
            paint->setColors(gradient.getColorRange());
            paint->setRadius(static_cast<float>(gradient.getProperty(kGradientPropertyRadius).asNumber()));
            paint->setCenter(Point {
                static_cast<float>(gradient.getProperty(kGradientPropertyCenterX).asNumber()),
                static_cast<float>(gradient.getProperty(kGradientPropertyCenterY).asNumber())
            });
            paint->setSpreadMethod(spreadMethod);
            paint->setUseBoundingBox(useBoundingBox);
            return paint;
        }
    }

    return std::make_shared<ColorPaint>();
}

PaintPtr
paint(const GraphicPatternPtr& pattern, float opacity, Transform2D transform)
{
    assert(pattern);

    auto paint = std::make_shared<PatternPaint>();
    paint->setOpacity(opacity);
    paint->setTransform(transform);
    paint->setSize(Size{
        static_cast<float>(pattern->getWidth()),
        static_cast<float>(pattern->getHeight())
    });

    SceneGraphUpdates updates;

    NodePtr node = nullptr;
    for (auto it = pattern->getItems().rbegin() ; it != pattern->getItems().rend() ; it++) {
        auto child = (*it)->buildSceneGraph(false, updates)->node();
        if (child)
            node = child->setNext(node);
    }

    paint->setNode(node);
    return paint;
}

PaintPtr
paint(const Object& object, float opacity, Transform2D transform)
{
    if (object.is<Color>())
        return paint(static_cast<Color>(object.getColor()), opacity);

    if (object.is<Gradient>())
        return paint(object.get<Gradient>(), opacity, transform);

    if (object.is<GraphicPattern>())
        return paint(object.get<GraphicPattern>(), opacity, transform);

    return std::make_shared<ColorPaint>();
}

PathOpPtr
fill(PaintPtr paint, FillType fillType)
{
    auto op = std::make_shared<FillPathOp>();
    op->paint = std::move(paint);
    op->fillType = fillType;
    return op;
}


// ****************************************************************************

ShadowPtr
shadow(Color color, Point offset, float radius)
{
    if (color == Color::TRANSPARENT || (offset.empty() && radius <= 0.0))
        return nullptr;

    auto shadow = std::make_shared<Shadow>();
    shadow->setColor(color);
    shadow->setOffset(offset);
    shadow->setRadius(radius);
    return shadow;
}

// ****************************************************************************

AccessibilityPtr
accessibility(CoreComponent& component)
{
    auto label = component.getCalculated(kPropertyAccessibilityLabel).getString();
    auto role = component.getCalculated(kPropertyRole).asEnum<Role>();
    const auto& actions = component.getCalculated(kPropertyAccessibilityActions).getArray();
    auto adjustableRange = component.getCalculated(kPropertyAccessibilityAdjustableRange);
    auto adjustableValue = component.getCalculated(kPropertyAccessibilityAdjustableValue).asString();

    if (label.empty() && actions.empty() && role == kRoleNone && adjustableRange.isNull() && adjustableValue.empty())
        return nullptr;

    auto weak = std::weak_ptr<CoreComponent>(component.shared_from_corecomponent());
    auto acc = std::make_shared<Accessibility>([weak](const std::string& name) {
        auto strong = weak.lock();
        if (strong)
            strong->update(kUpdateAccessibilityAction, name);
    });

    acc->setLabel(label);
    acc->setRole(role);

    for (const auto& m : actions) {
        const auto& ptr = m.get<AccessibilityAction>();
        acc->appendAction(ptr->getName(), ptr->getLabel(), ptr->enabled());
    }

    acc->setAdjustableRange(adjustableRange);
    acc->setAdjustableValue(adjustableValue);

    return acc;
}

// ****************************************************************************

FilterPtr
filter(MediaObjectPtr mediaObject)
{
    auto result = std::make_shared<MediaObjectFilter>();
    result->mediaObject = std::move(mediaObject);
    return result;
}

// Note:  Blend operations are not automatically created
FilterPtr
blend(FilterPtr back, FilterPtr front, BlendMode blendMode)
{
    if (!front || !front->visible())
        return back;

    if (!back || !back->visible())
        return front;

    auto result = std::make_shared<BlendFilter>();
    result->back = std::move(back);
    result->front = std::move(front);
    result->blendMode = blendMode;
    return result;
}

FilterPtr
blur(FilterPtr filter, float radius)
{
    if (radius <= 0 || !filter)
        return filter;

    auto result = std::make_shared<BlurFilter>();
    result->filter = std::move(filter);
    result->radius = radius;
    return result;
}

FilterPtr
grayscale(FilterPtr filter, float amount)
{
    if (amount <= 0 || !filter)
        return filter;

    if (amount > 1.0)
        amount = 1.0;

    auto result = std::make_shared<GrayscaleFilter>();
    result->filter = std::move(filter);
    result->amount = amount;
    return result;
}

FilterPtr
noise(FilterPtr filter, NoiseFilterKind kind, bool useColor, float sigma )
{
    if (sigma <= 0.0 || !filter)
        return filter;

    auto result = std::make_shared<NoiseFilter>();
    result->filter = std::move(filter);
    result->kind = kind;
    result->useColor = useColor;
    result->sigma = sigma;
    return result;
}

FilterPtr
saturate(FilterPtr filter, float amount)
{
    if (amount < 0 || !filter)
        return filter;

    auto result = std::make_shared<SaturateFilter>();
    result->filter = std::move(filter);
    result->amount = amount;
    return result;
}

FilterPtr
solid(PaintPtr paint)
{
    auto result = std::make_shared<SolidFilter>();
    result->paint = std::move(paint);
    return result;
}

// ****************************************************************************

stroke::stroke(sg::PaintPtr paint)
{
    mStroke = std::make_shared<StrokePathOp>();
    mStroke->paint = paint;
}

stroke::stroke(std::shared_ptr<StrokePathOp> op)
    : mStroke(std::move(op))
{
}

stroke&
stroke::strokeWidth(float value)
{
    if (mStroke)
        mStroke->strokeWidth = value;
    return *this;
}

stroke&
stroke::miterLimit(float value)
{
    if (mStroke)
        mStroke->miterLimit = value;
    return *this;
}

stroke&
stroke::pathLength(float value)
{
    if (mStroke)
        mStroke->pathLength = value;
    return *this;
}

stroke&
stroke::dashOffset(float value)
{
    if (mStroke)
        mStroke->dashOffset = value;
    return *this;
}

stroke&
stroke::lineCap(GraphicLineCap value)
{
    if (mStroke)
        mStroke->lineCap = value;
    return *this;
}

stroke&
stroke::lineJoin(GraphicLineJoin value)
{
    if (mStroke)
        mStroke->lineJoin = value;
    return *this;
}

stroke&
stroke::dashes(const Object& value)
{
    if (mStroke && value.isArray()) {
        mStroke->dashes.clear();
        const auto& objArray = value.getArray();
        for (const auto& m : objArray)
            mStroke->dashes.push_back(m.asNumber());

        // If there are an odd number, insert it again
        if (objArray.size() % 2 != 0)
            for (const auto& m : objArray)
                mStroke->dashes.push_back(m.asNumber());
    }
    return *this;
}


} // namespace sg
} // namesapce apl
