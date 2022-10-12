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
#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/scenegraph.h"
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
                 {kGraphicPropertyTransform,         Object::IDENTITY_2D(), nullptr,   kPropOut},
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

    if (outTransform != mValues.get(kGraphicPropertyTransform).getTransform2D()) {
        // TODO: We do not explicitly update "single property" values as going to deprecate them to kPropIn-only
        //  after viewhosts act on that.
        mValues.set(kGraphicPropertyTransform, Object(std::move(outTransform)));
        if (useDirtyFlag) {
            mDirtyProperties.emplace(kGraphicPropertyTransform);
            markAsDirty();
        }
    }
}

#ifdef SCENEGRAPH
sg::NodePtr
GraphicElementGroup::buildSceneGraph(sg::SceneGraphUpdates& sceneGraph)
{
    auto clip = sg::clip(sg::path(getValue(kGraphicPropertyClipPath).asString()), nullptr);

    for (const auto& m : mChildren) {
        auto child = m->getSceneGraph(sceneGraph);
        if (child)
            clip->appendChild(child);
    }

    return sg::opacity(
        getValue(kGraphicPropertyOpacity),
        sg::transform(
            getValue(kGraphicPropertyTransform),
            clip));
}

void
GraphicElementGroup::updateSceneGraphInternal(sg::ModifiedNodeList& modList, const sg::NodePtr& node)
{
    const auto clipChanged = isDirty(kGraphicPropertyClipPath);
    const auto opacityChanged = isDirty(kGraphicPropertyOpacity);
    const auto transformChanged = isDirty(kGraphicPropertyTransform);

    if (!clipChanged && !opacityChanged && !transformChanged)
        return;

    auto* opacity = sg::OpacityNode::cast(node);
    if (opacityChanged && opacity->setOpacity(getValue(kGraphicPropertyOpacity).asFloat()))
        modList.contentChanged(opacity);

    auto* transform = sg::TransformNode::cast(opacity->child());
    if (transformChanged &&
        transform->setTransform(getValue(kGraphicPropertyTransform).getTransform2D()))
        modList.contentChanged(transform);

    auto* clip = sg::ClipNode::cast(transform->child());
    if (clipChanged && clip->setPath(sg::path(getValue(kGraphicPropertyClipPath).asString())))
        modList.contentChanged(clip);
}
#endif // SCENEGRAPH
} // namespace apl
