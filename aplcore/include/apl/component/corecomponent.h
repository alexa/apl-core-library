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

#ifndef _APL_CORE_COMPONENT_H
#define _APL_CORE_COMPONENT_H

#include <climits>
#include <stack>

#include <yoga/YGNode.h>

#include "apl/component/component.h"
#include "apl/component/textmeasurement.h"
#include "apl/engine/properties.h"
#include "apl/engine/context.h"
#include "apl/engine/recalculatetarget.h"
#include "apl/primitives/keyboard.h"
#include "apl/utils/range.h"

namespace apl {

class ComponentPropDef;
class ComponentPropDefSet;
class LayoutRebuilder;
class Pointer;
struct PointerEvent;

extern const std::string VISUAL_CONTEXT_TYPE_MIXED;
extern const std::string VISUAL_CONTEXT_TYPE_GRAPHIC;
extern const std::string VISUAL_CONTEXT_TYPE_TEXT;
extern const std::string VISUAL_CONTEXT_TYPE_VIDEO;
extern const std::string VISUAL_CONTEXT_TYPE_EMPTY;

inline float nonNegative(float value) { return value < 0 ? 0 : value; }

using EventPropertyGetter = std::function<Object(const CoreComponent*)>;
using EventPropertyMap = std::map<std::string, EventPropertyGetter>;

inline EventPropertyMap eventPropertyMerge(const EventPropertyMap& first, EventPropertyMap&& second) {
    second.insert(first.begin(), first.end());
    return std::move(second);
}

/**
 * The native interface to a primitive APL Component.
 *
 * This class is exposed to the view host layer.  The component hierarchy is automatically
 * inflated by the RootContext class and the top-level component is available through
 * that root.  The view host layer should walk the component hierarchy and create native
 * views as necessary to render each component.
 *
 * The position of the component within its container is accessed through the "bounds"
 * property.  This position is defined in display-independent pixels (or points).
 *
 * The dirty flag will be set when one or more output properties of the component have
 * changed. The dirty flags must be explicitly cleared.  Note that the dirty flag is
 * only set for an *output* property change.
 */
class CoreComponent : public Component,
                      public RecalculateTarget<PropertyKey> {

public:
    CoreComponent(const ContextPtr& context,
                  Properties&& properties,
                  const std::string& path);

    virtual ~CoreComponent() {
        YGNodeFree(mYGNodeRef);  // TODO: Check to make sure we're deallocating correctly
    }

    /**
     * Release this component and all children.  This component may still be in
     * its parents child list.
     */
    void release() override;

    /**
     * Visitor pattern for walking the component hierarchy. We are interested in the components
     * that the user can see/interact with.  Overrides that have knowledge about which children are off screen or otherwise
     * invalid/unattached should use that knowledge to reduce the number of nodes walked or avoid walking otherwise invalid
     * components they may have stashed in their children.
     * @param visitor
     */
    virtual void accept(Visitor<CoreComponent>& visitor) const;

    /**
     * Visitor pattern for walking the component hierarchy in reverse order.  We are interested in the components
     * that the user can see/interact with.  Overrides that have knowledge about which children are off screen or otherwise
     * invalid/unattached should use that knowledge to reduce the number of nodes walked or avoid walking otherwise invalid
     * components they may have stashed in their children.
     * @param visitor
     */
    virtual void raccept(Visitor<CoreComponent>& visitor) const;

    /**
     * Find a component at or below this point in the hierarchy with the given id or uniqueId.
     * @param id The id or uniqueId to search for.
     * @return The component or nullptr if it is not found.
     */
    ComponentPtr findComponentById(const std::string& id) const override;

    /**
     * Find a visible component at or below this point in the hierarchy containing the given position.
     * @param position The position expressed in component-relative coordinates.
     * @return The component or nullptr if it is not found
     */
    ComponentPtr findComponentAtPosition(const Point& position) const override;

    /**
     * @return The number of children.
     */
    size_t getChildCount() const override { return mChildren.size(); }

    /**
     * Retrieve a child at an index.  Throws an exception if out of bounds.
     * @param index The zero-based index of the child.
     * @return The child.
     */
    ComponentPtr getChildAt(size_t index) const override { return mChildren.at(index); }

    /**
     * Convenience routine for internal methods that don't want to write a casting
     * operation on the returned child from getChildAt()
     * @param index The index of the child.
     * @return The child
     */
    CoreComponentPtr getCoreChildAt(size_t index) const { return mChildren.at(index); }

    // Documentation from component.h
    bool insertChild(const ComponentPtr& child, size_t index) override {
        return canInsertChild() && insertChild(child, index, true);
    }

    // Documentation from component.h
    bool appendChild(const ComponentPtr& child) override {
        return canInsertChild() && appendChild(child, true);
    }

    // Documentation from component.h
    bool remove() override;

    // Documentation will be inherited
    bool canInsertChild() const override {
        // Child insertion is permitted if (a) there isn't a layout rebuilder and (b) there is space for a child.
        return !mRebuilder && ((singleChild() && mChildren.empty()) || multiChild());
    }

    // Documentation will be inherited
    bool canRemoveChild() const override {
        // Child removal is permitted if (a) there isn't a layout rebuilder and (b) there is at least one child
        return !mRebuilder && !mChildren.empty();
    }

    // Documentation will be inherited
    void update(UpdateType type, float value) override;

    // Documentation will be inherited
    void update(UpdateType type, const std::string& value) override;

    /**
     * Set the value of a component property by key. This method is commonly invoked
     * by the "SetValue" command.
     * @param key The property to set.
     * @param value The value to set.
     * @return True if this was a valid property that could be set on this component.
     *         Note that this will return true for valid properties even if the actual
     *         property value did not change.
     */
    bool setProperty(PropertyKey key, const Object& value);

    /**
     * Set the value of component property by name.  This generalized method may
     * set component properties, but it can also be used to modify data binding.
     * This method is normally called only by commands that change property values.
     * @param key The property or data-binding to set.
     * @param value The value to set.
     */
    void setProperty(const std::string& key, const Object& value);

    /**
     * Return true if this property has been assigned for this component.  Normally
     * used to see if a command handler needs to be attached.  For example, if
     * hasProperty(kCommandPropertyOnPress) returns true, then the rendering layer
     * should check for press events and pass them to the component.
     * @param key The property key to inspect.
     * @return True if this property key has an assigned value.
     */
    bool hasProperty(PropertyKey key) const { return mAssigned.count(key) != 0; }

    /**
     * Mark a property as being changed.  This only applies to properties set to
     * mutable arguments such as transformations.
     * @param key The property key to mark.
     */
    void markProperty(PropertyKey key);

    /**
     * A context that this property depends upon has changed value.  Update the
     * value of the property and set the dirty flag if it changes.
     * @param key The property to recalculate
     * @param value The new value to assign
     */
    void updateProperty(PropertyKey key, const Object& value);

    /**
     * Change the state of the component.  This may trigger a style change in
     * this component or a descendant.
     * @param stateProperty The state property to modify.
     * @param value The new value of the state property.
     */
    void setState(StateProperty stateProperty, bool value);

    /**
     * @return The current state of the component.
     */
    const State& getState() const { return mState; }

    /**
     * Mark a property key as dirty
     * @param key The key to mark
     */
    void setDirty(PropertyKey key);

    /**
     * @return The current parent of this component.  May be nullptr.
     */
    ComponentPtr getParent() const override { return mParent; }

    /**
     * Call this to ensure that the component has a layout.  This method must be used by
     * children of a sequence to before retrieving the layout bounds.
     */
    void ensureLayout(bool useDirtyFlag) override;

    /**
     * Run measure and layout on this component.  This method should not be used
     * by the view host layer.
     * @param width Target width.
     * @param height Target height.
     * @param useDirtyFlag If true, set dirty properties if the layout changes.
     */
    void layout(int width, int height, bool useDirtyFlag);

    /**
     * @return True if the yoga node needs to run a layout pass.
     */
    bool needsLayout() const;

    /**
     * @return inheritParentState property
     */
    bool getInheritParentState() const { return mInheritParentState; }

    /**
     * @return The value for this component.  Used by the SendEvent "components" array.
     */
    virtual Object getValue() const { return Object::NULL_OBJECT(); }

    /**
     * Retrieve an event property by key value (e.g., "event.target.uid").
     * @return A pair where the first value is true if the property was found and the second value is the
     *         value of the property.
     */
    std::pair<bool, Object> getEventProperty(const std::string& key) const;

    /**
     * @return The number of event properties (e.g., "event.target.XXX" has some number of XXX values).
     */
    size_t getEventPropertySize() const;

    /**
     * The component hierarchy signature is a unique text string that represents the type
     * of this component and all of the components below it in the hierarchy.  This signature
     * in mainly intended for use in recycling views where native layouts are re-used for
     * new component hierarchies.
     *
     * @return A unique signature of the component hierarchy
     */
    std::string getHierarchySignature() const override;

    /**
     * Update the output transformation
     */
    void fixTransform(bool useDirtyFlag);

    /**
     * Calculate component's relative visibility.
     * @param parentRealOpacity cumulative opacity value
     * @param parentVisibleRect component visible rect
     */
    float calculateVisibility(float parentRealOpacity, const Rect& parentVisibleRect);

    /**
     * Calculate component visible rect.
     * @param parentVisibleRect parents visible rect.
     * @return visible rectangle of component
     */
    Rect calculateVisibleRect(const Rect& parentVisibleRect);

    /**
     * Create the default event data-binding context for this component.
     * @param handler Handler name.
     * @return The event data-binding context.
     */
    ContextPtr createDefaultEventContext(const std::string& handler) const {
        return createEventContext(handler);
    }

    /**
     * Convert this component into a JSON object
     * @param allocator
     * @return The object.
     */
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override;

    /**
     * Convert this component and all of its properties into a human-readable JSON object
     * @param allocator
     * @return The object
     */
    rapidjson::Value serializeAll(rapidjson::Document::AllocatorType& allocator) const override;

    /**
     * Convert the dirty properties of this component into a JSON object.
     * @param allocator
     * @return The obje
     */
    rapidjson::Value serializeDirty(rapidjson::Document::AllocatorType& allocator) override;

    // Documentation from component.h
    std::string provenance() const override { return mPath; };

    /**
     * Retrieve component's visual context as a JSON object.
     */
    rapidjson::Value serializeVisualContext(rapidjson::Document::AllocatorType& allocator) override;

    /**
     * Serialize the event portion of this component
     */
    virtual void serializeEvent(rapidjson::Value& out, rapidjson::Document::AllocatorType& allocator) const;

    /**
     * Set the height dimension for this component.
     */
    virtual void setHeight(const Dimension& height);

    /**
     * Set the width dimension for this component.
     */
    virtual void setWidth(const Dimension& width);

    /**
     * @return True if this component supports a single child
     */
    virtual bool singleChild() const { return false; }

    /**
     * @return True if this component supports more than one child
     */
    virtual bool multiChild() const { return false; }

    /**
     * @return True if this component is scrollable
     */
    virtual bool scrollable() const { return false; }

    /**
     * Execute any "onBlur" commands associated with this component.  These commands
     * will be run in fast mode.
     */
    virtual void executeOnBlur() {}

    /**
     * Execute any "onFocus" commands associated with this component.  These commands
     * will be run in fast mode.
     */
    virtual void executeOnFocus() {}

    /**
     * Execute any "onCursorEnter" commands associated with this component.  These commands
     * will be run in fast mode.
     */
    virtual void executeOnCursorEnter();

    /**
     * Execute any "onCursorExit" commands associated with this component.  These commands
     * will be run in fast mode.
     */
    virtual void executeOnCursorExit();

    /**
     * Execute the component key handlers if present.
     * @param type The key handler type (up/down).
     * @param keyboard The keyboard update.
     * @return True, if consumed.
     */
    virtual bool executeKeyHandlers(KeyHandlerType type, const ObjectMapPtr& keyboard) { return false; };

    /**
     * Execute the intrinsic actions for given keys if appropriate.  These key updates are not forwarded to the document.
     * @param type The key handler type (up/down).
     * @param keyboard The keyboard update.
     * @return True, if consumed.
 */
    virtual bool executeIntrinsicKeyHandlers(KeyHandlerType type, const ObjectMapPtr& keyboard) { return false; };

    /**
     * Create an event data-binding context. Standard value of component will be used unless explicitly specified.
     * @param handler Handler name.
     * @param optional Optional key->value map to include on event context.
     * @param value override for standard component value.
     * @result The event data-binding context.
     */
    ContextPtr createEventContext(const std::string& handler,
                                  const ObjectMapPtr& optional = nullptr,
                                  const Object& value = Object::NULL_OBJECT()) const;

    /**
     * Create the keyboard event data-binding context for this component.
     * @param handler Handler name.
     * @param keyboard The keyboard update.
     * @return The event data-binding context.
     */
    ContextPtr createKeyboardEventContext(const std::string& handler, const ObjectMapPtr& keyboard) const;

    virtual const ComponentPropDefSet& propDefSet() const;

    /**
     * Common initialization method.  This is called right after the component is created with a shared pointer.
     */
    virtual void initialize();

    /**
     * Initial property assignment at component inflation. If you override this, be
     * sure to call the base class.
     * @param propDefSet The property definitions for this component.
     */
    virtual void assignProperties(const ComponentPropDefSet& propDefSet);

    /**
     * Walk the hierarchy updating child boundaries.
     * @param useDirtyFlag
     */
    virtual void processLayoutChanges(bool useDirtyFlag);

    /**
     * Update the event object map with additional properties.  These fill out "event.XXX" values other
     * than the "event.source" and "event.target" properties. Subclasses should call the parent class
     * to fill out the object map.
     * @param event The object map to populate.
     */
    virtual void addEventProperties(ObjectMap& event) const {}

    /**
     * @return The current calculated style.  This may be null.
     */
    const StyleInstancePtr getStyle() const;

    /**
     * Update the style of the component
     */
    virtual void updateStyle();

    /**
     * Attach Component's visual context tags to provided Json object.
     * NOTE: Required to be called explicitly from overriding methods.
     * @param outMap object to add tags to.
     * @param allocator
     * @return true if actionable, false otherwise
     */
    virtual bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator);

    /**
     * @return true if component could be focused.
     */
    virtual bool isFocusable() const { return false; }

    /**
     * @return true if component can react to pointer events.
     */
    virtual bool isTouchable() const { return false; }


    /**
     * Get visible children of component and respective visibility values.
     * @param realOpacity cumulative opacity.
     * @param visibleRect component's visible rect.
     * @return Map of visible children indexes to respective visibility values.
     */
    virtual std::map<int, float> getChildrenVisibility(float realOpacity, const Rect& visibleRect) const;

    /**
     * @return Type of visual context.
     */
    virtual std::string getVisualContextType();

    /**
     * Calculate visual layer.
     * @param visibleIndexes Map of visible children indexes to respective visibility values.
     * @param visibleRect component's visible rect.
     * @param visualLayer component's visual layer.
     * @return map from child index to it's visual layer value.
     */
    virtual std::map<int, int> calculateChildrenVisualLayer(const std::map<int, float>& visibleIndexes,
                                                            const Rect& visibleRect, int visualLayer);

    /**
    * Calculate real opacity of component.
    * @param parentRealOpacity parent component real opacity.
    * @return component cumulative opacity.
    */
    float calculateRealOpacity(float parentRealOpacity);

    /**
     * Calculate real opacity of component.
     * Note: it's recursive so better to utilize @see calculateRealOpacity(float parentRealOpacity) when possible.
     * @return component cumulative opacity.
     */
    float calculateRealOpacity();

    /**
     * Calculate component visible rect.
     * Note: it's recursive so better to utilize @see calculateVisibleRect(const Rect& parentVisibleRect) when possible.
     * @return visible rectangle of component
     */
    Rect calculateVisibleRect();

    /**
     * @param index index of child.
     * @return True if child should be automatically Yoga-attached to this component
     */
    virtual bool shouldAttachChildYogaNode(int index) const { return true; }

    /**
     * Checks to see if this Component inherits state from another Component. State
     * is inherited if compare Component is an ancestor, and inheritParentState = true for this Component
     * and any ancestor up to the compare Component.
     * @param component The component to compare to.
     * @return True if this component equals the component, or inherits it's state from this component.
    */
    bool inheritsStateFrom(const CoreComponentPtr& component);

    /**
     * Finds the Component that owns the state for this Component. If this Component
     * inheritParentState = true, the parent Component hierarchy is walked until the
     * first Component not inheritingParentState is found.
     * @return The Component responsible for the state for this Component.
     */
    CoreComponentPtr findStateOwner();

    /**
     * Check if component attached to yoga hierarchy.
     */
    bool isAttached() const;

    static void resolveDrawnBorder(Component& component);

    void calculateDrawnBorder(bool useDirtyFlag);

    /**
     * @param position Point in global coordinates.
     * @return Whether that point is within the bounds of this component.
     */
    bool containsGlobalPosition(const Point& position) const;

    /**
     * Update the spacing to specified value if any.
     * @param reset Reset spacing to 0.
     */
    void fixSpacing(bool reset = false);

    /**
     * @return Yoga node reference for the component.
     */
    YGNodeRef getNode() const { return mYGNodeRef; }

    /**
     * Call this method to get shared ptr of CoreComponent
     * @return shared ptr of type CoreComponent.
     */
    std::shared_ptr<CoreComponent> shared_from_corecomponent()
    {
        return std::static_pointer_cast<CoreComponent>(shared_from_this());
    }

    /**
     * Call this method to get shared ptr of const CoreComponent
     * @return shared ptr of type const CoreComponent.
     */
    std::shared_ptr<const CoreComponent> shared_from_corecomponent() const
    {
        return std::static_pointer_cast<const CoreComponent>(shared_from_this());
    }

    /**
     * This inline function casts YGMeasureMode enum to MeasureMode enum.
     * @param YGMeasureMode
     * @return MeasureMode
     */
    static inline MeasureMode toMeasureMode(YGMeasureMode ygMeasureMode)
    {
        switch(ygMeasureMode){
            case YGMeasureModeExactly:
                return Exactly;
            case YGMeasureModeAtMost:
                return AtMost;
                //default case will execute when mode is YGMeasureModeUndefined as well as other than specified cases in case of Yoga lib update.
            default:
                return Undefined;
        }
    }

    /**
     * This inline function used in TextComponent and EditTextComponent class for TextMeasurement.
     * @param YGNodeRef node
     * @param float width
     * @param YGMeasureMode widthMode
     * @param float height
     * @param YGMeasureMode heightMode
     * @return YGSize
     */
    template <class T>
    static inline YGSize
    textMeasureFunc( YGNodeRef node,
                     float width,
                     YGMeasureMode widthMode,
                     float height,
                     YGMeasureMode heightMode )
    {
        T *component = static_cast<T*>(node->getContext());
        assert(component);

        LayoutSize layoutSize = component->getContext()->measure()->measure(component, width, toMeasureMode(widthMode), height, toMeasureMode(heightMode));
        return YGSize({layoutSize.width, layoutSize.height});
    }

    /**
     * This inline function used in TextComponent and EditTextComponent class for TextMeasurement.
     * @param YGNodeRef node
     * @param float width
     * @param float height
     * @return float
     */
    template <class T>
    static inline float
    textBaselineFunc( YGNodeRef node, float width, float height )
    {
        T *component = static_cast<T*>(node->getContext());
        assert(component);

        return component->getContext()->measure()->baseline(component, width, height);
    }

protected:
    // internal, do not call directly
    virtual bool insertChild(const ComponentPtr& child, size_t index, bool useDirtyFlag);
    virtual void removeChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag);
    virtual void reportLoaded(size_t index);

    void ensureChildAttached(const CoreComponentPtr& child, int targetIdx);

    virtual const EventPropertyMap& eventPropertyMap() const;

private:
    friend streamer& operator<<(streamer&, const Component&);

    friend class Builder;
    friend class LayoutRebuilder;
    friend class ChildWalker;

    void ensureChildAttached(const CoreComponentPtr& child);

    bool attachChild(const CoreComponentPtr& child, size_t index);

    bool appendChild(const ComponentPtr& child, bool useDirtyFlag);

    void attachedToParent(const CoreComponentPtr& parent);

    void removeChild(const CoreComponentPtr& child, bool useDirtyFlag);

    void removeChildAt(size_t index, bool useDirtyFlag);

    void markRemoved();

    void markAdded();

    void updateInheritedState();

    bool setPropertyInternal(const ComponentPropDefSet& propDefSet,
                             PropertyKey key, const Object& value);

    virtual bool setPropertyInternal(const std::string& key, const Object& value) { return false; }

    bool markPropertyInternal(const ComponentPropDefSet& propDefSet, PropertyKey key);

    void handlePropertyChange(const ComponentPropDef& def, const Object& value);

    void updateMixedStateProperty(PropertyKey key, bool value);

    const ComponentPropDefSet* getLayoutPropDefSet() const;

    void updateStyleInternal(const StyleInstancePtr& stylePtr, const ComponentPropDefSet& propDefSet);

    virtual const ComponentPropDefSet* layoutPropDefSet() const { return nullptr; };

    void updateNodeProperties();

    void serializeVisualContextInternal(rapidjson::Value& outArray, rapidjson::Document::AllocatorType& allocator,
                                        float realOpacity, float visibility, const Rect& visibleRect, int visualLayer);

    void attachRebuilder(const std::shared_ptr<LayoutRebuilder>& rebuilder) { mRebuilder = rebuilder; }

    std::shared_ptr<ObjectMap> createEventProperties(const std::string& handler, const Object& value) const;

    void notifyChildChanged(size_t index, const std::string& uid, const std::string& action);

    virtual void finalizePopulate() {}

    void attachYogaNodeIfRequired(const CoreComponentPtr& coreChild, int index);

    void scheduleTickHandler(const Object& handler, double delay);
    void processTickHandlers();

protected:
    bool                             mInheritParentState;
    State                            mState;       // Operating state (pressed, checked, etc)
    std::string                      mStyle;       // Name of the current STYLE
    Properties                       mProperties;  // Assigned properties from JSON
    std::set<PropertyKey>            mAssigned;    // Properties that have been assigned from JSON or SetValue
    std::vector<CoreComponentPtr>    mChildren;
    CoreComponentPtr                 mParent;
    YGNodeRef                        mYGNodeRef;
    std::string                      mPath;
    std::shared_ptr<LayoutRebuilder> mRebuilder;
    Range                            mEnsuredChildren;
};

}  // namespace apl

#endif //_APL_CORE_COMPONENT_H
