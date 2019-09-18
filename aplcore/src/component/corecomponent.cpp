/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/component/corecomponent.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/yogaproperties.h"
#include "apl/engine/focusmanager.h"
#include "apl/engine/hovermanager.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/builder.h"
#include "apl/engine/componentdependant.h"
#include "apl/content/rootconfig.h"
#include "apl/time/sequencer.h"
#include "apl/primitives/keyboard.h"
#include "apl/utils/session.h"

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

void CoreComponent::initialize()
{
    // TODO: Would be nice to work this in with the regular properties more cleanly.
    mState.set(kStateChecked, mProperties.asBoolean(*mContext, "checked", false));
    mState.set(kStateDisabled, mProperties.asBoolean(*mContext, "disabled", false));

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
CoreComponent::accept(Visitor<Component>& visitor) const
{
    visitor.visit(*this);
    visitor.push();
    for (auto&& m : mChildren)
        m->accept(visitor);
    visitor.pop();
}

ComponentPtr
CoreComponent::findComponentById(const std::string& id) const
{
    if (id.empty())
        return nullptr;

    if (mId == id || mUniqueId == id)
        return std::const_pointer_cast<CoreComponent>(shared_from_this());

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
    if (getCalculated(kPropertyDisplay).asInt() != kDisplayNormal ||
            getCalculated(kPropertyOpacity).asNumber() <= 0.0 )
        return nullptr;

    auto bounds = getCalculated(kPropertyBounds).getRect();
    if (!bounds.contains(position))
        return nullptr;

    // Convert to the components internal coordinate system
    auto point = position - bounds.getTopLeft() + scrollPosition();
    auto child = findChildAtPosition(point);
    if (child != nullptr)
        return child;

    return std::const_pointer_cast<CoreComponent>(shared_from_this());
}

ComponentPtr
CoreComponent::findChildAtPosition(const Point& position) const {
    // Walk the child list in reverse order because later children overlap earlier
    for (auto it = mChildren.rbegin() ; it != mChildren.rend() ; it++) {
        auto child = (*it)->findComponentAtPosition(position);
        if (child != nullptr)
            return child;
    }

    return nullptr;
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
                fm.setFocus(shared_from_this(), false);
            else
                fm.releaseFocus(shared_from_this(), false);
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


/**
 * Add a child component.  The child already has a reference to the parent.
 * @param child The component to add.
 */
bool
CoreComponent::appendChild(const ComponentPtr& child, bool useDirtyFlag)
{
    return insertChild(child, mChildren.size(), useDirtyFlag);
}

bool
CoreComponent::insertChild(const ComponentPtr& child, size_t index, bool useDirtyFlag)
{
    if (!child || child->getParent())
        return false;

    bool canInsert = (singleChild() && mChildren.empty()) || multiChild();
    if (!canInsert)
        return false;

    auto coreChild = std::static_pointer_cast<CoreComponent>(child);

    if (index > mChildren.size())
        index = mChildren.size();

    mChildren.insert(mChildren.begin() + index, coreChild);

    if (useDirtyFlag) {
        setDirty(kPropertyNotifyChildrenChanged);
        // If we add a view hierarchy with dirty flags, we need to update the context
        coreChild->markAdded();
    }

    // For most layouts we just attach the Yoga node.
    // For sequences we don't attach until ensureLayout is called.
    if (alwaysAttachChildYogaNode()) {
        if (DEBUG_ENSURE) LOG(LogLevel::DEBUG) << "attaching yoga node index=" << index << " this=" << *this;
        YGNodeInsertChild(mYGNodeRef, coreChild->getNode(), index);
    }

    coreChild->attachedToParent(shared_from_this());
    return true;
}

bool
CoreComponent::remove()
{
    if (!mParent)
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

    mParent->removeChild(shared_from_this(), true);
    mParent = nullptr;
    return true;
}

void
CoreComponent::removeChild(const CoreComponentPtr& child, bool useDirtyFlag)
{
    auto it = std::find(mChildren.begin(), mChildren.end(), child);
    assert(it != mChildren.end());

    // Release focus for this child and descendants.  Also remove them from the dirty set
    child->markRemoved();

    YGNodeRemoveChild(mYGNodeRef, child->getNode());
    mChildren.erase(it);

    // The parent component has changed the number of children
    if (useDirtyFlag)
        setDirty(kPropertyNotifyChildrenChanged);
}

/**
 * This component has been removed from the DOM.  Walk the hierarchy and make sure that
 * it and its children are not focused and not dirty.  Note that we don't clear dirty
 * flags from the component hierarchy.
 */
void
CoreComponent::markRemoved()
{
    auto self = shared_from_this();

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
    auto component = shared_from_this();
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
CoreComponent::ensureChildAttached(const ComponentPtr& child)
{
    // This routine guarantees that all the children up to and including this one are attached.
    for (auto index = 0 ; index < mChildren.size() ; index++) {
        const auto& c = mChildren.at(index);
        if (!c->isAttached()) {
            LOG_IF(DEBUG_ENSURE) << "attaching yoga node index=" << index << " this=" << *this;
            YGNodeInsertChild(mYGNodeRef, c->getNode(), index);
            c->updateNodeProperties();
        }
        if (c == child)
            break;
    }
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
            auto p = mProperties.find(pd.name);
            if (p != mProperties.end()) {
                // If the user assigned a string, we need to check for data binding
                if (p->second.isString()) {
                    auto tmp = parseDataBinding(*mContext, p->second.getString());  // Expand data-binding
                    if (tmp.isNode()) {
                        std::set<std::string> symbols;
                        tmp.symbols(symbols);
                        auto self = std::static_pointer_cast<CoreComponent>(shared_from_this());
                        for (const auto& symbol : symbols) {
                            auto c = mContext->findContextContaining(symbol);
                            if (c != nullptr)
                                ComponentDependant::create(c, symbol, self, pd.key);
                        }
                    }
                    value = pd.calculate(*mContext, evaluate(*mContext, tmp));  // Calculate the final value
                    mAssigned[pd.key] = tmp;
                }
                else {
                    value = pd.calculate(*mContext, p->second);
                    mAssigned[pd.key] = value;
                }
            } else {
                // Make sure this wasn't a required property
                if ((pd.flags & kPropRequired) != 0) {
                    mIsValid = false;
                    CONSOLE_CTP(mContext) << "Missing required property: " << "pd.name";
                }

                // Check for a styled property
                if ((pd.flags & kPropStyled) != 0 && stylePtr) {
                    auto s = stylePtr->find(pd.name);
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
    if (assigned != mAssigned.end() && assigned->second.isNode()) {
        // Erase all upstream dependants that drive this key
        removeUpstream(key);
    }

    // Mark this property in the "assigned" set of properties.
    mAssigned[key] = value;

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

bool
CoreComponent::recalculatePropertyInternal(const ComponentPropDefSet& propDefSet,
                                           PropertyKey key,
                                           const Object& node)
{
    auto it = propDefSet.dynamic().find(key);
    if (it == propDefSet.dynamic().end())
        return false;

    const ComponentPropDef& def = it->second;
    handlePropertyChange(def, def.calculate(*mContext, evaluate(*mContext, node)));
    return true;
}

void
CoreComponent::recalculateProperty(PropertyKey key)
{
    auto it = mAssigned.find(key);
    if (it != mAssigned.end() && it->second.isNode()) {
        // The property could be a standard component property or a layout property
        if (!recalculatePropertyInternal(propDefSet(), key, it->second)) {
            auto layoutPDS = getLayoutPropDefSet();
            if (layoutPDS)
                recalculatePropertyInternal(*layoutPDS, key, it->second);
        }
    }
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
        auto s = stylePtr->find(pd.name);
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
                mContext->focusManager().releaseFocus(shared_from_this(), true);
            }
        }

        for (auto& child: mChildren)
            child->updateInheritedState();

        if (stateProperty == kStateDisabled) {
            // notify the hover manager that the disable state has changed
            mContext->hoverManager().componentToggledDisabled(shared_from_this());
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

    auto stateOwner = shared_from_this();

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
    auto ptr = shared_from_this();
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

    // Inform all children that they should re-check their bounds.
    for (auto& child : mChildren)
        child->processLayoutChanges(useDirtyFlag);
}


std::shared_ptr<ObjectMap>
CoreComponent::createEventProperties(const std::string& handler, const Object& value) const {

    auto source = std::make_shared<ObjectMap>();
    const auto& type = sComponentTypeBimap.at(getType());
    source->emplace("source", type);  // Legacy value to support old implementation
    source->emplace("type", type);    // As per the APL specification
    source->emplace("handler", handler);
    source->emplace("id", getId());
    source->emplace("uid", getUniqueId());
    source->emplace("value", value);
    source->emplace("focused", mState.get(kStateFocused));

    auto event = std::make_shared<ObjectMap>();
    event->emplace("source", source);
    addEventSourceProperties(*event);

    return event;
}

ContextPtr
CoreComponent::createEventContext(const std::string& handler, const Object& value) const
{
    ContextPtr ctx = Context::create(mContext);
    auto event = createEventProperties(handler, value);
    ctx->putConstant("event", event);
    return ctx;
}


ContextPtr
CoreComponent::createKeyboardEventContext(const std::string& handler, const ObjectMapPtr& keyboard) const
{
    ContextPtr ctx = Context::create(mContext);
    auto event = createEventProperties(handler, Object::NULL_OBJECT());
    event->emplace("keyboard", keyboard);
    ctx->putConstant("event", event);
    return ctx;
}

std::shared_ptr<ObjectMap>
CoreComponent::getEventTargetProperties() const
{
    auto target = std::make_shared<ObjectMap>();
    target->emplace("disabled", mState.get(kStateDisabled));
    target->emplace("focused", mState.get(kStateFocused));
    target->emplace("width", YGNodeLayoutGetWidth(mYGNodeRef));
    target->emplace("height", YGNodeLayoutGetHeight(mYGNodeRef));
    target->emplace("id", getId());
    target->emplace("uid", getUniqueId());
    target->emplace("opacity", mCalculated.get(kPropertyOpacity));
    return target;
}


static const char sHierarchySig[] = "CFIPSQTWGV";  // Must match ComponentType

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
        if ((pds.second.flags & kPropOut) != 0)
            component.AddMember(
                rapidjson::StringRef(pds.second.name.c_str()),   // We assume long-lived strings here
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
                rapidjson::StringRef(pds.second.name.c_str()),   // We assume long-lived strings here
                rapidjson::StringRef(pds.second.map->at(mCalculated.get(pds.first).asInt()).c_str()),
                allocator);
        } else {
            component.AddMember(
                rapidjson::StringRef(pds.second.name.c_str()),   // We assume long-lived strings here
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
CoreComponent::getChildrenVisibility(float realOpacity, const Rect &visibleRect) {
    // Multi children components have specific implementation. Just return single one if it's there.
    assert(mChildren.size() <= 1);
    std::map<int, float> result;
    if(!mChildren.empty()) {
        auto child = mChildren.at(0);
        auto childVisibility = child->calculateVisibility(realOpacity, visibleRect);
        if(childVisibility > 0.0) {
            result.emplace(0, childVisibility);
        }
    }

    return result;
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

    if(mParent && mParent->getType() == kComponentTypeSequence) {
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
    mContext->sequencer().executeCommands(command, eventContext, shared_from_this(), true);
}

void
CoreComponent::executeOnCursorExit() {
    auto eventContext = createDefaultEventContext("CursorExit");
    auto command = getCalculated(kPropertyOnCursorExit);
    mContext->sequencer().executeCommands(command, eventContext, shared_from_this(), true);
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
          {kPropertyTransformAssigned,  Object::NULL_OBJECT(), asTransform,         kPropIn |
                                                                                    kPropDynamic, inlineFixTransform},
          {kPropertyTransform,          Object::IDENTITY_2D(), nullptr,             kPropOut},
          {kPropertyUser,               Object::NULL_OBJECT(), nullptr,             kPropOut},
          {kPropertyWidth,              Dimension(),           asDimension,         kPropIn,      yn::setWidth, defaultWidth},
          {kPropertyOnCursorEnter,      Object::EMPTY_ARRAY(), asCommand,           kPropIn},
          {kPropertyOnCursorExit,       Object::EMPTY_ARRAY(), asCommand,           kPropIn}
      });

    return sCommonComponentProperties;
}


}  // namespace apl
