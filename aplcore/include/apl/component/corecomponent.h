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

#include "apl/apl_config.h"

#include "apl/component/component.h"
#include "apl/component/textmeasurement.h"
#include "apl/engine/context.h"
#include "apl/engine/properties.h"
#include "apl/engine/recalculatetarget.h"
#include "apl/focus/focusdirection.h"
#include "apl/primitives/keyboard.h"
#include "apl/primitives/size.h"
#include "apl/primitives/transform2d.h"
#include "apl/utils/flags.h"

#ifdef SCENEGRAPH
#include "apl/scenegraph/common.h"
#endif // SCENEGRAPH

namespace apl {

class ComponentPropDef;
class ComponentPropDefSet;
class LayoutRebuilder;
class Pointer;
struct PointerEvent;
class StickyChildrenTree;

using ConstComponentPropIterator = std::map<PropertyKey, ComponentPropDef>::const_iterator;

extern const std::string VISUAL_CONTEXT_TYPE_MIXED;
extern const std::string VISUAL_CONTEXT_TYPE_GRAPHIC;
extern const std::string VISUAL_CONTEXT_TYPE_TEXT;
extern const std::string VISUAL_CONTEXT_TYPE_VIDEO;
extern const std::string VISUAL_CONTEXT_TYPE_EMPTY;

inline float nonNegative(float value) { return value < 0 ? 0 : value; }
inline float nonPositive(float value) { return value > 0 ? 0 : value; }

using EventPropertyGetter = std::function<Object(const CoreComponent*)>;
using EventPropertyMap = std::map<std::string, EventPropertyGetter>;

inline EventPropertyMap eventPropertyMerge(const EventPropertyMap& first, EventPropertyMap&& second) {
    second.insert(first.begin(), first.end());
    return std::move(second);
}

enum PointerCaptureStatus {
    /// The pointer has not been captured by any component
    kPointerStatusNotCaptured,
    /// The pointer has been captured by a component
    kPointerStatusCaptured,
    /// A component wants to capture the pointer, but is allowing other components to process the
    // same pointer event first
    kPointerStatusPendingCapture
};

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
                  const Path& path);

    virtual ~CoreComponent() {
        YGNodeFree(mYGNodeRef);  // TODO: Check to make sure we're deallocating correctly
    }

    /**
     * Release this component and all children.  This component may still be in
     * its parent's child list.
     */
    void release() override;

    /**
     * Clear any active component state. This may include animations/timers/caches/etc.
     */
    void clearActiveState();

    /**
     * Visitor pattern for walking the component hierarchy. We are interested in the components
     * that the user can see/interact with.  Overrides that have knowledge about which children are off screen or otherwise
     * invalid/unattached should use that knowledge to reduce the number of nodes walked or avoid walking otherwise invalid
     * components they may have stashed in their children.
     * @param visitor The component visitor
     */
    virtual void accept(Visitor<CoreComponent>& visitor) const;

    /**
     * Visitor pattern for walking the component hierarchy in reverse order.  We are interested in the components
     * that the user can see/interact with.  Overrides that have knowledge about which children are off screen or otherwise
     * invalid/unattached should use that knowledge to reduce the number of nodes walked or avoid walking otherwise invalid
     * components they may have stashed in their children.
     * @param visitor The component visitor
     */
    virtual void raccept(Visitor<CoreComponent>& visitor) const;

    /**
     * Find a component at or below this point in the hierarchy with the given id or uniqueId.
     * This variant of findComponentById must only be used by clients of Core, and not Core itself.
     * Core code must use the variant accepting 'traverseHost', setting 'traverseHost' to false.
     * @param id The id or uniqueId to search for.
     * @return The component or nullptr if it is not found.
     * @deprecated Use @see CoreComponent::findComponentById(const std::string&, bool)
     */
    APL_DEPRECATED ComponentPtr findComponentById(const std::string& id) const override;

    /**
     * Special variant of findComponentId providing a signal to HostComponent indicating whether or
     * not the 'child' of the HostComponent should be included in the search.
     * @param id The id or uniqueId to search for.
     * @param traverseHost the 'child' of HostComponent will be included iff true
     * @return The component or nullptr if it is not found.
     */
    virtual ComponentPtr findComponentById(const std::string& id, bool traverseHost) const;

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
    ComponentPtr getChildAt(size_t index) const override { return std::static_pointer_cast<Component>(mChildren.at(index)); }

    /**
     * Return the index of a particular child.
     * @param child The child to search for.
     * @return The index of that child or -1 if it is not found
     */
    int getChildIndex(const CoreComponentPtr& child) const {
        auto it = std::find(mChildren.begin(), mChildren.end(), child);
        return it != mChildren.end() ? static_cast<int>(std::distance(mChildren.begin(), it)) : -1;
    }

    /**
     * Convenience routine for internal methods that don't want to write a casting
     * operation on the returned child from getChildAt()
     * @param index The index of the child.
     * @return The child
     */
    CoreComponentPtr getCoreChildAt(size_t index) const { return mChildren.at(index); }

    /**
     * Marks the display of this component as stale, and thus needing to be recalculated
     * at the next opportunity.
     *
     * This only marks the current component as stale and not any of its children.
     */
    void markDisplayedChildrenStale(bool useDirtyFlag);

    /**
     * Check if child of a components is in the list of displayed children.
     * @param child Component's child.
     * @return true if displayed, false otherwise.
     */
    bool isDisplayedChild(const CoreComponent& child) const;

    /// Component overrides
    size_t getDisplayedChildCount() const override;
    ComponentPtr getDisplayedChildAt(size_t drawIndex) const override;
    bool insertChild(const ComponentPtr& child, size_t index) override;
    bool appendChild(const ComponentPtr& child) override;
    bool remove() override;
    bool remove(bool useDirtyFlag);
    bool canInsertChild() const override {
        // Child insertion is permitted if (a) there isn't a layout rebuilder and (b) there is space for a child.
        return !mRebuilder && ((singleChild() && mChildren.empty()) || multiChild());
    }
    bool canRemoveChild() const override {
        // Child removal is permitted if (a) there isn't a layout rebuilder and (b) there is at least one child
        return !mRebuilder && !mChildren.empty();
    }
    void update(UpdateType type, float value) override;
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
     * Return the value and writeable state of a component property.  This is the opposite of the
     * setProperty(const std::string& key, const Object& value) method.  It checks for (a) intrinsic
     * component properties [such as "width"], (b) data-bindings accessible to the component, and
     * (c) internal component properties that are exposed (currently just the parameters passed
     * to a vector graphic).  This method is used by animators to read the current value of a
     * property and to verify that the property can be written with setProperty().
     * @param key The name of the property
     * @return The current value of the property and true if the property is writeable.  If the
     *         property does not exist, returns null and false.
     */
    std::pair<Object, bool> getPropertyAndWriteableState(const std::string& key) const;

    /**
     * Return the value of a component property.  This is the opposite of the
     * setProperty(const std::string& key, const Object& value) method.  It checks for (a) intrinsic
     * component properties [such as "width"], (b) data-bindings accessible to the component, and
     * (c) internal component properties that are exposed (currently just the parameters passed
     * to a vector graphic). This method is used by the "preserve" property to save existing
     * property values.
     * @param key The property or data-binding to retrieve
     * @return The value or null if the property does not exist
     */
    Object getProperty(const std::string& key) const { return getPropertyAndWriteableState(key).first; }

    /**
     * Return the value of a component property.  This is the opposite of the
     * setProperty(PropertyKey key, const Object& value) method.
     * @param key The property to retrieve
     * @return The value or null if the property does not exist
     */
    Object getProperty(PropertyKey key) const;

    /**
     * Mark a property as being changed.  This only applies to properties set to
     * mutable arguments such as transformations.
     * @param key The property key to mark.
     */
    void markProperty(PropertyKey key);

    void setValue(PropertyKey key, const Object& value, bool useDirtyFlag) override;

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

    const std::set<PropertyKey>& getDirty() override;
    void clearDirty() override;

    /**
     * Check to see if a property has been marked as dirty
     * @param key The key to check
     * @return True if the property is dirty
     */
    bool isDirty(PropertyKey key) const { return mDirty.find(key) != mDirty.end(); }

    /**
     * @return The current parent of this component.  May be nullptr.
     */
    ComponentPtr getParent() const override { return std::static_pointer_cast<Component>(mParent); }

    /**
     * @return The current parent of this component if it is in the same document. May be nullptr.
     */
    CoreComponentPtr getParentIfInDocument() const;

    /**
     * Guarantees that this component has been laid out, so that layout bounds are fully calculated.
     * This method will conduct a full layout pass if it is required, which is expensive, so avoid
     * calling this method unless you absolutely must guarantee that a specific component has been laid out.
     */
    void ensureLayoutInternal(bool useDirtyFlag);

    /**
     * Guarantees that this component's child'has been laid out, so that layout bounds are fully
     * calculated.
     */
    virtual void ensureChildLayout(const CoreComponentPtr& child, bool useDirtyFlag);

    /**
     * @return True if the yoga node needs to run a layout pass.
     */
    bool needsLayout() const;

    /**
     * @return inheritParentState property
     */
    bool getInheritParentState() const { return mCoreFlags.isSet(kCoreComponentFlagInheritParentState); }

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
     * Return the event property at a given offset into the map.  This method is used when iterating
     * over the set of event properties.
     * @param index The offset of the property in the map.
     * @return A pair where the first value is the key and the second property is the event property value.
     */
    std::pair<std::string, Object> getEventPropertyAt(size_t index) const;

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
     * Update the padding
     */
    void fixPadding();

    /**
     * Update the output layoutDirection
     */
    void fixLayoutDirection(bool useDirtyFlag);

    /**
     * Calculate component's relative visibility.
     * @param parentRealOpacity cumulative opacity value
     * @param parentVisibleRect component visible rect
     */
    float calculateVisibility(float parentRealOpacity, const Rect& parentVisibleRect) const;

    /**
     * Calculate component visible rect.
     * @param parentVisibleRect parents visible rect.
     * @return visible rectangle of component
     */
    Rect calculateVisibleRect(const Rect& parentVisibleRect) const;

    /**
     * Create the default event data-binding context for this component.
     * @param handler Handler name.
     * @return The event data-binding context.
     */
    ContextPtr createDefaultEventContext(const std::string& handler) const {
        return createEventContext(handler);
    }

    /**
     *  Marks the visual context for this component, and the parent hierarchy, dirty. This method
     *  is called for a subset of property/state/hierarchy changes that impact the reported visual
     *  context.
     */
    void setVisualContextDirty();

    /**
     * Mark component visibility state as dirty.
     */
    void setVisibilityDirty();

    /**
     * Convert this component into a JSON object
     * @param allocator RapidJSON memory allocator
     * @return The object.
     */
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override;

    /**
     * Convert this component and all of its properties into a human-readable JSON object
     * @param allocator RapidJSON memory allocator
     * @return The object.
     */
    rapidjson::Value serializeAll(rapidjson::Document::AllocatorType& allocator) const override;

    /**
     * Convert the dirty properties of this component into a JSON object.
     * @param allocator RapidJSON memory allocator
     * @return The object.
     */
    rapidjson::Value serializeDirty(rapidjson::Document::AllocatorType& allocator) override;

    // Documentation from component.h
    std::string provenance() const override { return mPath.toString(); };

    /**
     * Return path object used to generate provenance.
     */
    Path getPathObject() const { return mPath; }

    /**
     * Retrieve component's visual context as a JSON object.
     * @deprecated use RootContext->serializeVisualContext()
     */
    rapidjson::Value serializeVisualContext(rapidjson::Document::AllocatorType& allocator) override;

    /**
     * Internal method.  Identifies when this components internal state, a property, or an update
     * caused a change that makes the RootContext visual context of the tree dirty.
     * @return true if the visual context has changed since the last call to
     * RootContext::serializeVisualContext, false otherwise.
     */
     bool isVisualContextDirty();

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
     * @return scrollables return the tree of children with position: sticky
     */
    virtual std::shared_ptr<StickyChildrenTree> getStickyTree() { return nullptr; }

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
     * Process key press targeted to the component.
     * @param type The key handler type (up/down).
     * @param keyboard The keyboard update.
     * @return True, if consumed.
     */
    virtual bool processKeyPress(KeyHandlerType type, const Keyboard& keyboard);

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
    ContextPtr createKeyEventContext(const std::string& handler, const ObjectMapPtr& keyboard) const;

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
     * Before layout even started we may need to process some of the property fixes/etc
     * @param useDirtyFlag true to notify runtime about changes with dirty properties
     */
    virtual void preLayoutProcessing(bool useDirtyFlag);

    /**
     * Walk the hierarchy updating child boundaries.
     * @param useDirtyFlag true to notify runtime about changes with dirty properties
     * @param first if this is the first layout for current template, false otherwise
     */
    virtual void processLayoutChanges(bool useDirtyFlag, bool first);

    /**
     * After a layout has been completed, call this to execute any actions that may occur after a layout
     * @param first if this is the first layout for current template, false otherwise
     */
    virtual void postProcessLayoutChanges(bool first);

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
    StyleInstancePtr getStyle() const;

    /**
     * Update the style of the component
     */
    virtual void updateStyle();

    /**
     * Attach Component's visual context tags to provided Json object.
     * NOTE: Required to be called explicitly from overriding methods.
     * @param outMap object to add tags to.
     * @param allocator RapidJSON memory allocator
     * @return true if actionable, false otherwise
     */
    virtual bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator);

    /**
     * @return true if component could be focused.
     */
    bool isFocusable() const override { return false; }

    /**
     * @return true if component can react to pointer events.
     */
    virtual bool isTouchable() const { return false; }

    /**
     * @return true if component can receive interactions.
     */
    virtual bool isActionable() const { return false; }

    /// Documentation inherited from component.h
    bool isAccessible() const override {
        return isFocusable() || !getCalculated(apl::kPropertyAccessibilityLabel).empty();
    }

    /**
     * Refresh accessibility actions set in case if any relevant parameters changed.
     */
    void refreshAccessibilityActions(bool useDirtyFlag);

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
    virtual std::string getVisualContextType() const;

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
     * @return true when the component has 'normal' display property and an
     * opacity greater than zero and is not disallowed.
     */
    bool isDisplayable() const;

    /**
     * @return true when the component has been disallowed by the runtime, false otherwise.
    */
    bool isDisallowed() const { return mCoreFlags.isSet(kCoreComponentFlagIsDisallowed); }

    /**
     * Calculate real opacity of component.
     * @param parentRealOpacity parent component real opacity.
     * @return component cumulative opacity.
     */
    float calculateRealOpacity(float parentRealOpacity) const;

    /**
     * Calculate real opacity of component.
     * Note: it's recursive so better to utilize @see calculateRealOpacity(float parentRealOpacity) when possible.
     * @return component cumulative opacity.
     */
    float calculateRealOpacity() const;

    /**
     * Calculate component visible rect.
     * Note: it's recursive so better to utilize @see calculateVisibleRect(const Rect& parentVisibleRect) when possible.
     * @return visible rectangle of component
     */
    Rect calculateVisibleRect() const;

    /**
     * @param index index of child.
     * @return True if child should be automatically Yoga-attached to this component
     */
    virtual bool shouldAttachChildYogaNode(int index) const { return true; }

    /**
     * @param index index of child.
     * @return True if component should be fully inflated. False if it should be left
     * up to lazy inflation controlled by parent component.
     */
    virtual bool shouldBeFullyInflated(int index) const { return true; }

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
     * Check if this component is attached to a yoga hierarchy.
     *
     * Note that depending on the components used, an APL document may be laid
     * out using multiple independent yoga trees, which can introduce
     * discontinuities in the overall hierarchy. For example, a component may
     * choose to treat each child as a top-level yoga node for layout purposes
     * (e.g. Pager). This has the effect that a child may not be "attached" to
     * its parent component (i.e. isAttached() will return false).
     *
     * @return true if the component's yoga node has a parent
     */
    bool isAttached() const;

    /**
     * Determines whether a component is laid out. This cannot be reliably used before the initial layout pass.
     *
     * @return True if the component is attached to the yoga hierarchy and there are no dirty ancestor nodes
     */
    bool isLaidOut() const;

    static void resolveDrawnBorder(Component& component);

    void calculateDrawnBorder(bool useDirtyFlag);

    /**
     * @param position Point in local coordinates.
     * @return Whether that point is within the bounds of this component.
     */
    bool containsLocalPosition(const Point& position) const;

    /**
     * Converts a point in global coordinates to this component's coordinate space. If the
     * conversion is not possible (e.g. due to a singular transform), returns a point with
     * Not-A-Number (NaN) coordinates.
     *
     * @param globalPoint A point in global coordinates.
     * @return  The computed point in local coordinate space, or @c Point(NaN, NaN) otherwise.
     */
    Point toLocalPoint(const Point& globalPoint) const;

    /**
     * Converts a point in local coordinates to global coordinates.  If the conversion
     * is not possible due to singularities, return a point with NaN coordinates.
     * @param position A point in local coordinates
     * @return The computed point in global coordinate space
     */
    Point localToGlobal(Point position) const override;

    /**
     * @return true if this component's bounds intersect with it's parent viewport.
     */
    bool inParentViewport() const;

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
     * @return Direction in which component is laid out.
     */
    YGDirection getLayoutDirection() const;

    /**
     * Call this method to get shared ptr of CoreComponent
     * @return shared ptr of type CoreComponent.
     */
    CoreComponentPtr shared_from_corecomponent()
    {
        return std::static_pointer_cast<CoreComponent>(shared_from_this());
    }

    /**
     * Call this method to get shared ptr of const CoreComponent
     * @return shared ptr of type const CoreComponent.
     */
    ConstCoreComponentPtr shared_from_corecomponent() const
    {
        return std::static_pointer_cast<const CoreComponent>(shared_from_this());
    }

    /**
     * Execute a given handler in the specified mode with any additional parameters required.
     * @param event Event handler name.
     * @param commands Array of commands to execute.
     * @param fastMode true for fast mode, false for normal.
     * @param optional Optional key->value map to include on event context.
     */
    void executeEventHandler(const std::string& event, const Object& commands, bool fastMode = true,
                             const ObjectMapPtr& optional = nullptr);

    /**
     * This inline function casts YGMeasureMode enum to MeasureMode enum.
     * @param ygMeasureMode Yoga definition of the measuring mode
     * @return APL definition of the measuring mode
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
     * @param node The yoga node
     * @param width Width in dp
     * @param widthMode Width measuring mode - at most, exactly, or undefined
     * @param height Height in dp
     * @param heightMode Height measuring mode - at most, exactly, or undefined
     * @return Size of measured text, in dp
     */
    static YGSize textMeasureFunc(YGNodeRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode);

    /**
     * This inline function used in TextComponent and EditTextComponent class for TextMeasurement.
     * @param node The yoga node
     * @param width The width
     * @param height The height
     * @return The baseline of the text in dp
     */
    static float textBaselineFunc(YGNodeRef node, float width, float height);

    /**
     * Executes a given handler by name with a specific position.
     * @param handlerKey The handler to execute.
     * @param point The Point at which the event that triggered this was created.
     */
    virtual void executePointerEventHandler(PropertyKey handlerKey, const Point& point) {}

    /**
     * Defer pointer processing to component.
     * @param event pointer event.
     * @param timestamp event timestamp.
     * @param onlyProcessGestures specify whether we only process gestures.
     * @return the status of the pointer following processing by this component
     */
    virtual PointerCaptureStatus processPointerEvent(const PointerEvent& event, apl_time_t timestamp, bool onlyProcessGestures);

    /**
     * @return The root configuration provided by the viewhost
     */
    const RootConfig& getRootConfig() const;

    /**
     * @return a @c Transform2D that maps global coordinates to this component's local coordinate space.
     */
    const Transform2D &getGlobalToLocalTransform() const;


    /**
     * Marks the local transform stored for this component as stale, and thus needing to be recalculated
     * at the next opportunity.
     *
     * This only marks the current component as stale and not any of its children. This is to avoid visiting a
     * potentially very large number of children at critical times. Instead, all components look for a stale
     * parent when they need to determine whether their own cached transform is stale. This has the advantage
     * of scaling with the depth of a component in the tree, and not the total size of the tree.
     */
    void markGlobalToLocalTransformStale() { mCoreFlags.set(kCoreComponentFlagGlobalToLocalIsStale); }

    /**
     * Check if component can consume focus event coming from particular direction (by taking focus or performing some
     * internal processing).
     * @param direction Focus movement direction.
     * @param fromInside true if check done by component's parent false otherwise.
     * @return true if event could be consumed, false otherwise.
     */
    virtual bool canConsumeFocusDirectionEvent(FocusDirection direction, bool fromInside) { return false; }

    /**
     * Process focus exit passed from component's child. Used in case if focus direction does not point to another
     * visible peer of this child and processing should be deferred to actionable parent that could bring some new
     * children in view.
     * @param direction focus movement direction.
     * @param origin origin rectangle relative to self.
     * @return component that should be focused.
     */
    virtual CoreComponentPtr takeFocusFromChild(FocusDirection direction, const Rect& origin) { return nullptr; }

    /**
     * Get next component to be focused based on component-specified preferences.
     * @param direction direction of search.
     * @return Found component if exists, nullptr otherwise.
     */
    virtual CoreComponentPtr getUserSpecifiedNextFocus(FocusDirection direction) { return nullptr; }

    /**
     * The Yoga Flexbox engine is called on the top component in the DOM and the children of a PagerComponent
     * with the size of the screen or PagerComponent.  The layout size used is cached with the component.  This
     * method returns that cached size.
     *
     * @return The layout size last used when laying out this component.  May be (0,0), which indicates that
     *         a new Flexbox layout pass is required if this component is the top of the DOM or a direct child
     *         of a PagerComponent.
     */
    Size getLayoutSize() const { return mLayoutSize; }

    /**
     * Set the cached layout size of the component.  This is only used with the top component in the DOM and
     * direct children of a PagerComponent.
     * @param layoutSize The layout size to cache.
     */
    void setLayoutSize(Size layoutSize) { mLayoutSize = layoutSize; }

    /**
     * Certain Flexbox layout properties (such as "spacing") may only be set on a component once the component's
     * Yoga node has been attached to its parent's Yoga node.  This method ensures that all node-dependent layout
     * properties have been set.
     */
    void updateNodeProperties();

    /**
     * Sets a calculated property value.
     *
     * @param key The key to set
     * @param value The property value
     */
    void setCalculated(PropertyKey key, const Object &value) { mCalculated.set(key, value); }

    /**
     * Get the offset applied to this component if it's position property is "sticky"
     */
    const Point& getStickyOffset() const { return mStickyOffset; }

    /**
     * Set the offset applied to this component if it's position property is "sticky"
     */
    void setStickyOffset(Point stickyOffset) { mStickyOffset = stickyOffset; }

    /**
     * Perform any operations which is not layout based, but may depend on previous processing.
     */
    void postClearPending();

    /**
     * @param component Pointer to cast.
     * @return Casted pointer to this type, nullptr if not possible.
     */
    static CoreComponentPtr cast(const ComponentPtr& component);

    /**
     * Mark this component needing accessibility actions refreshed.
     */
    void markAccessibilityDirty();

    /**
     * Register this component for visibility calculation and tracking. No-op if component has no
     * VisibilityChange handler.
     */
    void registerForVisibilityTrackingIfRequired();

    /**
     * Deregister this component from visibility calculation. No-op if it's not registered for such
     * in a first place.
     */
    void deregisterFromVisibilityTracking();

    /**
     * Add child as valid visibility target.
     * @param child this component's child.
     */
    void addDownstreamVisibilityTarget(const CoreComponentPtr& child);

    /**
     * Queue up item rebuild
     * @param childContext current child context.
     */
    void scheduleRebuildChange(const ContextPtr& childContext);

    /**
     * Apply all pending rebuild changes.
     */
    void processRebuildChanges();

    /**
     * Stash child-related context holding rebuild dependency. Only required when there are no
     * existing child to hold the same (initially evaluated to nothing).
     * @param context context to stash.
     */
    void stashRebuildContext(const ContextPtr& context);

#ifdef SCENEGRAPH
    /**
     * @return The current scene graph node.
     */
    sg::LayerPtr getSceneGraph(sg::SceneGraphUpdates& sceneGraph);

    /**
     * Update the scene graph based on dirty properties.
     */
    void updateSceneGraph(sg::SceneGraphUpdates& sceneGraph);
#endif // SCENEGRAPH

    // Test only
#ifdef DEBUG_MEMORY_USE
    bool isTempCacheClean() const {
        return (!mChildrenChanges || mChildrenChanges->empty()) &&
               (!mPendingRebuildChanges || mPendingRebuildChanges->empty());
    }
#endif

protected:
    // internal, do not call directly
    virtual bool insertChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag);
    virtual void reportLoaded(size_t index);

    // Attach the yoga node of this child
    virtual void attachYogaNode(const CoreComponentPtr& child);

    virtual const EventPropertyMap& eventPropertyMap() const;
    virtual void invokeStandardAccessibilityAction(const std::string& name) {}

    virtual bool processGestures(const PointerEvent& event, apl_time_t timestamp) { return false; }

    virtual void handlePropertyChange(const ComponentPropDef& def, const Object& value);

    CoreComponentPtr getLayoutRoot();

    /**
     * Allow derived classes to react to layout direction change
     */
    virtual void handleLayoutDirectionChange(bool useDirtyFlag) {};

    /**
     * Execute the component key handlers if present.
     * @param type The key handler type (up/down).
     * @param keyboard The keyboard update.
     * @return True, if consumed.
     */
    virtual bool executeKeyHandlers(KeyHandlerType type, const Keyboard& keyboard) { return false; };

    /**
     * Execute the intrinsic actions for given keys if appropriate.  These key updates are not forwarded to the document.
     * @param type The key handler type (up/down).
     * @param keyboard The keyboard update.
     * @return True, if consumed.
    */
    virtual bool executeIntrinsicKeyHandlers(KeyHandlerType type, const Keyboard& keyboard) { return false; };

    virtual void finalizePopulate() {}

    /**
     * Call this to ensure that the displayed child components have been calculated. The order
     * of displayed children matches the intended rendering order. The number of displayed children
     * can be accessed with getDisplayedChildCount().  The ordered list of children can
     * be accessed with getDisplayedChildAt(displayIndex);
     */
    virtual void ensureDisplayedChildren();

    /**
     * @return True if layout change calculations should be propagated to component's children. Usually the case
     * when component itself is part of the layout tree.
     */
    bool shouldPropagateLayoutChanges() const;

    /**
     * @return hash of properties that could affect TextMeasurement.
     */
    size_t textMeasurementHash() const;

    /**
     * Update text measurement hash
     */
    void fixTextMeasurementHash();

    /**
     * Update visual hash
     */
    void fixVisualHash(bool useDirtyFlag);

    /**
     * Traverse the component hierarchy rooted at this component, invoking pre on each component
     * before traversing all children, and post on each component after traversing all children.
     * @param pre pre-order traversal function accepting CoreComponent&
     * @param post post-order traversal function accepting CoreComponent&
     */
    template<typename Pre, typename Post>
    void traverse(const Pre& pre, const Post& post);

    /**
     * Traverse the component hierarchy rooted at this component, invoking pre on each component
     * before traversing each child.
     * @param pre pre-order traversal function accepting CoreComponent&
     */
    template<typename Pre>
    void traverse(const Pre& pre) { traverse(pre, [](CoreComponent& c) {}); };

    /**
     * Operation to perform before actual component release.
     */
    virtual void preRelease() {};

    /**
     * Release this component. This component may still be in its parent's child list. This does
     * not release children of this component, nor does it clear this component's list of children.
     */
    virtual void releaseSelf();

    /**
     * Clear any component specific delayed processing (timers/animations/etc)
     */
    virtual void clearActiveStateSelf();

    virtual void removeChildAfterMarkedRemoved(const CoreComponentPtr& child, size_t index, bool useDirtyFlag);

#ifdef SCENEGRAPH
    /*
     * Used by getSceneGraph() to build the component's scene graph (and attached children).
     * Subclasses that override this method should call the parent method first.
     * @return The scene graph
     */
    virtual sg::LayerPtr constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph);

    /**
     * Override this method in subclasses which watch for dirty property changes
     * and update the scene graph.
     * @param sg Scene graph builder component
     * @return True if one of the nodes changed (will mark the contents as needed to be drawn)
     */
    virtual bool updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph) { return false; }
#endif // SCENEGRAPH

    /**
     * Set an internal property that is component-specific and not part of the component definition.
     * This should be overridden in a subclass that supports internal properties.
     * @param key The name of the property.
     * @param value The value to set it to.
     * @return True if the property was found and changed.
     */
    virtual bool setPropertyInternal(const std::string& key, const Object& value) { return false; }

    /**
     * Retrieve an internal property that is component-specific and not part of the component
     * definition.  This should be overridden in a subclass that supports internal properties.
     * @param key The name of the property
     * @return A Object-Boolean pair.  Returns true and the value of the property if it exists and
     *         can be read.  Returns { NULL, false } if the property is not found.
     */
    virtual std::pair<Object, bool> getPropertyInternal(const std::string& key) const {
        return { Object::NULL_OBJECT(), false };
    }

    /**
     * @return true if children of this component should be included in the visual context, false
     * otherwise.
     */
    virtual bool includeChildrenInVisualContext() const { return true; }

    /**
     * @param result Supported standard accessibility actions, paired with true if implicit
     *               (does not need to be enabled)
     */
    virtual void getSupportedStandardAccessibilityActions(std::map<std::string, bool>& result) const {}

    /**
     * @param child Component to check
     * @return true if current component is hierarchical parent of provided one, false otherwise.
     */
    bool isParentOf(const CoreComponentPtr& child);

    /**
     * Measure callback
     * @param width Requested width
     * @param widthMode Width measure mode
     * @param height Requested height
     * @param heightMode Height measure mode
     * @return Yoga size of measured text
     */
    virtual YGSize textMeasure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode);

    /**
     * Text baseline callback
     * @param width Requested width
     * @param height Requested height
     * @return Baseline width.
     */
    virtual float textBaseline(float width, float height);

private:
    friend streamer& operator<<(streamer&, const Component&);

    friend class Builder;
    friend class LayoutRebuilder;
    friend class LayoutManager;
    friend class ChildWalker;
    friend class HostComponent; // for access to attachedToParent

    enum ChildChangeAction {
        kChildChangeActionInsert,
        kChildChangeActionRemove
    };

    bool appendChild(const ComponentPtr& child, bool useDirtyFlag);

    void attachedToParent(const CoreComponentPtr& parent);

    void removeChild(const CoreComponentPtr& child, bool useDirtyFlag);

    void removeChildAt(size_t index, bool useDirtyFlag);

    void markSelfRemoved();

    void markAdded();

    void updateInheritedState();

    std::pair<bool, ConstComponentPropIterator> find(PropertyKey key) const;

    bool setPropertyInternal(ConstComponentPropIterator it, const Object& value);
    std::pair<Object, bool> getPropertyInternal(ConstComponentPropIterator it) const;

    bool markPropertyInternal(const ComponentPropDefSet& propDefSet, PropertyKey key);

    void updateMixedStateProperty(PropertyKey key, bool value);

    const ComponentPropDefSet* getLayoutPropDefSet() const;

    void updateStyleInternal(const StyleInstancePtr& stylePtr, const ComponentPropDefSet& propDefSet);

    virtual const ComponentPropDefSet* layoutPropDefSet() const { return nullptr; };

    void serializeVisualContextInternal(
        rapidjson::Value& outArray, rapidjson::Document::AllocatorType& allocator, float realOpacity,
        float visibility, const Rect& visibleRect, int visualLayer);

    void attachRebuilder(const std::shared_ptr<LayoutRebuilder>& rebuilder) { mRebuilder = rebuilder; }

    std::shared_ptr<ObjectMap> createEventProperties(const std::string& handler, const Object& value) const;

    void notifyChildChanged(size_t index, const CoreComponentPtr& component, ChildChangeAction action);

    /**
     * The default behavior of the child insertion is to attach the child when it happens.
     * Override this function for cases when such behavior is not required.
     */
    virtual void attachYogaNodeIfRequired(const CoreComponentPtr& coreChild, int index);

    void scheduleTickHandler(const Object& handler, double delay);
    void processTickHandlers();

    /**
     * Recomputes the transformation to this component's coordinate space (from the global coordinate space), if stale.
     * If the local transform has not been marked stale, this has no effect (see @c markLocalTransformStale).
     */
    void ensureGlobalToLocalTransform();

    void fixAccessibilityActions();

    void processChildrenChanges();

    void removeDownstreamVisibilityTarget(const CoreComponentPtr& child);

    /**
     * Rebuild children set based on previously scheduled changes. Multi-child components defined
     * through items set have constant elements (indexes/ordinals/etc) which may require a full
     * rebuild.
     * @see #scheduleRebuildChange(const ContextPtr& childContext)
     */
    void rebuildItems();

    /**
     * Replace particular child with an new rebuilt one.
     */
    void replaceChild(const ObjectArray& items, const CoreComponentPtr& child, const ContextPtr& childContext, int originIndex, int childIndex);

    static std::string toStringAction(ChildChangeAction action);

protected:
    /**
     * Various flags used by component.
     */
    enum CoreComponentFlags : uint8_t {
        kCoreComponentFlagInheritParentState = 1u << 0,
        kCoreComponentFlagDisplayedChildrenStale = 1u << 1,
        kCoreComponentFlagIsDisallowed = 1u << 2,
        kCoreComponentFlagGlobalToLocalIsStale = 1u << 3,
        kCoreComponentFlagTextMeasurementHashStale = 1u << 4,
        kCoreComponentFlagVisualHashStale = 1u << 5,
        kCoreComponentFlagAccessibilityDirty = 1u << 6,
    };

    State                            mState;       // Operating state (pressed, checked, etc)
    std::string                      mStyle;       // Name of the current STYLE
    Properties                       mProperties;  // Assigned properties from JSON
    std::set<PropertyKey>            mAssigned;    // Properties that have been assigned from JSON or SetValue
    std::vector<CoreComponentPtr>    mChildren;    // Children of this component
    std::vector<CoreComponentPtr>    mDisplayedChildren; // ordered list of children to be drawn
    CoreComponentPtr                 mParent;
    YGNodeRef                        mYGNodeRef;
    Path                             mPath;
    std::shared_ptr<LayoutRebuilder> mRebuilder;
    Size                             mLayoutSize;
    Flags<CoreComponentFlags>        mCoreFlags;
#ifdef SCENEGRAPH
    sg::LayerPtr                     mSceneGraphLayer;
#endif // SCENEGRAPH

private:
    // The members below are used to store cached values for performance reasons, and not part of
    // the state of this component.
    struct ChildChange {
        std::weak_ptr<CoreComponent> component;
        std::string uid;
        ChildChangeAction action;
        size_t index;
    };

    Transform2D                                mGlobalToLocal;
    Point                                      mStickyOffset;
    size_t                                     mTextMeasurementHash;
    timeout_id                                 mTickHandlerId = 0;

    /// Permanent caches
    std::unique_ptr<WeakPtrSet<CoreComponent>> mAffectedByVisibilityChange;
    std::unique_ptr<std::map<int, ContextPtr>> mStashedRebuildCtxs;

    /// Temporary caches
    std::unique_ptr<std::vector<ChildChange>>  mChildrenChanges;
    std::unique_ptr<std::set<int>> mPendingRebuildChanges;
};

}  // namespace apl

#endif //_APL_CORE_COMPONENT_H
