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

#ifndef _APL_SG_GRAPHIC_FRAGMENT
#define _APL_SG_GRAPHIC_FRAGMENT

#include <vector>

#include "apl/common.h"
#include "apl/scenegraph/common.h"

namespace apl {

class Object;

namespace sg {

/**
 * When a vector graphic is first inflated into a scene graph, each graphic element returns
 * a GraphicFragment containing the logic to render that graphic element.  Graphic groups and
 * the top-level container assemble the fragments and merge them together into a final fragment
 * containing a tree of layers where each layer may have a content node.
 *
 * An individual fragment is either empty, contains just a Node, or contains a Layer which
 * may have sub-layers and a content node.  The Type of the fragment tracks if the content
 * of the fragment (either the direct Node or the content node of the Layer) is fixed (i.e.,
 * never changes) or mutable (from a style or parameter change).  The Type may also indicate
 * that the Layer properties mutate (these are the layer transform, opacity, and clipping outline).
 *
 * The fragment also accumulates a list of GraphicElements.  When a GraphicElement property
 * changes it needs to know the scene graph Layer to update.  As the GraphicFragments are merged
 * and updated to create the final scene graph we track which elements are associated with a
 * particular Node or Layer and eventually use this list to assign each GraphicElement to a specific
 * Layer.
 */
class GraphicFragment {
public:
    enum Type {
        kEmpty,
        kNodeContentFixed,
        kNodeContentMutable,      // Only possible in a layer-free design
        kLayerFixedContentFixed,  // Includes the case with no content
        kLayerFixedContentMutable,
        kLayerMutable
    };

    static GraphicFragmentPtr create(const GraphicElementPtr& element);
    static GraphicFragmentPtr create(const GraphicElementPtr& element, const sg::NodePtr& node,
                                     Type flags);
    static GraphicFragmentPtr create(const GraphicElementPtr& element, const sg::LayerPtr& layer,
                                     Type flags);

    GraphicFragment() = default;
    bool empty() const { return !mNode && !mLayer; }

    sg::NodePtr node() const { return mNode; }
    sg::LayerPtr layer() const { return mLayer; }

    bool isNode() const { return mNode != nullptr; }
    bool isLayer() const { return mLayer != nullptr; }

    void setNode(const sg::NodePtr& node) { mNode = node; }

    Type getType() const { return mType; }
    void setType(Type type) { mType = type; }

    GraphicFragment& operator=(const GraphicFragment&) = default;

    bool mergeWith(const GraphicFragmentPtr& otherPtr);
    void addChild(const GraphicFragmentPtr& otherPtr, sg::SceneGraphUpdates& sceneGraph);
    void ensureLayer(sg::SceneGraphUpdates& sceneGraph);

    void fixBoundingBox();
    void applyFilters(const Object& filters);
    void clearElements() { mElements.clear(); }
    void assignToLayer(const sg::LayerPtr& containingLayer);

    std::string toDebugString() const;

private:
    void addShadow(const sg::ShadowPtr& shadow);

private:
    std::vector<GraphicElementPtr> mElements;  // Elements that refer to this content
    sg::NodePtr mNode;
    sg::LayerPtr mLayer;
    Type mType = kEmpty;
};

} // namespace sg
} // namespace apl

#endif // _APL_SG_GRAPHIC_FRAGMENT
