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
LongPressGesture::create(const TouchablePtr& touchable, const Context& context, const Object& object) {
    Object onLongPressStart = arrayifyPropertyAsObject(context, object, "onLongPressStart");
    Object onLongPressEnd = arrayifyPropertyAsObject(context, object, "onLongPressEnd");

    return std::make_shared<LongPressGesture>(touchable, std::move(onLongPressStart), std::move(onLongPressEnd));
}

void
LongPressGesture::reset() {
    Gesture::reset();
}

LongPressGesture::LongPressGesture(const TouchablePtr& touchable, Object&& onLongPressStart, Object&& onLongPressEnd) :
        Gesture(touchable),
        mOnLongPressStart(std::move(onLongPressStart)),
        mOnLongPressEnd(std::move(onLongPressEnd)),
        mLongPressTimeout(touchable->getContext()->getRootConfig().getLongPressTimeout()) {}

void
LongPressGesture::onDown(const PointerEvent& event, apl_time_t timestamp) {
    mStartTime = timestamp;
}

void
LongPressGesture::onMove(const PointerEvent& event, apl_time_t timestamp) {
    if (!mTriggered && timestamp >= mStartTime + mLongPressTimeout) {
        mTouchable->executePointerEventHandler(kPropertyOnCancel, event.pointerEventPosition);
        auto params = mTouchable->createTouchEventProperties(event.pointerEventPosition);
        params->emplace("inBounds", mTouchable->containsGlobalPosition(event.pointerEventPosition));
        mTouchable->executeEventHandler("LongPressStart", mOnLongPressStart, true, params);
        mTriggered = true;
    }
}

void
LongPressGesture::onUp(const PointerEvent& event, apl_time_t timestamp) {
    if (mTriggered) {
        auto params = mTouchable->createTouchEventProperties(event.pointerEventPosition);
        params->emplace("inBounds", mTouchable->containsGlobalPosition(event.pointerEventPosition));
        mTouchable->executeEventHandler("LongPressEnd", mOnLongPressEnd, false, params);
    }
    reset();
}

} // namespace apl
