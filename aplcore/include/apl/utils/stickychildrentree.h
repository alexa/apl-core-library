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

#ifndef APL_STICKYCHILDRENTREE_H
#define APL_STICKYCHILDRENTREE_H

#include "apl/common.h"
#include "apl/component/corecomponent.h"

namespace apl {

class StickyNode;
using StickyChildrenTreePtr = std::shared_ptr<StickyChildrenTree>;
/**
 * Used by scrollable components to keep track of descendants with position: sticky. To calculate
 * nested sticky components need to keep a tree of sticky descendants so that we can update them
 * in order from ancestor to descendant.
 */
class StickyChildrenTree {
public:
    StickyChildrenTree(CoreComponent &scrollable);

    /**
     * Handle when a components position type is set to 'sticky'
     */
    void handleChildStickySet();

    /**
     * Handle when a components position type is set from 'sticky' to something else
     */
    void handleChildStickyUnset();

    /**
     * When a child is inserted we must check the child and it's descendants to see if there
     * is any sticky components we need to add to our tree of sticky components
     */
    void handleChildInsert(const CoreComponentPtr& component);

    /**
     * When a child is removed we must update our tree of sticky components in case a sticky
     * component has been removed
     */
    void handleChildRemove();

    /**
     * Recalculate and update the sticky offsets applied to all the sticky components in this tree
     */
    void updateStickyOffsets();

private:
    std::shared_ptr<StickyNode> mRoot;
    CoreComponent& mScrollable;//The scrollable that contains this tree
};

} // namespace apl

#endif // APL_STICKYCHILDRENTREE_H