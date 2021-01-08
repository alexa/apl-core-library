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

#ifndef APL_POINTER_MANAGER_H
#define APL_POINTER_MANAGER_H


#include <map>

#include "apl/touch/pointer.h"

namespace apl {

class RootContextData;

/**
 * PointerManager holds onto all active pointers.  For touch pointers, these are ephemeral and come and go as users press
 * and lift their fingers.  Mouse pointers persist.
 *
 * Pointer objects are opportunistically created when PointerEvents are received, using the ids contained within those events
 * to either create or retrieve an existing pointer.  It is important that the viewhost ensure that there are no duplicate
 * ids among the set of active pointers.  In order to achieve this, semantics for pointer lifetimes follow the following rules:
 *
 * - The pointer id will be active from an initiating down event (mouse button or press typically) until *either* an associated
 *   lifting or system interruption (e.g. another application's window pops up after a finger comes down on the screen).
 *
 * - Mouse style pointers that can inject move events while unpressed will not be considered "active" and the associated
 *   pointer id will not be cached.  These events will be forwarded along to processing for hover if any is required.
 *
 */
class PointerManager {
public:
    /**
     * Constructs a PointerManager with no pointers.
     * @param core Reference to RootContextData, used to delegate touch events to hover manager when applicable
     */
    explicit PointerManager(const RootContextData& core);

    /**
     * Handles a PointerEvent.  If the pointer has never been seen, it will create a new pointer for the id in the associated
     * event.  The following event types are handled:
     *
     * - Cancel: Occurs when the viewhost indicates that some occurrence has caused the current interaction with this pointer
     *           to be interrupted.  This could be due to loss of window foreground or other similar events.  This is distinct
     *           from the onCancel that the document may issue for a component.  While Cancel here from the viewhost will
     *           result in an onCancel for a touchable component, other events can occur that will cause the component to
     *           handle an onCancel as well (such as a gesture taking over touch handling).
     *
     * - Down:   Occurs when a mouse button is clicked or a touch starts.  The top-most touchable component will be found
     *           and set as the target for this pointer.  The target will persist until a cancel or up event.  All subsequent
     *           events will be routed to the target, regardless of being within that target's bounds or not.
     *
     * - Move:   Occurs while a touch is down and has moved, in this state, a component is generally in the pressed state.
     *           For mouse pointers, this can occur without a preceding down; if the pointer is down, the move events will
     *           be routed to the target previously identified.  If there is either no target, or the pointer is up, move
     *           events will be delegated to hover manager.
     *
     * - Up:     Occurs when a finger is lifted and a touch ends or a mouse button is lifted.  The event will be routed to
     *           the target and if the event occurs within the bounds of the target, a press event will also be fired.  In
     *           all cases, the target will be disassociated from this pointer and the pressed state of the target will be
     *           removed.  For touch type pointers, the pointer will be removed from the manager.
     *
     * @param pointerEvent The PointerEvent to handle.
     * @param event timestamp.
     * @return true if was consumed and should not be passed through any platform handling.
     */
    bool handlePointerEvent(const PointerEvent& pointerEvent, apl_time_t timestamp);

    /**
     * Function to notify all interested parties about pointer related time updates.
     */
    void handleTimeUpdate(apl_time_t timestamp);

private:
    /**
     * Checks to see if we have an active pointer for the associated id.  If so, it retrieves it, if not, it will create
     * a new Pointer object.  It will *not* persist this as an active pointer.
     * @param pointerEvent The pointer event that is passed in.
     * @return Either the retrieved pointer or a new object.
     */
    std::shared_ptr<Pointer> getOrCreatePointer(const PointerEvent &pointerEvent);

    /* The internal handle methods are responsible for validation, state maintenance and event forwarding i.e. hover */

    /**
     * Handles events that indicate the initiation of an active pointer.  This will find the targeted component for the
     * Down event, validate that there are no other pointers trying to interact with this component (multi-touch), persist
     * state to handle further events for this pointer and suppress multi-touch for this component.
     * @param pointer The pointer for this event.
     * @param pointerEvent The pointer event that is passed in.
     * @return The targeted component for this pointer event or null if no suitable target is found.  This could be because
     *         there was not a ActionableComponent under the pointer or that there was already a pointer actioning on the
     *         ActionableComponent that was found.
     */
    ActionableComponentPtr handlePointerStart(const std::shared_ptr<Pointer>& pointer, const PointerEvent& pointerEvent);

    /**
     * Handles update type pointer events.  At the moment this entirely consists of move events.  For moves that have no
     * target and originate from a mouse pointer, we pass along to hover manager.
     * @param pointer The pointer for this event.
     * @param pointerEvent The pointer event that is passed in.
     * @return The targeted component associated with this pointer.
     */
    ActionableComponentPtr handlePointerUpdate(const std::shared_ptr<Pointer>& pointer, const PointerEvent& pointerEvent);

    /**
     * Handles events that indicates the completion of an active pointer: cancellation or release.  This maintains state
     * in the direction opposite to handlePointerStart.  It will ensure this pointer is no longer tracked and that the
     * component that was tagged as its target is free to be interacted with once more.
     * @param pointer The pointer for this event.
     * @param pointerEvent The pointer event that is passed in.
     * @return The targeted component associated with this pointer, notably, this will be the target that had been assigned
     *         prior to state cleanup.
     */
    ActionableComponentPtr handlePointerEnd(const std::shared_ptr<Pointer>& pointer, const PointerEvent& pointerEvent);

    /**
     * Called when a new active target is selected. Causes the previous target state to be
     * reset if needed, e.g. by aborting unfinished gestures.
     *
     * @param pointer The pointer that was last active, i.e. that contains the previous target. Can be @c nullptr.
     * @param newTarget The new target, can be @c nullptr;
     * @param timestamp The event timestamp
     */
    void handleNewTarget(const std::shared_ptr<Pointer>& pointer,
                         const CoreComponentPtr& newTarget,
                         apl_time_t timestamp);

private:
    const RootContextData& mCore;
    std::shared_ptr<Pointer> mActivePointer;
    std::shared_ptr<Pointer> mLastActivePointer;
};

}

#endif //APL_POINTER_MANAGER_H
