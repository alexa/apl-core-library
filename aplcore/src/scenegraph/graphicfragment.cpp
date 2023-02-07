/*
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
 *
 */

/**
 * Merging layers is only possible if the following conditions are met:
 *
 *   1. Both layers must have kCharacteristicImmutableProperties set.  That is, the layers won't transform
 *      or change opacity
 *   2. If the current layer has child layers, the merged layer must not have content
 *      (otherwise the drawing order would be wrong)
 *
 * If the current layer has:
 *    No Content     --- The merge is okay.  Copy the merged layer kCharacteristicImmutableContent flag setting.
 *    kCharacteristicImmutableContent  --- The merged layer must have kCharacteristicImmutableContent or no content
 *    !kCharacteristicImmutableContent --- The merged layer must have !kCharacteristicImmutableContent or no content
 */
#include "apl/graphic/graphicelement.h"
#include "apl/graphic/graphicfilter.h"
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/graphicfragment.h"
#include "apl/scenegraph/scenegraphupdates.h"

namespace apl {
namespace sg {

GraphicFragmentPtr
GraphicFragment::create(const GraphicElementPtr& element)
{
    auto result = std::make_shared<GraphicFragment>();
    result->mElements.push_back(element);
    return result;
}

GraphicFragmentPtr
GraphicFragment::create(const GraphicElementPtr& element,
                        const sg::NodePtr& node, Type flags)
{
    assert(flags == kNodeContentFixed || flags == kNodeContentMutable);

    auto result = std::make_shared<GraphicFragment>();
    result->mElements.push_back(element);
    result->mNode = node;
    result->mType = flags;
    return result;
}

GraphicFragmentPtr
GraphicFragment::create(const GraphicElementPtr& element,
                        const sg::LayerPtr& layer, Type flags)
{
    assert(layer);
    assert(flags != kNodeContentFixed && flags != kNodeContentMutable);
    element->assignSceneGraphLayer(layer);

    auto result = std::make_shared<GraphicFragment>();
    result->mElements.push_back(element);
    result->mLayer = layer;
    result->mType = flags;
    return result;
}

/**
 * Merge a node or layer into this node or layer.  Note that anything that is a Node is considered
 * to be immutable; it doesn't vary over time.  Anything that is a Layer may or may not be mutable.
 *
 * If both are nodes (and hence both are fixed), combine them into a node and return true.
 *
 * If this is a node and the other is a layer, return false.
 *
 * If this is a layer and the other is a node, then convert the other to a node and set CONTENT/LAYER fixed
 * on that node.
 *
 * If this is a layer and the other is a layer, then they can be combined only if:
 *   (1) this has no child layers AND
 *   (2) this and the other both have the same CONTENT_FIXED flag AND
 *   (3) neither this nor the other has the LAYER_FIXED flag unset (i.e., both must be fixed) AND
 *   (4) neither this nor the other has a transform or opacity applied.
 *
 *
 * @param other
 * @return
 */
bool
GraphicFragment::mergeWith(const GraphicFragmentPtr& otherPtr)
{
    // An empty fragment should never call mergeWith().  Empty fragments are only used
    // to accumulate children (see GraphicElement::ensureSceneGraphChildren)
    assert(!empty());

    if (!otherPtr)
        return true;

    auto& other = *otherPtr.get();
    if (other.empty())
        return true;

    // Check if this is a node
    if (isNode()) {
        if (other.isLayer())
            return false;

        // Merge the nodes together
        mNode = sg::Node::appendSiblingToNode(mNode, other.node());
        mElements.insert(mElements.end(), other.mElements.begin(), other.mElements.end());
        return true;
    }

    if (other.isNode())
        return false;

    // Two layers.  They can't merge unless both have fixed layer properties.
    if (mType == kLayerMutable || other.mType == kLayerMutable)
        return false;

    // Two fixed layers.
    // Do not merge if one has mutating content and the other has non-null fixed content
    if ((mType == kLayerFixedContentMutable && other.mType == kLayerFixedContentFixed && other.mNode) ||
        (mType == kLayerFixedContentFixed && other.mType == kLayerFixedContentMutable && mNode))
        return false;

    // Don't merge layers with shadows
    if (mLayer->getShadow() != nullptr || other.mLayer->getShadow() != nullptr)
        return false;

    // Don't merge layers with different outlines
    if (mLayer->getOutline() != other.mLayer->getOutline())
        return false;

    // Don't merge layers with different transforms
    // TODO: This could be allowed if the layers only have content (no sub-layers) and we
    // TODO: push the transform into a content node
    if (mLayer->getTransform() != other.mLayer->getTransform())
        return false;

    // Don't merge layers with different opacities
    // TODO: Allowable if the layers only have content (no sub-layers) and the opacity is pushed
    // TODO: into the content nodes
    if (mLayer->getOpacity() != other.mLayer->getOpacity())
        return false;

    // If this layer has child layers and the other layer has content, don't allow the merge
    // because the draw order will be messed up.
    // TODO: We could move the other layer's content into a layer and merge anyway
    if (!mLayer->children().empty() && other.layer()->content())
        return false;

    // The merge is okay.  Combine the sub-layers
    mLayer->appendChildren(other.layer()->children());

    // Join the content together and calculate a new bounding box
    auto node = mLayer->content();
    auto otherNode = other.layer()->content();
    mLayer->setContent(sg::Node::appendSiblingToNode(node, otherNode));
    fixBoundingBox();

    // Update the other elements to point to the new layer
    assert(!other.mElements.empty());
    for (const auto& m : other.mElements)
        m->assignSceneGraphLayer(mLayer);
    mElements.insert(mElements.end(), other.mElements.begin(), other.mElements.end());
    return true;
}

void
GraphicFragment::addChild(const GraphicFragmentPtr& otherPtr, sg::SceneGraphUpdates& sceneGraph)
{
    if (!otherPtr)
        return;

    auto& other = *otherPtr.get();
    if (other.empty())
        return;

    // If the other is a layer, we force this to be a layer and add the other to our children list
    if (other.isLayer()) {
        ensureLayer(sceneGraph);
        mLayer->appendChild(other.layer());
        return;
    }

    // If we're a node and the other is a node (as tested above), just append the nodes
    if (!isLayer()) {
        mNode = sg::Node::appendSiblingToNode(mNode, other.node());
        if (mType != kNodeContentMutable)
            mType = other.mType;
        mElements.insert(mElements.begin(), other.mElements.begin(), other.mElements.end());
        return;
    }

    // ==== We're a layer and the other is a node ====
    assert(other.mType == kNodeContentFixed); // If it was mutable, it would be a layer

    // We have at least one child already.  Force the other to become a layer and append it
    if (!mLayer->children().empty()) {
        other.ensureLayer(sceneGraph);
        mLayer->appendChild(other.layer());
        return;
    }

    // We have no children.  The other node can be appended onto our content.
    // Fix the bounding box and copy over the element list from the other fragment
    mLayer->setContent(sg::Node::appendSiblingToNode(mLayer->content(), other.node()));
    fixBoundingBox();
    for (const auto& m : other.mElements)
        m->assignSceneGraphLayer(mLayer);
    mElements.insert(mElements.begin(), other.mElements.begin(), other.mElements.end());
}

void
GraphicFragment::ensureLayer(sg::SceneGraphUpdates& sceneGraph)
{
    if (!mLayer) {
        assert(!mElements.empty());
        mLayer = sg::layer(mElements.front()->getUniqueId()+"_sub", Rect(0,0,0,0), 1.0f, Transform2D());
        sceneGraph.created(mLayer);
        mLayer->setContent(mNode);
        mLayer->setCharacteristic(sg::Layer::kCharacteristicRenderOnly |
                                  sg::Layer::kCharacteristicDoNotClipChildren);
        mNode = nullptr;
        mType = (mType == kNodeContentMutable ? kLayerFixedContentMutable : kLayerFixedContentFixed);
        for (const auto& m : mElements)
            m->assignSceneGraphLayer(mLayer);
        fixBoundingBox();
    }
}

void
GraphicFragment::fixBoundingBox()
{
    if (mLayer) {
        Rect bb = sg::Node::calculateBoundingBox(mLayer->content());
        if (!bb.empty()) {
            mLayer->setBounds(bb);
            mLayer->setContentOffset(bb.getTopLeft());
            mLayer->setChildOffset(bb.getTopLeft());
        }
        else {
            mLayer->setBounds(Rect(0,0,0,0));
        }
    }
}

void
GraphicFragment::applyFilters(const Object &filters)
{
    // Build up the filter list in reverse order
    for (int i = 0; i < filters.size(); i++) {
        const auto& filter = filters.at(i).get<GraphicFilter>();
        switch (filter.getType()) {
            case kGraphicFilterTypeDropShadow: {
                addShadow(sg::shadow(
                    filter.getValue(kGraphicPropertyFilterColor).getColor(),
                    Point{filter.getValue(kGraphicPropertyFilterHorizontalOffset).asFloat(),
                          filter.getValue(kGraphicPropertyFilterVerticalOffset).asFloat()},
                    filter.getValue(kGraphicPropertyFilterRadius).asFloat()));
            } break;
        }
    }
}

void
GraphicFragment::assignToLayer(const sg::LayerPtr& containingLayer)
{
    for (const auto& m : mElements)
        m->assignSceneGraphLayer(containingLayer);
}

std::string
GraphicFragment::toDebugString() const
{
    std::string result = "";

    switch (mType) {
        case kEmpty:
            result += "NodeEmpty<";
            break;
        case kNodeContentFixed:
            result += "NodeContentFixed<";
            break;
        case kNodeContentMutable:
            result += "NodeContentMutable<";
            break;
        case kLayerFixedContentFixed:
            result += "LayerFixedContentFixed<";
            break;
        case kLayerFixedContentMutable:
            result += "LayerFixedContentMutable<";
            break;
        case kLayerMutable:
            result += "LayerMutable<";
            break;
    }

    if (mLayer)
        result += "layer=" + mLayer->getName() + " sublayers=" + std::to_string(mLayer->children().size());
    else if (mNode)
        result += "node=" + mNode->toDebugString();

    for (const auto& m : mElements)
        result += " element=" + m->toDebugString();

    return result + ">";
}

void
GraphicFragment::addShadow(const sg::ShadowPtr& shadow)
{
    if (mLayer) {
        mLayer->setShadow(shadow);
    }
    else if (mNode) {
        mNode = sg::shadowNode(shadow, mNode);
    }
}

} // namespace sg
} // namsspace apl
