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

#include "apl/touch/gestures/doublepressgesture.h"

#include "apl/engine/evaluate.h"
#include "apl/component/touchablecomponent.h"
#include "apl/utils/session.h"
#include "apl/engine/propdef.h"

#include "apl/content/rootconfig.h"

namespace apl {

std::shared_ptr<DoublePressGesture>
DoublePressGesture::create(const ActionablePtr& actionable, const Context& context, const Object& object) {
    Object onDoublePress = arrayifyPropertyAsObject(context, object, "onDoublePress");
    Object onSinglePress = arrayifyPropertyAsObject(context, object, "onSinglePress");

    return std::make_shared<DoublePressGesture>(actionable, std::move(onDoublePress), std::move(onSinglePress));
}

DoublePressGesture::DoublePressGesture(const ActionablePtr& actionable, Object&& onDoublePress, Object&& onSinglePress) :
        Gesture(actionable),
        mOnDoublePress(std::move(onDoublePress)),
        mOnSinglePress(std::move(onSinglePress)),
        mStartTime(0),
        mBetweenPresses(false),
        mDoublePressTimeout(actionable->getRootConfig().getDoublePressTimeout()) {}

void
DoublePressGesture::reset() {
    Gesture::reset();
    mBetweenPresses = false;
}

bool
DoublePressGesture::invokeAccessibilityAction(const std::string& name)
{
    if (name == "doubletap") {
        mActionable->executeEventHandler("DoublePress", mOnDoublePress, false,
                                         mActionable->createTouchEventProperties(Point()));
        return true;
    }
    return false;
}

void
DoublePressGesture::onFirstUpInternal(const PointerEvent& event, apl_time_t timestamp) {
    mBetweenPresses = true;

    if (timestamp >= mStartTime + mDoublePressTimeout) {
        // Too long for double press already. Next time update will trigger single press.
        return;
    }

    // Otherwise "reset" timeout.
    mStartTime = timestamp;
    mTriggered = true;
    // Pass through
    passPointerEventThrough(event);

}
void
DoublePressGesture::onSecondDownInternal(const PointerEvent& event, apl_time_t timestamp) {
    mBetweenPresses = false;
    // Pass through
    passPointerEventThrough(event);
}

void
DoublePressGesture::onSecondUpInternal(const PointerEvent& event, apl_time_t timestamp) {
    Point localPoint = mActionable->toLocalPoint(event.pointerEventPosition);

    // Send cancel as we found double press at this point
    mActionable->executePointerEventHandler(kPropertyOnCancel, localPoint);

    // Execute on DoublePress and reset
    mActionable->executeEventHandler("DoublePress", mOnDoublePress, false,
                                     mActionable->createTouchEventProperties(localPoint));

    reset();
}

bool
DoublePressGesture::onDown(const PointerEvent& event, apl_time_t timestamp) {
    mStartTime = timestamp;
    if (mBetweenPresses) {
        onSecondDownInternal(event, timestamp);
    }
    return true;
}

bool
DoublePressGesture::onTimeUpdate(const apl::PointerEvent& event, apl::apl_time_t timestamp) {
    // Will only do something when in between presses
    if (timestamp >= mStartTime + mDoublePressTimeout && mBetweenPresses) {
        Point localPoint = mActionable->toLocalPoint(event.pointerEventPosition);
        mActionable->executeEventHandler("SinglePress", mOnSinglePress, false,
                                         mActionable->createTouchEventProperties(localPoint));
        reset();
    }
    return true;
}

bool
DoublePressGesture::onUp(const PointerEvent& event, apl_time_t timestamp) {
    if (mTriggered) {
        onSecondUpInternal(event, timestamp);
    } else {
        onFirstUpInternal(event, timestamp);
    }
    return true;
}

} // namespace apl
