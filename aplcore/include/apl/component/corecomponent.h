/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/component/component.h"
#include "apl/engine/properties.h"
#include "apl/engine/context.h"
#include "apl/engine/recalculatetarget.h"
#include "apl/primitives/keyboard.h"

namespace apl {

class ComponentPropDef;
class ComponentPropDefSet;

extern const std::string VISUAL_CONTEXT_TYPE_MIXED;
extern const std::string VISUAL_CONTEXT_TYPE_GRAPHIC;
extern const std::string VISUAL_CONTEXT_TYPE_TEXT;
extern const std::string VISUAL_CONTEXT_TYPE_VIDEO;
extern const std::string VISUAL_CONTEXT_TYPE_EMPTY;

inline float nonNegative(float value) { return value < 0 ? 0 : value; }

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
                      public RecalculateTarget<PropertyKey>,
                      public std::enable_shared_from_this<CoreComponent> {

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
     * Visitor pattern for walking the component hierarchy.
     * @param visitor
     */
    void accept(Visitor<Component>& visitor) const override;

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
        return insertChild(child, index, true);
    }

    // Documentation from component.h
    bool appendChild(const ComponentPtr& child) override {
        return appendChild(child, true);
    }

    // Documentation from component.h
    bool remove() override;

    // Documentation will be inherited
    void update(UpdateType type, float value) override;

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
     * A context that this property depends upon has changed value.  Recalculate the
     * value of the property
     * @param key The property to recalculate
     */
    void recalculateProperty(PropertyKey key);

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
     * Get Component target properties with values.
     * @return Component dynamic properties with values.
     * @deprecated Hide this method
     */
    virtual std::shared_ptr<ObjectMap> getEventTargetProperties() const;

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
        return createEventContext(handler, getValue());
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
     * @return True if this component supports a single child
     */
    virtual bool singleChild() const { return false; }

    /**
     * @return True if this component supports more than one child
     */
    virtual bool multiChild() const { return false; }

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
     * Execute the component "handleKeyDown" key handlers if present.
     * @param type The key handler.
     * @param keyboard The keyboard update.
     * @return True, if consumed.
     */
    virtual bool executeKeyHandlers(KeyHandlerType type, const ObjectMapPtr& keyboard) {return false;};

    /**
     * Create an event data-binding context, passing in the "value" to be used for this component.
     * @param handler Handler name.
     * @result The event data-binding context.
     */
    ContextPtr createEventContext(const std::string& handler, const Object& value) const;

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
     * Update the event object map with source properties suitable for the event.source.XX
     * context. Subclasses should call the parent class to fill out the object map.
     * @param event The object map to populate.
     */
    virtual void addEventSourceProperties(ObjectMap& event) const {}

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
     * Get visible children of component and respective visibility values.
     * @param realOpacity cumulative opacity.
     * @param visibleRect component's visible rect.
     * @return Map of visible children indexes to respective visibility values.
     */
    virtual std::map<int, float> getChildrenVisibility(float realOpacity, const Rect& visibleRect);

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
     * @return True if children should be automatically Yoga-attached to this component
     */
    virtual bool alwaysAttachChildYogaNode() const { return true; }

    /**
     * Ensure provided child is attached to component Yoga tree.
     * @param child Child to attach.
     */
    virtual void ensureChildAttached(const ComponentPtr& child);

    /**
     * Find and return a child component at the local coordinates.
     * @param position The position in local coordinates.
     * @return The child containing that position or nullptr;
     */
    virtual ComponentPtr findChildAtPosition(const Point& position) const;

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

private:
    friend streamer& operator<<(streamer&, const Component&);

    friend class Builder;

    bool insertChild(const ComponentPtr& child, size_t index, bool useDirtyFlag);

    bool appendChild(const ComponentPtr& child, bool useDirtyFlag);

    void attachedToParent(const CoreComponentPtr& parent);

    void removeChild(const CoreComponentPtr& child, bool useDirtyFlag);

    void markRemoved();

    void markAdded();

    void updateInheritedState();

    bool setPropertyInternal(const ComponentPropDefSet& propDefSet,
                             PropertyKey key, const Object& value);

    virtual bool setPropertyInternal(const std::string& key, const Object& value) { return false; }

    bool markPropertyInternal(const ComponentPropDefSet& propDefSet, PropertyKey key);

    bool recalculatePropertyInternal(const ComponentPropDefSet& propDefSet, PropertyKey key, const Object& node);

    void handlePropertyChange(const ComponentPropDef& def, const Object& value);

    void updateMixedStateProperty(PropertyKey key, bool value);

    const ComponentPropDefSet* getLayoutPropDefSet() const;

    void updateStyleInternal(const StyleInstancePtr& stylePtr, const ComponentPropDefSet& propDefSet);

    virtual const ComponentPropDefSet* layoutPropDefSet() const { return nullptr; };

    bool isAttached() const;

    void updateNodeProperties();

    void serializeVisualContextInternal(rapidjson::Value& outArray, rapidjson::Document::AllocatorType& allocator,
                                        float realOpacity, float visibility, const Rect& visibleRect, int visualLayer);


private:
    YGNodeRef getNode() const { return mYGNodeRef; }

    std::shared_ptr<ObjectMap> createEventProperties(const std::string& handler, const Object& value) const;


protected:
    bool                           mInheritParentState;
    State                          mState;       // Operating state (pressed, checked, etc)
    std::string                    mStyle;       // Name of the current STYLE
    Properties                     mProperties;  // Assigned properties from JSON
    std::map<PropertyKey, Object>  mAssigned;    // Assigned properties from either JSON or SetValue
    std::vector<CoreComponentPtr>  mChildren;
    CoreComponentPtr               mParent;
    YGNodeRef                      mYGNodeRef;
    std::string                    mPath;

};

}  // namespace apl

#endif //_APL_CORE_COMPONENT_H
