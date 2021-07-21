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

#ifndef _APL_LAYOUT_MANAGER_H
#define _APL_LAYOUT_MANAGER_H

#include <set>
#include <map>

#include "apl/common.h"
#include "apl/primitives/object.h"
#include "apl/primitives/size.h"
#include "apl/component/componentproperties.h"

namespace apl {

class RootContextData;
class ConfigurationChange;

/**
 * The LayoutManager keeps track of which components have properties that have changed and need to have
 * their layout recalculated.
 *
 * Each Component has a Yoga node.  In most cases the Yoga node is attached to a parent (owner) node.
 * When the node is marked "dirty" (that is, when a Yoga property on the node has changed), the dirty
 * flag is propagated up through the node hierarchy to the top node.  Note that adding and removing Yoga nodes
 * does NOT dirty the Yoga hierarchy.  Any time you add or remove a component, you need to inform the
 * layout manager.
 *
 * The top node in the hierarchy has a Yoga "dirtied_" function attached to it.  When the dirty flag
 * reaches a node with a "dirtied_" function, it calls the LayoutManager and passes the component that owns
 * the node.  The LayoutManager maintains an ordered list of components that have the dirty flag.  The
 * RootContext "clearPending()" method checks that LayoutManager to see if any components are dirty and
 * executes any pending layouts.
 *
 * There are two cases where a component may have a node that is NOT attached to a parent.
 *
 * Case #1: Children of a Pager
 *
 * A Pager does not lay out its children using the Flexbox algorithm.  Instead, each child is placed at
 * the top-left corner of the Pager (rather like absolute positioning).  Each child node is assigned a Yoga
 * "dirtied_" function which informs the LayoutManager if the Pager child needs to be laid out.  If the
 * Pager itself changes size, the LayoutManager is informed that the children need to be laid out.
 *
 * Case #2: Children of a Sequence or GridSequence (MultiChildScrollableComponent)
 *
 * Sequences can support an infinite number of children.  Instead of laying out all of the children,
 * the MultiChildScrollableComponent keeps an "ensured range" of children which have Yoga nodes attached
 * to the node hierarchy.  As the component scrolls the ensured range is updated and additional nodes
 * are attached to the hierarchy.
 */

class LayoutManager {
public:
    explicit LayoutManager(const RootContextData& core);

    /**
     * Stop all layout processing (and future layout processing)
     */
    void terminate();

    /**
     * @return True if there are components that need a layout pass
     */
    bool needsLayout() const;

    /**
     * First layout - set up the "top" component dirty method and lay
     * out all components without setting dirty flags
     */
    void firstLayout();

    /**
     * Layout all pending components
     * @param useDirtyFlag If true, updated properties will set a dirty flag in components
     * @param first if true - it's a first layout for this document
     */
    void layout(bool useDirtyFlag, bool first = false);

    /**
     * Flash any non-inflated components in hierarchy, where supported.
     */
    void flushLazyInflation();

    /**
     * Inform the layout manager of a configuration change.  If the configuration change
     * affects the layout, this will schedule a layout pass.
     * @param change The configuration change
     */
    void configChange(const ConfigurationChange& change);

    /**
     * Mark this component as the top of a Yoga hierarchy
     * @param component The component
     */
    void setAsTopNode(const CoreComponentPtr& component);

    /**
     * Unmark this component as the top of a Yoga hierarchy.
     * @param component
     */
    void removeAsTopNode(const CoreComponentPtr& component);

    /**
     * Mark this component as dirty and needing layout.  This component must be at the top
     * of a node hierarchy (i.e., it has a "dirtied_" method and no parent).
     * @param component The component
     * @param force If true, guarantee that a layout will be performed. If false, the layout is only
     *              performed if the new layout size is different from the old layout size.
     */
    void requestLayout(const CoreComponentPtr& component, bool force);

    /**
     * Remove this component from the pending layout list.  Normally used when a component is removed
     * from the DOM.
     * @param component The component to remove
     */
    void remove(const CoreComponentPtr& component);

    /**
     * Ensure that this component has been laid out.
     * @param component The component to ensure.
     * @return True if a layout pass is needed
     */
    bool ensure(const CoreComponentPtr& component);

    /**
     * Add a component property to the list to be executed after layout is completed
     * @param component The associated component
     * @param key The property to set
     * @param value The value to set on the property
     */
    void addPostProcess(const CoreComponentPtr& component, PropertyKey key, const Object& value );

    /**
     * Notify LayoutManager that additional processing pass required after layout.
     */
    void needToReProcessLayoutChanges() { mNeedToReProcessLayoutChanges = true; }

private:
    void layoutComponent(const CoreComponentPtr& component, bool useDirtyFlag, bool first);
    void flushLazyInflationInternal(const CoreComponentPtr& comp);

private:
    using PPKey = std::pair<CoreComponentPtr, PropertyKey>;

    const RootContextData& mCore;
    std::set<CoreComponentPtr> mPendingLayout;
    Size mConfiguredSize;
    bool mTerminated = false;
    bool mInLayout = false;    // Guard against recursive calls to layout
    bool mNeedToReProcessLayoutChanges = false;
    std::map<PPKey, Object> mPostProcess;   // Collection of elements to post-process
};

} // namespace apl

#endif // _APL_LAYOUT_MANAGER_H
