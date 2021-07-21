/*
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
 *
 * A single component
 */

#ifndef _APL_COMPONENT_H
#define _APL_COMPONENT_H

#include <set>

#include "apl/common.h"
#include "componentproperties.h"
#include "apl/utils/counter.h"
#include "apl/engine/propertymap.h"
#include "apl/primitives/rect.h"
#include "apl/engine/state.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/visitor.h"
#include "apl/utils/userdata.h"

namespace apl {

class Context;
class MediaState;
class GraphicContent;

using CalculatedPropertyMap = PropertyMap<PropertyKey, sComponentPropertyBimap>;

/**
 * Updates from the view host to the component.  Call the Component::update() method and
 * pass the update type and an optional float argument with data.
 */
enum UpdateType {

    /**
     * This component (generally a touch wrapper) has been "pressed" and should execute the
     * onPress commands.
     */
     ///@deprecated
    kUpdatePressed,

    /**
     * This component should take keyboard focus.
     */
    kUpdateTakeFocus,

    /**
     * This component is being touched by the user.  This happens before the kUpdatePressed message.
     * Pass an non-zero argument (1) to indicate that there is a touch-down event in the component.
     * Pass a zero argument (0) to indicate that there is a touch-up or touch-exit event in the component.
     */
     ///@deprecated
    kUpdatePressState,

    /**
     * Update the current scroll position.  The argument is the updated scroll position in dp.
     * Scroll positions are non-negative.
     */
    kUpdateScrollPosition,

    /**
     * A pager has been moved by the user.  The argument is the new page number (0-based index).
     */
    kUpdatePagerPosition,

    /**
     * A pager has been moved in response to a SetPage event.  The argument is the new page number (0-based index).
     */
    kUpdatePagerByEvent,

    /**
     * The user has pressed the submit button associated with a EditText component.
     */
    kUpdateSubmit,

    /**
     * The user has changed the text in the edit text component.
     */
     kUpdateTextChange,

     /**
      * Invoke an accessibility action by name.  The argument is the string name of the action to invoke.
      */
     kUpdateAccessibilityAction,
};

/**
 * Valid scroll directions for this component
 */
enum ScrollType {
    kScrollTypeNone,
    kScrollTypeVertical,
    kScrollTypeVerticalPager,
    kScrollTypeHorizontal,
    kScrollTypeHorizontalPager,
};

/**
 * Valid directions for paging for this component.  Changes dynamically.
 */
enum PageDirection {
    kPageDirectionNone,
    kPageDirectionForward,
    kPageDirectionBack,
    kPageDirectionBoth
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
class Component : public Counter<Component>,
                  public UserData<Component>,
                  public NonCopyable,
                  public std::enable_shared_from_this<Component> {

public:
    ~Component() override = default;

    /**
     * Release this component and all children.  This component may still be in
     * its parents child list.
     */
    virtual void release() = 0;

    /**
     * @return The common name of the component
     */
    std::string name() const;

    /**
     * @return The number of children.
     */
    virtual size_t getChildCount() const = 0;

    /**
     * Retrieve a child at an index.  Throws an exception if out of bounds.
     * @param index The zero-based index of the child.
     * @return The child.
     */
    virtual ComponentPtr getChildAt(size_t index) const = 0;

    /**
     * Append a new child at the end of the child list.
     * @param child The component to append.
     * @return True if the component was added.
     */
    virtual bool appendChild(const ComponentPtr& child) = 0;

    /**
     * Insert a component in the child list. The component is placed at the specified index; all
     * pre-existing components at or beyond this index are shifted back.  If the index falls beyond
     * the last component in the child list, the component is append to the end of the list.
     * @param child The component to insert.
     * @param index The index where the child should be inserted.
     * @return True if the component was added.
     */
    virtual bool insertChild(const ComponentPtr& child, size_t index) = 0;

    /**
     * Remove this component from its parent
     */
    virtual bool remove() = 0;

    /**
     * @return True if this component supports dynamically adding a child
     */
    virtual bool canInsertChild() const = 0;

    /**
     * @return True if this component supports dynamically removing a child
     */
    virtual bool canRemoveChild() const = 0;

    /**
     * @return The set of properties that have changed in this component
     *         since the last time the component was marked as clean.
     */
    const std::set<PropertyKey>& getDirty() { return mDirty; }

    /**
     * Clear the set of properties that have been changed.
     */
    virtual void clearDirty();

    /**
     * @return The current map of property name to value set on this component.
     */
    const CalculatedPropertyMap& getCalculated() const { return mCalculated; }

    /**
     * Return a single property value by PropertyKey.
     * @param key The PropertyKey of the prop to return.
     * @return The current value assigned to that property.
     */
    const Object& getCalculated( PropertyKey key ) const { return mCalculated.get(key); }

    /**
     * @return The primitive type of the component.
     */
    virtual ComponentType getType() const = 0;

    /**
     * @return The unique ID assigned to this component by the system.
     */
    std::string getUniqueId() const { return mUniqueId; }

    /**
     * @return The ID assigned to this component by the APL author.  If not
     *         assigned, return the empty string.
     */
    std::string getId() const { return mId; }

    /**
     * @return The current parent of this component.  May be nullptr.
     */
    virtual ComponentPtr getParent() const = 0;

    /**
     * @return This component's context
     */
    ContextPtr getContext() { return mContext; }

    /**
     * @return This component's context
     */
    std::shared_ptr<const Context> getContext() const { return mContext; }

    /**
     * @return True if this component was properly created with all required
     *         properties specified.
     */
    bool isValid() { return (mFlags & kComponentFlagInvalid) == 0; }

    /**
     * @return True if this component has been inflated and should now run event handlers on a SetValue or equivalent.
     */
    bool allowEventHandlers() { return (mFlags & kComponentFlagAllowEventHandlers) != 0; }

    /**
     * An update message from the viewhost.  This method is used for all updates that take
     * no parameters or a single numeric or boolean parameter.
     * @param type The type of update message.
     * @param value The update message parameter.
     */
    virtual void update(UpdateType type, float value=0) = 0;

    /**
     * An update message from the viewhost.  This method is used for all updates that take
     * a single string.
     * @param type The type of update message.
     * @param value The update message parameter.
     */
    virtual void update(UpdateType type, const std::string & value) = 0;

    /**
     * Update component media state. Not applicable for every component.
     * @param state component's MediaState.
     */
    virtual void updateMediaState(const MediaState& state, bool fromEvent = false);

    /**
     * Update graphics display.  Not applicable for every component.
     * @param json graphic content
     */
    virtual bool updateGraphic(const GraphicContentPtr& json);

    /*
    * @return The number of children displayed.
    */
    virtual size_t getDisplayedChildCount() const = 0;

    /**
     * Retrieve a displayed child by index.  The order of displayed children
     * matches the intended rendering order.
     * The display index is not guaranteed to match the getChildAt() result.
     * Throws an exception if out of bounds.
     *
     * Consumers using this method for drawing may implement a loop as follows:
     *
     *  void draw(Component c, Paint paint) {
     *
     *       auto display  = getCalculated(kPropertyDisplay).asInt();
     *       auto opacity = getCalculated(kPropertyDisplay).asInt();
     *       if (display == kDisplayNormal && opacity > 0.0) {
     *
     *          // Copy the current paint and apply opacity
     *          auto p = paint;
     *          p.opacity *= opacity;
     *
     *          // Apply clip bounds, exit if nothing visible
     *          auto bounds = getCalculated(kPropertyBounds).getRect();
     *          p.addClipping( bounds );
     *          if (p.clipRegionEmpty())
     *              return;
     *
     *          // Transform to the local coordinate space
     *          p.translate( bounds.getTopLeft() );
     *          auto transform = getCalculated(kPropertyTransform).getTransform2D();
     *          p.applyTransform( c.getTransform() );
     *
     *          // Draw self, then children
     *          drawInternal(opacity, clip);
     *          for (size_t i = 0 ; i < c.getDisplayedChildCount() ; i++)
     *               draw( c.getDisplayedChildAt(i), opacity );
     *
     *      }
     *  }
     *
     * @param index The zero-based display index of the child.
     * @return The child.
     */
    virtual ComponentPtr getDisplayedChildAt(size_t displayIndex) const = 0;

    /**
     * Call this to ensure that the component has a layout.  This method must be used by
     * children of a sequence to before retrieving the layout bounds.
     * @deprecated Should not be used. No-op.
     */
    virtual void ensureLayout(bool useDirtyFlag = false) {}

    /**
     * The bounds of this component within an ancestor.
     * @param ancestor An ancestor of this component or nullptr for the root
     * @param out Store the resulting rectangle here.
     * @return True if the ancestor was valid; false if not.
     */
    bool getBoundsInParent(const ComponentPtr& ancestor, Rect& out) const;

    /**
     * @return Global bounds for this component
     */
    Rect getGlobalBounds() const { Rect r; getBoundsInParent(nullptr, r); return r; }

    /**
     * @return The type of scrolling supported by this component.
     */
    virtual ScrollType scrollType() const { return kScrollTypeNone; }

    /**
     * @return The current scroll position
     */
    virtual Point scrollPosition() const { return Point(); }

    /**
     * Given a requested point to scroll to, trim it back to a point that is a valid scroll position.
     * @param point The requested point.
     * @return A valid point to scroll to.
     */
    virtual Point trimScroll(const Point& point) { return Point(); }

    /**
     * @return The valid directions that can be paged from the current page. This depends on the navigation setting.
     */
    virtual PageDirection pageDirection() const { return kPageDirectionNone; }

    /**
     * @return The current page of the pager
     */
    virtual int pagePosition() const { return 0; }

    /**
     * @return true if component like Pager or Scrollable can move forward.
     */
    virtual bool allowForward() const { return false; }

    /**
     * @return true if component like Pager or Scrollable can move backwards.
     */
    virtual bool allowBackwards() const { return false; }

    /**
     * The component hierarchy signature is a unique text string that represents the type
     * of this component and all of the components below it in the hierarchy.  This signature
     * in mainly intended for use in recycling views where native layouts are re-used for
     * new component hierarchies.
     *
     * @return A unique signature of the component hierarchy
     */
    virtual std::string getHierarchySignature() const = 0;

    /**
     * Serialize a component and its children into a rapidjson object.
     * @param allocator
     * @return
     */
    virtual rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const = 0;

    /**
     * Convert this component and all of its properties into a human-readable JSON object.
     * This method is intended to be used by debugging and testing tools; it is not intended
     * for viewhosts.
     * @param allocator
     * @return The object
     */
    virtual rapidjson::Value serializeAll(rapidjson::Document::AllocatorType& allocator) const = 0;

    /**
     * Serialize all dirty component parameters into a rapidjson array. This clears the dirty
     * flags.
     * @param allocator
     * @return
     */
    virtual rapidjson::Value serializeDirty(rapidjson::Document::AllocatorType& allocator) = 0;

    /**
     * @return The descriptive path of the source that created this component
     * @deprecated Replace with provenance
     */
    virtual std::string getPath() const { return provenance(); };

    /**
     * @return The descriptive path of the source that created this component
     */
    virtual std::string provenance() const = 0;

    /**
     * Serialize a component and its children visual context into a rapidjson object.
     * @deprecated use RootContext->serializeVisualContext()
     * @param allocator Allocator for allocating memory for the DOM
     * @return a json representation of the visual context.
     */
    virtual rapidjson::Value serializeVisualContext(rapidjson::Document::AllocatorType& allocator) = 0;

    /**
     * Find a component at or below this point in the hierarchy with the given id or uniqueId.
     * @param id The id or uniqueId to search for.
     * @return The component or nullptr if it is not found.
     */
    virtual ComponentPtr findComponentById(const std::string& id) const = 0;

    /**
     * Find a visible component at or below this point in the hierarchy containing the given position.
     * @param position The position expressed in component-relative coordinates.
     * @return The visible component or nullptr if there is no visible component at this position.
     */
    virtual ComponentPtr findComponentAtPosition(const Point& position) const = 0;

    /**
     * @return This component formatted suitable for printing on a debug line
     */
    std::string toDebugString() const;

    /**
     * @return This component condensed formatted suitable for printing on a debug line
     */
    std::string toDebugSimpleString() const;

    /**
     * Equality operator override
     */
    bool operator==(const ComponentPtr& other) const { return mUniqueId == other->getUniqueId(); }

    virtual bool isCharacterValid(const wchar_t wc) const;

    /**
     * This function will be called for dynamic component inflation
     */
    ComponentPtr inflateChildAt(const rapidjson::Value& component, size_t index);

    /**
     * @return true if component could be focused with input focus.
     */
    virtual bool isFocusable() const { return false; }

    /**
     * @return true if component should participate be reported to the accessibility system.
     */
    virtual bool isAccessible() const { return false; }

protected:
    Component(const ContextPtr& context, const std::string& id);

    friend streamer& operator<<(streamer&, const Component&);
    friend class Builder;

    static id_type             sUniqueIdGenerator;

    ContextPtr                 mContext;
    std::string                mUniqueId;
    std::string                mId;
    CalculatedPropertyMap      mCalculated;  // Current calculated object properties
    std::set<PropertyKey>      mDirty;

    enum {
        kComponentFlagInvalid = 0x01,  // Marks a component missing a required property
        kComponentFlagAllowEventHandlers = 0x02,  // Event handlers don't run when the component is first inflated
    };

    unsigned int               mFlags = 0;
};

}  // namespace apl

#endif // _APL_COMPONENT_H
