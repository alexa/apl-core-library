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
#include "apl/component/yogaproperties.h"
#include "apl/content/configurationchange.h"
#include "apl/document/coredocumentcontext.h"
#include "apl/engine/corerootcontext.h"
#include "apl/livedata/layoutrebuilder.h"
#include "apl/utils/tracing.h"

namespace apl {

static const bool DEBUG_LAYOUT_MANAGER = false;

static void
yogaNodeDirtiedCallback(YGNodeRef node)
{
    auto component = static_cast<CoreComponent*>(YGNodeGetContext(node));
    assert(component);
    LOG_IF(DEBUG_LAYOUT_MANAGER).session(*component) << "dirty top node";
    component->getContext()->layoutManager().requestLayout(component->shared_from_corecomponent(), false);
}

LayoutManager::LayoutManager(const CoreRootContext& coreRootContext, ViewportSize size)
    : mRoot(coreRootContext),
      mConfiguredSize(size)
{
}

void
LayoutManager::terminate()
{
    mTerminated = true;
    mPendingLayout.clear();
}

void
LayoutManager::setSize(ViewportSize size)
{
    mConfiguredSize = size;
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

    auto top = CoreComponent::cast(mRoot.topComponent());
    setAsTopNode(top);
    mPendingLayout.emplace(top);
    layout(false, true);
}

void
LayoutManager::configChange(const ConfigurationChange& change,
                            const CoreDocumentContextPtr& document)
{
    if (mTerminated)
        return;

    if (!change.hasSizeChange())
        return;

    // Update the global size to match the configuration change only if we are not embedded
    auto size = change.getSize(mRoot.getPxToDp());
    if (!document->isEmbedded())
        mConfiguredSize = size;

    auto top = CoreComponent::cast(document->topComponent());
    if (top && (!size.isFixed() || top->getLayoutSize() != size.nominalSize()))
        requestLayout(top, true);
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

    for (const auto& m : postProcess) {
        auto component = m.first.first.lock();
        if (component)
            component->setProperty(m.first.second, m.second);
    }

    // After layout has completed we mark individual components as allowing event handlers
    for (const auto& m : laidOut)
        m->postProcessLayoutChanges();
}

void
LayoutManager::flushLazyInflation()
{
    flushLazyInflationInternal(CoreComponent::cast(mRoot.topComponent()));
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
    assert(component);
    LOG_IF(DEBUG_LAYOUT_MANAGER) << component->toDebugSimpleString();
    YGNodeSetDirtiedFunc(component->getNode(), yogaNodeDirtiedCallback);
}

void
LayoutManager::removeAsTopNode(const CoreComponentPtr& component)
{
    assert(component);
    LOG_IF(DEBUG_LAYOUT_MANAGER) << component->toDebugSimpleString();
    YGNodeSetDirtiedFunc(component->getNode(), nullptr);

    // Also remove from pending list
    remove(component);
}

bool
LayoutManager::isTopNode(const ConstCoreComponentPtr& component) const
{
    assert(component);

    return YGNodeGetDirtiedFunc(component->getNode());
}

std::pair<float, float>
LayoutManager::getMinMaxWidth(const CoreComponent& component) const
{
    auto minWidth = 1.0f;
    auto maxWidth = mConfiguredSize.maxWidth;

    if (component.getCalculated(kPropertyMinWidth).asNumber() != 0)
        minWidth = YGNodeStyleGetMinWidth(component.getNode()).value;

    if (!component.getCalculated(kPropertyMaxWidth).isNull())
        maxWidth = std::min(maxWidth, YGNodeStyleGetMaxWidth(component.getNode()).value);

    return std::make_pair(minWidth, maxWidth);
}

std::pair<float, float>
LayoutManager::getMinMaxHeight(const CoreComponent& component) const
{
    auto minHeight = 1.0f;
    auto maxHeight = mConfiguredSize.maxHeight;

    if (component.getCalculated(kPropertyMinHeight).asNumber() != 0)
        minHeight = YGNodeStyleGetMinHeight(component.getNode()).value;

    if (!component.getCalculated(kPropertyMaxHeight).isNull())
        maxHeight = std::min(maxHeight, YGNodeStyleGetMaxHeight(component.getNode()).value);

    return std::make_pair(minHeight, maxHeight);
}

void
LayoutManager::layoutComponent(const CoreComponentPtr& component, bool useDirtyFlag, bool first)
{
    APL_TRACE_BLOCK("LayoutManager:layoutComponent");
    auto parent = CoreComponent::cast(component->getParent());

    LOG_IF(DEBUG_LAYOUT_MANAGER) << "component=" << component->toDebugSimpleString()
                                 << " dirty_flag=" << useDirtyFlag
                                 << " parent=" << (parent ? parent->toDebugSimpleString() : "none");
    Size size;
    ViewportSize viewportSize = {};
    float overallWidth, overallHeight;
    auto node = component->getNode();

    if (!parent) { // Top component
        viewportSize = mConfiguredSize;
        size = viewportSize.layoutSize();  // This will have -1 for variable width/height sizes
        overallWidth = viewportSize.width;
        overallHeight = viewportSize.height;

        if (viewportSize.isAutoWidth() && YGNodeStyleGetWidth(node) == YGValueAuto)
            overallWidth = YGUndefined;

        if (viewportSize.isAutoHeight() && YGNodeStyleGetHeight(node) == YGValueAuto)
            overallHeight = YGUndefined;
    } else {
        size = parent->getCalculated(kPropertyInnerBounds).get<Rect>().getSize();

        auto autoWidth = parent->getCalculated(kPropertyWidth).isAutoDimension();
        auto autoHeight = parent->getCalculated(kPropertyHeight).isAutoDimension();

        // This check is irrelevant for autosizing
        if (size == Size() && !(autoWidth || autoHeight)) return;

        overallWidth = size.getWidth();
        overallHeight = size.getHeight();

        if (autoWidth) {
            overallWidth = YGUndefined;
            auto minmax = getMinMaxWidth(*parent);
            viewportSize.minWidth = minmax.first;
            viewportSize.maxWidth = minmax.second;
        }
        if (autoHeight) {
            overallHeight = YGUndefined;
            auto minmax = getMinMaxHeight(*parent);
            viewportSize.minHeight = minmax.first;
            viewportSize.maxHeight = minmax.second;
        }

        size = Size(autoWidth ? -1 : size.getWidth(),
                    autoHeight ? -1 : size.getHeight());
    }

    // Layout the component if it has a dirty Yoga node OR if the cached size doesn't match the target size
    // The top-level component may get laid out multiple times if it auto sizes.
    if (YGNodeIsDirty(node) || size != component->getLayoutSize()) {
        component->preLayoutProcessing(useDirtyFlag);
        APL_TRACE_BEGIN("LayoutManager:YGNodeCalculateLayout");

        YGNodeCalculateLayout(node, overallWidth, overallHeight, component->getLayoutDirection());

        // If we were allowing the overall width to vary, then the node width was "auto".
        // Re-layout the node with a fixed width that is clipped to min/max
        if (YGFloatIsUndefined(overallWidth)) {
            overallWidth = YGNodeLayoutGetWidth(node);
            if (overallWidth > viewportSize.maxWidth)
                overallWidth = viewportSize.maxWidth;
            if (overallWidth < viewportSize.minWidth)
                overallWidth = viewportSize.minWidth;
            YGNodeCalculateLayout(node, overallWidth, overallHeight, component->getLayoutDirection());
        }
        else if (viewportSize.isAutoWidth() && YGNodeStyleGetWidth(node).unit == YGUnit::YGUnitPoint) {
            overallWidth = YGNodeLayoutGetWidth(node);
            if (overallWidth > viewportSize.maxWidth)
                overallWidth = viewportSize.maxWidth;
            if (overallWidth < viewportSize.minWidth)
                overallWidth = viewportSize.minWidth;
        }

        // If we were allowing the overall height to vary, then the node height was "auto".
        // Re-layout the node with a fixed height that is clipped to min/max
        if (YGFloatIsUndefined(overallHeight)) {
            overallHeight = YGNodeLayoutGetHeight(node);
            if (overallHeight > viewportSize.maxHeight)
                overallHeight = viewportSize.maxHeight;
            if (overallHeight < viewportSize.minHeight)
                overallHeight = viewportSize.minHeight;
            YGNodeCalculateLayout(node, overallWidth, overallHeight, component->getLayoutDirection());
        }
        else if (viewportSize.isAutoHeight() && YGNodeStyleGetHeight(node).unit == YGUnit::YGUnitPoint) {
            overallHeight = YGNodeLayoutGetHeight(node);
            if (overallHeight > viewportSize.maxHeight)
                overallHeight = viewportSize.maxHeight;
            if (overallHeight < viewportSize.minHeight)
                overallHeight = viewportSize.minHeight;
        }

        APL_TRACE_END("LayoutManager:YGNodeCalculateLayout");
        component->processLayoutChanges(useDirtyFlag, first);

        if (mNeedToReProcessLayoutChanges) {
            // Previous call may have changed sizes for auto-sized components if any laziness involved. Apply this changes.
            component->processLayoutChanges(useDirtyFlag, first);
            mNeedToReProcessLayoutChanges = false;
        }
    }

    if (!parent)
        mRoot.setViewportSize(overallWidth, overallHeight);
    else {
        // Set parent size to something reasonable (and not auto) and request parent's
        // re-layout, if auto
        if (parent->getCalculated(kPropertyWidth).isAutoDimension()) {
            yn::setWidth(parent->getNode(), overallWidth, *parent->getContext());
            requestLayout(CoreComponent::cast(parent->getContext()->topComponent()), false);
        }
        if (parent->getCalculated(kPropertyHeight).isAutoDimension()) {
            yn::setHeight(parent->getNode(), overallHeight, *parent->getContext());
            requestLayout(CoreComponent::cast(parent->getContext()->topComponent()), false);
        }
    }

    // Cache the laid-out size of the component.  -1 values are for variable viewport sizes
    component->setLayoutSize(size);
}


void
LayoutManager::requestLayout(const CoreComponentPtr& component, bool force)
{
    LOG_IF(DEBUG_LAYOUT_MANAGER) << component->toDebugSimpleString() << " force=" << force;
    assert(component);

    if (mTerminated)
        return;

    assert(isTopNode(component));
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
        auto parent = CoreComponent::cast(child->getParent());

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
