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
#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/scenegraph.h"
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

#ifdef SCENEGRAPH
sg::NodePtr
GraphicElementPath::buildSceneGraph(sg::SceneGraphUpdates& sceneGraph)
{
    auto path = sg::path(getValue(kGraphicPropertyPathData).asString());
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

    // Return nothing if there is no path and the path never mutates
    const auto pathMutates = hasUpstream(kGraphicPropertyPathData);
    if (path->empty() && !pathMutates)
        return nullptr;

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

    // If there are no drawing operations, return a null node.
    return op ? sg::draw(path, op) : nullptr;
}

void
GraphicElementPath::updateSceneGraphInternal(sg::ModifiedNodeList& modList, const sg::NodePtr& node)
{
    const auto pathChanged = isDirty(kGraphicPropertyPathData);
    const auto fillChanged = isDirty(kGraphicPropertyFill) ||
                             isDirty(kGraphicPropertyFillOpacity) ||
                             isDirty(kGraphicPropertyFillTransform);
    const auto strokePaintChanged = isDirty(kGraphicPropertyStroke) ||
                                    isDirty(kGraphicPropertyStrokeOpacity) ||
                                    isDirty(kGraphicPropertyStrokeTransform);

    const auto strokeChanged =
        isDirty(kGraphicPropertyStrokeWidth) || isDirty(kGraphicPropertyStrokeMiterLimit) ||
        isDirty(kGraphicPropertyPathLength) || isDirty(kGraphicPropertyStrokeDashOffset) ||
        isDirty(kGraphicPropertyStrokeLineCap) || isDirty(kGraphicPropertyStrokeLineJoin) ||
        isDirty(kGraphicPropertyStrokeDashArray);

    if (pathChanged || fillChanged || strokePaintChanged || strokeChanged) {
        auto* draw = sg::DrawNode::cast(node);

        if (pathChanged && draw->setPath(sg::path(getValue(kGraphicPropertyPathData).asString())))
            modList.contentChanged(draw);

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
            modList.contentChanged(draw);
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
                modList.contentChanged(draw);
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
                modList.contentChanged(draw);
            }
        }
    }
}
#endif // SCENEGRAPH
}  // namespace apl
