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

#include "apl/engine/layoutmanager.h"
#include "apl/component/corecomponent.h"
#include "apl/content/configurationchange.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/livedata/layoutrebuilder.h"
#include "apl/primitives/size.h"
#include "apl/utils/tracing.h"

namespace apl {

static const bool DEBUG_LAYOUT_MANAGER = false;

static void
yogaNodeDirtiedCallback(YGNodeRef node)
{
    LOG_IF(DEBUG_LAYOUT_MANAGER) << "dirty top node";

    auto component = static_cast<CoreComponent*>(YGNodeGetContext(node));
    assert(component);
    component->getContext()->layoutManager().requestLayout(component->shared_from_corecomponent(), false);
}

LayoutManager::LayoutManager(const apl::RootContextData& core)
    : mCore(core),
      mConfiguredSize(mCore.getSize())
{
}

void
LayoutManager::terminate()
{
    mTerminated = true;
    mPendingLayout.clear();
}

bool
LayoutManager::needsLayout() const
{
    if (mTerminated)
        return false;

    return !mPendingLayout.empty();
}

void
LayoutManager::firstLayout()
{
    LOG_IF(DEBUG_LAYOUT_MANAGER) << mTerminated;

    if (mTerminated)
        return;

    APL_TRACE_BLOCK("LayoutManager:firstLayout");

    assert(mCore.top());
    YGNodeSetDirtiedFunc(mCore.top()->getNode(), yogaNodeDirtiedCallback);
    mPendingLayout.emplace(mCore.top());
    layout(false, true);
}

void
LayoutManager::configChange(const ConfigurationChange& change)
{
    if (mTerminated)
        return;

    // Update the global size to match the configuration change
    mConfiguredSize = change.mergeSize(mConfiguredSize) * mCore.getPxToDp();

    // If there is a size mismatch, schedule a layout
    auto top = mCore.top();
    if (top && top->getLayoutSize() != mConfiguredSize)
        mPendingLayout.emplace(top);
}

static bool
compareComponents(const CoreComponentPtr& a, const CoreComponentPtr& b)
{
    if (a == b)
        return false;

    // Check if 'b' is a child of 'a'
    auto parent = b->getParent();
    while (parent) {
        if (parent == a)
            return true;
        parent = parent->getParent();
    }

    return false;
}

void
LayoutManager::layout(bool useDirtyFlag, bool first)
{
    LOG_IF(DEBUG_LAYOUT_MANAGER) << mTerminated << " dirty_flag=" << useDirtyFlag;

    if (mTerminated || mInLayout)
        return;

    APL_TRACE_BLOCK("LayoutManager:layout");

    std::set<CoreComponentPtr> laidOut;

    mInLayout = true;
    while (needsLayout()) {
        LOG_IF(DEBUG_LAYOUT_MANAGER) << "Laying out " << mPendingLayout.size() << " component(s)";

        // Copy the pending components into a vector and sort them from top to bottom
        std::vector<CoreComponentPtr> dirty(mPendingLayout.begin(), mPendingLayout.end());
        std::sort(dirty.begin(), dirty.end(), compareComponents);
        mPendingLayout.clear();

        for (const auto& m : dirty) {
            layoutComponent(m, useDirtyFlag, first);
            laidOut.emplace(m);
        }
    }
    mInLayout = false;

    // Post-process all of the layouts.  This may result in scroll commands or other "jumping around"
    // actions, which can toggle more pending layouts.
    auto postProcess = mPostProcess;
    mPostProcess.clear();

    for (const auto& m : postProcess)
        m.first.first->setProperty(m.first.second, m.second);

    // After layout has completed we mark individual components as allowing event handlers
    for (const auto& m : laidOut)
        m->postProcessLayoutChanges();
}

void
LayoutManager::flushLazyInflation()
{
    flushLazyInflationInternal(mCore.top());
}

void
LayoutManager::flushLazyInflationInternal(const CoreComponentPtr& comp)
{
    for (const auto& child : comp->mChildren) {
        if (comp->mRebuilder) {
            comp->mRebuilder->inflateIfRequired(child);
        }
        flushLazyInflationInternal(child);
    }
}

void
LayoutManager::setAsTopNode(const CoreComponentPtr& component)
{
    LOG_IF(DEBUG_LAYOUT_MANAGER) << component->toDebugSimpleString();
    assert(component);
    YGNodeSetDirtiedFunc(component->getNode(), yogaNodeDirtiedCallback);
}

void
LayoutManager::removeAsTopNode(const CoreComponentPtr& component)
{
    LOG_IF(DEBUG_LAYOUT_MANAGER) << component->toDebugSimpleString();
    assert(component);
    YGNodeSetDirtiedFunc(component->getNode(), nullptr);
}

void
LayoutManager::layoutComponent(const CoreComponentPtr& component, bool useDirtyFlag, bool first)
{
    APL_TRACE_BLOCK("LayoutManager:layoutComponent");
    auto parent = component->getParent();

    LOG_IF(DEBUG_LAYOUT_MANAGER) << "component=" << component->toDebugSimpleString()
                                 << " dirty_flag=" << useDirtyFlag
                                 << " parent=" << (parent ? parent->toDebugSimpleString() : "none");

    Size size = parent ? parent->getCalculated(kPropertyInnerBounds).getRect().getSize() : mConfiguredSize;
    if (size.empty())
        return;

    auto node = component->getNode();

    // Layout the component if it has a dirty Yoga node OR if the cached size doesn't match the target size
    if (YGNodeIsDirty(node) || size != component->getLayoutSize()) {
        APL_TRACE_BEGIN("LayoutManager:YGNodeCalculateLayout");
        YGNodeCalculateLayout(node, size.getWidth(), size.getHeight(), component->getLayoutDirection());
        APL_TRACE_END("LayoutManager:YGNodeCalculateLayout");
        component->processLayoutChanges(useDirtyFlag, first);

        if (mNeedToReProcessLayoutChanges) {
            // Previous call may have changed sizes for auto-sized components if any lazyness involved. Apply this changes.
            component->processLayoutChanges(useDirtyFlag, first);
            mNeedToReProcessLayoutChanges = false;
        }
    }

    // Cache the laid-out size of the component.
    component->setLayoutSize(size);
}


void
LayoutManager::requestLayout(const CoreComponentPtr& component, bool force)
{
    LOG_IF(DEBUG_LAYOUT_MANAGER) << component->toDebugSimpleString() << " force=" << force;
    assert(component);

    if (mTerminated)
        return;

    assert(YGNodeGetDirtiedFunc(component->getNode()));
    mPendingLayout.emplace(component);
    if (force)
        component->setLayoutSize({});
}


void
LayoutManager::remove(const CoreComponentPtr& component)
{
    mPendingLayout.erase(component);
}


/**
 * Calling "ensure" on ANY component guarantees that it and all of its ancestors have properly
 * attached Yoga nodes.  This method ascends the DOM hierarchy checking that each component
 * has an attached Yoga node or "dirtied_" method.  It schedules components for layout as needed.
 *
 * Components fall into one of the following categories:
 *
 * Parent   | Dirtied_ | NodeOwner | Description                         | Action
 * -------- | -------- | --------- | ----------------------------------- | -----------
 * null     | exists   | none      | Top of the DOM                      | Schedule a layout pass if a descendant requested
 * non-null | exists   | none      | Child of a Pager                    | Schedule a layout pass (will only run if needed)
 * non-null | none     | none      | Child of a Sequence/GridSequence    | Attach this node; request layout pass from ancestor
 * non-null | none     | exists    | Already attached - no work required | None
 *
 * @param component The component to ensure.
 * @return True if a layout pass should be run
 */
bool
LayoutManager::ensure(const CoreComponentPtr& component)
{
    LOG_IF(DEBUG_LAYOUT_MANAGER) << component->toDebugSimpleString();

    // Walk up the component hierarchy and ensure that Yoga nodes are correctly attached
    bool result = false;
    bool attachedYogaNodeNeedsLayout = false;
    auto child = component;
    while (child->getParent()) {
        auto parent = std::dynamic_pointer_cast<CoreComponent>(child->getParent());

        // If the child is attached to its parent, we don't need to do anything
        if (!child->getNode()->getOwner()) {
            result = true;
            if (child->getNode()->getDirtied()) {    // This child has a dirtied_ method; it should not be attached
                mPendingLayout.emplace(child);       // Schedule this child for layout.  It will only run if it is needed
                if (attachedYogaNodeNeedsLayout) {   // If a child node was attached, force the layout
                    child->setLayoutSize({});
                    attachedYogaNodeNeedsLayout = false;
                }
            } else {   // This child needs to be attached to its parent
                LOG_IF(DEBUG_LAYOUT_MANAGER) << "Attaching yoga node from: " << child->toDebugSimpleString();
                parent->attachYogaNode(child);
                attachedYogaNodeNeedsLayout = true;
            }
        }

        child = parent;
    }

    // If there is a dangling node that was attached, force a layout pass on the top node.
    if (attachedYogaNodeNeedsLayout) {
        mPendingLayout.emplace(child);
        child->setLayoutSize({});
    }

    return result;
}

void
LayoutManager::addPostProcess(const CoreComponentPtr& component, PropertyKey key, const Object& value)
{
    mPostProcess[{component, key}] = value;
}

} // namespace apl