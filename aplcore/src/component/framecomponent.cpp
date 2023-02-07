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

#include "apl/component/framecomponent.h"

#include "apl/component/componentpropdef.h"
#include "apl/component/yogaproperties.h"
#include "apl/primitives/color.h"
#include "apl/primitives/radii.h"
#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/node.h"
#include "apl/scenegraph/path.h"
#include "apl/scenegraph/scenegraph.h"
#endif // SCENEGRAPH

namespace apl {


static void checkBorderRadii(Component& component)
{
    auto& frame = static_cast<FrameComponent&>(component);
    frame.fixBorder(true);
}

CoreComponentPtr
FrameComponent::create(const ContextPtr& context,
                       Properties&& properties,
                       const Path& path)
{
    auto ptr = std::make_shared<FrameComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

FrameComponent::FrameComponent(const ContextPtr& context,
                               Properties&& properties,
                               const Path& path)
    : CoreComponent(context, std::move(properties), path)
{
    // TODO: Auto-sized Frame just wraps the children.  Fix this for ScrollView and other containers?
    YGNodeStyleSetAlignItems(mYGNodeRef, YGAlignFlexStart);
}

const ComponentPropDefSet&
FrameComponent::propDefSet() const
{
    static ComponentPropDefSet sFrameComponentProperties(CoreComponent::propDefSet(), {
        {kPropertyBackgroundColor,         Color(),               asColor,                        kPropInOut |
                                                                                                  kPropStyled |
                                                                                                  kPropDynamic |
                                                                                                  kPropVisualHash},
        {kPropertyBorderRadii,             Radii(),               nullptr,                        kPropOut |
                                                                                                  kPropVisualHash},
        {kPropertyBorderColor,             Color(),               asColor,                        kPropInOut |
                                                                                                  kPropStyled |
                                                                                                  kPropDynamic |
                                                                                                  kPropVisualHash},
        {kPropertyBorderWidth,             Dimension(0),          asNonNegativeAbsoluteDimension, kPropInOut |
                                                                                                  kPropStyled |
                                                                                                  kPropDynamic |
                                                                                                  kPropVisualHash,
                                                                                                  yn::setBorder<YGEdgeAll>, resolveDrawnBorder},

        // These are input-only properties that trigger the calculation of the output properties
        {kPropertyBorderBottomLeftRadius,  Object::NULL_OBJECT(), asAbsoluteDimension,            kPropIn |
                                                                                                  kPropStyled |
                                                                                                  kPropDynamic, checkBorderRadii},
        {kPropertyBorderBottomRightRadius, Object::NULL_OBJECT(), asAbsoluteDimension,            kPropIn |
                                                                                                  kPropStyled |
                                                                                                  kPropDynamic, checkBorderRadii },
        {kPropertyBorderRadius,            Dimension(0),          asAbsoluteDimension,            kPropIn |
                                                                                                  kPropStyled |
                                                                                                  kPropDynamic, checkBorderRadii },
        {kPropertyBorderTopLeftRadius,     Object::NULL_OBJECT(), asAbsoluteDimension,            kPropIn |
                                                                                                  kPropStyled |
                                                                                                  kPropDynamic, checkBorderRadii },
        {kPropertyBorderTopRightRadius,    Object::NULL_OBJECT(), asAbsoluteDimension,            kPropIn |
                                                                                                  kPropStyled |
                                                                                                  kPropDynamic, checkBorderRadii },

        // The width of the drawn border.  If borderStrokeWith is set, the drawn border is the min of borderWidth
        // and borderStrokeWidth.  If borderStrokeWidth is unset, the drawn border defaults to borderWidth
        {kPropertyBorderStrokeWidth, Object::NULL_OBJECT(), asNonNegativeAbsoluteDimension,       kPropIn |
                                                                                                  kPropStyled |
                                                                                                  kPropDynamic, resolveDrawnBorder},
        {kPropertyDrawnBorderWidth,  Object::NULL_OBJECT(), asNonNegativeAbsoluteDimension,       kPropOut |
                                                                                                  kPropVisualHash},
    });

    return sFrameComponentProperties;
}

void
FrameComponent::setHeight(const Dimension& height) {
    CoreComponent::setHeight(height);
    if (height.isAbsolute() || height.getValue() == 0) {
        fixBorder(false);
    }
}

void
FrameComponent::setWidth(const Dimension& width) {
    CoreComponent::setWidth(width);
    if (width.isAbsolute() || width.getValue() == 0) {
        fixBorder(false);
    }
}

void
FrameComponent::fixBorder(bool useDirtyFlag)
{
    auto w = mCalculated.get(kPropertyWidth);
    auto h = mCalculated.get(kPropertyHeight);
    if ((w.isAbsoluteDimension() && w.getAbsoluteDimension() == 0) ||
        (h.isAbsoluteDimension() && h.getAbsoluteDimension() == 0)) {
        yn::setBorder<YGEdgeAll>(getNode(), Dimension(0), *mContext);
    }

    static std::vector<std::pair<PropertyKey, Radii::Corner >> BORDER_PAIRS = {
        {kPropertyBorderBottomLeftRadius,  Radii::kBottomLeft},
        {kPropertyBorderBottomRightRadius, Radii::kBottomRight},
        {kPropertyBorderTopLeftRadius,     Radii::kTopLeft},
        {kPropertyBorderTopRightRadius,    Radii::kTopRight}
    };

    float r = mCalculated.get(kPropertyBorderRadius).asAbsoluteDimension(*mContext).getValue();
    std::array<float, 4> result = {r, r, r, r};

    for (const auto& p : BORDER_PAIRS) {
        Object radius = mCalculated.get(p.first);
        if (radius != Object::NULL_OBJECT())
            result[p.second] = radius.asAbsoluteDimension(*mContext).getValue();
    }

    Radii radii(std::move(result));

    if (radii != mCalculated.get(kPropertyBorderRadii).get<Radii>()) {
        mCalculated.set(kPropertyBorderRadii, Object(std::move(radii)));
        if (useDirtyFlag)
            setDirty(kPropertyBorderRadii);
    }
}

#ifdef SCENEGRAPH
/**
 * The Frame component structure:
 *
 * Layer
 *   - Outline Path     [If the outline isn't a rectangle]
 *   - Child Clip Path  [If the borderWidth != 0]
 *   - Content
 *         Background DrawNode [optional]
 *         Border DrawNode [optional]
 */
sg::LayerPtr
FrameComponent::constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph)
{
    auto layer = CoreComponent::constructSceneGraphLayer(sceneGraph);
    assert(layer);

    const auto& radii = getCalculated(kPropertyBorderRadii).get<Radii>();
    auto borderWidth = static_cast<float>(getCalculated(kPropertyBorderWidth).asNumber());
    auto strokeWidth = static_cast<float>(getCalculated(kPropertyDrawnBorderWidth).asNumber());
    auto size = getCalculated(kPropertyBounds).get<Rect>().getSize();
    auto outline = RoundedRect(Rect{0, 0, size.getWidth(), size.getHeight()}, radii);

    // Set the outline only if it isn't a rectangle that matches the size
    if (!outline.isRect())
        layer->setOutline(sg::path(outline));

    // Set the child clip path if there is a border width
    if (borderWidth > 0)
        layer->setChildClip(sg::path(outline.inset(borderWidth)));

    auto background = sg::draw(sg::path(outline.inset(strokeWidth)),
                               sg::fill(sg::paint(getCalculated(kPropertyBackgroundColor))));
    auto border = sg::draw(sg::path(outline, strokeWidth),
                           sg::fill(sg::paint(getCalculated(kPropertyBorderColor))));

    background->setNext(border);
    layer->setContent(background);
    return layer;
}


bool
FrameComponent::updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph)
{
    auto outlineChanged = isDirty(kPropertyBorderRadii) ||
                          isDirty(kPropertyBounds);

    auto borderWidthChanged = isDirty(kPropertyBorderWidth);
    auto drawnBorderWidthChanged = isDirty(kPropertyDrawnBorderWidth);
    auto backgroundChanged = isDirty(kPropertyBackgroundColor);
    auto borderColorChanged = isDirty(kPropertyBorderColor);
    auto strokeWidthChanged = isDirty(kPropertyBorderStrokeWidth);

    if (!outlineChanged && !borderWidthChanged && !drawnBorderWidthChanged &&
        !backgroundChanged && !borderColorChanged && !strokeWidthChanged)
        return false;

    const auto& radii = getCalculated(kPropertyBorderRadii).get<Radii>();
    auto borderWidth = getCalculated(kPropertyBorderWidth).asFloat();
    auto strokeWidth = getCalculated(kPropertyDrawnBorderWidth).asFloat();
    auto size = getCalculated(kPropertyBounds).get<Rect>().getSize();
    auto outline = RoundedRect(Rect{0, 0, size.getWidth(), size.getHeight()}, radii);

    auto *layer = mSceneGraphLayer.get();
    auto result = false;

    // Fix the outline
    if (outlineChanged)
        result |= layer->setOutline(outline.isRect() ? nullptr : sg::path(outline));

    // Fix the child clipping region
    if (outlineChanged || borderWidthChanged)
        result |= layer->setChildClip(borderWidth > 0 ? sg::path(outline.inset(borderWidth)) : nullptr);

    assert(layer->content());
    auto background = layer->content();
    assert(background->next());
    auto border = background->next();
    assert(!border->next());

    // Fix the background.  Actually, this should already be clipped, so we could use a rectangular fill
    if (outlineChanged || borderWidthChanged || backgroundChanged) {
        auto *draw = sg::DrawNode::cast(background);

        if (outlineChanged || borderWidthChanged) {
            auto* path = sg::RoundedRectPath::cast(draw->getPath());
            result |= path->setRoundedRect(outline.inset(borderWidth));
        }

        if (backgroundChanged) {
            auto* fill = sg::FillPathOp::cast(draw->getOp());
            auto* paint = sg::ColorPaint::cast(fill->paint);
            result |= paint->setColor(getCalculated(kPropertyBackgroundColor).getColor());
        }
    }

    // Fix the border drawing
    if (outlineChanged || borderWidthChanged || drawnBorderWidthChanged || borderColorChanged) {
        auto *draw = sg::DrawNode::cast(border);

        if (outlineChanged || borderWidthChanged || drawnBorderWidthChanged) {
            auto* path = sg::FramePath::cast(draw->getPath());
            result |= path->setRoundedRect(outline.inset(borderWidth - strokeWidth));
            result |= path->setInset(strokeWidth);
        }

        if (borderColorChanged) {
            auto* fill = sg::FillPathOp::cast(draw->getOp());
            auto* paint = sg::ColorPaint::cast(fill->paint);
            result |= paint->setColor(getCalculated(kPropertyBorderColor).getColor());
        }
    }

    return result;
}
#endif // SCENEGRAPH

/*
 * Initial assignment of properties.  Don't set any dirty flags here; this
 * all should be running in the constructor.
 *
 * This method initializes the values of the border corners.
*/

void
FrameComponent::assignProperties(const ComponentPropDefSet& propDefSet)
{
    CoreComponent::assignProperties(propDefSet);
    fixBorder(false);
    calculateDrawnBorder(false);
}

} // namespace apl
