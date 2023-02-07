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

#include "apl/graphic/graphicelementgroup.h"

#include "apl/graphic/graphicpropdef.h"
#include "apl/primitives/transform2d.h"

#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/graphicfragment.h"
#include "apl/scenegraph/scenegraph.h"
#include "apl/scenegraph/scenegraphupdates.h"
#endif // SCENEGRAPH

namespace apl {

GraphicElementPtr
GraphicElementGroup::create(const GraphicPtr& graphic,
                            const ContextPtr& context,
                            const Object& json)
{
    Properties properties;
    properties.emplace(json);

    auto group = std::make_shared<GraphicElementGroup>(graphic, context);
    if (!group->initialize(graphic, json))
        return nullptr;  // this fork is impossible to reach since group has no required properties

    return group;
}


const GraphicPropDefSet&
GraphicElementGroup::propDefSet() const
{
    static GraphicPropDefSet sGroupProperties = GraphicPropDefSet()
        .add({
                 {kGraphicPropertyClipPath,          "",                    asString,  kPropInOut | kPropDynamic},
                 {kGraphicPropertyFilters,           Object::EMPTY_ARRAY(), asGraphicFilterArray,    kPropInOut },
                 {kGraphicPropertyOpacity,           1.0,                   asOpacity, kPropInOut | kPropDynamic},
                 {kGraphicPropertyTransform,         Transform2D(),         nullptr,   kPropOut},
                 {kGraphicPropertyTransformAssigned, "",                    asString,  kPropIn    | kPropDynamic, fixTransform},
                 {kGraphicPropertyRotation,          0,                     asNumber,  kPropIn    | kPropDynamic, fixTransform},
                 {kGraphicPropertyPivotX,            0,                     asNumber,  kPropIn    | kPropDynamic, fixTransform},
                 {kGraphicPropertyPivotY,            0,                     asNumber,  kPropIn    | kPropDynamic, fixTransform},
                 {kGraphicPropertyScaleX,            1.0,                   asNumber,  kPropIn    | kPropDynamic, fixTransform},
                 {kGraphicPropertyScaleY,            1.0,                   asNumber,  kPropIn    | kPropDynamic, fixTransform},
                 {kGraphicPropertyTranslateX,        0,                     asNumber,  kPropIn    | kPropDynamic, fixTransform},
                 {kGraphicPropertyTranslateY,        0,                     asNumber,  kPropIn    | kPropDynamic, fixTransform},
             });

    return sGroupProperties;
}


bool
GraphicElementGroup::initialize(const GraphicPtr& graphic, const Object& json)
{
    if (!GraphicElement::initialize(graphic, json))
        return false;

    // Update output transforms
    updateTransform(*mContext, false);

    return true;
}


void
GraphicElementGroup::updateTransform(const Context& context, bool useDirtyFlag)
{
    Transform2D updated;

    auto inTransformStr = mValues.get(kGraphicPropertyTransformAssigned).asString();

    Transform2D outTransform;
    if (!inTransformStr.empty()) {
        outTransform = inTransformStr.empty() ? Transform2D()
                                              : Transform2D::parse(context.session(), inTransformStr);
    } else {
        auto scaleX = mValues.get(kGraphicPropertyScaleX).asFloat();
        auto scaleY = mValues.get(kGraphicPropertyScaleY).asFloat();
        auto pivotX = mValues.get(kGraphicPropertyPivotX).asFloat();
        auto pivotY = mValues.get(kGraphicPropertyPivotY).asFloat();
        auto rotation = mValues.get(kGraphicPropertyRotation).asFloat();
        auto translateX = mValues.get(kGraphicPropertyTranslateX).asFloat();
        auto translateY = mValues.get(kGraphicPropertyTranslateY).asFloat();

        // Remember that transformations apply from right-to-left.  The documented order is:
        // "translate(tx ty) translate(px py) rotate(rotation) scale(sx sy) translate(-px -py)"
        // This follows the common model of scale and rotate about the pivot point, followed by translation.

        outTransform = Transform2D::translate(translateX + pivotX, translateY + pivotY);
        outTransform *= Transform2D::rotate(rotation);
        outTransform *= Transform2D::scale(scaleX, scaleY);
        outTransform *= Transform2D::translate(-pivotX, -pivotY);
    }

    if (outTransform != mValues.get(kGraphicPropertyTransform).get<Transform2D>()) {
        // TODO: We do not explicitly update "single property" values as going to deprecate them to kPropIn-only
        //  after viewhosts act on that.
        mValues.set(kGraphicPropertyTransform, Object(std::move(outTransform)));
        if (useDirtyFlag) {
            mDirtyProperties.emplace(kGraphicPropertyTransform);
            markAsDirty();
        }
    }
}


void
GraphicElementGroup::release()
{
    GraphicElement::release();
#ifdef SCENEGRAPH
    mSceneGraphLayer = nullptr;
    mSceneGraphNode = nullptr;
#endif
}


#ifdef SCENEGRAPH
sg::GraphicFragmentPtr
GraphicElementGroup::buildSceneGraph(bool allowLayers, sg::SceneGraphUpdates& sceneGraph)
{
    // Clear cached items
    mSceneGraphLayer = nullptr;
    mSceneGraphNode = nullptr;

    const auto hasStyle = !mStyle.empty();

    // A group with zero opacity will never be rendered, so we can ignore it - unless it has
    // an upstream driver or a style assigned, in which case it may be toggled to be visible
    // in the future.
    auto opacity = static_cast<float>(getValue(kGraphicPropertyOpacity).asNumber());
    if (opacity == 0 && !hasUpstream(kGraphicPropertyOpacity) && !hasStyle)
        return {};

    // No children?  Ignore the group
    auto result = ensureSceneGraphChildren(allowLayers, sceneGraph);
    if (result->empty())
        return {};

    auto clipPath = sg::path(getValue(kGraphicPropertyClipPath).asString());
    auto transform = getValue(kGraphicPropertyTransform).get<Transform2D>();

    // Force a layer if this group can be modified
    const auto groupMutable = hasStyle || hasUpstream();
    if (allowLayers && groupMutable)
        result->ensureLayer(sceneGraph);

    if (result->isLayer()) {
        auto layer = result->layer();
        mContainingLayer = layer;

        result->fixBoundingBox();
        auto offset = layer->getContentOffset();
        if (!offset.empty())
            transform = Transform2D::translate(-offset) * transform * Transform2D::translate(offset);

        layer->setOutline(clipPath);
        layer->setOpacity(opacity);
        layer->setTransform(transform);

        if (groupMutable) {
            result->setType(sg::GraphicFragment::kLayerMutable);
            mSceneGraphLayer = layer;
        }
    }
    else {
        // Not a layer; configure as a node
        auto node = result->node();

        if (!clipPath->empty() || hasUpstream(kGraphicPropertyClipPath))
            node = sg::clip(clipPath, node);
        if (!transform.empty() || hasUpstream(kGraphicPropertyTransformAssigned) ||
            hasUpstream(kGraphicPropertyRotation) || hasUpstream(kGraphicPropertyPivotX) ||
            hasUpstream(kGraphicPropertyPivotY) || hasUpstream(kGraphicPropertyScaleX) ||
            hasUpstream(kGraphicPropertyScaleY) || hasUpstream(kGraphicPropertyTranslateX) ||
            hasUpstream(kGraphicPropertyTranslateY))
            node = sg::transform(transform, node);
        if (opacity < 1.0f || hasUpstream(kGraphicPropertyOpacity))
            node = sg::opacity(opacity, node);

        result->setNode(node);

        if (groupMutable) {
            result->setType(sg::GraphicFragment::kNodeContentMutable);
            mSceneGraphNode = node;
        }
    }

    result->applyFilters(mValues.get(kGraphicPropertyFilters));
    return result;
}

void
GraphicElementGroup::updateSceneGraph(sg::SceneGraphUpdates& sceneGraph)
{
    if (!mSceneGraphLayer && !mSceneGraphNode)
        return;

    const auto clipChanged = isDirty(kGraphicPropertyClipPath);
    const auto opacityChanged = isDirty(kGraphicPropertyOpacity);
    const auto transformChanged = isDirty(kGraphicPropertyTransform);

    if (!clipChanged && !opacityChanged && !transformChanged)
        return;

    if (mSceneGraphLayer) {
        auto layer = mSceneGraphLayer;

        if (opacityChanged && layer->setOpacity(getValue(kGraphicPropertyOpacity).asFloat()))
            sceneGraph.changed(layer);

        if (transformChanged) {
            auto offset = mSceneGraphLayer->getContentOffset();
            auto transform = getValue(kGraphicPropertyTransform).get<Transform2D>();
            transform = Transform2D::translate(-offset) * transform * Transform2D::translate(offset);
            if (layer->setTransform(transform))
                sceneGraph.changed(layer);
        }

        if (clipChanged &&
            layer->setOutline(sg::path(getValue(kGraphicPropertyClipPath).asString()))) {
            sceneGraph.changed(layer);
            requestSizeCheck(sceneGraph);
        }
    }
    else {
        auto node = mSceneGraphNode;

        if (sg::OpacityNode::is_type(node)) {
            auto* opacity = sg::OpacityNode::cast(node);
            if (opacityChanged && opacity->setOpacity(getValue(kGraphicPropertyOpacity).asFloat()))
                requestRedraw(sceneGraph);
            node = node->child();
        }

        if (sg::TransformNode::is_type(node)) {
            auto* transform = sg::TransformNode::cast(node);
            if (transformChanged &&
                transform->setTransform(getValue(kGraphicPropertyTransform).get<Transform2D>())) {
                requestRedraw(sceneGraph);
                requestSizeCheck(sceneGraph);
            }
            node = node->child();
        }

        if (sg::ClipNode::is_type(node)) {
            auto* clip = sg::ClipNode::cast(node);
            if (clipChanged &&
                clip->setPath(sg::path(getValue(kGraphicPropertyClipPath).asString()))) {
                requestRedraw(sceneGraph);
                requestSizeCheck(sceneGraph);
            }
        }
    }
}
#endif // SCENEGRAPH
} // namespace apl
