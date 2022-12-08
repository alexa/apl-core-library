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

#include "apl/graphic/graphicelementtext.h"
#include "apl/graphic/graphicpropdef.h"

#include "apl/component/componentproperties.h"
#include "apl/content/rootconfig.h"

#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/scenegraph.h"
#include "apl/scenegraph/textchunk.h"
#include "apl/scenegraph/textlayout.h"
#include "apl/scenegraph/textmeasurement.h"
#include "apl/scenegraph/utilities.h"
#endif // SCENEGRAPH

namespace apl {

GraphicElementPtr
GraphicElementText::create(const GraphicPtr &graphic,
                           const ContextPtr &context,
                           const Object &json)
{
    Properties properties;
    properties.emplace(json);

    auto text = std::make_shared<GraphicElementText>(graphic, context);
    if (!text->initialize(graphic, json))
        return nullptr;

    return text;
}

inline Object
defaultFontFamily(GraphicElement&, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultFontFamily());
}

const GraphicPropDefSet&
GraphicElementText::propDefSet() const
{
    static auto fixTextTrigger = [](GraphicElement& element) -> void {
#ifdef SCENEGRAPH
      auto& text = (GraphicElementText&)element;
      text.mTextProperties = nullptr;
      text.mLayout = nullptr;
#endif // SCENEGRAPH
    };

    static auto fixTextChunkTrigger = [](GraphicElement& element) -> void {
#ifdef SCENEGRAPH
        auto& text = (GraphicElementText&)element;
        text.mTextChunk = nullptr;
        text.mLayout = nullptr;
#endif // SCENEGRAPH
    };

    static GraphicPropDefSet sTextProperties = GraphicPropDefSet()
        .add({
                 {kGraphicPropertyFill,                    Color(Color::BLACK),     asAvgFill,               kPropInOut | kPropDynamic | kPropEvaluated},
                 {kGraphicPropertyFillOpacity,             1,                       asOpacity,               kPropInOut | kPropDynamic},
                 {kGraphicPropertyFillTransform,           Transform2D(),           nullptr,                 kPropOut},
                 {kGraphicPropertyFillTransformAssigned,   "",                      asString,                kPropIn    | kPropDynamic, fixFillTransform},
                 {kGraphicPropertyFilters,                 Object::EMPTY_ARRAY(),   asGraphicFilterArray,    kPropInOut },
                 {kGraphicPropertyFontFamily,              "",                      asString,                kPropInOut | kPropDynamic, fixTextTrigger, defaultFontFamily},
                 {kGraphicPropertyFontSize,                40,                      asNonNegativeNumber,     kPropInOut | kPropDynamic, fixTextTrigger},
                 {kGraphicPropertyFontStyle,               kFontStyleNormal,        sFontStyleMap,           kPropInOut | kPropDynamic, fixTextTrigger},
                 {kGraphicPropertyFontWeight,              400,                     sFontWeightMap,          kPropInOut | kPropDynamic, fixTextTrigger},
                 {kGraphicPropertyLetterSpacing,           0,                       asNumber,                kPropInOut | kPropDynamic, fixTextTrigger},
                 {kGraphicPropertyStroke,                  Color(),                 asAvgFill,               kPropInOut | kPropDynamic | kPropEvaluated},
                 {kGraphicPropertyStrokeOpacity,           1,                       asOpacity,               kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeTransform,         Transform2D(),           nullptr,                 kPropOut},
                 {kGraphicPropertyStrokeTransformAssigned, "",                      asString,                kPropIn    | kPropDynamic, fixStrokeTransform},
                 {kGraphicPropertyStrokeWidth,             0,                       asNonNegativeNumber,     kPropInOut | kPropDynamic},
                 {kGraphicPropertyText,                    "",                      asFilteredText,          kPropInOut | kPropRequired | kPropDynamic, fixTextChunkTrigger},
                 {kGraphicPropertyTextAnchor,              kGraphicTextAnchorStart, sGraphicTextAnchorBimap, kPropInOut | kPropDynamic},
                 {kGraphicPropertyCoordinateX,             0,                       asNumber,                kPropInOut | kPropDynamic},
                 {kGraphicPropertyCoordinateY,             0,                       asNumber,                kPropInOut | kPropDynamic}
             });

    return sTextProperties;
}


bool
GraphicElementText::initialize(const GraphicPtr& graphic, const Object& json)
{
    if (!GraphicElement::initialize(graphic, json))
        return false;

    // Update output transforms
    updateTransform(*mContext, kGraphicPropertyFillTransformAssigned, kGraphicPropertyFillTransform, false);
    updateTransform(*mContext, kGraphicPropertyStrokeTransformAssigned, kGraphicPropertyStrokeTransform, false);

    return true;
}

#ifdef SCENEGRAPH
sg::NodePtr
GraphicElementText::buildSceneGraph(sg::SceneGraphUpdates& sceneGraph)
{
    ensureSGTextLayout();

    auto fill = sg::fill(sg::paint(getValue(kGraphicPropertyFill),
                                   getValue(kGraphicPropertyFillOpacity).asFloat(),
                                   getValue(kGraphicPropertyFillTransform).get<Transform2D>()));
    auto stroke =
        sg::stroke(sg::paint(getValue(kGraphicPropertyStroke),
                             getValue(kGraphicPropertyStrokeOpacity).asFloat(),
                             getValue(kGraphicPropertyStrokeTransform).get<Transform2D>()))
            .strokeWidth(getValue(kGraphicPropertyStrokeWidth).asFloat())
            .get();


    // Include the fill operation if it is visible or if it can change to be visible
    const auto includeFill = fill->visible() ||
                             hasUpstream(kGraphicPropertyFill) ||
                             hasUpstream(kGraphicPropertyFillOpacity);

    // Include the stroke operation if it is visible or if it can change to be visible
    const auto includeStroke = stroke->visible() ||
                               hasUpstream(kGraphicPropertyStroke) ||
                               hasUpstream(kGraphicPropertyStrokeOpacity) ||
                               hasUpstream(kGraphicPropertyStrokeWidth);

    // Assemble the operations list in reverse order so that fill occurs before stroke.
    auto op = includeStroke ? stroke : nullptr;
    if (includeFill) {
        fill->nextSibling = op;
        op = fill;
    }

    // If there are no drawing operations, return a null node
    return op ? sg::transform(getPosition(), sg::text(mLayout, op)) : nullptr;
}

/*
 * This method is called if the GraphicElementText has a pre-existing scene graph.
 * Nominally "isDirty(PROPERTY)" tells us if a component property has changed and hence
 * may be need to be updated in the scene graph.  A shortcut is available since changing
 * some properties clears mLayout.
 *
 * These properties clear mLayout:
 *
 *   kGraphicPropertyFontFamily
 *   kGraphicPropertyFontSize
 *   kGraphicPropertyFontStyle
 *   kGraphicPropertyFontWeight
 *   kGraphicPropertyLetterSpacing
 *   kGraphicPropertyText
 *
 * These properties are needed to calculate paint and positioning of the text layout
 *
 *   kGraphicPropertyFill
 *   kGraphicPropertyFillOpacity
 *   kGraphicPropertyFillTransform
 *   kGraphicPropertyFilters
 *   kGraphicPropertyStroke
 *   kGraphicPropertyStrokeOpacity
 *   kGraphicPropertyStrokeTransform
 *   kGraphicPropertyStrokeWidth
 *   kGraphicPropertyTextAnchor
 *   kGraphicPropertyCoordinateX
 *   kGraphicPropertyCoordinateY
 *
 *   kGraphicPropertyFilters is not dynamic, so it doesn't change
 */
void
GraphicElementText::updateSceneGraphInternal(sg::ModifiedNodeList& modList, const sg::NodePtr& node)
{
    const auto textChanged = !mLayout;
    const auto fillChanged = isDirty(kGraphicPropertyFill) ||
                             isDirty(kGraphicPropertyFillOpacity) ||
                             isDirty(kGraphicPropertyFillTransform);
    const auto strokePaintChanged = isDirty(kGraphicPropertyStroke) ||
                                    isDirty(kGraphicPropertyStrokeOpacity) ||
                                    isDirty(kGraphicPropertyStrokeTransform);
    const auto strokeWidthChanged = isDirty(kGraphicPropertyStrokeWidth);
    const auto transformChanged = isDirty(kGraphicPropertyCoordinateX) ||
                                  isDirty(kGraphicPropertyCoordinateY) ||
                                  isDirty(kGraphicPropertyTextAnchor);

    ensureSGTextLayout();

    auto* transform = sg::TransformNode::cast(node);
    if ((textChanged || transformChanged) &&
        transform->setTransform(Transform2D::translate(getPosition()))) {
        modList.contentChanged(transform);
    }

    if (textChanged || fillChanged || strokePaintChanged || strokeWidthChanged) {
        auto* text = sg::TextNode::cast(transform->child());

        if (textChanged)
            text->setTextLayout(mLayout);

        auto op = text->getOp();
        if (sg::FillPathOp::is_type(op) && fillChanged) {
            if (isDirty(kGraphicPropertyFill)) {
                // If the fill changes, we create a new paint object.
                // TODO: Shouldn't allocate memory here if the paint hasn't changed type
                op->paint = sg::paint(getValue(kGraphicPropertyFill),
                                      getValue(kGraphicPropertyFillOpacity).asFloat(),
                                      getValue(kGraphicPropertyFillTransform).get<Transform2D>());
            }
            else {
                op->paint->setOpacity(getValue(kGraphicPropertyFillOpacity).asFloat());
                op->paint->setTransform(getValue(kGraphicPropertyFillTransform).get<Transform2D>());
            }
        }

        // Advance to the stroke operation if the fill was defined
        if (op->nextSibling)
            op = op->nextSibling;

        if (sg::StrokePathOp::is_type(op)) {
            if (strokePaintChanged) {
                if (isDirty(kGraphicPropertyStroke)) {
                    // If the stroke changes, we create a new paint object.
                    // TODO: Shouldn't allocate memory here if the paint hasn't changed type
                    op->paint =
                        sg::paint(getValue(kGraphicPropertyStroke),
                                  getValue(kGraphicPropertyStrokeOpacity).asFloat(),
                                  getValue(kGraphicPropertyStrokeTransform).get<Transform2D>());
                }
                else {
                    op->paint->setOpacity(getValue(kGraphicPropertyStrokeOpacity).asFloat());
                    op->paint->setTransform(
                        getValue(kGraphicPropertyStrokeTransform).get<Transform2D>());
                }
            }

            if (strokeWidthChanged)
                sg::stroke(sg::StrokePathOp::castptr(op))
                    .strokeWidth(getValue(kGraphicPropertyStrokeWidth).asFloat());
        }

        modList.contentChanged(text);
    }
}


void
GraphicElementText::ensureSGTextLayout()
{
    if (mLayout)
        return;

    const auto* context = mContext.get();
    if (!context)
        return;

    assert(mContext->measure()->sceneGraphCompatible());
    auto measure = std::static_pointer_cast<sg::TextMeasurement>(mContext->measure());

    ensureTextProperties();
    mLayout = measure->layout(mTextChunk, mTextProperties, INFINITY, MeasureMode::Undefined,
                              INFINITY, MeasureMode::Undefined);
}

void
GraphicElementText::ensureTextProperties()
{
    if (!mTextChunk)
        mTextChunk = sg::TextChunk::create(
            StyledText::createRaw(getValue(kGraphicPropertyText).getString()));

    if (!mTextProperties)
        mTextProperties = sg::TextProperties::create(
            mContext->textPropertiesCache(),
            sg::splitFontString(mContext->getRootConfig(),
                                getValue(kGraphicPropertyFontFamily).getString()),
            getValue(kGraphicPropertyFontSize).asFloat(),
            static_cast<FontStyle>(getValue(kGraphicPropertyFontStyle).getInteger()),
            getValue(kGraphicPropertyFontWeight).getInteger(),
            getValue(kGraphicPropertyLetterSpacing).asFloat());
}

Point
GraphicElementText::getPosition() const
{
    assert(mLayout);
    auto x = static_cast<float>(getValue(kGraphicPropertyCoordinateX).asFloat());
    auto y = static_cast<float>(getValue(kGraphicPropertyCoordinateY).asFloat()) - mLayout->getBaseline();
    auto coordinate = Point{x, y};

    switch (static_cast<GraphicTextAnchor>(getValue(kGraphicPropertyTextAnchor).getInteger())) {
        case kGraphicTextAnchorStart:
            break;
        case kGraphicTextAnchorMiddle:
            coordinate -= Point(mLayout->getSize().getWidth() / 2, 0);
            break;
        case kGraphicTextAnchorEnd:
            coordinate -= Point(mLayout->getSize().getWidth(), 0);
            break;
    }

    return coordinate;
}
#endif // SCENEGRAPH
} // namespace apl
