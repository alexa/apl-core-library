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
#include "apl/time/sequencer.h"

namespace apl {

static const bool DEBUG_SCROLL_GESTURE = false;

std::shared_ptr<ScrollGesture>
ScrollGesture::create(const ActionablePtr& actionable) {
    return std::make_shared<ScrollGesture>(actionable);
}

ScrollGesture::ScrollGesture(const ActionablePtr& actionable)
  : FlingGesture(actionable)
{
    mResourceHolder = ExecutionResourceHolder::create(kExecutionResourcePosition, actionable, [&](){reset();});
}

void
ScrollGesture::release()
{
    LOG_IF(DEBUG_SCROLL_GESTURE) << "Release";
    mResourceHolder->release();
    reset();
}

void
ScrollGesture::reset()
{
    LOG_IF(DEBUG_SCROLL_GESTURE) << "Reset";
    FlingGesture::reset();

    if (isTriggered())
        mResourceHolder->releaseResource();

    mScroller = nullptr;
}

bool
ScrollGesture::onMove(const PointerEvent& event, apl_time_t timestamp)
{
    if (!FlingGesture::onMove(event, timestamp))
        return false;

    auto scrollable = getScrollable();
    auto position = scrollable->toLocalPoint(event.pointerEventPosition);
    auto delta = position - mStartPosition;
    if (!delta.isFinite()) {
        // TODO: log singularity once to the session once this functionality is available
        return false;
    }

    if (!isTriggered()) {
        auto triggerDistance = scrollable->isHorizontal() ? delta.getX() : delta.getY();
        auto flingTriggerDistanceThreshold = toLocalThreshold(scrollable->getRootConfig().getPointerSlopThreshold());
        if (std::abs(triggerDistance) > flingTriggerDistanceThreshold) {
            if (!isSlopeWithinTolerance(position)) {
                reset();
                return false;
            }
            LOG_IF(DEBUG_SCROLL_GESTURE) << "Triggering";
            mTriggered = true;
            mResourceHolder->takeResource();
        }
    }

    if (isTriggered()) {
        auto scrollPosition = scrollable->scrollPosition();
        scrollPosition = scrollable->trimScroll(scrollPosition - (position - mLastLocalPosition));

        scrollable->update(UpdateType::kUpdateScrollPosition, scrollable->isHorizontal() ? scrollPosition.getX()
                                                                                          : scrollPosition.getY());
    }

    mLastLocalPosition = position;
    return true;
}

bool
ScrollGesture::onDown(const PointerEvent& event, apl_time_t timestamp)
{
    if (!FlingGesture::onDown(event, timestamp)) return false;
    if (isTriggered()) {
        // We stop animation but allow for chaining of scrolls
        // TODO: When deceleration physics is it it makes sense to pass impulse in instead of chaining.
        mScroller = nullptr;
    }
    mLastLocalPosition = mStartPosition;
    return true;
}

double
ScrollGesture::getVelocityLimit(const Point& travel) {
    auto scrollable = getScrollable();
    const auto& rootConfig = scrollable->getRootConfig();

    auto maxTravel = scrollable->isVertical() ? mActionable->getContext()->height()
                                              : mActionable->getContext()->width();
    auto velocityEasing = scrollable->isVertical() ? rootConfig.getProperty(RootProperty::kScrollFlingVelocityLimitEasingVertical).getEasing()
                                                   : rootConfig.getProperty(RootProperty::kScrollFlingVelocityLimitEasingHorizontal).getEasing();

    auto directionalTravel = std::abs(scrollable->isVertical() ? travel.getY() : travel.getX());

    LOG_IF(DEBUG_SCROLL_GESTURE) << "maxTravel " << maxTravel << " directionalTravel " << directionalTravel;

    auto maxVelocity = toLocalThreshold(rootConfig.getProperty(RootProperty::kMaximumFlingVelocity).getDouble());

    auto limit = maxVelocity;
    // If fling distance is lower than defined limit - limit the velocity to mimic existing device behavior more
    if (directionalTravel <= maxTravel) {
        auto alpha = std::min(1.0, directionalTravel/maxTravel);
        // Just linear distribution from minVelocity to maxVelocity across 0 to longFlingStart as limit
        limit = maxVelocity * velocityEasing->calc(alpha);

        LOG_IF(DEBUG_SCROLL_GESTURE) << " maxVelocity " << maxVelocity << " alpha " << alpha << " limit " << limit;
    }

    return limit / time::MS_PER_SECOND;
}

bool
ScrollGesture::onUp(const PointerEvent& event, apl_time_t timestamp)
{
    if (!FlingGesture::onUp(event, timestamp))
        return false;

    auto scrollable = getScrollable();
    auto velocities = toLocalVector(mVelocityTracker->getEstimatedVelocity());
    auto velocity = scrollable->isVertical() ? velocities.getY() : velocities.getX();
    if (std::isnan(velocity)) {
        CONSOLE_CTP(scrollable->getContext()) << "Singularity encountered during scroll, resetting";
        reset();
        return false;
    }

    // TODO: Acting on "significant" direction. Will will need this to be changed when we have
    //  multidirectional scrolling.
    const auto& rootConfig = scrollable->getRootConfig();
    if (std::abs(velocity) >= toLocalThreshold(rootConfig.getMinimumFlingVelocity()) /
    time::MS_PER_SECOND) {
        auto position =  scrollable->toLocalPoint(event.pointerEventPosition);
        auto delta = position - mStartPosition;
        auto velocityLimit = getVelocityLimit(delta);
        if (std::abs(velocity) > velocityLimit) {
            LOG_IF(DEBUG_SCROLL_GESTURE)
                << "Velocity " << velocity << " is too fast, reset to " << velocityLimit;
            velocity = velocity > 0 ? velocityLimit : -velocityLimit;
            velocities = Point(velocity, velocity);
        }

        // We have enough velocity to perform fling action. Create AutoScroller and drive it with further time updates.
        auto weak_ptr = std::weak_ptr<ScrollGesture>(shared_from_this());
        mScroller = AutoScroller::make(rootConfig, scrollable,
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
    return true;
}

bool
ScrollGesture::onTimeUpdate(const PointerEvent& event, apl_time_t timestamp)
{
    if (mTriggered && mScroller)
        mScroller->update(timestamp);
    return true;
}

void
ScrollGesture::scrollToSnap()
{
    auto scrollable = getScrollable();
    auto snapOffset = scrollable->getSnapOffset();

    if (snapOffset == Point()) {
        reset();
        return;
    }

    auto weak_ptr = std::weak_ptr<ScrollGesture>(shared_from_this());
    const auto& rootConfig = scrollable->getRootConfig();
    mScroller = AutoScroller::make(rootConfig, scrollable, [weak_ptr]() {
      auto self = weak_ptr.lock();
      if (self) {
          self->reset();
      }
    }, snapOffset, rootConfig.getScrollSnapDuration());
}

float
ScrollGesture::toLocalThreshold(float threshold)
{
    auto scrollable = getScrollable();
    if (scrollable->isHorizontal()) {
        return std::abs(threshold * scrollable->getGlobalToLocalTransform().getXScaling());
    } else {
        return std::abs(threshold * scrollable->getGlobalToLocalTransform().getYScaling());
    }
}

bool
ScrollGesture::isSlopeWithinTolerance(Point localPosition)
{
    if (!localPosition.isFinite() || !mStartPosition.isFinite()) {
        return false;
    }

    auto& config = mActionable->getRootConfig();
    auto pointerDelta = localPosition - mStartPosition;
    auto scrollable = getScrollable();
    if (scrollable->isHorizontal()) {
        return std::abs(pointerDelta.getX()) * config.getProperty(RootProperty::kScrollAngleSlopeHorizontal).getDouble()
            >= std::abs(pointerDelta.getY());
    } else {
        return std::abs(pointerDelta.getY()) * config.getProperty(RootProperty::kScrollAngleSlopeVertical).getDouble()
            >= std::abs(pointerDelta.getX());
    }
}

} // namespace apl
