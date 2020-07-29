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
Gesture::create(const std::shared_ptr<TouchableComponent>& touchable, const Object& object) {
    if (!object.isMap())
        return nullptr;

    auto context = *touchable->getContext();

    auto type = propertyAsMapped<GestureType>(context, object, "type", kGestureTypeDoublePress, sGestureTypeBimap);
    if (type == static_cast<GestureType>(-1)) {
        CONSOLE_CTX(context) << "Unrecognized type field in gesture handler";
        return nullptr;
    }

    auto method = sGestureFunctions.find(type);
    if (method != sGestureFunctions.end()) {
        return method->second(touchable, context, object);
    }

    return nullptr;
}

void
Gesture::reset() {
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
        case PointerEventType::kPointerUp:
            if (mStarted) onUp(event, timestamp);
            break;
        case PointerEventType::kPointerCancel:
            reset();
            break;
    }

    return mTriggered;
}

} // namespace apl