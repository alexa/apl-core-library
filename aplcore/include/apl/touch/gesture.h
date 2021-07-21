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

#ifndef _APL_GESTURE_H
#define _APL_GESTURE_H

#include "apl/common.h"
#include "apl/touch/pointerevent.h"
#include "apl/touch/gesturestep.h"
#include "apl/utils/bimap.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"

namespace apl {

class Object;
class ActionableComponent;

using ActionablePtr = std::shared_ptr<ActionableComponent>;

/**
 * Enumeration of gesture types
 */
enum GestureType {
    kGestureTypeDoublePress,
    kGestureTypeLongPress,
    kGestureTypeSwipeAway,
};

extern Bimap<GestureType, std::string> sGestureTypeBimap;

/**
 * Base class for any APL defined gestures. Every pointer event is routed to every gesture defined on component, until
 * one of them is triggered. After that triggered gesture considered active and will consume all events until
 * finished/reset.
 */
class Gesture : public Counter<Gesture>,
                public NonCopyable {
public:
    static std::shared_ptr<Gesture> create(const ActionablePtr& actionable, const Object& object);

    /**
     * Release this gesture and any dependencies.  The component attached to this gesture is no longer usable
     */
    virtual void release();

    /**
     * Process pointer event through gesture.
     * @param event Pointer event.
     * @param timestamp Event timestamp.
     * @return true if gesture became triggered and should be considered active, false otherwise.
     */
    virtual bool consume(const PointerEvent& event, apl_time_t timestamp);

    /**
     * Reset internal gesture state.
     */
    virtual void reset();

    /**
     * @return true if triggered, false otherwise.
     */
    bool isTriggered() const { return mTriggered; }

    /**
     * An accessibility action has been invoked on this gesture's component, but no user-defined commands
     * were found. If the name of the accessibility action is the predefined name associated with this gesture,
     * then execute the appropriate commands from this gesture.
     * @param name The name of the accessibility action
     * @return True if this gesture matched and executed
     */
    virtual bool invokeAccessibilityAction(const std::string& name) { return false; }

protected:
    Gesture(const ActionablePtr& actionable) : mActionable(actionable), mStarted(false), mTriggered(false) {}

    /// Following handlers should be overriden by specific gesture in order to respond to incoming events.
    /// See existing gestures for an example.

    /**
     * Handle move event
     * @param event pointer event to process.
     * @param timestamp pointer event timestamp.
     * @return true if handler executed properly, false in anything went wrong.
     */
    virtual bool onMove(const PointerEvent& event, apl_time_t timestamp) { return true; }

    /**
     * Handle time update
     * @param event pointer event to process (simulated in this case, so should be generally ignored)
     * @param timestamp pointer event timestamp.
     * @return true if handler executed properly, false in anything went wrong.
     */
    virtual bool onTimeUpdate(const PointerEvent& event, apl_time_t timestamp) { return true; }

    /**
     * Handle pointer entry/down event
     * @param event pointer event to process.
     * @param timestamp pointer event timestamp.
     * @return true if handler executed properly, false in anything went wrong.
     */
    virtual bool onDown(const PointerEvent& event, apl_time_t timestamp) { return true; }

    /**
     * Handle pointer exit/up event
     * @param event pointer event to process.
     * @param timestamp pointer event timestamp.
     * @return true if handler executed properly, false in anything went wrong.
     */
    virtual bool onUp(const PointerEvent& event, apl_time_t timestamp) { return true; }

    /**
     * Handle pointer cancel
     * @param event pointer event to process.
     * @param timestamp pointer event timestamp.
     * @return true if handler executed properly, false in anything went wrong.
     */
    virtual bool onCancel(const PointerEvent& event, apl_time_t timestamp) { if(mTriggered) reset(); return true; }

    /**
     * Converts a vector in global coordinate space to the current target's local coordinate space.
     * The vector's starting point is considered to be the origin (i.e. (0,0)) in the current target's coordinate space.
     *
     * @param vector The vector from the current target's origin
     * @return The vector converted to local coordinate space.
     */
    Point toLocalVector(const Point& vector);

    /**
     * Simple helper to execute pointer event handling regardless of gesture being triggered.
     *
     * @param event pointer event.
     */
    void passPointerEventThrough(const PointerEvent& event);

    ActionablePtr mActionable;
    bool mStarted;
    bool mTriggered;
};

using GestureFunc = std::function<std::shared_ptr<Gesture>(const ActionablePtr&, const Context&, const Object& object)>;

} // namespace apl

#endif //_APL_GESTURE_H
