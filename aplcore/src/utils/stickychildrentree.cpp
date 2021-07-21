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

#include "apl/utils/stickyfunctions.h"
#include "apl/utils/stickychildrentree.h"

namespace apl {

/*
 * We create a StickyNode for each child of a scrollable with position: sticky for use in the StickyChildrenTree
 */
class StickyNode {
public:
    StickyNode() = default;

    StickyNode(std::weak_ptr<CoreComponent> component) :
        mComponent(component) {};

    void updateStickyOffsets() const;

    std::weak_ptr<CoreComponent> mComponent;
    std::map<CoreComponentPtr, std::shared_ptr<StickyNode>> mStickyChildren;
};

StickyChildrenTree::StickyChildrenTree(CoreComponent &scrollable) :
    mRoot(std::make_shared<StickyNode>()), mScrollable(scrollable) {};

/**
 * Recursively build a tree of the descendants with position: sticky
 * @param currentNode The nearest ancestor sticky component or scrollable
 * @param c The component we will we traverse down and build sticky nodes from
 * @param scrollableType We need to know the scroll type of this scrollable so we can ignore children
 * of descendant scrollables of the same type
 */
static void
buildStickyTreeInternal(std::shared_ptr<StickyNode> currentNode, const CoreComponentPtr& c, ScrollType scrollableType) {
    std::shared_ptr<StickyNode> nextNode = currentNode;
    if (c->getCalculated(kPropertyPosition) == kPositionSticky) {
        auto node = std::make_shared<StickyNode>(c);
        nextNode->mStickyChildren.insert(std::make_pair(c, node));
        nextNode = node;
    }

    for (int i = 0; i < c->getChildCount(); i++) {
        auto child = c->getCoreChildAt(i);
        // We only traverse children for which the current scrollable is the nearest ancestor scrollable of 'scrollableType'
        if (child->scrollType() != scrollableType)
            buildStickyTreeInternal(nextNode, child, scrollableType);
    }
}

static void
rebuildStickyTree(std::shared_ptr<StickyNode> node, CoreComponentPtr c, ScrollType scrollableType) {
    node->mStickyChildren.clear();
    buildStickyTreeInternal(node, c, scrollableType);
}

void
StickyNode::updateStickyOffsets() const {
    if (auto component = mComponent.lock()) {
        stickyfunctions::updateStickyOffset(component);
        for (auto& stickyChild : mStickyChildren) {
            stickyChild.second->updateStickyOffsets();
        }
    }
}

void
StickyChildrenTree::handleChildStickySet() {
    // Rebuild the entire tree. This is inefficient but simple
    rebuildStickyTree(mRoot, mScrollable.shared_from_corecomponent(), mScrollable.scrollType());
}

void
StickyChildrenTree::handleChildStickyUnset() {
    // Rebuild the entire tree. This is inefficient but simple
    rebuildStickyTree(mRoot, mScrollable.shared_from_corecomponent(), mScrollable.scrollType());
}

void
StickyChildrenTree::handleChildRemove() {
    // Rebuild the entire tree. This is inefficient but simple
    rebuildStickyTree(mRoot, mScrollable.shared_from_corecomponent(), mScrollable.scrollType());
}

void
StickyChildrenTree::handleChildInsert(const CoreComponentPtr& component) {
    // We call insertChild for each child during a document inflate and so we want to avoid traversing
    // the every child of our ancestor scrollables each time we insert a child. Instead of rebuilding
    // the entire tree we just add the extra nodes and only traverse the children of the inserted child

    auto parent = std::dynamic_pointer_cast<CoreComponent>(component->getParent());

    // Find the StickyNode we need to change by finding all the ancestors with position: sticky
    // and their corresponding sticky nodes
    auto ancestor = parent;
    // To find the StickyNode associated with @component we need to find it's position in the tree
    // by finding all the StickyNodes associated with any of it's ancestors who are sticky up until
    // the scrollable
    std::stack<CoreComponentPtr> parentNodeComponents;
    while (ancestor != nullptr) {
        if (ancestor->scrollType() == mScrollable.scrollType())
            break;

        if (ancestor->getCalculated(kPropertyPosition) == kPositionSticky)
            parentNodeComponents.push(ancestor);

        ancestor = std::dynamic_pointer_cast<CoreComponent>(ancestor->getParent());
    }
    assert(ancestor && ancestor->scrollType() == mScrollable.scrollType());

    // We now find each sticky node associated with each of @component's sticky ancestors to find
    // the position of the StickyNode associated with @component
    std::shared_ptr<StickyNode> currentNode = mRoot;
    while (!parentNodeComponents.empty()) {
        currentNode = currentNode->mStickyChildren[parentNodeComponents.top()];
        assert(currentNode);
        parentNodeComponents.pop();
    }

    buildStickyTreeInternal(currentNode, component, mScrollable.scrollType());
}

void
StickyChildrenTree::updateStickyOffsets() {
    for (auto& child : mRoot->mStickyChildren) {
        child.second->updateStickyOffsets();
    }
}

} // namespace apl