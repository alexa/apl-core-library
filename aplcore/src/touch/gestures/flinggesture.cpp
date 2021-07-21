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

#include "apl/touch/gestures/flinggesture.h"

#include "apl/component/actionablecomponent.h"
#include "apl/content/rootconfig.h"

#include "apl/engine/builder.h"
#include "apl/action/action.h"

#include "apl/animation/coreeasing.h"
#include "apl/primitives/timefunctions.h"
#include "apl/touch/utils/velocitytracker.h"
#include "apl/utils/session.h"
#include "apl/time/sequencer.h"

namespace apl {

FlingGesture::FlingGesture(const ActionablePtr& actionable)
  : Gesture(actionable),
    mStartTime(0),
    mVelocityTracker(new VelocityTracker(actionable->getRootConfig()))
{}

void
FlingGesture::reset()
{
    Gesture::reset();
    mVelocityTracker->reset();
}

bool
FlingGesture::onMove(const PointerEvent& event, apl_time_t timestamp)
{
    mVelocityTracker->addPointerEvent(event, timestamp);
    if (!passedScrollOrTapTimeout(timestamp))
        return false;
    return true;
}

bool
FlingGesture::onDown(const PointerEvent& event, apl_time_t timestamp)
{
    mVelocityTracker->reset();
    mVelocityTracker->addPointerEvent(event, timestamp);
    mStartPosition = mActionable->toLocalPoint(event.pointerEventPosition);
    mStartTime = timestamp;
    return true;
}

bool
FlingGesture::onUp(const PointerEvent& event, apl_time_t timestamp)
{
    if (!mTriggered) {
        reset();
        return false;
    }

    mVelocityTracker->addPointerEvent(event, timestamp);
    return true;
}

bool
FlingGesture::passedScrollOrTapTimeout(apl_time_t timestamp)
{
    // Wait for tap or scroll timeout to start checking for moves
    return (timestamp - mStartTime) >= mActionable->getRootConfig().getTapOrScrollTimeout();
}

bool
FlingGesture::isSlopeWithinTolerance(Point localPosition, bool horizontal)
{
    if (!localPosition.isFinite() || !mStartPosition.isFinite()) {
        return false;
    }

    auto pointerDelta = localPosition - mStartPosition;
    auto maxSlope = mActionable->getRootConfig().getSwipeAngleSlope();
    if (horizontal) {
        return std::abs(pointerDelta.getX()) * maxSlope >= std::abs(pointerDelta.getY());
    } else {
        return std::abs(pointerDelta.getY()) * maxSlope >= std::abs(pointerDelta.getX());
    }
}

} // namespace apl
