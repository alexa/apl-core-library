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

#include <yoga/YGNode.h>

#include "apl/common.h"
#include "apl/component/corecomponent.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/componenteventsourcewrapper.h"
#include "apl/component/componenteventtargetwrapper.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/contextwrapper.h"
#include "apl/engine/focusmanager.h"
#include "apl/engine/hovermanager.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/builder.h"
#include "apl/engine/componentdependant.h"
#include "apl/livedata/layoutrebuilder.h"
#include "apl/primitives/keyboard.h"
#include "apl/time/sequencer.h"
#include "apl/time/timemanager.h"
#include "apl/touch/pointerevent.h"
#include "apl/utils/session.h"
#include "apl/utils/searchvisitor.h"

namespace apl {

const std::string VISUAL_CONTEXT_TYPE_MIXED = "mixed";
const std::string VISUAL_CONTEXT_TYPE_GRAPHIC = "graphic";
const std::string VISUAL_CONTEXT_TYPE_TEXT = "text";
const std::string VISUAL_CONTEXT_TYPE_VIDEO = "video";
const std::string VISUAL_CONTEXT_TYPE_EMPTY = "empty";

/*****************************************************************/

const static bool DEBUG_BOUNDS = false;
const static bool DEBUG_ENSURE = false;

CoreComponent::CoreComponent(const ContextPtr& context,
                             Properties&& properties,
                             const std::string& path)
    : Component(context, properties.asLabel(*context, "id")),
      mInheritParentState(properties.asBoolean(*context, "inheritParentState", false)),
      mStyle(properties.asString(*context, "style", "")),
      mProperties(std::move(properties)),
      mParent(nullptr),
      mYGNodeRef(YGNodeNewWithConfig(context->ygconfig())),
      mPath(path)
{
}

void
CoreComponent::scheduleTickHandler(const Object& handler, double delay) {
    auto weak_ptr = std::weak_ptr<CoreComponent>(std::static_pointer_cast<CoreComponent>(shared_from_this()));
    // Lambda capture takes care of handler here as it's a copy.
    mContext->getRootConfig().getTimeManager()->setTimeout([weak_ptr, handler, delay]() {
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
                mContext->getRootConfig().getTickHandlerUpdateLimit());
        scheduleTickHandler(handler, delay);
    }
}

void
CoreComponent::initialize()
{
    // TODO: Would be nice to work this in with the regular properties more cleanly.
    mState.set(kStateChecked, mProperties.asBoolean(*mContext, "checked", false));
    mState.set(kStateDisabled, mProperties.asBoolean(*mContext, "disabled", false));
    mCalculated.set(kPropertyNotifyChildrenChanged, Object::EMPTY_MUTABLE_ARRAY());

    // Fix up the state variables that can be assigned as a property
    if (mInheritParentState && mParent)
        mState = mParent->getState();

    // Assign the built-in properties.
    assignProperties(propDefSet());

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

    // Process tick handlers here. Not same as onMount as it's a bad idea to go through every component on every tick
    // to collect handlers and run them on mass.
    processTickHandlers();
}

void
CoreComponent::release()
{
    // TODO: Must remove this component from any dirty lists
    mParent = nullptr;
    for (auto& child : mChildren)
        child->release();
    mChildren.clear();
}

/**
 * Accept a visitor pattern
 * @param visitor
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
 * @param visitor
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

ComponentPtr
CoreComponent::findComponentById(const std::string& id) const
{
    if (id.empty())
        return nullptr;

    if (mId == id || mUniqueId == id)
        return std::const_pointer_cast<CoreComponent>(shared_from_corecomponent());

    for (auto& m : mChildren) {
        auto result = m->findComponentById(id);
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
            LOG(LogLevel::WARN) << "Unexpected update command type " << type << " value=" << value;
            break;
    }
}


void
CoreComponent::update(UpdateType type, const std::string& value) {
    //no-op
}

/**
 * Add a child component.  The child already has a reference to the parent.
 * @param child The component to add.
 */
bool
CoreComponent::appendChild(const ComponentPtr& child, bool useDirtyFlag)
{
    return insertChild(child, mChildren.size(), useDirtyFlag);
}

void
CoreComponent::notifyChildChanged(size_t index, const std::string& uid, const std::string& action)
{
    auto &changes = mCalculated.get(kPropertyNotifyChildrenChanged).getMutableArray();
    auto change = std::make_shared<ObjectMap>();
    change->emplace("index", index);
    change->emplace("uid", uid);
    change->emplace("action", action);
    changes.emplace_back(change);

    setDirty(kPropertyNotifyChildrenChanged);
}

void
CoreComponent::attachYogaNodeIfRequired(const CoreComponentPtr& coreChild, int index)
{
    // For most layouts we just attach the Yoga node.
    // For sequences we don't attach until ensureLayout is called if outside of existing ensured range.
    if (shouldAttachChildYogaNode(index)
        || (mEnsuredChildren.contains(index) && index > mEnsuredChildren.lowerBound())) {
        LOG_IF(DEBUG_ENSURE) << "attaching yoga node index=" << index << " this=" << *this;
        auto offset = mEnsuredChildren.insert(index);
        YGNodeInsertChild(mYGNodeRef, coreChild->getNode(), offset);
    } else if (!mEnsuredChildren.empty() && index <= mEnsuredChildren.lowerBound()) {
        mEnsuredChildren.shift(1);
    }
}

bool
CoreComponent::insertChild(const ComponentPtr& child, size_t index, bool useDirtyFlag)
{
    if (!child || child->getParent())
        return false;

    auto coreChild = std::static_pointer_cast<CoreComponent>(child);

    if (index > mChildren.size())
        index = mChildren.size();

    attachYogaNodeIfRequired(coreChild, index);

    mChildren.insert(mChildren.begin() + index, coreChild);

    if (useDirtyFlag) {
        notifyChildChanged(index, child->getUniqueId(), "insert");
        // If we add a view hierarchy with dirty flags, we need to update the context
        coreChild->markAdded();
    }

    coreChild->attachedToParent(shared_from_corecomponent());
    return true;
}

bool
CoreComponent::remove()
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

    mParent->removeChild(shared_from_corecomponent(), true);
    mParent = nullptr;
    return true;
}

void
CoreComponent::removeChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag)
{
    // Release focus for this child and descendants.  Also remove them from the dirty set
    child->markRemoved();

    YGNodeRemoveChild(mYGNodeRef, child->getNode());
    mChildren.erase(mChildren.begin() + index);

    // The parent component has changed the number of children
    if (useDirtyFlag)
        notifyChildChanged(index, child->getUniqueId(), "remove");

    if (mEnsuredChildren.contains(index)) {
        mEnsuredChildren.remove(index);
    } else if (!mEnsuredChildren.empty() && (int)index < mEnsuredChildren.lowerBound()) {
        mEnsuredChildren.shift(-1);
    }
}

void
CoreComponent::removeChild(const CoreComponentPtr& child, bool useDirtyFlag)
{
    auto it = std::find(mChildren.begin(), mChildren.end(), child);
    assert(it != mChildren.end());
    size_t index = std::distance(mChildren.begin(), it);
    removeChild(child, index, useDirtyFlag);
}

void
CoreComponent::removeChildAt(size_t index, bool useDirtyFlag)
{
    if (index >= mChildren.size())
        return;
    auto child = mChildren.at(index);
    removeChild(child, index, useDirtyFlag);
}

/**
 * This component has been removed from the DOM.  Walk the hierarchy and make sure that
 * it and its children are not focused and not dirty.  Note that we don't clear dirty
 * flags from the component hierarchy.
 */
void
CoreComponent::markRemoved()
{
    auto self = shared_from_corecomponent();

    mContext->clearDirty(self);
    mContext->focusManager().releaseFocus(self, true);

    for (auto& child : mChildren)
        std::static_pointer_cast<CoreComponent>(child)->markRemoved();
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
        std::static_pointer_cast<CoreComponent>(child)->markAdded();
}

void
CoreComponent::ensureLayout(bool useDirtyFlag)
{
    // Walk up the component hierarchy and ensure each component is attached to its parent
    auto component = shared_from_corecomponent();
    bool needsLayoutCalculation = false;

    while(component->getParent()) {
        auto parent = std::static_pointer_cast<CoreComponent>(component->getParent());
        if(!component->isAttached()) {
            parent->ensureChildAttached(component);
            needsLayoutCalculation = true;
        }
        component = parent;
    }

    // Run layout calculation if required on the top most component.
    if(needsLayoutCalculation) {
        auto width = YGNodeLayoutGetWidth(component->getNode());
        auto height = YGNodeLayoutGetHeight(component->getNode());
        LOG_IF(DEBUG_ENSURE) << "Re-running parent layout with width=" << width << " height=" << height;
        component->layout(width, height, useDirtyFlag);
    }
}

void
CoreComponent::reportLoaded(size_t index) {
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

void
CoreComponent::ensureChildAttached(const CoreComponentPtr& child, int targetIdx)
{
    if (mEnsuredChildren.empty() || mEnsuredChildren.above(targetIdx)) {
        // Ensure from upperBound to target
        for (int index = mEnsuredChildren.empty() ? 0 : mEnsuredChildren.upperBound() + 1; index <= targetIdx ; index++) {
            const auto& c = mChildren.at(index);
            if (attachChild(c, mEnsuredChildren.size())) {
                mEnsuredChildren.expandTo(index);
            }
        }
    } else if (mEnsuredChildren.below(targetIdx)) {
        // Ensure from lowerBound down to target
        for (int index = mEnsuredChildren.lowerBound() - 1; index >= targetIdx ; index--) {
            const auto& c = mChildren.at(index);
            if (attachChild(c, 0)) {
                mEnsuredChildren.expandTo(index);
            }
        }
    } else {
        // Just attach single one inside of ensured range if needed.
        attachChild(child, targetIdx - mEnsuredChildren.lowerBound());
    }
}

void
CoreComponent::ensureChildAttached(const CoreComponentPtr& child)
{
    // This routine guarantees that all the children up to and including this one are attached.
    auto it = std::find(mChildren.begin(), mChildren.end(), child);
    int targetIdx = std::distance(mChildren.begin(), it);
    ensureChildAttached(child, targetIdx);
}

bool
CoreComponent::attachChild(const CoreComponentPtr& child, size_t index)
{
    if (child->isAttached()) {
        return false;
    }
    LOG_IF(DEBUG_ENSURE) << "attaching yoga node index=" << index << " this=" << *this;
    YGNodeInsertChild(mYGNodeRef, child->getNode(), index);
    child->updateNodeProperties();
    return true;
}

bool
CoreComponent::isAttached() const
{
    return mYGNodeRef->getOwner() != nullptr;
}

/**
 * Certain properties can only be set when we are attached to the node layout hierarchy.
 * These properties should already have been calculated; we call the layout method to
 * ensure that they have been set on the node.
 */
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
};

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
                // If the user assigned a string, we need to check for data binding
                if (p->second.isString()) {
                    auto tmp = parseDataBinding(*mContext, p->second.getString());  // Expand data-binding
                    if (tmp.isEvaluable()) {
                        auto self = std::static_pointer_cast<CoreComponent>(shared_from_this());
                        ComponentDependant::create(self, pd.key, tmp, mContext, pd.getBindingFunction());
                    }
                    value = pd.calculate(*mContext, evaluate(*mContext, tmp));  // Calculate the final value
                }
                else if ((pd.flags & kPropEvaluated) != 0) {
                    // Explicitly marked for evaluation, so do it.
                    // Will not attach dependant if no valid symbols.
                    auto tmp = parseDataBindingRecursive(*mContext, p->second);
                    auto self = std::static_pointer_cast<CoreComponent>(shared_from_this());
                    ComponentDependant::create(self, pd.key, tmp, mContext, pd.getBindingFunction());
                    value = pd.calculate(*mContext, p->second);
                }
                else {
                    value = pd.calculate(*mContext, p->second);
                }
                mAssigned.emplace(pd.key);
            } else {
                // Make sure this wasn't a required property
                if ((pd.flags & kPropRequired) != 0) {
                    mIsValid = false;
                    CONSOLE_CTP(mContext) << "Missing required property: " << "pd.name";
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

        //Apply this property to the yn if we care about it
        if (pd.layoutFunc != nullptr)
            pd.layoutFunc(mYGNodeRef, value, *mContext);
    }

}

void
CoreComponent::handlePropertyChange(const ComponentPropDef& def, const Object& value) {
    // If the mixed state inherits from the parent, we block the change.
    if ((def.flags & kPropMixedState) != 0 && mInheritParentState)
        return;

    if (value != getCalculated(def.key)) {
        mCalculated.set(def.key, value);

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
        if ((def.flags & kPropLayout) != 0)
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
 * A property has been set on the component.
 * @param pds The property definition set.
 * @param key The key of the property that is being changed.
 * @param value The new value of the property.
 * @return True if the property was found in this set (used for chaining property definition sets).
 */
bool
CoreComponent::setPropertyInternal( const ComponentPropDefSet& pds, PropertyKey key, const Object& value ) {
    // Verify that this property is a valid dynamic property
    auto it = pds.dynamic().find(key);
    if (it == pds.dynamic().end())
        return false;

    // If this property was previously assigned we need to clear any dependants
    auto assigned = mAssigned.find(key);
    if (assigned != mAssigned.end()) // Erase all upstream dependants that drive this key
        removeUpstream(key);

    // Mark this property in the "assigned" set of properties.
    mAssigned.emplace(key);

    // Check to see if the actual value of the property changed and update appropriately
    const ComponentPropDef& def = it->second;
    handlePropertyChange(def, def.calculate(*mContext, value));

    return true;
}

bool
CoreComponent::setProperty(PropertyKey key, const Object& value)
{
    // Try setting this as a base property
    if (setPropertyInternal(propDefSet(), key, value))
        return true;

    // Try setting this as a layout property
    if (mParent) {
        auto layoutPDS = getLayoutPropDefSet();
        if (layoutPDS && setPropertyInternal(*layoutPDS, key, value))
            return true;
    }

    CONSOLE_CTP(mContext) << "Invalid property key '" << sComponentPropertyBimap.at(key) << "' for this component";
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

    CONSOLE_CTP(mContext) << "Unknown property name " << key;
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
CoreComponent::updateProperty(PropertyKey key, const Object& value)
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

    // We should not reach this point.  Only an assigned equation calls updateProperty
    CONSOLE_CTP(mContext) << "Property " << sComponentPropertyBimap.at(key) << " is not dynamic and can't be updated." ;
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
    for (auto child : mChildren) {
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
        CONSOLE_CTP(mContext) << "Cannot assign state properties to a child that inherits parent state";
        return;
    }

    if (mState.set(stateProperty, value)) {
        if (stateProperty == kStateDisabled) {
            if (value) {
                mState.set(kStatePressed, false);
                mState.set(kStateHover, false);
                mContext->focusManager().releaseFocus(shared_from_corecomponent(), true);
            }
        }

        for (auto& child: mChildren)
            child->updateInheritedState();

        if (stateProperty == kStateDisabled) {
            // notify the hover manager that the disable state has changed
            mContext->hoverManager().componentToggledDisabled(shared_from_corecomponent());
        }

        updateStyle();
    }
}

void
CoreComponent::setDirty( PropertyKey key )
{
    if (mDirty.emplace(key).second)
        mContext->setDirty(shared_from_this());
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
        stateOwner = std::static_pointer_cast<CoreComponent>(stateOwner->getParent());
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
        ptr = std::static_pointer_cast<CoreComponent>(ptr->getParent());
    }
    return ptr;
}

const StyleInstancePtr
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

void
CoreComponent::layout(int width, int height, bool useDirtyFlag)
{
    LOG_IF(DEBUG_ENSURE) << " width=" << width << " height=" << height
        << " useDirty=" << useDirtyFlag << " this=" << *this;
    YGNodeCalculateLayout(mYGNodeRef, width, height, YGDirection::YGDirectionLTR);
    processLayoutChanges(useDirtyFlag);
}

bool
CoreComponent::needsLayout() const
{
    return YGNodeIsDirty(mYGNodeRef);
}

void
CoreComponent::processLayoutChanges(bool useDirtyFlag)
{
    if (DEBUG_BOUNDS) YGNodePrint(mYGNodeRef, YGPrintOptions::YGPrintOptionsLayout);

    float left = YGNodeLayoutGetLeft(mYGNodeRef);
    float top = YGNodeLayoutGetTop(mYGNodeRef);
    float width = YGNodeLayoutGetWidth(mYGNodeRef);
    float height = YGNodeLayoutGetHeight(mYGNodeRef);

    // Update the transformation matrix
    fixTransform(useDirtyFlag);

    Rect rect(left, top, width, height);
    bool changed = rect != mCalculated.get(kPropertyBounds).getRect();

    if (changed) {
        mCalculated.set(kPropertyBounds, std::move(rect));
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
    changed = inner != mCalculated.get(kPropertyInnerBounds).getRect();

    if (changed) {
        mCalculated.set(kPropertyInnerBounds, std::move(inner));
        if (useDirtyFlag)
            setDirty(kPropertyInnerBounds);
    }

    // Inform all children that they should re-check their bounds. No need to do that for not attached ones.
    for (auto& child : mChildren)
        if (child->isAttached())
            child->processLayoutChanges(useDirtyFlag);

    if (!mCalculated.get(kPropertyLaidOut).asBoolean() && !mCalculated.get(kPropertyBounds).getRect().isEmpty()) {
        mCalculated.set(kPropertyLaidOut, true);
        if (useDirtyFlag)
            setDirty(kPropertyLaidOut);
    }
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
    ContextPtr ctx = Context::create(mContext);
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
CoreComponent::createKeyboardEventContext(const std::string& handler, const ObjectMapPtr& keyboard) const
{
    ContextPtr ctx = Context::create(mContext);
    auto event = createEventProperties(handler, getValue());
    event->emplace("keyboard", keyboard);
    ctx->putConstant("event", event);
    return ctx;
}

const EventPropertyMap &
CoreComponent::eventPropertyMap() const {
    static EventPropertyMap sCoreEventProperties(
            {
                    {"bind",     [](const CoreComponent *c) { return ContextWrapper::create(c->getContext()); }},
                    {"checked",  [](const CoreComponent *c) { return c->getState().get(kStateChecked); }},
                    {"disabled", [](const CoreComponent *c) { return c->getState().get(kStateDisabled); }},
                    {"focused",  [](const CoreComponent *c) { return c->getState().get(kStateFocused); }},
                    {"id",       [](const CoreComponent *c) { return c->getId(); }},
                    {"uid",      [](const CoreComponent *c) { return c->getUniqueId(); }},
                    {"width",    [](const CoreComponent *c) { return YGNodeLayoutGetWidth(c->mYGNodeRef); }},
                    {"height",   [](const CoreComponent *c) { return YGNodeLayoutGetHeight(c->mYGNodeRef); }},
                    {"opacity",  [](const CoreComponent *c) { return c->getCalculated(kPropertyOpacity); }},
                    {"pressed",  [](const CoreComponent *c) { return c->getState().get(kStatePressed); }},
                    {"type",     [](const CoreComponent *c) { return sComponentTypeBimap.at(c->getType()); }}
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

static const char sHierarchySig[] = "CEFMIPSQTWGV";  // Must match ComponentType

std::string
CoreComponent::getHierarchySignature() const
{
    std::string result(1, sHierarchySig[getType()]);
    if (mChildren.size()) {
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
    LOG_IF(DEBUG_TRANSFORM) << mCalculated.get(kPropertyTransform).getTransform2D();

    Transform2D updated;

    auto transform = mCalculated.get(kPropertyTransformAssigned);

    if (transform.isArray()) {
        transform = Transformation::create(*getContext(), transform.getArray());
        mCalculated.set(kPropertyTransformAssigned, transform);
    }

    if (transform.isTransform()) {
        float width = YGNodeLayoutGetWidth(mYGNodeRef);
        float height = YGNodeLayoutGetHeight(mYGNodeRef);
        updated = transform.getTransformation()->get(width, height);
    }

    auto value = getCalculated(kPropertyTransform).getTransform2D();
    if (updated != value) {
        mCalculated.set(kPropertyTransform, Object(std::move(updated)));
        if (useDirtyFlag)
            setDirty(kPropertyTransform);

        LOG_IF(DEBUG_TRANSFORM) << "updated to " << mCalculated.get(kPropertyTransform).getTransform2D();
    }
}

void CoreComponent::setHeight(const Dimension& height) {
    if (mYGNodeRef)
        yn::setHeight(mYGNodeRef, height, *mContext);
    else
        LOG(LogLevel::ERROR) << "setHeight:  Missing yoga node for component id '" << getId() << "'";
    mCalculated.set(kPropertyHeight, height);
}

void CoreComponent::setWidth(const Dimension& width) {
    if (mYGNodeRef)
        yn::setWidth(mYGNodeRef, width, *mContext);
    else
        LOG(LogLevel::ERROR) << "setWidth:  Missing yoga node for component id '" << getId() << "'";
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

    if (mChildren.size() > 0) {
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
    component.AddMember("__path", rapidjson::Value(mPath.c_str(), allocator), allocator);

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

    if (mChildren.size() > 0) {
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
CoreComponent::serializeVisualContextInternal(rapidjson::Value &outArray, rapidjson::Document::AllocatorType& allocator,
        float realOpacity, float visibility, const Rect& visibleRect, int visualLayer)
{
    if(visibility == 0.0 && mParent) {
        // Not visible and not viewport component.
        return;
    }

    // Decide if actionable
    bool actionable = !getCalculated(kPropertyEntities).empty();
    rapidjson::Value tags(rapidjson::kObjectType);
    actionable |= getTags(tags, allocator);
    bool includeInContext = !mParent || (visibility > 0.0 && actionable);

    rapidjson::Value children(rapidjson::kArrayType);

    // Process children
    if (!mChildren.empty() && visibility > 0.0) {
        auto visibleIndexes = getChildrenVisibility(realOpacity, visibleRect);
        auto visualLayers = calculateChildrenVisualLayer(visibleIndexes, visibleRect, visualLayer);
        for (auto childIdx : visibleIndexes) {
            const auto& child = mChildren.at(childIdx.first);
            auto childVisibleRect = child->calculateVisibleRect(visibleRect);
            auto childRealOpacity = child->calculateRealOpacity(realOpacity);
            auto childVisibility = childIdx.second;
            auto childVisualLayer = visualLayers.at(childIdx.first);
            child->serializeVisualContextInternal(
                    includeInContext? children : outArray, allocator,
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
    auto transform = getCalculated(kPropertyTransform).getTransform2D();
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
CoreComponent::getVisualContextType() {
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
CoreComponent::calculateVisibility(float parentRealOpacity, const Rect& parentVisibleRect) {
    auto realOpacity = calculateRealOpacity(parentRealOpacity);
    if(realOpacity == 0 || (getCalculated(kPropertyDisplay).asInt() != kDisplayNormal)) {
        return 0.0;
    }

    auto boundingRect = getGlobalBounds();
    if(boundingRect.area() == 0) {
        return 0.0;
    }

    auto visibleRect = calculateVisibleRect(parentVisibleRect);
    return visibleRect.area()/boundingRect.area() * realOpacity;
}

bool
CoreComponent::getTags(rapidjson::Value &outMap, rapidjson::Document::AllocatorType &allocator) {
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

    if(!mParent) {
        outMap.AddMember("viewport", rapidjson::Value(rapidjson::kObjectType), allocator);
        actionable |= true;
    }

    return actionable;
}

Rect
CoreComponent::calculateVisibleRect(const Rect& parentVisibleRect) {
    return getGlobalBounds().intersect(parentVisibleRect);
}

Rect
CoreComponent::calculateVisibleRect() {
    auto rect = getGlobalBounds();
    if(!mParent) {
        float viewportWidth = mContext->width();
        float viewportHeight = mContext->height();
        return rect.intersect({0, 0, viewportWidth, viewportHeight});
    }

    return rect.intersect(mParent->calculateVisibleRect());
}

float
CoreComponent::calculateRealOpacity(float parentRealOpacity) {
    auto assignedOpacity = getCalculated(kPropertyOpacity).asNumber();
    return assignedOpacity * parentRealOpacity;
}

float
CoreComponent::calculateRealOpacity() {
    auto assignedOpacity = getCalculated(kPropertyOpacity).asNumber();
    if(!mParent) {
        return assignedOpacity;
    }

    return assignedOpacity * mParent->calculateRealOpacity();
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

/*****************************************************************/

inline void
inlineFixTransform(Component& component)
{
    auto& core = static_cast<CoreComponent&>(component);
    core.fixTransform(true);
}

inline Object
defaultWidth(Component& component, const RootConfig& rootConfig)
{
    return rootConfig.getDefaultComponentWidth(component.getType());
}

inline Object
defaultHeight(Component& component, const RootConfig& rootConfig)
{
    return rootConfig.getDefaultComponentHeight(component.getType());
}

void
CoreComponent::fixSpacing(bool reset) {
    auto spacing = getCalculated(kPropertySpacing).asDimension(*mContext);
    if (reset) spacing = 0;
    if (spacing.isAbsolute()) {
        YGNodeRef parent = YGNodeGetParent(mYGNodeRef);
        if (!parent)
            return;

        YGEdge edge = YGNodeStyleGetFlexDirection(parent) == YGFlexDirectionColumn ? YGEdgeTop : YGEdgeLeft;
        float currentValue = YGNodeStyleGetMargin(mYGNodeRef, edge).value;
        if (std::isnan(currentValue) || std::abs(currentValue - spacing.getValue()) > 0.1) {
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
    auto strokeWidthProp = getCalculated(kPropertyBorderStrokeWidth);
    float borderWidth = getCalculated(kPropertyBorderWidth).asAbsoluteDimension(*mContext).getValue();
    float drawnWidth = borderWidth; // default the drawn width to the border width

    if (strokeWidthProp == Object::NULL_OBJECT()) {
        // no stroke width - default draw border width to border width
        // initialize stroke width to border width
        mCalculated.set(kPropertyBorderStrokeWidth, Object(Dimension(borderWidth)));
    } else {
        // stroke width - clamp the drawn border to the border width
        float strokeWidth = strokeWidthProp.getAbsoluteDimension();
        if (strokeWidth < borderWidth)
            drawnWidth = strokeWidth;
    }

    Dimension dimension(drawnWidth);

    auto drawnWidthProp = getCalculated(kPropertyDrawnBorderWidth);
    if (drawnWidthProp == Object::NULL_OBJECT() ||
        dimension != mCalculated.get(kPropertyDrawnBorderWidth).asAbsoluteDimension(*mContext)) {
        mCalculated.set(kPropertyDrawnBorderWidth, Object(std::move(dimension)));
        if (useDirtyFlag)
            setDirty(kPropertyBorderStrokeWidth);
    }
}


const ComponentPropDefSet&
CoreComponent::propDefSet() const {
    static ComponentPropDefSet sCommonComponentProperties = ComponentPropDefSet().add({
          {kPropertyAccessibilityLabel, "",                    asString,            kPropInOut},
          {kPropertyBounds,             Object::EMPTY_RECT(),  nullptr,             kPropOut},
          {kPropertyChecked,            false,                 asBoolean,           kPropInOut |
                                                                                    kPropDynamic |
                                                                                    kPropMixedState},
          {kPropertyDescription,        "",                    asString,            kPropIn},
          {kPropertyDisplay,            kDisplayNormal,        sDisplayMap,         kPropInOut |
                                                                                    kPropStyled |
                                                                                    kPropDynamic, yn::setDisplay},
          {kPropertyDisabled,           false,                 asBoolean,           kPropInOut |
                                                                                    kPropDynamic |
                                                                                    kPropMixedState},
          {kPropertyEntities,           Object::EMPTY_ARRAY(), asDeepArray,         kPropIn},
          {kPropertyFocusable,          false,                 nullptr,             kPropOut},
          {kPropertyHandleTick,         Object::EMPTY_ARRAY(), asArray,             kPropIn},
          {kPropertyHeight,             Dimension(),           asDimension,         kPropIn,      yn::setHeight, defaultHeight},
          {kPropertyInnerBounds,        Object::EMPTY_RECT(),  nullptr,             kPropOut},
          {kPropertyMaxHeight,          Object::NULL_OBJECT(), asNonAutoDimension,  kPropIn,      yn::setMaxHeight},
          {kPropertyMaxWidth,           Object::NULL_OBJECT(), asNonAutoDimension,  kPropIn,      yn::setMaxWidth},
          {kPropertyMinHeight,          Dimension(0),          asNonAutoDimension,  kPropIn,      yn::setMinHeight},
          {kPropertyMinWidth,           Dimension(0),          asNonAutoDimension,  kPropIn,      yn::setMinWidth},
          {kPropertyOnMount,            Object::EMPTY_ARRAY(), asCommand,           kPropIn},
          {kPropertyOpacity,            1.0,                   asOpacity,           kPropInOut |
                                                                                    kPropStyled |
                                                                                    kPropDynamic},
          {kPropertyPaddingBottom,      Dimension(0),          asAbsoluteDimension, kPropIn, yn::setPadding<YGEdgeBottom>},
          {kPropertyPaddingLeft,        Dimension(0),          asAbsoluteDimension, kPropIn, yn::setPadding<YGEdgeLeft>},
          {kPropertyPaddingRight,       Dimension(0),          asAbsoluteDimension, kPropIn, yn::setPadding<YGEdgeRight>},
          {kPropertyPaddingTop,         Dimension(0),          asAbsoluteDimension, kPropIn, yn::setPadding<YGEdgeTop>},
          {kPropertyShadowColor,        Color(),               asColor,             kPropInOut | kPropStyled },
          {kPropertyShadowHorizontalOffset, Dimension(0),      asAbsoluteDimension, kPropInOut | kPropStyled },
          {kPropertyShadowRadius,       Dimension(0),          asAbsoluteDimension, kPropInOut | kPropStyled },
          {kPropertyShadowVerticalOffset, Dimension(0),        asAbsoluteDimension, kPropInOut | kPropStyled },
          {kPropertySpeech,             "",                    asString,            kPropIn},
          {kPropertyTransformAssigned,  Object::NULL_OBJECT(), asTransformOrArray,  kPropIn |
                                                                                    kPropDynamic |
                                                                                    kPropEvaluated, inlineFixTransform},
          {kPropertyTransform,          Object::IDENTITY_2D(), nullptr,             kPropOut},
          {kPropertyUser,               Object::NULL_OBJECT(), nullptr,             kPropOut},
          {kPropertyWidth,              Dimension(),           asDimension,         kPropIn,      yn::setWidth, defaultWidth},
          {kPropertyOnCursorEnter,      Object::EMPTY_ARRAY(), asCommand,           kPropIn},
          {kPropertyOnCursorExit,       Object::EMPTY_ARRAY(), asCommand,           kPropIn},
          {kPropertyLaidOut,            false,                 asBoolean,           kPropOut},
      });

    return sCommonComponentProperties;
}

bool
CoreComponent::containsGlobalPosition(const Point &position) const {
    Rect bounds = getGlobalBounds();
    return bounds.contains(position);
}

bool
CoreComponent::containsLocalPosition(const Point &position) const {
    auto bounds = getCalculated(kPropertyBounds).getRect();
    Rect localBounds(0, 0, bounds.getWidth(), bounds.getHeight());
    return localBounds.contains(position);
}

bool
CoreComponent::inParentViewport() const {
    if (!mParent) {
        return false;
    }

    Rect bounds = getCalculated(kPropertyBounds).getRect();
    auto parentBounds = mParent->getCalculated(kPropertyBounds).getRect();
    // Reset to "viewport"
    parentBounds = Rect(0, 0, parentBounds.getWidth(), parentBounds.getHeight());
    // Shift by scroll position if any
    parentBounds.offset(mParent->scrollPosition());

    return !parentBounds.intersect(bounds).isEmpty();
}

bool
CoreComponent::getCoordinateTransformFromParent(const ComponentPtr& ancestor, Transform2D& out) const {
    Transform2D result;

    auto component = shared_from_this();
    while (component && component != ancestor) {
        auto componentTransform = component->getCalculated(kPropertyTransform).getTransform2D();
        if (componentTransform.singular()) {
            // Singular transform encountered, a transformation cannot be computed
            return false;
        }

        // To transform from the coordinate space of the parent component to the
        // coordinate space of the child component, first offset by the position of
        // the child in the parent, then undo the child transformation:
        auto boundsInParent = component->getCalculated(kPropertyBounds).getRect();
        auto offsetInParent = boundsInParent.getTopLeft();
        result = result
            * componentTransform.inverse()
            * Transform2D::translate(-offsetInParent.getX(), -offsetInParent.getY());

        // Account for the parent's scroll position. The scroll position only affects the coordinate space
        // of children, so we account for it in the child component transformation.
        auto parent = component->getParent();
        if (parent) {
            auto scrollPosition = parent->scrollPosition();
            result = Transform2D::translate(scrollPosition.getX(), scrollPosition.getY()) * result;
        }

        component = parent;
    }

    if (component == ancestor) {
        out = result;
        return true;
    } else {
        return false;
    }
}


/*
 * END SearchVisitor IMPL
 */

}  // namespace apl
