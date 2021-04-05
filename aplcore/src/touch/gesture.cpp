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

#include "apl/touch/gesture.h"

#include "apl/engine/evaluate.h"
#include "apl/component/touchablecomponent.h"
#include "apl/utils/session.h"

#include "apl/touch/gestures/doublepressgesture.h"
#include "apl/touch/gestures/longpressgesture.h"
#include "apl/touch/gestures/swipeawaygesture.h"

namespace apl {

static Transform2D aboutOrigin(const Transform2D &transform) {
    auto data = transform.get();
    // Remove translation
    data[4] = 0;
    data[5] = 0;
    return Transform2D(std::move(data));
}

Bimap<GestureType, std::string> sGestureTypeBimap = {
        {kGestureTypeDoublePress, "DoublePress"},
        {kGestureTypeLongPress,   "LongPress"},
        {kGestureTypeSwipeAway,   "SwipeAway"},
};

static std::map<GestureType, GestureFunc> sGestureFunctions =
    {
        {kGestureTypeDoublePress, DoublePressGesture::create},
        {kGestureTypeLongPress,   LongPressGesture::create},
        {kGestureTypeSwipeAway,   SwipeAwayGesture::create},
    };

std::shared_ptr<Gesture>
Gesture::create(const ActionablePtr& actionable, const Object& object) {
    if (!object.isMap())
        return nullptr;

    auto contextPtr = actionable->getContext();

    auto type = propertyAsMapped<GestureType>(*contextPtr, object, "type", kGestureTypeDoublePress, sGestureTypeBimap);
    if (type == static_cast<GestureType>(-1)) {
        CONSOLE_CTX(*contextPtr) << "Unrecognized type field in gesture handler";
        return nullptr;
    }

    auto method = sGestureFunctions.find(type);
    if (method != sGestureFunctions.end()) {
        return method->second(actionable, *contextPtr, object);
    }

    return nullptr;
}

void
Gesture::release()
{
    reset();
}

void
Gesture::reset()
{
    mTriggered = false;
    mStarted = false;
}

bool
Gesture::consume(const PointerEvent& event, apl_time_t timestamp) {
    switch(event.pointerEventType) {
        case PointerEventType::kPointerDown:
            mStarted = true;
            onDown(event, timestamp);
            break;
        case PointerEventType::kPointerMove:
            if (mStarted) onMove(event, timestamp);
            break;
        case PointerEventType::kPointerTimeUpdate:
            if (mStarted) onTimeUpdate(event, timestamp);
            break;
        case PointerEventType::kPointerUp:
            if (mStarted) onUp(event, timestamp);
            break;
        case PointerEventType::kPointerTargetChanged:
            if (mTriggered) reset();
            break;
        case PointerEventType::kPointerCancel:
            if (mTriggered) onCancel(event, timestamp);
            break;
    }

    return mTriggered;
}

void
Gesture::passPointerEventThrough(const PointerEvent& event) {
    Point localPoint = mActionable->toLocalPoint(event.pointerEventPosition);
    mActionable->executePointerEventHandler(sEventHandlers.at(event.pointerEventType), localPoint);
}

Point
Gesture::toLocalVector(const Point& vector) {
    // Convert the vector to local space. Because the vector starts at (0,0) in local space, remove
    // the translation to avoid over-compensating for the position.
    return aboutOrigin(mActionable->getGlobalToLocalTransform()) * vector;
}

} // namespace apl