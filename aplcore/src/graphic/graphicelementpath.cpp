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

#include "apl/graphic/graphicelementpath.h"

#include "apl/graphic/graphicpropdef.h"
#include "apl/primitives/color.h"
#include "apl/primitives/transform2d.h"

#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/graphicfragment.h"
#include "apl/scenegraph/scenegraph.h"
#include "apl/scenegraph/scenegraphupdates.h"
#endif // SCENEGRAPH

namespace apl {

GraphicElementPtr
GraphicElementPath::create(const GraphicPtr& graphic,
                           const ContextPtr& context,
                           const Object& json)
{
    Properties properties;
    properties.emplace(json);

    auto path = std::make_shared<GraphicElementPath>(graphic, context);
    if (!path->initialize(graphic, json))
        return nullptr;

    return path;
}


const GraphicPropDefSet&
GraphicElementPath::propDefSet() const
{
    static GraphicPropDefSet sPathProperties = GraphicPropDefSet()
        .add({
                 {kGraphicPropertyFill,                    Color(),               asAvgFill,             kPropInOut | kPropDynamic | kPropEvaluated},
                 {kGraphicPropertyFillOpacity,             1.0,                   asOpacity,             kPropInOut | kPropDynamic},
                 {kGraphicPropertyFillTransform,           Transform2D(),         nullptr,               kPropOut},
                 {kGraphicPropertyFillTransformAssigned,   "",                    asString,              kPropIn | kPropDynamic, fixFillTransform},
                 {kGraphicPropertyFilters,                 Object::EMPTY_ARRAY(), asGraphicFilterArray,  kPropInOut},
                 {kGraphicPropertyPathData,                "",                    asString,              kPropInOut | kPropRequired | kPropDynamic},
                 {kGraphicPropertyPathLength,              0,                     asNumber,              kPropInOut | kPropDynamic},
                 {kGraphicPropertyStroke,                  Color(),               asAvgFill,             kPropInOut | kPropDynamic | kPropEvaluated},
                 {kGraphicPropertyStrokeDashArray,         Object::EMPTY_ARRAY(), asDashArray,           kPropInOut | kPropDynamic | kPropEvaluated},
                 {kGraphicPropertyStrokeDashOffset,        0,                     asNumber,              kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeLineCap,           kGraphicLineCapButt,   sGraphicLineCapBimap,  kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeLineJoin,          kGraphicLineJoinMiter, sGraphicLineJoinBimap, kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeMiterLimit,        4,                     asNumber,              kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeOpacity,           1.0,                   asOpacity,             kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeTransform,         Transform2D(),         nullptr,               kPropOut},
                 {kGraphicPropertyStrokeTransformAssigned, "",                    asString,              kPropIn | kPropDynamic, fixStrokeTransform},
                 {kGraphicPropertyStrokeWidth,             1.0,                   asNonNegativeNumber,   kPropInOut | kPropDynamic}
             });

    return sPathProperties;
}

bool
GraphicElementPath::initialize(const GraphicPtr& graphic, const Object& json)
{
    if (!GraphicElement::initialize(graphic, json))
        return false;

    // Update output transforms
    updateTransform(*mContext, kGraphicPropertyFillTransformAssigned, kGraphicPropertyFillTransform, false);
    updateTransform(*mContext, kGraphicPropertyStrokeTransformAssigned, kGraphicPropertyStrokeTransform, false);

    return true;
}

void
GraphicElementPath::release()
{
#ifdef SCENEGRAPH
    mSceneGraphNode = nullptr;
#endif

    GraphicElement::release();
}

#ifdef SCENEGRAPH
sg::GraphicFragmentPtr
GraphicElementPath::buildSceneGraph(bool allowLayers, sg::SceneGraphUpdates& sceneGraph)
{
    // Clear cached items
    mSceneGraphNode = nullptr;

    auto path = sg::path(getValue(kGraphicPropertyPathData).asString());

    // Return nothing if there is no path and the path never mutates
    const auto pathMutates = hasUpstream(kGraphicPropertyPathData);
    const auto hasStyle = !mStyle.empty();
    if (path->empty() && !pathMutates && !hasStyle)
        return {};

    // Calculate the current fill and stroke
    auto fill = sg::fill(sg::paint(getValue(kGraphicPropertyFill),
                                   getValue(kGraphicPropertyFillOpacity).asFloat(),
                                   getValue(kGraphicPropertyFillTransform).get<Transform2D>()));
    auto stroke =
        sg::stroke(sg::paint(getValue(kGraphicPropertyStroke),
                             getValue(kGraphicPropertyStrokeOpacity).asFloat(),
                             getValue(kGraphicPropertyStrokeTransform).get<Transform2D>()))
            .strokeWidth(getValue(kGraphicPropertyStrokeWidth).asFloat())
            .miterLimit(getValue(kGraphicPropertyStrokeMiterLimit).asFloat())
            .pathLength(getValue(kGraphicPropertyPathLength).asFloat())
            .dashOffset(getValue(kGraphicPropertyStrokeDashOffset).asFloat())
            .lineCap(static_cast<GraphicLineCap>(getValue(kGraphicPropertyStrokeLineCap).asInt()))
            .lineJoin(
                static_cast<GraphicLineJoin>(getValue(kGraphicPropertyStrokeLineJoin).asInt()))
            .dashes(getValue(kGraphicPropertyStrokeDashArray))
            .get();

    // Include the fill operation if it is visible or if it can change to be visible
    const auto includeFill = hasStyle ||
        (includeInSceneGraph(kGraphicPropertyFill) &&
                                          includeInSceneGraph(kGraphicPropertyFillOpacity));

    // Include the stroke operation if it is visible or if it can change to be visible
    const auto includeStroke = hasStyle ||
                               (includeInSceneGraph(kGraphicPropertyStroke) &&
                                            includeInSceneGraph(kGraphicPropertyStrokeOpacity) &&
                                            includeInSceneGraph(kGraphicPropertyStrokeWidth));

    // Assemble the operations list in reverse order so that fill occurs before stroke.
    auto op = includeStroke ? stroke : nullptr;
    if (includeFill) {
        fill->nextSibling = op;
        op = fill;
    }

    // If there are no drawing operations, leave this node empty
    if (!op)
        return {};

    auto node = sg::draw(path, op);

    // The content is mutable if there is an upstream dependency OR a style that can change the
    // properties.
    // TODO: Check what properties the style can change for a more fine-grained test.
    const auto pathMutable = hasStyle || hasUpstream();
    if (pathMutable)
        mSceneGraphNode = node;

    // If mutable and layers are allowed, set up a layer
    sg::GraphicFragmentPtr result;
    if (allowLayers && pathMutable) {
        auto layer = sg::layer(getUniqueId() + "_path", Rect(), 1.0f, Transform2D());
        layer->setCharacteristic(sg::Layer::kCharacteristicRenderOnly |
                                            sg::Layer::kCharacteristicDoNotClipChildren);
        sceneGraph.created(layer);
        layer->setContent(node);
        mContainingLayer = layer;

        auto bounds = node->boundingBox(Transform2D());
        layer->setContentOffset(bounds.getTopLeft());
        layer->setBounds(bounds);
        result = sg::GraphicFragment::create(shared_from_this(), layer,
                                             sg::GraphicFragment::kLayerFixedContentMutable);
    }
    else {
        result =
            sg::GraphicFragment::create(shared_from_this(), node,
                                             pathMutable ? sg::GraphicFragment::kNodeContentMutable
                                                       : sg::GraphicFragment::kNodeContentFixed);
    }

    result->applyFilters(mValues.get(kGraphicPropertyFilters));
    return result;
}

void
GraphicElementPath::updateSceneGraph(sg::SceneGraphUpdates& sceneGraph)
{
    if (!mSceneGraphNode)
        return;

    const auto pathChanged = isDirty(kGraphicPropertyPathData);
    const auto fillChanged =
        isDirty({kGraphicPropertyFill, kGraphicPropertyFillOpacity, kGraphicPropertyFillTransform});
    const auto strokePaintChanged = isDirty(
        {kGraphicPropertyStroke, kGraphicPropertyStrokeOpacity, kGraphicPropertyStrokeTransform});

    const auto strokeChanged = isDirty(
        {kGraphicPropertyStrokeWidth, kGraphicPropertyStrokeMiterLimit, kGraphicPropertyPathLength,
         kGraphicPropertyStrokeDashOffset, kGraphicPropertyStrokeLineCap,
         kGraphicPropertyStrokeLineJoin, kGraphicPropertyStrokeDashArray});

    // Fix up the drawing content
    if (pathChanged || fillChanged || strokePaintChanged || strokeChanged) {
        auto* draw = sg::DrawNode::cast(mSceneGraphNode);

        if (pathChanged && draw->setPath(sg::path(getValue(kGraphicPropertyPathData).asString()))) {
            requestRedraw(sceneGraph);
            requestSizeCheck(sceneGraph);
        }

        auto op = draw->getOp();
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

            // TODO: Do a fine-grained check to see if the fill actually changed
            requestRedraw(sceneGraph);
        }

        // Advance to the stroke operation if the fill was defined.
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
                requestRedraw(sceneGraph);
            }

            if (strokeChanged) {
                sg::stroke(sg::StrokePathOp::castptr(op))
                    .strokeWidth(getValue(kGraphicPropertyStrokeWidth).asFloat())
                    .miterLimit(getValue(kGraphicPropertyStrokeMiterLimit).asFloat())
                    .pathLength(getValue(kGraphicPropertyPathLength).asFloat())
                    .dashOffset(getValue(kGraphicPropertyStrokeDashOffset).asFloat())
                    .lineCap(static_cast<GraphicLineCap>(
                        getValue(kGraphicPropertyStrokeLineCap).asInt()))
                    .lineJoin(static_cast<GraphicLineJoin>(
                        getValue(kGraphicPropertyStrokeLineJoin).asInt()))
                    .dashes(getValue(kGraphicPropertyStrokeDashArray));
                requestRedraw(sceneGraph);
                requestSizeCheck(sceneGraph);
            }
        }
    }
}

#endif // SCENEGRAPH
}  // namespace apl
