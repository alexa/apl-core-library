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

#ifndef _APL_HOVER_MANAGER_H
#define _APL_HOVER_MANAGER_H

#include <memory>

namespace apl {

class CoreComponent;

class RootContextData;


/// HoveManager is responsible for managing the hover state of a Component and
/// executing the OnCursorEnter and OnCursorExit handlers of the component.
///
/// Hover State:
/// The hover state reflects if the component currently has a cursor (mouse pointer)
/// over its active region. The hover state is false when the component is initialized.
/// The component, when enabled, sets the hover state to true when the cursor
/// enters the active region, and sets the hover state to false when the cursor
/// exits the component's active region.
///
/// Components that inherit state from their parent, will set the parent hover state.
///
/// Disabled components do not respond to changes in cursor events or change hover
/// states, but they do consume the cursor behavior over their active region as
/// if they were enabled. A disabled child component of an enabled parent does not
/// pass-through the cursor position within its active region to its parent.
///
/// OnCursorEnter:
/// Command(s) to execute when a cursor enters the component's active region.
/// Components with the disabled state set to true do not respond to changes
/// in cursor events, and do not execute any commands assigned to the onCursorEnter
/// event handler. If the cursor is over a disabled component and that component is enabled,
/// an onCursorEnter event will be generated for the component.
///
/// OnCursorExit:
/// Command(s) to execute when a cursor exits the component's active region.
/// Components with the disabled state set to true do not respond to changes
/// in cursor events, and do not execute any commands assigned to the onCursorExit
/// event handler. If the cursor is over an enabled component and that component is disabled,
/// an onCursorExit event will be generated for the component.
///
class HoverManager {
public:
    explicit HoverManager(const RootContextData& core) : mCore(core) {}

    /**
     * @return The cursor position.
     */
    Point cursorPosition() const {
        return mCursorPosition;
    }

    /**
     * Set cursor position
     * @param cursorPosition The cursor position.
     */
    void setCursorPosition(const Point& cursorPosition);

    /**
     * @return The component that currently has hover or nullptr
     */
    CoreComponentPtr getHover() { return mHover.lock(); }

    /**
     * @param position The cursor position.
     * @return The component that has hover at position or nullptr
     */
    CoreComponentPtr findHoverByPosition(const Point& position) const;

    /**
     * This method is called when a Component has the disabled state toggled. If the Component is the
     * the Component currently under the cursor the hover state is updated and OnCursor handlers are
     * executed. The state is derived from the disabled state:  hover = !disabled.
     * @param component The Component.
    */
    void componentToggledDisabled(const CoreComponentPtr& component);

private:
    const RootContextData& mCore;
    std::weak_ptr<CoreComponent> mHover;
    Point mCursorPosition;

    void update(const CoreComponentPtr& previous, const CoreComponentPtr& target);


};

} // namespace apl

#endif //_APL_HOVER_MANAGER_H
