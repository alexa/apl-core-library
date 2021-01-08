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

#include "apl/touch/gestures/scrollgesture.h"

#include <cmath>

#include "apl/component/scrollablecomponent.h"
#include "apl/content/rootconfig.h"

#include "apl/engine/builder.h"
#include "apl/action/action.h"

#include "apl/animation/coreeasing.h"
#include "apl/primitives/timefunctions.h"
#include "apl/touch/utils/autoscroller.h"
#include "apl/touch/utils/velocitytracker.h"
#include "apl/utils/session.h"

namespace apl {

static const bool DEBUG_SCROLL_GESTURE = false;

std::shared_ptr<ScrollGesture>
ScrollGesture::create(const ActionablePtr& actionable) {
    return std::make_shared<ScrollGesture>(actionable);
}

ScrollGesture::ScrollGesture(const ActionablePtr& actionable)
  : Gesture(actionable),
    mVelocityTracker(new VelocityTracker(mActionable->getRootConfig()))
{}

void
ScrollGesture::reset() {
    Gesture::reset();

    mVelocityTracker->reset();
    mScroller = nullptr;
}

void
ScrollGesture::onMove(const PointerEvent& event, apl_time_t timestamp) {
    mVelocityTracker->addPointerEvent(event, timestamp);

    auto scrollable = std::static_pointer_cast<ScrollableComponent>(mActionable);
    auto delta = mActionable->toLocalPoint(event.pointerEventPosition) - mLastLocalPosition;
    if (!delta.isFinite()) {
        // TODO: log singularity once to the session once this functionality is available
        return;
    }
    auto distance = scrollable->isHorizontal() ? delta.getX() : delta.getY();

    if (!mTriggered) {
        auto flingTriggerDistanceThreshold = toLocalThreshold(mActionable->getRootConfig().getPointerSlopThreshold());
        if (std::abs(distance) > flingTriggerDistanceThreshold) {
            mTriggered = true;
        }
    }

    mLastLocalPosition = mActionable->toLocalPoint(event.pointerEventPosition);

    if (isTriggered()) {
        auto scrollPosition = scrollable->scrollPosition();
        scrollPosition = scrollable->trimScroll(scrollPosition - delta);

        mActionable->update(UpdateType::kUpdateScrollPosition, scrollable->isHorizontal() ? scrollPosition.getX()
                                                                                          : scrollPosition.getY());
    }
}

void
ScrollGesture::onDown(const PointerEvent& event, apl_time_t timestamp) {
    if (isTriggered()) {
        // We stop animation but allow for chaining of scrolls
        // TODO: When deceleration physics is it it makes sense to pass impulse in instead of chaining.
        mScroller = nullptr;
    } else {
        mVelocityTracker->reset();
    }

    mVelocityTracker->addPointerEvent(event, timestamp);
    mLastLocalPosition = mActionable->toLocalPoint(event.pointerEventPosition);
}

void
ScrollGesture::onUp(const PointerEvent& event, apl_time_t timestamp) {
    if (!isTriggered()) {
        reset();
        return;
    }

    mVelocityTracker->addPointerEvent(event, timestamp);

    auto scrollable = std::dynamic_pointer_cast<ScrollableComponent>(mActionable);
    auto velocities = toLocalVector(mVelocityTracker->getEstimatedVelocity());
    auto velocity = scrollable->isVertical() ? velocities.getY() : velocities.getX();
    if (std::isnan(velocity)) {
        CONSOLE_CTP(mActionable->getContext()) << "Singularity encountered during scroll, resetting";
        reset();
        return;
    }

    // TODO: Acting on "significant" direction. Will will need this to be changed when we have
    //  multidirectional scrolling.
    if (std::abs(velocity) >= toLocalThreshold(mActionable->getRootConfig().getMinimumFlingVelocity()) /
    time::MS_PER_SECOND) {
        auto velocityLimit =
            toLocalThreshold(mActionable->getRootConfig().getMaximumFlingVelocity()) / time::MS_PER_SECOND;
        if (std::abs(velocity) > velocityLimit) {
            LOG_IF(DEBUG_SCROLL_GESTURE)
                << "Velocity " << velocity << " is too fast, reset to " << velocityLimit;
            velocity = velocity > 0 ? velocityLimit : -velocityLimit;
        }

        // We have enough velocity to perform fling action. Create AutoScroller and drive it with further time updates.
        auto weak_ptr = std::weak_ptr<ScrollGesture>(shared_from_this());
        mScroller = AutoScroller::make(mActionable->getRootConfig(), scrollable,
           [weak_ptr]() {
               auto self = weak_ptr.lock();
               if (self) {
                   self->scrollToSnap();
               }
           },
           velocities);

        if (!mScroller) {
            reset();
        }
    } else if (scrollable->shouldForceSnap()) {
        scrollToSnap();
    } else {
        LOG_IF(DEBUG_SCROLL_GESTURE) << "Velocity " << velocity << " is too low, do not fling.";
        reset();
    }
}

void
ScrollGesture::onTimeUpdate(const PointerEvent& event, apl_time_t timestamp) {
    if (mTriggered && mScroller) {
        mScroller->update(timestamp);
    }
}

void
ScrollGesture::scrollToSnap() {
    auto snapOffset = std::dynamic_pointer_cast<ScrollableComponent>(mActionable)->getSnapOffset();

    if (snapOffset == Point()) {
        reset();
        return;
    }

    auto weak_ptr = std::weak_ptr<ScrollGesture>(shared_from_this());
    mScroller = AutoScroller::make(mActionable->getRootConfig(),
                               std::dynamic_pointer_cast<ScrollableComponent>(mActionable), [weak_ptr]() {
      auto self = weak_ptr.lock();
      if (self) {
          self->reset();
      }
    }, snapOffset, mActionable->getRootConfig().getScrollSnapDuration());
}

float
ScrollGesture::toLocalThreshold(float threshold) {
    auto scrollable = std::static_pointer_cast<ScrollableComponent>(mActionable);
    if (scrollable->isHorizontal()) {
        return std::abs(threshold * scrollable->getGlobalToLocalTransform().getXScaling());
    } else {
        return std::abs(threshold * scrollable->getGlobalToLocalTransform().getYScaling());
    }
}

} // namespace apl
