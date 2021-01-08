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

#ifndef APL_POINTER_EVENT_H
#define APL_POINTER_EVENT_H

#include "apl/common.h"
#include "apl/component/componentproperties.h"
#include "apl/primitives/point.h"

namespace apl {

/**
 * PointerEventType enumerates the various events that APLCore expects to process.  This generalizes both cursors
 * (e.g. mice) and touches.  In particular, APL internally handles detection and distribution of higher level events
 * to the appropriate components.  Specifically, viewhosts should not rely on underlying mouse/touch/pointer events
 * such as onmouseenter and friends, but rather pass these raw events directly in and rely on Core to determine and
 * fire events.
 */
enum PointerEventType {
    /**
     * This indicates that a pointer has ended its interaction with the component due to some system level cancellation.
     * This is analagous to Android's ACTION_CANCEL, but should *not* be confused with the onCancel APL Touchable
     * component event which can occur for other reasons than viewhosts passing along a cancel event
     */
    kPointerCancel,

    /**
     * This indicates that the pointer has touched down, corresponding to either a mouse button or touch down.  This is
     * analogous to HTML's touchstart and onmousedown events, Android's ACTION_DOWN.  This indicates the beginning
     * point of a pointer interaction.
     */
    kPointerDown,

    /**
     * This indicates that the pointer has lifted up, corresponding to either a mouse button or touch.  This is
     * analogous to HTML's touchend and onmouseup events, Android's ACTION_UP.  This indicates completion of a
     * touch event sequence that started with kPointerDown.
     */
    kPointerUp,

    /**
     * This indicates that the pointer has moved.  This event will occur for all moving pointers meaning that for
     * pointer type devices like mice kPointerMove events occur whenever the pointer moves, whether or not a button
     * has been pressed.
     */
    kPointerMove,

    /**
     * This indicates time update propagated to pointer target. Should not be used directly. In case if there is current
     * pointer interaction it will be directed to current pointer target, if no interaction going on it will be
     * propagated to the last known target.
     */
    kPointerTimeUpdate,

    /**
     * This indicates that the pointer target has changed. Should not be used directly, this is issued to the
     * last known target internally.
     */
     kPointerTargetChanged
};

/**
 * Allows distinguishing the type of pointer to handle inputs differently if needed
 */
enum PointerType {
    /// Indicates the pointer is a mouse type
    kMousePointer,
    /// Indicates the pointer originates as a touch input
    kTouchPointer
};

/**
 * CursorEvent is an immutable class that encapsulates the event type and its associated location.
 */
struct PointerEvent
{
    /**
     * Constructs a pointer event instance.  This encapsulates a singular event which must have a PointerEventType
     * and a position where that event occurred.
     * @param pointerEventType The type of pointer event.
     * @param pointerEventPosition The position in coordinates of the viewport in which this event occurred.
     * @param pointerId The id of the pointer, or 0 by default.
     * @param pointerType The type of the pointer, of kMousePointer by default.
     */
    PointerEvent(PointerEventType pointerEventType,
                 const Point& pointerEventPosition,
                 id_type pointerId = 0,
                 PointerType pointerType = kMousePointer)
        : pointerEventType(pointerEventType),
          pointerEventPosition(pointerEventPosition),
          pointerId(pointerId),
          pointerType(pointerType) {};

    /**
     * The type of the pointer event
     */
    const PointerEventType pointerEventType;

    /**
     * The position at which this event occurred.
     */
    const Point pointerEventPosition;

    /**
     * The id associated with the pointer that caused this event.
     */
    const id_type pointerId;

    /**
     * The type of the pointer
     */
    const PointerType pointerType;
};

extern Bimap<PointerEventType, PropertyKey> sEventHandlers;

} // namespace apl

#endif //APL_POINTER_EVENT_H
