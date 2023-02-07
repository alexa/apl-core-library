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

#include "apl/touch/gestures/tapgesture.h"

#include <cmath>

#include "apl/component/touchablecomponent.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/propdef.h"
#include "apl/primitives/timefunctions.h"

namespace apl {

std::shared_ptr<TapGesture>
TapGesture::create(const ActionablePtr& actionable, const Context& context, const Object& object) {
    Object onTap = arrayifyPropertyAsObject(context, object, "onTap");

    return std::make_shared<TapGesture>(actionable, std::move(onTap));
}

TapGesture::TapGesture(const ActionablePtr& actionable, Object&& onTap) :
        Gesture(actionable),
        mOnTap(std::move(onTap)),
        mStartTime(0),
        mStartPoint(Point()),
        mMaximumTapVelocity(actionable->getRootConfig().getProperty(RootProperty::kMaximumTapVelocity).getInteger()),
        mMaximumTapTravel(actionable->getRootConfig().getProperty(RootProperty::kPointerSlopThreshold).getInteger()) {}

void
TapGesture::reset() {
    Gesture::reset();

    mStartTime = 0;
    mStartPoint = Point();
}

bool
TapGesture::invokeAccessibilityAction(const std::string& name)
{
    if (name == "activate") {
        mActionable->executeEventHandler("Tap", mOnTap, false, mActionable->createTouchEventProperties(Point()));
        return true;
    }

    return false;
}

bool
TapGesture::onDown(const PointerEvent& event, apl_time_t timestamp) {
    mStartTime = timestamp;
    mStartPoint = mActionable->toLocalPoint(event.pointerEventPosition);

    return true;
}

bool
TapGesture::onUp(const PointerEvent& event, apl_time_t timestamp) {
    float elapsedTimeInSeconds = (timestamp - mStartTime) / 1000;
    Point endPoint = mActionable->toLocalPoint(event.pointerEventPosition);

    auto dx = endPoint.getX() - mStartPoint.getX();
    auto dy = endPoint.getY() - mStartPoint.getY();
    auto distance = std::sqrt(dx*dx + dy*dy);

    // Consider zero time to be zero velocity rather than infinite because two events may have
    // arrived before time could be updated, this is effectively going to pass the velocity check
    auto velocity = (elapsedTimeInSeconds == 0) ? 0 : distance / elapsedTimeInSeconds;

    auto globalToLocal = mActionable->getGlobalToLocalTransform();
    auto dxRatio = distance > 0 ? std::abs(dx / (dx + dy)) : 1;
    auto dyRatio = distance > 0 ? std::abs(dy / (dx + dy)) : 1;
    auto combinedScaling = globalToLocal.getXScaling() * dxRatio + globalToLocal.getYScaling() * dyRatio;
    auto maxDistance = mMaximumTapTravel * combinedScaling;
    auto maxVelocity = mMaximumTapVelocity * combinedScaling;

    if (distance <= maxDistance && velocity <= maxVelocity) {
        mActionable->executePointerEventHandler(kPropertyOnCancel, endPoint);
        mActionable->executeEventHandler("Tap", mOnTap, false, mActionable->createTouchEventProperties(endPoint));
        reset();
    }

    return true;
}

} // namespace apl
