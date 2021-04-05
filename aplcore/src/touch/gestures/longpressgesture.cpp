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

#include "apl/touch/gestures/longpressgesture.h"

#include "apl/engine/evaluate.h"
#include "apl/component/touchwrappercomponent.h"
#include "apl/utils/session.h"
#include "apl/engine/propdef.h"

#include "apl/content/rootconfig.h"

namespace apl {

std::shared_ptr<LongPressGesture>
LongPressGesture::create(const ActionablePtr& actionable, const Context& context, const Object& object) {
    Object onLongPressStart = arrayifyPropertyAsObject(context, object, "onLongPressStart");
    Object onLongPressEnd = arrayifyPropertyAsObject(context, object, "onLongPressEnd");

    return std::make_shared<LongPressGesture>(actionable, std::move(onLongPressStart), std::move(onLongPressEnd));
}

LongPressGesture::LongPressGesture(const ActionablePtr& actionable, Object&& onLongPressStart, Object&& onLongPressEnd) :
        Gesture(actionable),
        mStartTime(0),
        mOnLongPressStart(std::move(onLongPressStart)),
        mOnLongPressEnd(std::move(onLongPressEnd)),
        mLongPressTimeout(actionable->getRootConfig().getLongPressTimeout()) {}

bool
LongPressGesture::invokeAccessibilityAction(const std::string& name)
{
    if (name == "longpress") {
        mActionable->executeEventHandler("LongPressEnd", mOnLongPressEnd, false,
                                         mActionable->createTouchEventProperties(Point()));
        return true;
    }
    return false;
}

bool
LongPressGesture::onDown(const PointerEvent& event, apl_time_t timestamp) {
    mStartTime = timestamp;
    return true;
}

bool
LongPressGesture::onTimeUpdate(const PointerEvent& event, apl_time_t timestamp) {
    if (!mTriggered && timestamp >= mStartTime + mLongPressTimeout) {
        Point localPoint = mActionable->toLocalPoint(event.pointerEventPosition);
        mActionable->executePointerEventHandler(kPropertyOnCancel, localPoint);
        auto params = mActionable->createTouchEventProperties(localPoint);
        params->emplace("inBounds", mActionable->containsLocalPosition(localPoint));
        mActionable->executeEventHandler("LongPressStart", mOnLongPressStart, true, params);
        mTriggered = true;
    }
    return true;
}

bool
LongPressGesture::onUp(const PointerEvent& event, apl_time_t timestamp) {
    if (mTriggered) {
        Point localPoint = mActionable->toLocalPoint(event.pointerEventPosition);
        auto params = mActionable->createTouchEventProperties(localPoint);
        params->emplace("inBounds", mActionable->containsLocalPosition(localPoint));
        mActionable->executeEventHandler("LongPressEnd", mOnLongPressEnd, false, params);
    }
    reset();
    return true;
}

} // namespace apl
