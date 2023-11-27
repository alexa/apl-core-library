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

#include "apl/component/corecomponent.h"

#include <cmath>
#include <yoga/YGNode.h>

#include "apl/common.h"

#include "apl/component/componenteventsourcewrapper.h"
#include "apl/component/componenteventtargetwrapper.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/builder.h"
#include "apl/engine/contextwrapper.h"
#include "apl/engine/hovermanager.h"
#include "apl/engine/layoutmanager.h"
#include "apl/engine/typeddependant.h"
#include "apl/focus/focusmanager.h"
#include "apl/livedata/layoutrebuilder.h"
#include "apl/livedata/livearray.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/primitives/accessibilityaction.h"
#include "apl/primitives/keyboard.h"
#include "apl/primitives/transform.h"
#include "apl/time/sequencer.h"
#include "apl/time/timemanager.h"
#include "apl/touch/pointerevent.h"
#include "apl/utils/hash.h"
#include "apl/utils/searchvisitor.h"
#include "apl/utils/session.h"
#include "apl/utils/stickychildrentree.h"
#include "apl/utils/stickyfunctions.h"
#include "apl/utils/tracing.h"

#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/scenegraph.h"
#endif // SCENEGRAPH

namespace apl {

const std::string VISUAL_CONTEXT_TYPE_MIXED = "mixed";
const std::string VISUAL_CONTEXT_TYPE_GRAPHIC = "graphic";
const std::string VISUAL_CONTEXT_TYPE_TEXT = "text";
const std::string VISUAL_CONTEXT_TYPE_VIDEO = "video";
const std::string VISUAL_CONTEXT_TYPE_EMPTY = "empty";

/*****************************************************************/

const std::string CHILDREN_CHANGE_UID = "uid";
const std::string CHILDREN_CHANGE_INDEX = "index";
const std::string CHILDREN_CHANGE_ACTION = "action";
const std::string CHILDREN_CHANGE_CHANGES = "changes";

/*****************************************************************/

const static bool DEBUG_BOUNDS = false;
const static bool DEBUG_ENSURE = false;
const static bool DEBUG_LAYOUTDIRECTION = false;
const static bool DEBUG_PADDING = false;
const static bool DEBUG_MEASUREMENT = false;

using ComponentDependant = TypedDependant<CoreComponent, PropertyKey>;

/**
 * A helper function to zero the insets for when a component's position is change to "sticky"
 */
static void
zeroComponentInsets(const CoreComponentPtr &component) {
    auto nodeRef = component->getNode();
    yn::setPosition<YGEdgeLeft>(nodeRef,    NAN, *component->getContext());
    yn::setPosition<YGEdgeBottom>(nodeRef,  NAN, *component->getContext());
    yn::setPosition<YGEdgeRight>(nodeRef,   NAN, *component->getContext());
    yn::setPosition<YGEdgeTop>(nodeRef,     NAN, *component->getContext());
    yn::setPosition<YGEdgeStart>(nodeRef,   NAN, *component->getContext());
    yn::setPosition<YGEdgeEnd>(nodeRef,     NAN, *component->getContext());
    component->setDirty(kPropertyBounds);
}

/**
 * A helper function to restore the zeroed insets for when a component's position is change from "sticky"
 * to something else
 */
static void
restoreComponentInsets(const CoreComponentPtr &component) {
    auto nodeRef = component->getNode();
    yn::setPosition<YGEdgeLeft>(nodeRef,   component->getCalculated(kPropertyLeft),   *component->getContext());
    yn::setPosition<YGEdgeBottom>(nodeRef, component->getCalculated(kPropertyBottom), *component->getContext());
    yn::setPosition<YGEdgeRight>(nodeRef,  component->getCalculated(kPropertyRight),  *component->getContext());
    yn::setPosition<YGEdgeTop>(nodeRef,    component->getCalculated(kPropertyTop),    *component->getContext());
    yn::setPosition<YGEdgeStart>(nodeRef,  component->getCalculated(kPropertyStart),  *component->getContext());
    yn::setPosition<YGEdgeEnd>(nodeRef,    component->getCalculated(kPropertyEnd),    *component->getContext());
    component->setDirty(kPropertyBounds);
}

CoreComponent::CoreComponent(const ContextPtr& context,
                             Properties&& properties,
                             const Path& path)
    : Component(context, properties.asLabel(*context, "id")),
      mInheritParentState(properties.asBoolean(*context, "inheritParentState", false)),
      mStyle(properties.asString(*context, "style", "")),
      mProperties(std::move(properties)),
      mParent(nullptr),
      mYGNodeRef(YGNodeNewWithConfig(context->ygconfig())),
      mPath(path),
      mDisplayedChildrenStale(true),
      mIsDisallowed(false),
      mGlobalToLocalIsStale(true),
      mTextMeasurementHashStale(true),
      mVisualHashStale(true) {
    YGNodeSetContext(mYGNodeRef, this);
}

CoreComponentPtr
CoreComponent::cast( const ComponentPtr& component) {
    return std::static_pointer_cast<CoreComponent>(component);
}

void
CoreComponent::scheduleTickHandler(const Object& handler, double delay) {
    auto weak_ptr = std::weak_ptr<CoreComponent>(shared_from_corecomponent());
    // Lambda capture takes care of handler here as it's a copy.
    mTickHandlerId = mContext->getRootConfig()
                         .getTimeManager()->setTimeout([weak_ptr, handler, delay]() {
        auto self = weak_ptr.lock();
        if (!self)
            return;

        auto ctx = self->createEventContext("Tick");
        if (propertyAsBoolean(*ctx, handler, "when", true)) {
            auto commands = Object(arrayifyProperty(*ctx, handler, "commands"));
            if (!commands.empty())
                ctx->sequencer().executeCommands(commands, ctx, self, true);
        }

        self->scheduleTickHandler(handler, delay);
    }, delay);
}

void
CoreComponent::processTickHandlers() {
    auto& tickHandlers = getCalculated(kPropertyHandleTick);
    if (tickHandlers.empty() || !tickHandlers.isArray())
        return;

    for (const auto& handler : tickHandlers.getArray()) {
        auto delay = std::max(propertyAsDouble(*mContext, handler, "minimumDelay", 1000),
                mContext->getRootConfig().getProperty(RootProperty::kTickHandlerUpdateLimit).getDouble());
        scheduleTickHandler(handler, delay);
    }
}

void
CoreComponent::initialize()
{
    APL_TRACE_BLOCK("CoreComponent:initialize");
    // TODO: Would be nice to work this in with the regular properties more cleanly.
    mState.set(kStateChecked, mProperties.asBoolean(*mContext, "checked", false));
    mState.set(kStateDisabled, mProperties.asBoolean(*mContext, "disabled", false));
    mCalculated.set(kPropertyNotifyChildrenChanged, Object::EMPTY_MUTABLE_ARRAY());

    // Fix up the state variables that can be assigned as a property
    if (mInheritParentState && mParent)
        mState = mParent->getState();

    // Assign the built-in properties.
    assignProperties(propDefSet());

    // Fix layout direction early to ensure correct hash calculation the first time
    fixLayoutDirection(false);

    // After all assigned calculate visual hash.
    fixVisualHash(false);

    // Mixed states always match their properties
    mState.set(kStateChecked, mCalculated.get(kPropertyChecked).asBoolean());
    mState.set(kStateDisabled, mCalculated.get(kPropertyDisabled).asBoolean());

    // Special handling for user properties - these are copied into an object map without evaluation
    auto user = std::make_shared<ObjectMap>();
    for (const auto& p : mProperties) {
        if (!std::strncmp("-user-", p.first.c_str(), 6))
            user->emplace(p.first.substr(6), p.second);
    }
    mCalculated.set(kPropertyUser, user);

    // Set notion of focusability. Here as we actually may need to know all other properties.
    mCalculated.set(kPropertyFocusable, isFocusable());

    // The component property definition set returns an array of raw accessibility action objects.
    // This code processes the raw array, sets up dependancy relationships, if any, and stores the
    // processed array under the same key.
    ObjectArray actions;
    for (const auto& m : mCalculated.get(kPropertyAccessibilityActionsAssigned).getArray() ) {
        auto aa = AccessibilityAction::create(shared_from_corecomponent(), m);
        if (aa)
            actions.emplace_back(std::move(aa));
    }
    mCalculated.set(kPropertyAccessibilityActionsAssigned, Object(std::move(actions)));

    if (!getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureDynamicAccessibilityActions))
        fixAccessibilityActions();

    // Process tick handlers here. Not same as onMount as it's a bad idea to go through every component on every tick
    // to collect handlers and run them on mass.
    processTickHandlers();

    // Add this component to the list of pending onMount elements if it has the handler.
    auto commands = getCalculated(kPropertyOnMount);
    if (commands.isArray() && !commands.empty()) {
        mContext->pendingOnMounts().emplace(shared_from_corecomponent());
    }
}

void
CoreComponent::release()
{
    traverse([](CoreComponent& c) { c.preRelease(); },
             [](CoreComponent& c) { c.releaseSelf(); });
}

void
CoreComponent::releaseSelf()
{
    // TODO: Must remove this component from any dirty lists
    mContext->layoutManager().remove(shared_from_corecomponent());
    RecalculateTarget::removeUpstreamDependencies();
    mParent = nullptr;
    mChildren.clear();
    clearActiveStateSelf();
}

void
CoreComponent::clearActiveState()
{
    traverse([](CoreComponent& c) {},
             [](CoreComponent& c) { c.clearActiveStateSelf(); });
}

void
CoreComponent::clearActiveStateSelf()
{
    if (mTickHandlerId) {
        mContext->getRootConfig().getTimeManager()->clearTimeout(mTickHandlerId);
        mTickHandlerId = 0;
    }
}

/**
 * Accept a visitor pattern
 * @param visitor The visitor for core components.
 */
void
CoreComponent::accept(Visitor<CoreComponent>& visitor) const
{
    visitor.visit(*this);
    visitor.push();
    for (auto it = mChildren.begin(); !visitor.isAborted() && it != mChildren.end(); it++)
        (*it)->accept(visitor);
    visitor.pop();
}

/**
 * Visitor pattern for walking the component hierarchy in reverse order.  We are interested in the components
 * that the user can see/interact with.  Child classes that have knowledge about which children are off screen or otherwise
 * invalid/unattached should use that knowledge to reduce the number of nodes walked or avoid walking otherwise invalid
 * components they may have stashed in their children.
 * @param visitor The visitor for core components
 */
void
CoreComponent::raccept(Visitor<CoreComponent>& visitor) const
{
    visitor.visit(*this);
    visitor.push();
    for (auto it = mChildren.rbegin(); !visitor.isAborted() && it != mChildren.rend(); it++)
        (*it)->raccept(visitor);
    visitor.pop();
}

template<typename Pre, typename Post>
void
CoreComponent::traverse(const Pre& pre, const Post& post)
{
    pre(*this);
    for (auto& child : mChildren)
        child->traverse(pre, post);
    post(*this);
}

ComponentPtr
CoreComponent::findComponentById(const std::string& id) const
{
    return findComponentById(id, true);
}

ComponentPtr
CoreComponent::findComponentById(const std::string& id, bool traverseHost) const
{
    if (id.empty())
        return nullptr;

    if (mId == id || mUniqueId == id)
        return std::const_pointer_cast<CoreComponent>(shared_from_corecomponent());

    for (auto& m : mChildren) {
        auto result = m->findComponentById(id, traverseHost);
        if (result)
            return result;
    }

    return nullptr;
}

ComponentPtr
CoreComponent::findComponentAtPosition(const Point& position) const
{
    auto visitor = TopAtPosition(position);
    raccept(visitor);
    return visitor.getResult();
}

// Call this when a child has been attached to the parent
void
CoreComponent::attachedToParent(const CoreComponentPtr& parent)
{
    assert(parent != nullptr);
    mParent = parent;

    // Can only be called after the yoga nodes are arranged.
    auto layoutPropDefSet = getLayoutPropDefSet();
    if (layoutPropDefSet)
        assignProperties(*layoutPropDefSet);

    // Update our state.
    if (mInheritParentState) {
        updateMixedStateProperty(kPropertyChecked, mParent->getCalculated(kPropertyChecked).asBoolean());
        updateMixedStateProperty(kPropertyDisabled, mParent->getCalculated(kPropertyDisabled).asBoolean());
        updateInheritedState();
        updateStyle();
    }
}

void
CoreComponent::update(UpdateType type, float value)
{
    switch (type) {
        case kUpdateTakeFocus: {
            auto& fm = getContext()->focusManager();
            if (value)
                fm.setFocus(shared_from_corecomponent(), false);
            else
                fm.releaseFocus(shared_from_corecomponent(), false);
        }
            break;
        case kUpdatePressState:
            setState(kStatePressed, value != 0);
            break;
        default:
            LOG(LogLevel::kWarn).session(mContext) << "Unexpected update command type " << type << " value=" << value;
            break;
    }
}


void
CoreComponent::update(UpdateType type, const std::string& value) {
    if (type == kUpdateAccessibilityAction) {
        auto accessibilityActions = getCalculated(kPropertyAccessibilityActions);

        // Find the first accessibility action in the array that matches the requested name.
        const auto& array = accessibilityActions.getArray();
        auto it = std::find_if(array.begin(), array.end(), [&](const Object& object) {
            return object.get<AccessibilityAction>()->getName() == value;
        });

        if (it != array.end()) {
            const auto& aa = it->get<AccessibilityAction>();
            if (aa->enabled()) {
                const auto& cmds = aa->getCommands();
                if (cmds.isArray() && !cmds.empty())
                    executeEventHandler(value, cmds, false);
                else
                    invokeStandardAccessibilityAction(value);
            }
        }
    }
}

/**
 * Add a child component.  The child already has a reference to the parent.
 * @param child The component to add.
 */
bool
CoreComponent::appendChild(const ComponentPtr& child, bool useDirtyFlag)
{
    return insertChild(CoreComponent::cast(child), mChildren.size(), useDirtyFlag);
}

void
CoreComponent::notifyChildChanged(size_t index, const CoreComponentPtr& component, ChildChangeAction action)
{
    mChildrenChanges.emplace_back(ChildChange{component, action, index});
    // Mark component as dirty so required processing will take place
    mContext->setDirty(shared_from_this());
}

void
CoreComponent::attachYogaNodeIfRequired(const CoreComponentPtr& coreChild, int index)
{
    YGNodeInsertChild(mYGNodeRef, coreChild->getNode(), index);
}

void
CoreComponent::attachYogaNode(const CoreComponentPtr& child)
{
    auto it = std::find(mChildren.begin(), mChildren.end(), child);
    assert(it != mChildren.end());

    YGNodeInsertChild(mYGNodeRef, child->getNode(), std::distance(mChildren.begin(), it));
    child->updateNodeProperties();
}

void
CoreComponent::markDisplayedChildrenStale(bool useDirtyFlag)
{
    // Children visibility can't be stale if component can't have one.
    if (multiChild() || singleChild()) {
        mDisplayedChildrenStale = true;
        if (useDirtyFlag) setDirty(kPropertyNotifyChildrenChanged);
    }
}

size_t
CoreComponent::getDisplayedChildCount() const
{
    auto& mutableThis = const_cast<CoreComponent&>(*this);
    mutableThis.ensureDisplayedChildren();
    return mDisplayedChildren.size();
}

ComponentPtr
CoreComponent::getDisplayedChildAt(size_t drawIndex) const
{
    auto& mutableThis = const_cast<CoreComponent&>(*this);
    mutableThis.ensureDisplayedChildren();
    return mDisplayedChildren.at(drawIndex);
}

bool
CoreComponent::isDisplayedChild(const CoreComponent& child) const
{
    auto& mutableThis = const_cast<CoreComponent&>(*this);
    mutableThis.ensureDisplayedChildren();
    return std::count(mDisplayedChildren.begin(), mDisplayedChildren.end(), child.shared_from_corecomponent());
}

void
CoreComponent::ensureDisplayedChildren()
{

    if (!mDisplayedChildrenStale)
        return;

    // clear previous calculations
    mDisplayedChildren.clear();

    // Identify this components local viewport using bounds and scroll position
    // top left of viewport is the most significant offset found when comparing
    // bounds offset and scroll offset
    const Rect& bounds = getCalculated(kPropertyBounds).get<Rect>();

    // Get viewport offset by scroll Position
    Rect viewportRect = Rect(0,0,bounds.getWidth(),bounds.getHeight());
    viewportRect.offset(scrollPosition());

    std::vector<CoreComponentPtr> sticky;
    // Process the children, identify those displayed within local viewport
    for (auto& child : mChildren) {
        // only visible children
        if (child->isDisplayable()) {
            // compare child rect, transformed as needed, against the viewport
            auto childBounds = child->getCalculated(kPropertyBounds).get<Rect>();
            const auto& transform = child->getCalculated(kPropertyTransform).get<Transform2D>();
            // The axis aligned bounding box is an approximation for checking bounds intersection.
            // The AABB test eliminates children that are guaranteed NOT to intersect. It does not
            // prove the parent and child do intersect.
            // TODO a complete solution applies the "separating axis theorem". The parent AABB is
            // TODO transformed into the child space and tested for intersection. If a separating axis cannot be
            // TODO identified using both tests, the parent and child intersect.

            // Note that the transform is applied assuming the top-left corner of the child is at (0,0)
            Point childBoundsTopLeft = childBounds.getTopLeft();
            childBounds = transform.calculateAxisAlignedBoundingBox(Rect{0, 0, childBounds.getWidth(), childBounds.getHeight()});
            childBounds.offset(childBoundsTopLeft);

            if (!viewportRect.intersect(childBounds).empty()) {
                if (child->getCalculated(kPropertyPosition) == kPositionSticky) {
                    sticky.emplace_back(child);
                } else {
                    mDisplayedChildren.emplace_back(child);
                }
            }
        }
    }

    // Insert the sticky elements at the end
    mDisplayedChildren.insert(mDisplayedChildren.end(), sticky.begin(), sticky.end());

    mDisplayedChildrenStale = false;
}

bool
CoreComponent::insertChild(const ComponentPtr& child, size_t index)
{
    return canInsertChild() && insertChild(CoreComponent::cast(child), index, true);
}

bool
CoreComponent::appendChild(const ComponentPtr& child)
{
    return insertChild(child, mChildren.size());
}

bool
CoreComponent::insertChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag)
{
    if (!child || child->getParent())
        return false;

    auto coreChild = CoreComponent::cast(child);

    if (index > mChildren.size())
        index = mChildren.size();

    attachYogaNodeIfRequired(coreChild, index);

    mChildren.insert(mChildren.begin() + index, coreChild);

    coreChild->attachedToParent(shared_from_corecomponent());
    if (useDirtyFlag) {
        notifyChildChanged(index, child, kChildChangeActionInsert);
        // If we add a view hierarchy with dirty flags, we need to update the context
        coreChild->markAdded();
    }

    coreChild->markGlobalToLocalTransformStale();
    markDisplayedChildrenStale(useDirtyFlag);
    setVisualContextDirty();

    // Update the position: sticky components tree
    auto p = stickyfunctions::getAncestorHorizontalAndVerticalScrollable(coreChild);
    auto horizontalScrollable   = std::get<0>(p);
    auto verticalScrollable     = std::get<1>(p);
    if (horizontalScrollable && horizontalScrollable->getStickyTree())
        horizontalScrollable->getStickyTree()->handleChildInsert(coreChild);
    if (verticalScrollable && verticalScrollable->getStickyTree())
        verticalScrollable->getStickyTree()->handleChildInsert(coreChild);

    return true;
}

bool
CoreComponent::remove()
{
    return remove(true);
}

bool
CoreComponent::remove(bool useDirtyFlag)
{
    if (!mParent || !mParent->canRemoveChild())
        return false;

    // When we've been removed, we need to clear Yoga properties that were set based on our parent type.
    // If we don't clear these, certain properties like "alignSelf" that apply only in Containers will mess
    // up the layout when we switch to a Sequence.
    auto propDefSet = mParent->layoutPropDefSet();
    if (propDefSet) {
        for (const auto& pd : *propDefSet) {
            if ((pd.second.flags & kPropResetOnRemove) != 0 && pd.second.layoutFunc) {
                auto value = pd.second.defaultFunc ? pd.second.defaultFunc(*this, mContext->getRootConfig()) : pd.second.defvalue;
                pd.second.layoutFunc(mYGNodeRef, value, *mContext);
            }
        }
    }

    mParent->removeChild(shared_from_corecomponent(), useDirtyFlag);
    mParent = nullptr;
    return true;
}

void
CoreComponent::removeChildAfterMarkedRemoved(const CoreComponentPtr& child, size_t index, bool useDirtyFlag)
{
    YGNodeRemoveChild(mYGNodeRef, child->getNode());
    mChildren.erase(mChildren.begin() + index);

    // The parent component has changed the number of children
    if (useDirtyFlag)
        notifyChildChanged(index, child, kChildChangeActionRemove);

    markDisplayedChildrenStale(useDirtyFlag);
    mDisplayedChildren.clear();

    if (useDirtyFlag) setVisualContextDirty();

    // Update the position: sticky components tree
    auto p = stickyfunctions::getHorizontalAndVerticalScrollable(shared_from_corecomponent());
    auto horizontalScrollable   = std::get<0>(p);
    auto verticalScrollable     = std::get<1>(p);
    if (horizontalScrollable && horizontalScrollable->getStickyTree())
        horizontalScrollable->getStickyTree()->handleChildRemove();
    if (verticalScrollable && verticalScrollable->getStickyTree())
        verticalScrollable->getStickyTree()->handleChildRemove();
}

void
CoreComponent::removeChild(const CoreComponentPtr& child, bool useDirtyFlag)
{
    auto it = std::find(mChildren.begin(), mChildren.end(), child);
    assert(it != mChildren.end());
    size_t index = std::distance(mChildren.begin(), it);
    child->traverse([](CoreComponent& c) { c.markSelfRemoved(); });
    removeChildAfterMarkedRemoved(child, index, useDirtyFlag);
}

void
CoreComponent::removeChildAt(size_t index, bool useDirtyFlag)
{
    if (index >= mChildren.size())
        return;
    auto child = mChildren.at(index);

    child->traverse(
        [](CoreComponent& c) { c.markSelfRemoved(); },
        [](CoreComponent& c) { c.releaseSelf(); });
    removeChildAfterMarkedRemoved(child, index, useDirtyFlag);
}

/**
 * This component has been removed from the DOM.  Make sure that it is not focused and not dirty.
 */
void
CoreComponent::markSelfRemoved()
{
    auto self = shared_from_corecomponent();

    mContext->clearDirty(self);
    auto& fm = mContext->focusManager();
    if (fm.getFocus() == self) {
        auto next = fm.find(kFocusDirectionForward);
        if (next)
            fm.setFocus(next, true);
        else // If nothing suitable is found - clear focus forcefully as we don't have another choice.
            fm.clearFocus(true, kFocusDirectionNone, true);
    }
}

/**
 * This component has been attached to the DOM.  Walk the hierarchy and make sure that
 * any dirty components are reflected in the RootContext
 */
void
CoreComponent::markAdded()
{
    auto self = shared_from_this();
    if (!mDirty.empty())
        mContext->setDirty(self);

    for (auto& child : mChildren)
        CoreComponent::cast(child)->markAdded();
}

void
CoreComponent::ensureLayoutInternal(bool useDirtyFlag)
{
    LOG_IF(DEBUG_ENSURE).session(mContext) << toDebugSimpleString() << " useDirtyFlag=" << useDirtyFlag;
    APL_TRACE_BLOCK("CoreComponent:ensureLayout");
    auto& lm = mContext->layoutManager();
    if (lm.ensure(shared_from_corecomponent()))
        lm.layout(useDirtyFlag);
}

void
CoreComponent::ensureChildLayout(const CoreComponentPtr& child, bool useDirtyFlag)
{
    child->ensureLayoutInternal(useDirtyFlag);
}

void
CoreComponent::reportLoaded(size_t index)
{
    if (mRebuilder) {
        if (mRebuilder->hasFirstItem() && index == 0) {
            index++;
        }

        if (mRebuilder->hasLastItem() && index == mChildren.size() - 1) {
            index--;
        }

        mRebuilder->notifyItemOnScreen(index);
    }
}

bool
CoreComponent::isAttached() const
{
    return mYGNodeRef->getOwner() != nullptr;
}

bool
CoreComponent::isLaidOut() const
{
    auto component = shared_from_corecomponent();
    while (true) {
        const auto& node = component->getNode();
        if (YGNodeIsDirty(node))
            return false;

        if (mContext->layoutManager().isTopNode(component))
            return true;

        auto parent = CoreComponent::cast(component->getParent());
        if (!parent)
            return true;

        if (node->getOwner() == nullptr)
            return false;

        component = parent;
    }
}

void
CoreComponent::updateNodeProperties()
{
    const auto pds = getLayoutPropDefSet();
    if (pds) {
        for (const auto& it : pds->needsNode()) {
            const ComponentPropDef& pd = it.second;
            pd.layoutFunc(mYGNodeRef, mCalculated.get(pd.key), *mContext);
        }
    }
}

#ifdef SCENEGRAPH
sg::LayerPtr
CoreComponent::getSceneGraph(sg::SceneGraphUpdates& sceneGraph)
{
    if (!mSceneGraphLayer) {
        mSceneGraphLayer = constructSceneGraphLayer(sceneGraph);

        // Attach children
        ensureDisplayedChildren();
        for (const auto& m : mDisplayedChildren)
            mSceneGraphLayer->appendChild(m->getSceneGraph(sceneGraph));

        mSceneGraphLayer->clearFlags();
    }

    return mSceneGraphLayer;
}

void
CoreComponent::updateSceneGraph(sg::SceneGraphUpdates& sceneGraph)
{
    if (!mSceneGraphLayer)
        return;

    auto* layer = mSceneGraphLayer.get();

    // Basic interaction property changes
    if (isDirty(kPropertyDisabled))
        layer->updateInteraction(sg::Layer::kInteractionDisabled,
                                 getCalculated(kPropertyDisabled).asBoolean());

    if (isDirty(kPropertyChecked))
        layer->updateInteraction(sg::Layer::kInteractionChecked,
                                 getCalculated(kPropertyChecked).asBoolean());

    // Check to see if any core layer properties changes
    bool needsRebuild = isDirty(kPropertyDisplay) || isDirty(kPropertyOpacity) ||
                        isDirty(kPropertyTransform) || isDirty(kPropertyBounds);

    if (needsRebuild) {
        layer->setBounds(getCalculated(kPropertyBounds).get<Rect>());
        layer->setOpacity(static_cast<float>(getCalculated(kPropertyOpacity).asNumber()));
        layer->setTransform(getCalculated(kPropertyTransform).get<Transform2D>());
    }

    // Check if the shadow changed
    auto shadowChanged = isDirty(kPropertyShadowColor) ||
                         isDirty(kPropertyShadowHorizontalOffset) ||
                         isDirty(kPropertyShadowVerticalOffset) || isDirty(kPropertyShadowRadius);

    if (shadowChanged)
        layer->setShadow(sg::shadow(getCalculated(kPropertyShadowColor).getColor(),
                                    Point{getCalculated(kPropertyShadowHorizontalOffset).asFloat(),
                                          getCalculated(kPropertyShadowVerticalOffset).asFloat()},
                                    getCalculated(kPropertyShadowRadius).asFloat()));

    // Check for an accessibility change
    auto accessibilityChanged = isDirty(kPropertyAccessibilityLabel) ||
                                isDirty(kPropertyAccessibilityActions) || isDirty(kPropertyRole);

    if (accessibilityChanged)
        layer->setAccessibility(sg::accessibility(*this));

    // Check to see if the children of this component changed; update as necessary
    // Note: This flag is not, in fact, reliable because the children may not have
    // changed.
    if (isDirty(kPropertyNotifyChildrenChanged)) {
        ensureDisplayedChildren();

        std::vector<sg::LayerPtr> display;
        for (const auto& m : mDisplayedChildren)
            display.emplace_back(m->getSceneGraph(sceneGraph));

        if (display != layer->children()) {
            layer->removeAllChildren();
            for (const auto& m : display)
                layer->appendChild(m);
        }
    }

    // Fix up any internal drawing
    if (updateSceneGraphInternal(sceneGraph))
        layer->setFlag(sg::Layer::kFlagRedrawContent);

    // Transfer any layer changes to the change map
    sceneGraph.changed(mSceneGraphLayer);

    // Clear all of the dirty flags on the component
    clearDirty();
}

sg::LayerPtr
CoreComponent::constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph)
{
    auto layer = sg::layer(mUniqueId, getCalculated(kPropertyBounds).get<Rect>(),
                           static_cast<float>(getCalculated(kPropertyOpacity).asNumber()),
                           getCalculated(kPropertyTransform).get<Transform2D>());
    sceneGraph.created(layer);

    if (getCalculated(kPropertyDisabled).truthy())
        layer->setInteraction(sg::Layer::kInteractionDisabled);
    if (getCalculated(kPropertyChecked).truthy())
        layer->setInteraction(sg::Layer::kInteractionChecked);

    layer->setAccessibility(sg::accessibility(*this));
    layer->setShadow(sg::shadow(getCalculated(kPropertyShadowColor).getColor(),
                                Point{getCalculated(kPropertyShadowHorizontalOffset).asFloat(),
                                      getCalculated(kPropertyShadowVerticalOffset).asFloat()},
                                getCalculated(kPropertyShadowRadius).asFloat()));

    // TODO: Fix visibility?
    return layer;
}
#endif // SCENEGRAPH

bool
CoreComponent::isParentOf(const CoreComponentPtr& child)
{
    auto self = shared_from_this();

    ComponentPtr parent = child->getParent();
    while (parent && parent != self) {
        parent = parent->getParent();
    }

    return self == parent;
}

/**
 * Initial assignment of properties.  Don't set any dirty flags here; this
 * all should be running in the constructor.
 * @param propDefSet The current property definition set to use.
 */
void
CoreComponent::assignProperties(const ComponentPropDefSet& propDefSet)
{
    auto stylePtr = getStyle();

    for (const auto& cpd : propDefSet) {
        const auto& pd = cpd.second;
        auto value = pd.defaultFunc ? pd.defaultFunc(*this, mContext->getRootConfig()) : pd.defvalue;

        if ((pd.flags & kPropIn) != 0) {
            // Check for user-defined property
            auto p = mProperties.find(pd.names);
            if (p != mProperties.end()) {
                if (p->second.isString() || (pd.flags & kPropEvaluated) != 0) {
                    auto result = parseAndEvaluate(*mContext, p->second);
                    if (!result.symbols.empty())
                        ComponentDependant::create(shared_from_corecomponent(),
                                                   pd.key,
                                                   result.expression,
                                                   mContext,
                                                   pd.getBindingFunction(),
                                                   std::move(result.symbols));
                    value = pd.calculate(*mContext, result.value);
                }
                else {
                    value = pd.calculate(*mContext, p->second);
                }
                mAssigned.emplace(pd.key);
            }
            else {
                // Make sure this wasn't a required property
                if ((pd.flags & kPropRequired) != 0) {
                    mFlags |= kComponentFlagInvalid;
                    CONSOLE(mContext) << "Missing required property: " << pd.names;
                }

                // Check for a styled property
                if ((pd.flags & kPropStyled) != 0 && stylePtr) {
                    auto s = stylePtr->find(pd.names);
                    if (s != stylePtr->end())
                        value = pd.calculate(*mContext, s->second);
                }
            }
        }

        mCalculated.set(pd.key, value);

        if (pd.key == kPropertyPosition && value == kPositionSticky) {
            zeroComponentInsets(shared_from_corecomponent());
            auto p = stickyfunctions::getAncestorHorizontalAndVerticalScrollable(shared_from_corecomponent());
            auto horizontalScrollable   = std::get<0>(p);
            auto verticalScrollable     = std::get<1>(p);
            if (horizontalScrollable && horizontalScrollable->getStickyTree())
                horizontalScrollable->getStickyTree()->handleChildStickySet();
            if (verticalScrollable && verticalScrollable->getStickyTree())
                verticalScrollable->getStickyTree()->handleChildStickySet();
        }

        //Apply this property to the yn if we care about it
        if (pd.layoutFunc != nullptr)
            pd.layoutFunc(mYGNodeRef, value, *mContext);
    }

    // Yoga padding values are calculated from a combination of two properties (e.g. "padding" and "paddingLeft").
    // Since they can't be set directly during normal property assignment with a layout function, we update them here.
    fixPadding();
}

void
CoreComponent::handlePropertyChange(const ComponentPropDef& def, const Object& value) {
    // If the mixed state inherits from the parent, we block the change.
    if ((def.flags & kPropMixedState) != 0 && mInheritParentState)
        return;

    auto previous = getCalculated(def.key);
    if (value != previous) {
        mCalculated.set(def.key, value);

        // Update the position: sticky components tree if needed
        if (def.key == kPropertyPosition && value == kPositionSticky) {
            zeroComponentInsets(shared_from_corecomponent());
            auto p = stickyfunctions::getAncestorHorizontalAndVerticalScrollable(shared_from_corecomponent());
            auto horizontalScrollable   = std::get<0>(p);
            auto verticalScrollable     = std::get<1>(p);
            if (horizontalScrollable && horizontalScrollable->getStickyTree())
                horizontalScrollable->getStickyTree()->handleChildStickySet();
            if (verticalScrollable && verticalScrollable->getStickyTree())
                verticalScrollable->getStickyTree()->handleChildStickySet();
        } else if (def.key == kPropertyPosition && previous == kPositionSticky) {
            restoreComponentInsets(shared_from_corecomponent());
            auto p = stickyfunctions::getAncestorHorizontalAndVerticalScrollable(shared_from_corecomponent());
            auto horizontalScrollable   = std::get<0>(p);
            auto verticalScrollable     = std::get<1>(p);
            if (horizontalScrollable && horizontalScrollable->getStickyTree())
                horizontalScrollable->getStickyTree()->handleChildStickyUnset();
            if (verticalScrollable && verticalScrollable->getStickyTree())
                verticalScrollable->getStickyTree()->handleChildStickyUnset();
        }

        // display change, or opacity change to/from 0, makes parent display stale
        if (mParent
            && (def.key == kPropertyDisplay
             ||(def.key == kPropertyOpacity && ((value.asNumber() == 0) != (previous.asNumber() == 0))))) {

                mParent->markDisplayedChildrenStale(true);
        }

        // Properties with the kPropVisualContext flag the visual context as dirty
        if ((def.flags & kPropVisualContext) != 0)
            setVisualContextDirty();

        // Properties with the kPropTextHash flag the text measurement hash as dirty
        if ((def.flags & kPropTextHash) != 0)
            mTextMeasurementHashStale = true;

        // Properties with kPropVisualHash flag the visual hash is dirty
        if ((def.flags & kPropVisualHash) != 0)
            mVisualHashStale = true;

        if ((def.flags & kPropAccessibility) != 0)
            markAccessibilityDirty();

        // Properties with the kPropOut flag mark the property as dirty
        if ((def.flags & kPropOut) != 0)
            setDirty(def.key);

        // Properties with a layout function will update the Yoga node
        if (def.layoutFunc != nullptr)
            def.layoutFunc(mYGNodeRef, value, *mContext);

        // Properties with a trigger function need to recompute other properties
        if (def.trigger != nullptr)
            def.trigger(*this);

        // If this property affects the layout, we'll need a new layout pass
        if ((def.flags & kPropLayout) != 0 && YGNodeHasMeasureFunc(mYGNodeRef))
            YGNodeMarkDirty(mYGNodeRef);

        // If this property affects the state, we'll do a SetState change
        if ((def.flags & kPropMixedState) != 0) {
            // Propagate the property change to the children
            for (auto& child : mChildren)
                child->updateMixedStateProperty(def.key, value.asBoolean());

            switch (def.key) {
                case kPropertyChecked:
                    setState(kStateChecked, value.asBoolean());
                    break;
                case kPropertyDisabled:
                    setState(kStateDisabled, value.asBoolean());
                    break;
                default:
                    break;
            }
        }
    }
}

CoreComponentPtr
CoreComponent::getLayoutRoot() {
    auto c = shared_from_corecomponent();
    while (c->mParent && c->getNode()->getOwner())
        c = c->mParent;
    return c;
}

/**
 * Walk down the hierarchy setting this mixed state/property to the new value.
 * Update dirty flags as necessary.
 * @param key The property to change
 * @param value The value to set
 */
void
CoreComponent::updateMixedStateProperty(apl::PropertyKey key, bool value)
{
    if (!mInheritParentState || value == mCalculated.get(key).asBoolean())
        return;

    mCalculated.set(key, value);
    setDirty(key);

    for (auto& child : mChildren)
        child->updateMixedStateProperty(key, value);
}


/**
 * Find a component property in the current component.  This could be a built-in component
 * property or it could be a layout property.
 * @param key The property to retrieve.
 * @return A pair of true if the property was found and the iterator pointing to the property definition.
 */
std::pair<bool, ConstComponentPropIterator>
CoreComponent::find(PropertyKey key) const
{
    // Check the main set of properties first
    const auto& pds = propDefSet();
    auto it = pds.find(key);
    if (it != pds.end())
        return {true, it};

    // Check layout properties second
    if (mParent) {
        const auto& layoutPDS = getLayoutPropDefSet();
        if (layoutPDS) {
            it = layoutPDS->find(key);
            if (it != layoutPDS->end())
                return {true, it};
        }
    }

    return {false, it};
}

/**
 * A property has been set on the component.
 * @param it The property iterator.  Contains the key of the property being changed and the definition.
 * @param value The new value of the property.
 * @return True if the property could be set (it must be dynamic).
 */
bool
CoreComponent::setPropertyInternal(ConstComponentPropIterator it, const Object& value)
{
    // Verify that this property is a valid dynamic property
    if ((it->second.flags & kPropDynamic) == 0)
        return false;

    // Some properties can only be set correctly if the component has been laid out
    if ((it->second.flags & kPropSetAfterLayout) != 0 && !isLaidOut()) {
        mContext->layoutManager().addPostProcess(shared_from_corecomponent(), it->first, value);
        return true;
    }

    // Properties with setters are called directly
    if (it->second.setterFunc) {
        it->second.setterFunc(*this, value);
        return true;
    }

    // If this property was previously assigned we need to clear any dependants
    auto assigned = mAssigned.find(it->first);
    if (assigned != mAssigned.end()) // Erase all upstream dependants that drive this key
        removeUpstream(it->first);

    // Mark this property in the "assigned" set of properties.
    mAssigned.emplace(it->first);

    // Check to see if the actual value of the property changed and update appropriately
    const ComponentPropDef& def = it->second;
    handlePropertyChange(def, def.calculate(*mContext, value));

    return true;
}

std::pair<Object, bool>
CoreComponent::getPropertyInternal(ConstComponentPropIterator it) const
{
    auto dynamic = (it->second.flags & kPropDynamic) != 0;
    if (it->second.getterFunc)
        return { it->second.getterFunc(*this), dynamic };

    auto v = mCalculated.get(it->first);
    // If it is a map we need to convert it back into a string
    if (it->second.map)
        return { it->second.map->get(v.asInt(), "<ERROR>"), dynamic };
    return { v, dynamic };
}

bool
CoreComponent::setProperty(PropertyKey key, const Object& value)
{
    auto findRef = find(key);
    if (findRef.first && setPropertyInternal(findRef.second, value))
        return true;

    CONSOLE(mContext) << "Invalid property key '" << sComponentPropertyBimap.at(key) << "' for this component";
    return false;
}

void
CoreComponent::setProperty( const std::string& key, const Object& value )
{
    if (sComponentPropertyBimap.has(key) &&
        setProperty(static_cast<PropertyKey>(sComponentPropertyBimap.at(key)), value))
        return;

    // If we got here, there was no matching property.  Assume it is a binding
    if (mContext->userUpdateAndRecalculate(key, value, true))
        return;

    // Certain component may provide their own special processing
    if (setPropertyInternal(key, value))
        return;

    CONSOLE(mContext) << "Unknown property name " << key;
}

std::pair<Object, bool>
CoreComponent::getPropertyAndWriteableState(const std::string& key) const
{
    // Check for a standard component property
    if (sComponentPropertyBimap.has(key)) {
        auto findRef = find(static_cast<PropertyKey>(sComponentPropertyBimap.at(key)));
        if (findRef.first)
            return getPropertyInternal(findRef.second);
    }

    // Check for a data binding
    auto ref = mContext->find(key);
    if (!ref.empty())
        return { ref.object().value(), ref.object().isUserWriteable() };

    // It may be an internal component property
    auto internal = getPropertyInternal(key);
    if (internal.second)
        return internal;

    CONSOLE(mContext) << "Unknown property name " << key;
    return { Object::NULL_OBJECT(), false };
}

Object
CoreComponent::getProperty(PropertyKey key) const
{
    if (sComponentPropertyBimap.has(key)) {
        auto findRef = find(key);
        if (findRef.first)
            return getPropertyInternal(findRef.second).first;
    }

    return Object::NULL_OBJECT();
}

bool
CoreComponent::markPropertyInternal(const ComponentPropDefSet& pds, PropertyKey key)
{
    // Verify that we have a valid dynamic property
    auto it = pds.dynamic().find(key);
    if (it == pds.dynamic().end())
        return false;

    const ComponentPropDef& def = it->second;

    // Properties with the kPropOut flag mark the property as dirty
    if ((def.flags & kPropOut) != 0)
        setDirty(def.key);

    // Properties with a layout function will update the Yoga node
    if (def.layoutFunc != nullptr)
        def.layoutFunc(mYGNodeRef, mCalculated.get(key), *mContext);

    // Properties with a trigger function need to recompute other properties
    if (def.trigger != nullptr)
        def.trigger(*this);

    // If this property affects the layout, we'll need a new layout pass
    if ((def.flags & kPropLayout) != 0)
        YGNodeMarkDirty(mYGNodeRef);

    return true;
}

void
CoreComponent::markProperty(PropertyKey key)
{
    if (!markPropertyInternal(propDefSet(), key) && mParent) {
        auto layoutPDS = getLayoutPropDefSet();
        if (layoutPDS)
            markPropertyInternal(*layoutPDS, key);
    }
}

void
CoreComponent::setValue(PropertyKey key, const Object& value, bool useDirtyFlag)
{
    auto it = mAssigned.find(key);
    if (it == mAssigned.end())
        return;

    // Check the standard properties first
    auto& corePDS = propDefSet().dynamic();
    auto pd = corePDS.find(key);
    if (pd != corePDS.end()) {
        handlePropertyChange(pd->second, value);
        return;
    }

    // Look for a layout property.  Not all components have a layout
    auto layoutPDS = getLayoutPropDefSet();
    if (!layoutPDS)
        return;

    // Check for a matching layout property
    auto& layoutDynamicPDS = layoutPDS->dynamic();
    pd = layoutDynamicPDS.find(key);
    if (pd != layoutDynamicPDS.end()) {
        handlePropertyChange(pd->second, value);
        return;
    }

    // We should not reach this point.  Only an assigned equation calls setValue
    CONSOLE(mContext) << "Property " << sComponentPropertyBimap.at(key)
                          << " is not dynamic and can't be updated.";
}

/**
 * The style of the component may (or may not) have changed.  If it has changed,
 * update each styled property in turn.  Then update any children that share their
 * parent state.
 *
 * Calling this method sets dirty flags.
 */
void
CoreComponent::updateStyleInternal(const StyleInstancePtr& stylePtr, const ComponentPropDefSet& pds) {

    // Check every property that has the "styled" flag.
    for (const auto& it : pds.styled()) {
        const ComponentPropDef& pd = it.second;

        // If the property was explicitly assigned by the user, the style won't change it.
        if (mAssigned.count(pd.key))
            continue;

        // Check to see if the value has changed.
        auto value = (pd.defaultFunc ? pd.defaultFunc(*this, mContext->getRootConfig()) : pd.defvalue);
        auto s = stylePtr->find(pd.names);
        if (s != stylePtr->end())
            value = pd.calculate(*mContext, s->second);

        handlePropertyChange(pd, value);
    }
}

/**
 * The style of the component may (or may not) have changed.  If it has changed,
 * update each styled property in turn.  Then update any children that share their
 * parent state.
 *
 * Calling this method sets dirty flags.
 *
 * Note that changing the style may force a new layout pass.
 */
void
CoreComponent::updateStyle()
{
    auto stylePtr = getStyle();
    if (stylePtr) {
        updateStyleInternal(stylePtr, propDefSet());
        const ComponentPropDefSet *layoutPDS = getLayoutPropDefSet();
        if (layoutPDS)
            updateStyleInternal(stylePtr, *layoutPDS);
    }
    for (const auto& child : mChildren) {
        if (child->mInheritParentState)
            child->updateStyle();
    }
}

/**
 * Update state of the component.  This may trigger style changes locally and in children
 * Do NOT call this method directly to set the CHECKED or DISABLED states.  Those calls should
 * go through setProperty.
 * @param stateProperty The enumerated state property
 * @param value The value to set.
 */
void
CoreComponent::setState( StateProperty stateProperty, bool value )
{
    if (mInheritParentState) {
        CONSOLE(mContext) << "Cannot assign state properties to a child that inherits parent state";
        return;
    }

    if (mState.set(stateProperty, value)) {
        auto self = shared_from_corecomponent();
        if (stateProperty == kStateChecked || stateProperty == kStateFocused
            || stateProperty == kStateDisabled) {
            setVisualContextDirty();
        }

        if (stateProperty == kStateDisabled) {
            if (value) {
                mState.set(kStatePressed, false);
                mState.set(kStateHover, false);
                auto& fm = mContext->focusManager();
                if (fm.getFocus() == self) {
                    auto next = fm.find(kFocusDirectionForward);
                    if (next)
                        fm.setFocus(next, true);
                    else // If nothing suitable is found - clear focus forcefully as we don't have another choice.
                        fm.clearFocus(true, kFocusDirectionNone, true);
                }
            }
        }

        for (auto& child: mChildren)
            child->updateInheritedState();

        if (stateProperty == kStateDisabled) {
            // notify the hover manager that the disable state has changed
            mContext->hoverManager().componentToggledDisabled(self);
        }

        updateStyle();
    }
}

void
CoreComponent::setDirty( PropertyKey key )
{
    if (mDirty.emplace(key).second) {
        mContext->setDirty(shared_from_this());

        if (!isVisualContextDirty() || !mTextMeasurementHashStale || !mVisualHashStale) {
            auto def = propDefSet().find(key);
            if (def == propDefSet().end()) return;

            // set the visual context dirty if this property causes change
            // called here because we handlePropertyChange may be bypassed in some circumstances
            // (for example scrolling)
            if (!isVisualContextDirty() && (def->second.flags & kPropVisualContext)) {
                setVisualContextDirty();
            }

            // Set text measurement hash as stale
            if (!mTextMeasurementHashStale && (def->second.flags & kPropTextHash)) {
                mTextMeasurementHashStale = true;
            }

            // Set visual hash as stale
            if (!mVisualHashStale && (def->second.flags & kPropVisualHash)) {
                mVisualHashStale = true;
            }

            if (def->second.flags & kPropAccessibility) {
                markAccessibilityDirty();
            }
        }
    }
}

const std::set<PropertyKey>&
CoreComponent::getDirty()
{
    // Do visual hash update if needed.
    fixVisualHash(true);

    return mDirty;
}

void
CoreComponent::clearDirty()
{
    Component::clearDirty();

    // Update hash without causing dirty flag, otherwise they may be marked dirty when they are not supposed to.
    // Only case when it may happen - initial inflation (immediate onMount for example).
    fixVisualHash(false);
}

CoreComponentPtr
CoreComponent::getParentIfInDocument() const
{
    return mParent && mParent->getType() != kComponentTypeHost
        ? mParent
        : nullptr;
}

void
CoreComponent::updateInheritedState()
{
    if (!mInheritParentState || !mParent)
        return;

    mState = mParent->getState();
    for (auto& child : mChildren)
        child->updateInheritedState();
}

bool
CoreComponent::inheritsStateFrom(const CoreComponentPtr& component) {

    auto stateOwner = shared_from_corecomponent();

    // the component is this component
    if (component == stateOwner) {
        return true;
    }

    // compare to ancestor components
    while (stateOwner && stateOwner->getInheritParentState()) {
        stateOwner = CoreComponent::cast(stateOwner->getParent());
        if (stateOwner == component) {
            return true;
        }
    }

    return false;
}

CoreComponentPtr
CoreComponent::findStateOwner() {
    auto ptr = shared_from_corecomponent();
    while (ptr && ptr->getInheritParentState()) {
        ptr = CoreComponent::cast(ptr->getParent());
    }
    return ptr;
}

StyleInstancePtr
CoreComponent::getStyle() const
{
    return mContext->getStyle(mStyle, mState);
}

const ComponentPropDefSet *
CoreComponent::getLayoutPropDefSet() const
{
    if (mParent)
        return mParent->layoutPropDefSet();

    return nullptr;
}

bool
CoreComponent::needsLayout() const
{
    return YGNodeIsDirty(mYGNodeRef);
}

bool
CoreComponent::shouldPropagateLayoutChanges() const
{
    return !mChildren.empty() && static_cast<Display>(getCalculated(kPropertyDisplay).getInteger()) != kDisplayNone;
}

std::string
CoreComponent::textMeasurementHash() const
{
    return mTextMeasurementHash;
}

void
CoreComponent::fixTextMeasurementHash()
{
    auto& pds = propDefSet();
    if (!mTextMeasurementHashStale) return;
    mTextMeasurementHashStale = false;

    size_t hash = 0;
    for (const auto& cpd : pds) {
        const auto &pd = cpd.second;
        if ((pd.flags & kPropTextHash) != 0) {
            hashCombine(hash, getCalculated(pd.key));
        }
    }

    // Need to keep as string, as double or int will lead to loss of precision.
    mTextMeasurementHash = std::to_string(hash);
}

void
CoreComponent::fixVisualHash(bool useDirtyFlag)
{
    if (!mVisualHashStale) return;
    mVisualHashStale = false;

    size_t hash = 0;
    for (const auto& cpd : propDefSet()) {
        const auto &pd = cpd.second;
        if ((pd.flags & kPropVisualHash) != 0) {
            hashCombine(hash, getCalculated(pd.key));
        }
    }

    // Need to keep as string, as double or int will lead to loss of precision.
    mCalculated.set(kPropertyVisualHash, std::to_string(hash));
    if (useDirtyFlag) setDirty(kPropertyVisualHash);
}

void
CoreComponent::preLayoutProcessing(bool useDirtyFlag)
{
    for (auto& child : mChildren)
        if (child->isAttached())
            child->preLayoutProcessing(useDirtyFlag);

    // Prior to layout, the visual hash may already be stale due to, for
    // example, pending commands and/or dynamic data changes. But view host
    // expects to be able to refer to a correct visual hash during layout, so
    // let's make it correct now. It's OK that the visual hash may change again
    // during layout, in which case the view host will use the new value.
    fixVisualHash(useDirtyFlag);
}

void
CoreComponent::processLayoutChanges(bool useDirtyFlag, bool first)
{
    if (DEBUG_BOUNDS) YGNodePrint(mYGNodeRef, YGPrintOptions::YGPrintOptionsLayout);
    APL_TRACE_BLOCK("CoreComponent:processLayoutChanges");

    float left = YGNodeLayoutGetLeft(mYGNodeRef);
    float top = YGNodeLayoutGetTop(mYGNodeRef);
    float width = YGNodeLayoutGetWidth(mYGNodeRef);
    float height = YGNodeLayoutGetHeight(mYGNodeRef);

    bool changed = false;
    // If no bounds set - set some now to get sticky stuff a chance to calculate on the very first pass
    if (first && mCalculated.get(kPropertyBounds).empty()) {
        changed = true;
        mCalculated.set(kPropertyBounds, Rect(left, top, width, height));
    }

    if (getCalculated(kPropertyPosition) == kPositionSticky) {
        mStickyOffset = stickyfunctions::calculateStickyOffset(shared_from_corecomponent());
        left += mStickyOffset.getX();
        top  += mStickyOffset.getY();
    }

    // Update the transformation matrix
    fixTransform(useDirtyFlag);
    // Update the layoutDirection.
    fixLayoutDirection(useDirtyFlag);

    Rect rect(left, top, width, height);
    changed |= rect != mCalculated.get(kPropertyBounds).get<Rect>();

    if (changed) {
        mCalculated.set(kPropertyBounds, std::move(rect));
        markGlobalToLocalTransformStale();
        markDisplayedChildrenStale(useDirtyFlag);
        if (mParent)
            mParent->markDisplayedChildrenStale(useDirtyFlag);
        setVisualContextDirty();
        if (useDirtyFlag)
            setDirty(kPropertyBounds);
    }

    // Update the inner drawing area (this takes into account both padding and borders
    float borderLeft = YGNodeLayoutGetBorder(mYGNodeRef, YGEdgeLeft);
    float borderTop = YGNodeLayoutGetBorder(mYGNodeRef, YGEdgeTop);
    float borderRight = YGNodeLayoutGetBorder(mYGNodeRef, YGEdgeRight);
    float borderBottom = YGNodeLayoutGetBorder(mYGNodeRef, YGEdgeBottom);

    float paddingLeft = YGNodeLayoutGetPadding(mYGNodeRef, YGEdgeLeft);
    float paddingTop = YGNodeLayoutGetPadding(mYGNodeRef, YGEdgeTop);
    float paddingRight = YGNodeLayoutGetPadding(mYGNodeRef, YGEdgeRight);
    float paddingBottom = YGNodeLayoutGetPadding(mYGNodeRef, YGEdgeBottom);

    Rect inner(borderLeft + paddingLeft,
                     borderTop + paddingTop,
                     width - (borderLeft + paddingLeft + borderRight + paddingRight),
                     height - (borderTop + paddingTop + borderBottom + paddingBottom));
    changed = inner != mCalculated.get(kPropertyInnerBounds).get<Rect>();

    if (changed) {
        mCalculated.set(kPropertyInnerBounds, std::move(inner));
        markDisplayedChildrenStale(useDirtyFlag);
        if (useDirtyFlag)
            setDirty(kPropertyInnerBounds);
    }

    if (shouldPropagateLayoutChanges()) {
        // Inform all children that they should re-check their bounds. No need to do that for not
        // attached ones. Note that children of a Pager are not attached, and hence they will not
        // be processed.
        for (auto& child : mChildren)
            if (child->isAttached())
                child->processLayoutChanges(useDirtyFlag, first);
    }

    if (!mCalculated.get(kPropertyLaidOut).asBoolean() && !mCalculated.get(kPropertyBounds).get<Rect>().empty()) {
        mCalculated.set(kPropertyLaidOut, true);
        if (useDirtyFlag)
            setDirty(kPropertyLaidOut);
        markAccessibilityDirty();
    }
}

/**
 * Note: It may be relatively expensive to run this operation, as it's executed for every dirty
 * component on clearPending. Any operation performed has to break out early if no change is
 * required.
 */
void
CoreComponent::postClearPending()
{
    // Process and report DOM (not layout) changes.
    processChildrenChanges();
    refreshAccessibilityActions(true);
}

std::string
CoreComponent::toStringAction(ChildChangeAction action) {
    return action == kChildChangeActionInsert ? "insert" : "remove";
}

void
CoreComponent::processChildrenChanges() {
    if (mChildrenChanges.empty())
        return;

    // Report children changes to the runtime
    {
        auto& changes = mCalculated.get(kPropertyNotifyChildrenChanged).getMutableArray();
        for (const auto& c : mChildrenChanges) {
            auto change = std::make_shared<ObjectMap>();
            change->emplace(CHILDREN_CHANGE_INDEX, c.index);
            change->emplace(CHILDREN_CHANGE_UID, c.component->getUniqueId());
            change->emplace(CHILDREN_CHANGE_ACTION, toStringAction(c.action));
            changes.emplace_back(change);
        }

        setDirty(kPropertyNotifyChildrenChanged);
    }

    // Execute ChildrenChanged handler, if defined
    auto commands = mCalculated.get(kPropertyOnChildrenChanged);
    if (multiChild() && !commands.empty()) {
        auto handlerChanges = std::make_shared<ObjectArray>();
        for (const auto& c : mChildrenChanges) {
            auto change = std::make_shared<ObjectMap>();
            if (c.action == kChildChangeActionInsert)
                change->emplace(
                    CHILDREN_CHANGE_INDEX,
                    getChildIndex(c.component)
                );
            change->emplace(CHILDREN_CHANGE_UID, c.component->getUniqueId());
            change->emplace(CHILDREN_CHANGE_ACTION, toStringAction(c.action));
            handlerChanges->emplace_back(change);
        }

        auto changesMap = std::make_shared<ObjectMap>();
        changesMap->emplace(CHILDREN_CHANGE_CHANGES, handlerChanges);
        mContext->sequencer().executeCommands(
            commands,
            createEventContext("ChildrenChanged", changesMap, getValue()),
            shared_from_corecomponent(), true);

    }
    mChildrenChanges.clear();
}

void
CoreComponent::postProcessLayoutChanges()
{
    // Mark this component as having been laid out at least once
    mFlags |= kComponentFlagAllowEventHandlers;

    for (auto& child : mChildren)
        if (child->isAttached())
            child->postProcessLayoutChanges();

    // update the displayed children
    ensureDisplayedChildren();
}


std::shared_ptr<ObjectMap>
CoreComponent::createEventProperties(const std::string& handler, const Object& value) const
{
    auto event = std::make_shared<ObjectMap>();
    event->emplace("source", Object(ComponentEventSourceWrapper::create(shared_from_corecomponent(), handler, value)));
    addEventProperties(*event);
    return event;
}


ContextPtr
CoreComponent::createEventContext(const std::string& handler, const ObjectMapPtr& optional, const Object& value) const
{
    ContextPtr ctx = Context::createFromParent(mContext);
    auto compValue = getValue();
    if (!value.isNull()) {
        compValue = value;
    }

    auto event = createEventProperties(handler, compValue);
    if (optional)
        event->insert(optional->begin(), optional->end());
    ctx->putConstant("event", event);
    return ctx;
}

ContextPtr
CoreComponent::createKeyEventContext(const std::string& handler, const ObjectMapPtr& keyboard) const
{
    ContextPtr ctx = Context::createFromParent(mContext);
    auto event = createEventProperties(handler, getValue());
    event->emplace("keyboard", keyboard);
    ctx->putConstant("event", event);
    return ctx;
}

const EventPropertyMap &
CoreComponent::eventPropertyMap() const {
    static EventPropertyMap sCoreEventProperties(
        {
            {"bind",            [](const CoreComponent *c) { return ContextWrapper::create(c->getContext()); }},
            {"checked",         [](const CoreComponent *c) { return c->getState().get(kStateChecked); }},
            {"disabled",        [](const CoreComponent *c) { return c->getState().get(kStateDisabled); }},
            {"focused",         [](const CoreComponent *c) { return c->getState().get(kStateFocused); }},
            {"id",              [](const CoreComponent *c) { return c->getId(); }},
            {"uid",             [](const CoreComponent *c) { return c->getUniqueId(); }},
            {"width",           [](const CoreComponent *c) { return YGNodeLayoutGetWidth(c->mYGNodeRef); }},
            {"height",          [](const CoreComponent *c) { return YGNodeLayoutGetHeight(c->mYGNodeRef); }},
            {"opacity",         [](const CoreComponent *c) { return c->getCalculated(kPropertyOpacity); }},
            {"pressed",         [](const CoreComponent *c) { return c->getState().get(kStatePressed); }},
            {"type",            [](const CoreComponent *c) { return sComponentTypeBimap.at(c->getType()); }},
            {"layoutDirection", [](const CoreComponent *c) { return sLayoutDirectionMap.at(c->getCalculated(kPropertyLayoutDirection).asInt()); }}
        });

    return sCoreEventProperties;
}

std::pair<bool, Object>
CoreComponent::getEventProperty(const std::string &key) const
{
    const auto& map = eventPropertyMap();
    auto it = map.find(key);
    if (it != map.end())
        return {true, it->second(this)};

    return {false, Object::NULL_OBJECT()};
}

size_t
CoreComponent::getEventPropertySize() const
{
    return eventPropertyMap().size();
}

void
CoreComponent::serializeEvent(rapidjson::Value& out, rapidjson::Document::AllocatorType& allocator) const
{
    const auto& map = eventPropertyMap();
    for (const auto& m : map)
        out.AddMember(rapidjson::Value(m.first.c_str(), allocator), m.second(this).serialize(allocator).Move(), allocator);
}

static const char sHierarchySig[] = "CEXFMHIPSQTWGV";  // Must match ComponentType

std::string
CoreComponent::getHierarchySignature() const
{
    std::string result(1, sHierarchySig[getType()]);
    if (!mChildren.empty()) {
        result += "[";
        for (const auto& child : mChildren)
            result += child->getHierarchySignature();
        result += "]";
    }
    return result;
}

void
CoreComponent::fixTransform(bool useDirtyFlag)
{
    LOG_IF(DEBUG_TRANSFORM).session(mContext) << mCalculated.get(kPropertyTransform).get<Transform2D>();

    Transform2D updated;

    auto transform = mCalculated.get(kPropertyTransformAssigned);

    if (transform.isArray()) {
        transform = Transformation::create(*getContext(), transform.getArray());
        mCalculated.set(kPropertyTransformAssigned, transform);
    }

    if (transform.is<Transformation>()) {
        float width = YGNodeLayoutGetWidth(mYGNodeRef);
        float height = YGNodeLayoutGetHeight(mYGNodeRef);
        updated = transform.get<Transformation>()->get(width, height);
    }

    auto value = getCalculated(kPropertyTransform).get<Transform2D>();
    if (updated != value) {
        mCalculated.set(kPropertyTransform, Object(std::move(updated)));
        markGlobalToLocalTransformStale();
        // transform change make parent display stale
        if (mParent) {
            mParent->markDisplayedChildrenStale(useDirtyFlag);
        }
        setVisualContextDirty();
        if (useDirtyFlag)
            setDirty(kPropertyTransform);

        LOG_IF(DEBUG_TRANSFORM).session(mContext) << "updated to " << mCalculated.get(kPropertyTransform).get<Transform2D>();
    }
}

static bool
setPaddingIfKeyFound(PropertyKey key, YGEdge edge, CalculatedPropertyMap& map, YGNodeRef nodeRef,
                     const ContextPtr& context) {
    auto v = map.find(key);
    bool found = v != map.end() && !v->second.isNull();
    if (found)
        yn::setPadding(nodeRef, edge, v->second, *context);
    return found;
}

void
CoreComponent::fixPadding()
{
    LOG_IF(DEBUG_PADDING).session(mContext) << mCalculated.get(kPropertyPadding);

    static std::vector<std::pair<PropertyKey, YGEdge>> EDGES = {
        {kPropertyPaddingLeft,   YGEdgeLeft},
        {kPropertyPaddingTop,    YGEdgeTop},
        {kPropertyPaddingRight,  YGEdgeRight},
        {kPropertyPaddingBottom, YGEdgeBottom},
    };

    auto commonPadding = mCalculated.get(kPropertyPadding);

    for (size_t i = 0 ; i < EDGES.size() ; i++) {
        const auto& edge = EDGES.at(i);
        // That value may be overridden by the specific paddingLeft/Top/Right/Bottom values
        if (!setPaddingIfKeyFound(edge.first, edge.second, mCalculated, mYGNodeRef, mContext)) {
            // If edge isn't set directly use padding value assigned by the "padding" property
            auto assigned = commonPadding.size() > i ? commonPadding.at(i) : Dimension(0);
            yn::setPadding(mYGNodeRef, edge.second, assigned, *mContext);
        }
    }

    // paddingStart overrides left padding if layout is "ltf" or right padding if "rtl"
    setPaddingIfKeyFound(kPropertyPaddingStart, YGEdgeStart, mCalculated, mYGNodeRef, mContext);

    // paddingEnd overrides left padding if layout is "ltf" or right padding if "rtl"
    setPaddingIfKeyFound(kPropertyPaddingEnd, YGEdgeEnd, mCalculated, mYGNodeRef, mContext);
}

void
CoreComponent::fixLayoutDirection(bool useDirtyFlag)
{
    LOG_IF(DEBUG_LAYOUTDIRECTION).session(mContext) << mCalculated.get(kPropertyLayoutDirection);
    auto reportedLayoutDirection = static_cast<LayoutDirection>(mCalculated.get(kPropertyLayoutDirection).asInt());
    auto currentLayoutDirection = getLayoutDirection() == YGDirectionLTR ? kLayoutDirectionLTR : kLayoutDirectionRTL;
    if (reportedLayoutDirection != currentLayoutDirection) {
        mCalculated.set(kPropertyLayoutDirection, currentLayoutDirection);
        if (useDirtyFlag) {
            setDirty(kPropertyLayoutDirection);
        }
        handleLayoutDirectionChange(useDirtyFlag);
        LOG_IF(DEBUG_LAYOUTDIRECTION).session(mContext) << "updated to " << mCalculated.get(kPropertyLayoutDirection);
    }
}

void CoreComponent::setHeight(const Dimension& height) {
    if (mYGNodeRef)
        yn::setHeight(mYGNodeRef, height, *mContext);
    else
        LOG(LogLevel::kError).session(mContext) << "setHeight:  Missing yoga node for component id '" << getId() << "'";
    mCalculated.set(kPropertyHeight, height);
}

void CoreComponent::setWidth(const Dimension& width) {
    if (mYGNodeRef)
        yn::setWidth(mYGNodeRef, width, *mContext);
    else
        LOG(LogLevel::kError).session(mContext) << "setWidth:  Missing yoga node for component id '" << getId() << "'";
    mCalculated.set(kPropertyWidth, width);
}

std::map<int,int>
CoreComponent::calculateChildrenVisualLayer(const std::map<int, float>& visibleIndexes,
                                            const Rect& visibleRect, int visualLayer) {
    std::map<int,int> result;
    // For general case child has it's parent's layer.
    for(const auto &vi : visibleIndexes) {
        result.emplace(vi.first, visualLayer);
    }

    return result;
}

rapidjson::Value
CoreComponent::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value component(rapidjson::kObjectType);

    component.AddMember("id", rapidjson::Value(mUniqueId.c_str(), allocator).Move(), allocator);
    component.AddMember("type", getType(), allocator);

    for (const auto& pds : propDefSet()) {
        if ((pds.second.flags & kPropOut) != 0 || (pds.second.flags & kPropRuntimeState) != 0)
            component.AddMember(
                rapidjson::StringRef(pds.second.names[0].c_str()),   // We assume long-lived strings here
                mCalculated.get(pds.first).serialize(allocator),
                allocator);
    }

    if (!mChildren.empty()) {
        rapidjson::Value children(rapidjson::kArrayType);
        for (const auto& child : mChildren)
            children.PushBack(child->serialize(allocator), allocator);
        component.AddMember("children", children, allocator);
    }

    return component;
}

rapidjson::Value
CoreComponent::serializeAll(rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value component(rapidjson::kObjectType);

    component.AddMember("id", rapidjson::Value(mUniqueId.c_str(), allocator).Move(), allocator);
    component.AddMember("type", rapidjson::StringRef(sComponentTypeBimap.at(getType()).c_str()), allocator);

    component.AddMember("__id", rapidjson::Value(mId.c_str(), allocator), allocator);
    component.AddMember("__inheritParentState", mInheritParentState, allocator);
    component.AddMember("__style", rapidjson::Value(mStyle.c_str(), allocator), allocator);
    component.AddMember("__path", rapidjson::Value(mPath.toString().c_str(), allocator), allocator);

    for (const auto& pds : propDefSet()) {
        if (pds.second.map) {
            component.AddMember(
                rapidjson::StringRef(pds.second.names[0].c_str()),   // We assume long-lived strings here
                rapidjson::StringRef(pds.second.map->at(mCalculated.get(pds.first).asInt()).c_str()),
                allocator);
        } else {
            component.AddMember(
                rapidjson::StringRef(pds.second.names[0].c_str()),   // We assume long-lived strings here
                mCalculated.get(pds.first).serialize(allocator),
                allocator);
        }
    }

    if (!mChildren.empty()) {
        rapidjson::Value children(rapidjson::kArrayType);
        for (const auto& child : mChildren)
            children.PushBack(child->serializeAll(allocator), allocator);
        component.AddMember("children", children, allocator);
    }

    return component;
}

rapidjson::Value
CoreComponent::serializeDirty(rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value component(rapidjson::kObjectType);

    component.AddMember("id", rapidjson::Value(mUniqueId.c_str(), allocator).Move(), allocator);
    for (auto& key : mDirty) {
        component.AddMember(
            rapidjson::Value(sComponentPropertyBimap.at(key).c_str(), allocator),
            mCalculated.get(key).serializeDirty(allocator),
            allocator);
    }
    mDirty.clear();
    return component;
}

rapidjson::Value
CoreComponent::serializeVisualContext(rapidjson::Document::AllocatorType& allocator) {
    float viewportWidth = mContext->width();
    float viewportHeight = mContext->height();
    Rect viewportRect(0, 0, viewportWidth, viewportHeight);
    auto topComponentVisibleRect = calculateVisibleRect(viewportRect);
    auto topComponentOpacity = calculateRealOpacity();
    auto topComponentVisibility = calculateVisibility(1.0, viewportRect);

    rapidjson::Value children(rapidjson::kArrayType);
    serializeVisualContextInternal(children, allocator, topComponentOpacity, topComponentVisibility,
            topComponentVisibleRect, 0);

    // We always have viewport component
    return children[0].GetObject();
}

void
CoreComponent::serializeVisualContextInternal(
    rapidjson::Value &outArray, rapidjson::Document::AllocatorType& allocator, float realOpacity,
    float visibility, const Rect& visibleRect, int visualLayer)
{
    auto parentInDocument = getParentIfInDocument();
    if(visibility == 0.0 && parentInDocument) {
        // Not visible and not viewport component.
        return;
    }

    // Decide if actionable
    bool actionable = !getCalculated(kPropertyEntities).empty();
    rapidjson::Value tags(rapidjson::kObjectType);
    actionable |= getTags(tags, allocator);

    bool isTopLevelElement = !mParent || !mParent->includeChildrenInVisualContext();
    if (isTopLevelElement) {
        tags.AddMember("viewport", rapidjson::Value(rapidjson::kObjectType), allocator);
        actionable |= true;
    }

    bool includeInContext = !parentInDocument || (visibility > 0.0 && actionable);

    rapidjson::Value children(rapidjson::kArrayType);

    // Process children
    if (!mChildren.empty() && includeChildrenInVisualContext() && visibility > 0.0) {
        auto visibleIndexes = getChildrenVisibility(realOpacity, visibleRect);
        auto visualLayers = calculateChildrenVisualLayer(visibleIndexes, visibleRect, visualLayer);
        for (auto childIdx : visibleIndexes) {
            const auto& child = mChildren.at(childIdx.first);
            auto childVisibleRect = child->calculateVisibleRect(visibleRect);
            auto childRealOpacity = child->calculateRealOpacity(realOpacity);
            auto childVisibility = childIdx.second;
            auto childVisualLayer = visualLayers.at(childIdx.first);
            child->serializeVisualContextInternal(
                    includeInContext ? children : outArray, allocator,
                    childRealOpacity, childVisibility, childVisibleRect, childVisualLayer);
        }
    }

    // we already should have included visible children on this point so break out if parent is not "actionable".
    if(!includeInContext) {
        return;
    }

    rapidjson::Value visualContext(rapidjson::kObjectType);

    if(!children.Empty()) {
        visualContext.AddMember("children", children.Move(), allocator);
    }

    // Get entities (if any)
    if(!getCalculated(kPropertyEntities).empty()) {
        visualContext.AddMember("entities", getCalculated(kPropertyEntities).serialize(allocator).Move(), allocator);
    }

    // Transform
    auto transform = getCalculated(kPropertyTransform).get<Transform2D>();
    if(!transform.isIdentity()) {
        visualContext.AddMember("transform", transform.serialize(allocator).Move(), allocator);
    }

    // Now calculate other related tags as component to be included.
    if(tags.MemberCount() > 0) {
        visualContext.AddMember("tags", tags, allocator);
    }
    if(!mId.empty()) {
        visualContext.AddMember("id", rapidjson::Value(mId.c_str(), allocator).Move(), allocator);
    }
    visualContext.AddMember("uid", rapidjson::Value(mUniqueId.c_str(), allocator).Move(), allocator);
    visualContext.AddMember("position", rapidjson::Value((getGlobalBounds().toString() + ":"
                            + std::to_string(visualLayer)).c_str(), allocator).Move(), allocator);
    visualContext.AddMember("type", rapidjson::Value(getVisualContextType().c_str(), allocator), allocator);

    if(visibility > 0.0 && visibility < 1.0) {
        visualContext.AddMember("visibility", visibility, allocator);
    }

    outArray.PushBack(visualContext.Move(), allocator);
}

std::string
CoreComponent::getVisualContextType() const
{
    std::string type;
    for(const auto& child : mChildren) {
        auto childType = child->getVisualContextType();
        if(type.empty()) {
            type = childType;
        }

        if(type != childType) {
            return VISUAL_CONTEXT_TYPE_MIXED;
        }
    }

    return !type.empty() ? type : VISUAL_CONTEXT_TYPE_EMPTY;
}

void
CoreComponent::setVisualContextDirty() {
    // set this component as dirty visual context
    mContext->setDirtyVisualContext(shared_from_this());
}

bool
CoreComponent::isVisualContextDirty() {
    return mContext->isVisualContextDirty(shared_from_this());
}

std::map<int, float>
CoreComponent::getChildrenVisibility(float realOpacity, const Rect &visibleRect) const {
    std::map<int, float> visibleIndexes;

    for(int index = 0; index < mChildren.size(); index++) {
        const auto& child = getCoreChildAt(index);
        float visibility = child->calculateVisibility(realOpacity, visibleRect);
        if(visibility > 0.0) {
            visibleIndexes.emplace(index, visibility);
        }
    }

    return visibleIndexes;
}

float
CoreComponent::calculateVisibility(float parentRealOpacity, const Rect& parentVisibleRect) const
{
    auto realOpacity = calculateRealOpacity(parentRealOpacity);
    if(realOpacity <= 0 || (getCalculated(kPropertyDisplay).asInt() != kDisplayNormal)) {
        return 0.0;
    }

    auto boundingRect = getGlobalBounds();
    if(boundingRect.area() <= 0) {
        return 0.0;
    }

    auto visibleRect = calculateVisibleRect(parentVisibleRect);
    auto visibility = visibleRect.area()/boundingRect.area() * realOpacity;

    // May be positive only, do simple math instead of including math.h
    return ((int)(visibility * 100 + .5f) / 100.0f);
}

bool
CoreComponent::getTags(rapidjson::Value &outMap, rapidjson::Document::AllocatorType &allocator)
{
    bool actionable = false;
    bool checked = mState.get(kStateChecked);
    bool disabled = mState.get(kStateDisabled);
    if(checked && !mInheritParentState) {
        outMap.AddMember("checked", checked, allocator);
    }

    if(disabled) {
        outMap.AddMember("disabled", disabled, allocator);
    }

    if(isFocusable()) {
        bool focused = mState.get(kStateFocused) && !disabled;
        outMap.AddMember("focused", focused, allocator);
    }

    if(mParent && mParent->scrollable() && mParent->multiChild()) {
        rapidjson::Value listItem(rapidjson::kObjectType);
        listItem.AddMember("index", mContext->opt("index").asInt(), allocator);
        outMap.AddMember("listItem", listItem, allocator);
    }

    if(mParent && mParent->getCalculated(kPropertyNumbered).truthy() && mContext->has("ordinal")) {
        outMap.AddMember("ordinal", mContext->opt("ordinal").asInt(), allocator);
    }

    if(!getCalculated(kPropertySpeech).empty()) {
        outMap.AddMember("spoken", true, allocator);
        actionable |= true;
    }

    return actionable;
}

Rect
CoreComponent::calculateVisibleRect(const Rect& parentVisibleRect) const
{
    return getGlobalBounds().intersect(parentVisibleRect);
}

Rect
CoreComponent::calculateVisibleRect() const
{
    auto rect = getGlobalBounds();
    if(!mParent) {
        float viewportWidth = mContext->width();
        float viewportHeight = mContext->height();
        return rect.intersect({0, 0, viewportWidth, viewportHeight});
    }

    return rect.intersect(mParent->calculateVisibleRect());
}

float
CoreComponent::calculateRealOpacity(float parentRealOpacity) const
{
    auto assignedOpacity = getCalculated(kPropertyOpacity).asNumber();
    return assignedOpacity * parentRealOpacity;
}

float
CoreComponent::calculateRealOpacity() const
{
    auto assignedOpacity = getCalculated(kPropertyOpacity).asNumber();
    if(!mParent) {
        return assignedOpacity;
    }

    return assignedOpacity * mParent->calculateRealOpacity();
}

bool
CoreComponent::isDisplayable() const {
    return (getCalculated(kPropertyDisplay).asInt() == kDisplayNormal)
    && (getCalculated(kPropertyOpacity).asNumber() > 0)
    && !mIsDisallowed;
}

void
CoreComponent::executeOnCursorEnter() {
    auto eventContext = createDefaultEventContext("CursorEnter");
    auto command = getCalculated(kPropertyOnCursorEnter);
    mContext->sequencer().executeCommands(command, eventContext, shared_from_corecomponent(), true);
}

void
CoreComponent::executeOnCursorExit() {
    auto eventContext = createDefaultEventContext("CursorExit");
    auto command = getCalculated(kPropertyOnCursorExit);
    mContext->sequencer().executeCommands(command, eventContext, shared_from_corecomponent(), true);
}

bool
CoreComponent::processKeyPress(KeyHandlerType type, const Keyboard& keyboard) {
    auto consumed = executeKeyHandlers(type, keyboard);
    // If no handler executed - go for Core specific (intrinsic) processing.
    if (!consumed) {
        consumed = executeIntrinsicKeyHandlers(type, keyboard);
    }

    return consumed;
}

/*****************************************************************/

static inline void
inlineFixTransform(Component& component)
{
    auto& core = (CoreComponent&)component;
    core.fixTransform(true);
}

static inline void
inlineFixPadding(Component& component)
{
    auto& core = (CoreComponent&)component;
    core.fixPadding();
}

static inline Object
defaultWidth(Component& component, const RootConfig& rootConfig)
{
    return rootConfig.getDefaultComponentWidth(component.getType());
}

static inline Object
defaultHeight(Component& component, const RootConfig& rootConfig)
{
    return rootConfig.getDefaultComponentHeight(component.getType());
}

void
CoreComponent::fixSpacing(bool reset) {
    auto spacing = getCalculated(kPropertySpacing).asDimension(*mContext);
    if (reset) spacing = 0;
    if (spacing.isAbsolute()) {
        auto parentNode = YGNodeGetParent(mYGNodeRef);
        auto parentComponent = getParentIfInDocument();
        if (!parentNode || !parentComponent)
            return;

        auto parentLayoutDirection = parentComponent->getCalculated(kPropertyLayoutDirection);
        auto dir = YGNodeStyleGetFlexDirection(parentNode);
        YGEdge edge = YGEdgeLeft;
        switch (dir) {
            case YGFlexDirectionColumn:        edge = YGEdgeTop;    break;
            case YGFlexDirectionColumnReverse: edge = YGEdgeBottom; break;
            case YGFlexDirectionRow:
                edge = (parentLayoutDirection == kLayoutDirectionLTR) ? YGEdgeLeft : YGEdgeRight;
                break;
            case YGFlexDirectionRowReverse:
                edge = (parentLayoutDirection == kLayoutDirectionLTR) ? YGEdgeRight : YGEdgeLeft;
                break;
        }

        float currentValue = YGNodeStyleGetMargin(mYGNodeRef, edge).value;
        if ((std::isnan(currentValue) && spacing.getValue() != 0) || std::abs(currentValue - spacing.getValue()) > 0.1) {
            YGNodeStyleSetMargin(mYGNodeRef, YGEdgeTop,    0);
            YGNodeStyleSetMargin(mYGNodeRef, YGEdgeBottom, 0);
            YGNodeStyleSetMargin(mYGNodeRef, YGEdgeLeft,   0);
            YGNodeStyleSetMargin(mYGNodeRef, YGEdgeRight,  0);
            YGNodeStyleSetMargin(mYGNodeRef, edge, spacing.getValue());
        }
    }
}

/**
 * Used by components with a displayed border.
 */
void CoreComponent::resolveDrawnBorder(Component& component) {
    auto& comp = static_cast<CoreComponent&>(component);
    comp.calculateDrawnBorder(true);
}

/**
 * Used by components with a displayed border.
 * Derive the width of the drawn border.  If borderStrokeWith is set, the drawn border is the min of borderWidth
 * and borderStrokeWidth.  If borderStrokeWidth is unset, the drawn border defaults to borderWidth.
 * @param useDirtyFlag true if the drawn border changed as a result of other property changes
 */
void
CoreComponent::calculateDrawnBorder(bool useDirtyFlag )
{
    auto strokeWidth = getCalculated(kPropertyBorderStrokeWidth);
    auto borderWidth = getCalculated(kPropertyBorderWidth);

    auto drawnBorderWidth = borderWidth;
    if (!strokeWidth.isNull())
        drawnBorderWidth = Object(Dimension(
            std::min(strokeWidth.getAbsoluteDimension(), borderWidth.getAbsoluteDimension())));

    if (drawnBorderWidth != getCalculated(kPropertyDrawnBorderWidth)) {
        mCalculated.set(kPropertyDrawnBorderWidth, drawnBorderWidth);
        if (useDirtyFlag)
            setDirty(kPropertyDrawnBorderWidth);
    }
}

void
CoreComponent::executeEventHandler(const std::string& event, const Object& commands, bool fastMode,
                                   const ObjectMapPtr& optional) {
    if (!mState.get(kStateDisabled)) {
        if (!fastMode)
            mContext->sequencer().reset();

        ContextPtr eventContext = createEventContext(event, optional);
        mContext->sequencer().executeCommands(
            commands,
            eventContext,
            shared_from_corecomponent(),
            fastMode);
    }
}

YGSize
CoreComponent::textMeasureInternal(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode)
{
    APL_TRACE_BLOCK("CoreComponent:textMeasureInternal");
    // Recalculate visual hash if marked as stale
    fixVisualHash(true);

    auto componentHash = textMeasurementHash();
    LOG_IF(DEBUG_MEASUREMENT).session(mContext)
        << "Measuring: " << getUniqueId()
        << " hash: " << componentHash
        << " width: " << width
        << " widthMode: " << widthMode
        << " height: " << height
        << " heightMode: " << heightMode;

    TextMeasureRequest tmr = {width, widthMode, height, heightMode, componentHash};
    auto& measuresCache = getContext()->cachedMeasures();
    if (measuresCache.has(tmr)) {
        return measuresCache.get(tmr);
    }

    APL_TRACE_BEGIN("CoreComponent:textMeasureInternal:runtimeMeasure");
    LayoutSize layoutSize = getContext()->measure()->measure(
            this, width, toMeasureMode(widthMode), height, toMeasureMode(heightMode));
    auto size = YGSize({layoutSize.width, layoutSize.height});
    measuresCache.put(tmr, size);
    LOG_IF(DEBUG_MEASUREMENT).session(mContext) << "Size: " << size.width << "x" << size.height;
    APL_TRACE_END("CoreComponent:textMeasureInternal:runtimeMeasure");
    return size;
}

float
CoreComponent::textBaselineInternal(float width, float height)
{
    APL_TRACE_BEGIN("CoreComponent:textBaselineInternal");
    TextMeasureRequest tmr = {
            width,
            YGMeasureMode::YGMeasureModeUndefined,
            height,
            YGMeasureMode::YGMeasureModeUndefined,
            textMeasurementHash()
    };
    auto& baselineCache = getContext()->cachedBaselines();
    if (baselineCache.has(tmr)) {
        return baselineCache.get(tmr);
    }

    APL_TRACE_BEGIN("CoreComponent:textBaselineInternal:runtimeMeasure");
    auto size = getContext()->measure()->baseline(this, width, height);
    baselineCache.put(tmr, size);
    APL_TRACE_END("CoreComponent:textBaselineInternal:runtimeMeasure");
    return size;
}

YGSize
CoreComponent::textMeasureFunc( YGNodeRef node,
                 float width,
                 YGMeasureMode widthMode,
                 float height,
                 YGMeasureMode heightMode )
{
    auto *component = static_cast<CoreComponent*>(node->getContext());
    assert(component);
    return component->textMeasureInternal(width, widthMode, height, heightMode);
}

float
CoreComponent::textBaselineFunc( YGNodeRef node, float width, float height )
{
    auto *component = static_cast<CoreComponent*>(node->getContext());
    assert(component);
    return component->textBaselineInternal(width, height);
}

// Old style static actions. Just copy defined and implicit to the output.
void
CoreComponent::fixAccessibilityActions() {
    auto current = getCalculated(kPropertyAccessibilityActionsAssigned).getArray();

    ObjectArray result;
    // Copy all defined
    for (const auto& ao : current) {
        result.emplace_back(ao);
    }

    std::map<std::string, bool> supportedActions;
    getSupportedStandardAccessibilityActions(supportedActions);

    // Copy all implicit
    for (const auto& a : supportedActions) {
        if (a.second)
            result.emplace_back(AccessibilityAction::create(shared_from_corecomponent(),
                                                            a.first, a.first));
    }

    setCalculated(kPropertyAccessibilityActions, std::move(result));
}

void
CoreComponent::markAccessibilityDirty()
{
    if (!getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureDynamicAccessibilityActions))
        return;

    mContext->setDirty(shared_from_this());
    mAccessibilityDirty = true;
}

// New style dynamic actions
void
CoreComponent::refreshAccessibilityActions(bool useDirtyFlag)
{
    if (!getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureDynamicAccessibilityActions))
        return;

    if (!mAccessibilityDirty) return;
    mAccessibilityDirty = false;

    auto current = getCalculated(kPropertyAccessibilityActions).getArray();
    if (getCalculated(kPropertyDisabled).asBoolean()) {
        if (!current.empty()) {
            mCalculated.set(kPropertyAccessibilityActions, Object::EMPTY_ARRAY());
            if (useDirtyFlag) setDirty(kPropertyAccessibilityActions);
        }
        return;
    }

    auto cmp = [](const AccessibilityActionPtr& left, const AccessibilityActionPtr& right) {
        return left->getName() < right->getName();
    };

    // Dedupe the list by the Action name, only first definition considered valid
    std::set<AccessibilityActionPtr, decltype(cmp)> deduped(cmp);

    // Add defined actions
    for (const auto& m : mCalculated.get(kPropertyAccessibilityActionsAssigned).getArray() ) {
        deduped.emplace(m.get<AccessibilityAction>());
    }

    // Get component supported standard actions (may be based on component state)
    std::map<std::string, bool> supportedActions;
    getSupportedStandardAccessibilityActions(supportedActions);

    // Add supported actions if implicit (does not have to be defined)
    for (const auto& m : supportedActions) {
        if (m.second)
            deduped.emplace(AccessibilityAction::create(shared_from_corecomponent(), m.first, m.first));
    }

    // There are 3 types of valid actions:
    // 1. Enabled custom
    // 2. Enabled standard action (component or gesture)
    // 3. Implicit actions (like scrolling), unless disabled
    ObjectArray result;
    for (const auto& aa : deduped) {
        if (aa->enabled() && (!aa->getCommands().empty() || supportedActions.count(aa->getName())))
            result.emplace_back(aa);
    }

    if (current != result) {
        mCalculated.set(kPropertyAccessibilityActions, Object(std::move(result)));
        if (useDirtyFlag) setDirty(kPropertyAccessibilityActions);
    }
}

const ComponentPropDefSet&
CoreComponent::propDefSet() const {
    static ComponentPropDefSet sCommonComponentProperties = ComponentPropDefSet().add({
      {kPropertyAccessibilityLabel,           "",                      asString,            kPropInOut |
                                                                                            kPropDynamic},
      {kPropertyAccessibilityActions,         Object::EMPTY_ARRAY(),   asArray,             kPropOut | kPropAccessibility},
      {kPropertyAccessibilityActionsAssigned, Object::EMPTY_ARRAY(),   asArray,             kPropIn},
      {kPropertyBounds,                       Rect(0,0,0,0),           nullptr,             kPropOut |
                                                                                            kPropVisualContext |
                                                                                            kPropVisualHash},
      {kPropertyChecked,                      false,                   asBoolean,           kPropInOut |
                                                                                            kPropDynamic |
                                                                                            kPropMixedState |
                                                                                            kPropVisualContext},
      {kPropertyDescription,                  "",                      asString,            kPropIn},
      {kPropertyDisplay,                      kDisplayNormal,          sDisplayMap,         kPropInOut |
                                                                                            kPropStyled |
                                                                                            kPropDynamic |
                                                                                            kPropVisualContext,  yn::setDisplay},
      {kPropertyDisabled,                     false,                   asBoolean,           kPropInOut |
                                                                                            kPropDynamic |
                                                                                            kPropMixedState |
                                                                                            kPropVisualContext |
                                                                                            kPropAccessibility},
      {kPropertyEntities,                     Object::EMPTY_ARRAY(),   asDeepArray,         kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropEvaluated |
                                                                                            kPropVisualContext},
      {kPropertyFocusable,                    false,                   nullptr,             kPropOut},
      {kPropertyHandleTick,                   Object::EMPTY_ARRAY(),   asArray,             kPropIn},
      {kPropertyHeight,                       Dimension(),             asDimension,         kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         yn::setHeight, defaultHeight},
      {kPropertyInnerBounds,                  Rect(0,0,0,0),           nullptr,             kPropOut |
                                                                                            kPropVisualContext |
                                                                                            kPropVisualHash},
      {kPropertyLayoutDirectionAssigned,      kLayoutDirectionInherit, sLayoutDirectionMap, kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         yn::setLayoutDirection},
      {kPropertyLayoutDirection,              kLayoutDirectionLTR,     sLayoutDirectionMap, kPropOut |
                                                                                            kPropTextHash |
                                                                                            kPropVisualHash},
      {kPropertyMaxHeight,                    Object::NULL_OBJECT(),   asNonAutoDimension,  kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         yn::setMaxHeight},
      {kPropertyMaxWidth,                     Object::NULL_OBJECT(),   asNonAutoDimension,  kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         yn::setMaxWidth},
      {kPropertyMinHeight,                    Dimension(0),            asNonAutoDimension,  kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         yn::setMinHeight},
      {kPropertyMinWidth,                     Dimension(0),            asNonAutoDimension,  kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         yn::setMinWidth},
      {kPropertyOnChildrenChanged,            Object::EMPTY_ARRAY(),   asCommand,           kPropIn},
      {kPropertyOnMount,                      Object::EMPTY_ARRAY(),   asCommand,           kPropIn},
      {kPropertyOnSpeechMark,                 Object::EMPTY_ARRAY(),   asCommand,           kPropIn},
      {kPropertyOpacity,                      1.0,                     asOpacity,           kPropInOut |
                                                                                            kPropStyled |
                                                                                            kPropDynamic |
                                                                                            kPropVisualContext |
                                                                                            kPropVisualHash},
      {kPropertyPadding,                      Object::EMPTY_ARRAY(),   asPaddingArray,      kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         inlineFixPadding},
      {kPropertyPaddingBottom,                Object::NULL_OBJECT(),   asAbsoluteDimension, kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         inlineFixPadding},
      {kPropertyPaddingLeft,                  Object::NULL_OBJECT(),   asAbsoluteDimension, kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         inlineFixPadding},
      {kPropertyPaddingRight,                 Object::NULL_OBJECT(),   asAbsoluteDimension, kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         inlineFixPadding},
      {kPropertyPaddingTop,                   Object::NULL_OBJECT(),   asAbsoluteDimension, kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         inlineFixPadding},
      {kPropertyPaddingStart,                 Object::NULL_OBJECT(),   asAbsoluteDimension, kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         inlineFixPadding},
      {kPropertyPaddingEnd,                   Object::NULL_OBJECT(),   asAbsoluteDimension, kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         inlineFixPadding},
      {kPropertyPreserve,                     Object::EMPTY_ARRAY(),   asArray,             kPropIn},
      {kPropertyRole,                         kRoleNone,               sRoleMap,            kPropInOut |
                                                                                            kPropStyled},
      {kPropertyShadowColor,                  Color(),                 asColor,             kPropInOut |
                                                                                            kPropDynamic |
                                                                                            kPropStyled |
                                                                                            kPropVisualHash},
      {kPropertyShadowHorizontalOffset,       Dimension(0),            asAbsoluteDimension, kPropInOut |
                                                                                            kPropDynamic |
                                                                                            kPropStyled |
                                                                                            kPropVisualHash},
      {kPropertyShadowRadius,                 Dimension(0),            asAbsoluteDimension, kPropInOut |
                                                                                            kPropDynamic |
                                                                                            kPropStyled |
                                                                                            kPropVisualHash},
      {kPropertyShadowVerticalOffset,         Dimension(0),            asAbsoluteDimension, kPropInOut |
                                                                                            kPropDynamic |
                                                                                            kPropStyled |
                                                                                            kPropVisualHash},
      {kPropertySpeech,                       "",                      asAny,               kPropIn |
                                                                                            kPropVisualContext},
      {kPropertyTransformAssigned,            Object::NULL_OBJECT(),   asTransformOrArray,  kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropEvaluated |
                                                                                            kPropVisualContext,  inlineFixTransform},
      {kPropertyTransform,                    Transform2D(),           nullptr,             kPropOut |
                                                                                            kPropVisualContext},
      {kPropertyUser,                         Object::NULL_OBJECT(),   nullptr,             kPropOut},
      {kPropertyWidth,                        Dimension(),             asDimension,         kPropIn |
                                                                                            kPropDynamic |
                                                                                            kPropStyled,         yn::setWidth,  defaultWidth},
      {kPropertyOnCursorEnter,                Object::EMPTY_ARRAY(),   asCommand,           kPropIn},
      {kPropertyOnCursorExit,                 Object::EMPTY_ARRAY(),   asCommand,           kPropIn},
      {kPropertyLaidOut,                      false,                   asBoolean,           kPropOut |
                                                                                            kPropVisualContext},
      {kPropertyVisualHash,                   "",                      asString,            kPropOut |
                                                                                            kPropRuntimeState},
    });

    return sCommonComponentProperties;
}

bool
CoreComponent::containsLocalPosition(const Point &position) const {
    const auto& bounds = getCalculated(kPropertyBounds).get<Rect>();
    Rect localBounds(0, 0, bounds.getWidth(), bounds.getHeight());
    return localBounds.contains(position);
}

Point
CoreComponent::toLocalPoint(const Point& globalPoint) const
{
    auto toLocal = getGlobalToLocalTransform();
    if (toLocal.singular()) {
        static float NaN = std::numeric_limits<float>::quiet_NaN();
        return {NaN, NaN};
    } else {
        return toLocal * globalPoint;
    }
}

Point
CoreComponent::localToGlobal(Point position) const
{
    auto toLocal = getGlobalToLocalTransform();
    if (toLocal.singular()) {
        static float NaN = std::numeric_limits<float>::quiet_NaN();
        return {NaN, NaN};
    } else {
        return toLocal.inverse() * position;
    }
}

bool
CoreComponent::inParentViewport() const {
    if (!mParent) {
        return false;
    }

    const auto& bounds = getCalculated(kPropertyBounds).get<Rect>();
    auto parentBounds = mParent->getCalculated(kPropertyBounds).get<Rect>();
    // Reset to "viewport"
    parentBounds = Rect(0, 0, parentBounds.getWidth(), parentBounds.getHeight());
    // Shift by scroll position if any
    parentBounds.offset(mParent->scrollPosition());

    return !parentBounds.intersect(bounds).empty();
}

PointerCaptureStatus
CoreComponent::processPointerEvent(const PointerEvent& event, apl_time_t timestamp, bool onlyProcessGestures)
{
    if (mState.get(kStateDisabled))
        return kPointerStatusNotCaptured;

    if (processGestures(event, timestamp))
        return kPointerStatusCaptured;

    if (onlyProcessGestures)
        return kPointerStatusNotCaptured;

    auto pointInCurrent = toLocalPoint(event.pointerEventPosition);
    auto it = sEventHandlers.find(event.pointerEventType);
    if (it != sEventHandlers.end())
        executePointerEventHandler(it->second, pointInCurrent);

    return kPointerStatusNotCaptured;
}

const RootConfig&
CoreComponent::getRootConfig() const {
    return mContext->getRootConfig();
}

void
CoreComponent::ensureGlobalToLocalTransform() {
    // Check for a stale parent, since marking a cache transform as stale does not
    // mark children (see markGlobalToLocalTransformStale()). If any parent is stale,
    // the mark will bubble up to this component during this phase.
    if (mParent) {
        mParent->ensureGlobalToLocalTransform();
    }

    if (!mGlobalToLocalIsStale) {
        return;
    }

    Transform2D newLocalTransform;

    auto componentTransform = getCalculated(kPropertyTransform).get<Transform2D>();
    // To transform from the coordinate space of the parent component to the
    // coordinate space of the child component, first offset by the position of
    // the child in the parent, then undo the child transformation:
    const auto& boundsInParent = getCalculated(kPropertyBounds).get<Rect>();
    auto offsetInParent = boundsInParent.getTopLeft();
    newLocalTransform = componentTransform.inverse() * Transform2D::translate(-offsetInParent);

    if (mParent) {
        // Account for the parent's scroll position. The scroll position only affects the
        // coordinate space of children, so we account for it in the child component
        // transformation.
        auto scrollPosition = mParent->scrollPosition();
        newLocalTransform = newLocalTransform
                * Transform2D::translate(scrollPosition)
                * mParent->getGlobalToLocalTransform();
    }

    if (newLocalTransform != mGlobalToLocal) {
        mGlobalToLocal = newLocalTransform;

        for (auto i = 0; i < getChildCount(); i++) {
            auto child = getCoreChildAt(i);
            if (child->getCalculated(kPropertyLaidOut).truthy()) {
                child->markGlobalToLocalTransformStale();
            }
        }
    }

    mGlobalToLocalIsStale = false;
}

const Transform2D&
CoreComponent::getGlobalToLocalTransform() const {
    auto &mutableThis = const_cast<CoreComponent&>(*this);
    mutableThis.ensureGlobalToLocalTransform();
    return mGlobalToLocal;
}

YGDirection
CoreComponent::getLayoutDirection() const
{
    auto direction = YGNodeStyleGetDirection(mYGNodeRef);
    if (direction == YGDirectionInherit) {
        auto parentInDocument = getParentIfInDocument();
        if (!parentInDocument)
            // Fallback to document level layoutDirection
            return mContext->getLayoutDirection() == kLayoutDirectionRTL ? YGDirectionRTL : YGDirectionLTR;

        return parentInDocument->getLayoutDirection();
    }
    return direction;
}

} // namespace apl
