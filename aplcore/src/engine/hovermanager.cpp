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

#include <chrono>

#include "apl/engine/event.h"
#include "apl/engine/hovermanager.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/component/corecomponent.h"

namespace apl {

static const bool DEBUG_HOVER = false;

/**
 * Updates the cursor position. Finds the Component under the cursor and assigns
 * it as the hover Component.
 *
 * The previous hover Component's state is set to hover = false.  If the Component
 * is enabled, the OnCursorExit handler is executed.
 *
 * The new hover Component's state is set to hover = !disabled.  If the Component
 * is enabled, the OnCursorEnter handler is executed.
 *
 * @param cursorPosition The cursor position.
 */
void
HoverManager::setCursorPosition(const Point& cursorPosition) {

    // do nothing if the cursor hasn't moved
    if (mCursorPosition == cursorPosition)
        return;

    // store the cursor position
    mCursorPosition = cursorPosition;

    // find the component under the cursor
    auto target = findHoverByPosition(mCursorPosition);
    auto previous = mHover.lock();

    // do nothing if the cursor is over the current hover Component
    if (target == previous)
        return;

    // update the components to reflect hover status.  The previous hover
    // component sets hover=false, the new hover component sets hover = !disabled;
    // OnCursor commands are executed if the component is enabled.
    update(previous, target);

    if (previous && !previous->getState().get(kStateDisabled)) {
        previous->executeOnCursorExit();
        LOG_IF(DEBUG_HOVER) << "Execute OnCursorExit: " << previous->toDebugSimpleString();
    }

    if (target && !target->getState().get(kStateDisabled)) {
        target->executeOnCursorEnter();
        LOG_IF(DEBUG_HOVER) << "Execute OnCursorEnter: " << target->toDebugSimpleString();
    }

    // store the new hover component, if any
    LOG_IF(DEBUG_HOVER) << "hover change -\n\tfrom:  " << previous << "\n\t  to:" << target;
    mHover = target;

}

/**
 * Find the Component under the cursor position.
 *
 * @param position The cursor position
 * @return The Component under the cursor, may be null.
 */
CoreComponentPtr
HoverManager::findHoverByPosition(const Point& position) const {
    auto top = mCore.top();
    if (!top)
        return nullptr;

    auto target = top->findComponentAtPosition(position);

    return std::static_pointer_cast<CoreComponent>(target);
}


/**
 * This method is called when a Component has the disabled state toggled. If the Component is the
 * the Component currently under the cursor the hover state is updated and OnCursor handlers are
 * executed. The state is derived from the disabled state:  hover = !disabled.
 */
void
HoverManager::componentToggledDisabled(const CoreComponentPtr& component) {

    assert(component);

    auto target = mHover.lock();
    if (target && target->inheritsStateFrom(component)) {

        // update the state of the hover component
        update(nullptr, target);

        // execute the OnCursor commands
        if (target->getState().get(kStateDisabled)) {
            target->executeOnCursorExit();
            LOG_IF(DEBUG_HOVER) << "Execute OnCursorExit: " << target->toDebugSimpleString();
        } else {
            target->executeOnCursorEnter();
            LOG_IF(DEBUG_HOVER) << "Execute OnCursorEnter: " << target->toDebugSimpleString();
        }

    }

}

/**
 * Transition hover state from the previous Component, if any, to a target Component, if any.
 * Evaluate inheritParentState and set the state on the state owner.  A parent/child relationship
 * between the two components results in both components with hover = true;  Its important to avoid
 * unnecessary state changes to minimize creating dirty properties.
 */
void
HoverManager::update(const CoreComponentPtr& previous, const CoreComponentPtr& target) {

    auto targetStateOwner = target? target->findStateOwner() : nullptr;
    auto previousStateOwner = previous ? previous->findStateOwner() : nullptr;

    // do nothing if the state owner is unchanged
    if (targetStateOwner == previousStateOwner)
        return;

    // If the previous Component is not related to the target Component,
    // UnSet the previous Component's hover state, and the ancestors it inherits state from, if any.
    if (previousStateOwner) {
        previousStateOwner->setState(kStateHover, false);
        LOG_IF(DEBUG_HOVER) << "Hover Previous:  " << previous->toDebugSimpleString() << " state: " << previous->getState();
    }

    // Set the target Components's hover state, and the ancestors it inherits state from, if any.
    if (targetStateOwner) {
        bool isHover = !targetStateOwner->getState().get(kStateDisabled);
        targetStateOwner->setState(kStateHover, isHover);
        LOG_IF(DEBUG_HOVER) << "Hover Target: " << target->toDebugSimpleString() << " state: " << target->getState();
    }

}


} // namespace apl
