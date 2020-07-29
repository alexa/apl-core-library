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
#include "apl/utils/bimap.h"
#include "apl/touch/pointerevent.h"
#include "apl/touch/gesturestep.h"

namespace apl {

class Object;
class TouchableComponent;

using TouchablePtr = std::shared_ptr<TouchableComponent>;

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
class Gesture {
public:
    static std::shared_ptr<Gesture> create(const TouchablePtr& touchable, const Object& object);

    /**
     * @return The type of the gesture
     */
    virtual GestureType getType() const = 0;

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
    bool isTriggered() { return mTriggered; }

protected:
    Gesture(const TouchablePtr& touchable) : mTouchable(touchable), mStarted(false), mTriggered(false) {}

    /// Following should be overriden by specific gesture in order to respond to incoming events.
    /// See existing gestures for an example.
    virtual void onMove(const PointerEvent& event, apl_time_t timestamp) {}
    virtual void onDown(const PointerEvent& event, apl_time_t timestamp) {}
    virtual void onUp(const PointerEvent& event, apl_time_t timestamp) {}

    TouchablePtr mTouchable;
    bool mStarted;
    bool mTriggered;
};

using GestureFunc = std::function<std::shared_ptr<Gesture>(const TouchablePtr&, const Context&, const Object& object)>;

} // namespace apl

#endif //_APL_GESTURE_H
