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

#include "apl/touch/gestures/pagerflinggesture.h"

#include <cmath>
#include <algorithm>

#include "apl/component/pagercomponent.h"
#include "apl/content/rootconfig.h"
#include "apl/time/sequencer.h"
#include "apl/time/timemanager.h"

#include "apl/engine/builder.h"
#include "apl/action/action.h"

#include "apl/animation/coreeasing.h"
#include "apl/touch/utils/velocitytracker.h"
#include "apl/primitives/timefunctions.h"
#include "apl/utils/session.h"

namespace apl {

static const bool DEBUG_FLING_GESTURE = false;

static inline float
getDistance(const ActionablePtr& actionable, const Point& startPosition, const Point& localPoint)
{
    auto shift = localPoint - startPosition;
    return actionable->isHorizontal() ? shift.getX() : shift.getY();
}

static inline float
getAnimationDistance(const ActionablePtr& actionable, PageDirection direction, float amount,
                     LayoutDirection layoutDirection)
{
    auto parentBounds = actionable->getCalculated(kPropertyBounds).getRect();
    auto wholeDistance = actionable->isHorizontal() ? parentBounds.getWidth() : parentBounds.getHeight();
    auto sign = direction == kPageDirectionForward ? 1.0f : -1.0f;
    if (actionable->isHorizontal() && layoutDirection == kLayoutDirectionRTL) {
        sign *= -1.0f;
    }

    return sign * amount * wholeDistance;
}

static inline float
getTranslationAmount(const ActionablePtr& actionable, float distance)
{
    auto parentBounds = actionable->getCalculated(kPropertyBounds).getRect();
    return std::abs(distance) /
           (actionable->isHorizontal() ? parentBounds.getWidth() : parentBounds.getHeight());
}

static inline size_t
calculateTargetPage(const ActionablePtr& actionable, PageDirection direction, size_t current)
{
    const int childCount = actionable->getChildCount();
    const int delta = direction == kPageDirectionForward ? 1 : childCount - 1;
    return (current + delta) % childCount;
}

std::shared_ptr<PagerFlingGesture>
PagerFlingGesture::create(const ActionablePtr& actionable)
{
    return std::make_shared<PagerFlingGesture>(actionable);
}

PagerFlingGesture::PagerFlingGesture(const ActionablePtr& actionable) :
    FlingGesture(actionable),
    mCurrentPage(0),
    mTargetPage(0),
    mPageDirection(kPageDirectionNone),
    mLastAnimationAmount(0.0f),
    mAmount(0.0f)
{
    mResourceHolder = ExecutionResourceHolder::create(kExecutionResourcePosition, actionable, [&](){reset();});
    mLayoutDirection = static_cast<LayoutDirection>(actionable->getCalculated(kPropertyLayoutDirection).asInt());
}

void
PagerFlingGesture::release()
{
    mResourceHolder->release();
    reset();
}

void
PagerFlingGesture::reset() {
    FlingGesture::reset();

    if (isTriggered())
        mResourceHolder->releaseResource();

    mPageDirection = kPageDirectionNone;
}

bool
PagerFlingGesture::onDown(const PointerEvent& event, apl_time_t timestamp)
{
    LOG_IF(DEBUG_FLING_GESTURE) << "event: " << event.pointerEventPosition.toString() << ", timestamp: " << timestamp;

    // We don't change layout direction during a gesture
    mLayoutDirection = static_cast<LayoutDirection>(mActionable->getCalculated(kPropertyLayoutDirection).asInt());
    auto startTime = mStartTime;
    if (!FlingGesture::onDown(event, timestamp)) return false;
    // If triggered and onDown received - animation currently happening
    if (isTriggered()) {
        // We stop animation but allow for chaining
        if (mAnimateAction) {
            mAnimateAction->terminate();
            mAnimateAction = nullptr;
        }

        // Figure how far we need to offset.
        auto lastDistance = getAnimationDistance(mActionable, mPageDirection, mLastAnimationAmount,
                                                 mLayoutDirection);

        // Reset non-significant (non-distance related) direction from the offset to avoid hitting angle restriction.
        auto distanceShift = mActionable->isHorizontal() ? Point(lastDistance, 0) : Point(0, lastDistance);

        // Offset start position by already covered distance.
        mStartPosition += distanceShift;
        // Restore old start time to avoid move timeout
        mStartTime = startTime;
        LOG_IF(DEBUG_FLING_GESTURE) << "Chaining. distanceShift:" << distanceShift
            << ", mStartPosition: " << mStartPosition.toString() << ", mAmount: " << mAmount
            << ", mLastAnimationAmount: " << mLastAnimationAmount;
        return true;
    }
    mCurrentPage = mTargetPage = mActionable->pagePosition();
    return true;
}

bool
PagerFlingGesture::onMove(const PointerEvent& event, apl_time_t timestamp)
{
    if (!FlingGesture::onMove(event, timestamp))
        return false;

    auto pager = std::dynamic_pointer_cast<PagerComponent>(mActionable);
    auto localPoint = mActionable->toLocalPoint(event.pointerEventPosition);
    auto distance = getDistance(mActionable, mStartPosition, localPoint);
    // Flip direction for RTL layout
    auto direction = (mActionable->isHorizontal() && mLayoutDirection == kLayoutDirectionRTL)
                         ? (distance < 0 ? kPageDirectionBack    : kPageDirectionForward)
                         : (distance < 0 ? kPageDirectionForward : kPageDirectionBack);

    LOG_IF(DEBUG_FLING_GESTURE) << "Distance: " << distance << ", direction: " << direction;

    if (mTriggered && direction != mPageDirection) {
        mTriggered = false;
        LOG_IF(DEBUG_FLING_GESTURE) << "Reverse direction from: " << mPageDirection;
    }

    auto horizontal = mActionable->isHorizontal();
    auto availableDirection = pager->pageDirection();
    auto globalToLocal = mActionable->getGlobalToLocalTransform();
    auto flingDistanceThreshold = mActionable->getRootConfig().getPointerSlopThreshold() *
        (horizontal ? globalToLocal.getXScaling() : globalToLocal.getYScaling());

    // Trigger only if distance is above the threshold AND navigation direction available
    if (!mTriggered
        && (std::abs(distance) > std::abs(flingDistanceThreshold))
        && (availableDirection == direction || availableDirection == kPageDirectionBoth)) {
        if (!isSlopeWithinTolerance(localPoint, horizontal)) {
            reset();
            return false;
        }

        mTriggered = true;
        LOG_IF(DEBUG_FLING_GESTURE) << "Triggered";
        mResourceHolder->takeResource();
        mPageDirection = direction;
        mTargetPage = calculateTargetPage(mActionable, mPageDirection, mCurrentPage);

        pager->startPageMove(mPageDirection, mCurrentPage, mTargetPage);
    }

    if (mTriggered) {
        mAmount = getTranslationAmount(mActionable, distance);
        if (mAmount > 1.0f) {
            LOG_IF(DEBUG_FLING_GESTURE) << "Moved over 100%, restarting with new page.";

            // Reset start of the gesture, we effectively switched the page
            pager->endPageMove(true, ActionRef(nullptr), false);

            // If our translation amount is over 100% then we are moving two pages. Hence We
            // need to check the available directions again for the second page. To do this we
            // need to finish the previous page move so we can check our availableDirections with
            // the second page.
            auto nextAvailableDirection = pager->pageDirection();
            if (nextAvailableDirection == direction || nextAvailableDirection == kPageDirectionBoth) {
                mStartPosition = localPoint;
                mAmount = mAmount - 1.0f;

                // Start new one from the new page
                mCurrentPage = mTargetPage;
                mTargetPage = calculateTargetPage(mActionable, mPageDirection, mCurrentPage);
                pager->startPageMove(mPageDirection, mCurrentPage, mTargetPage);
            } else {
                reset();
            }
        }
        mLastAnimationAmount = mAmount;
        pager->executePageMove(mAmount);
    }

    return true;
}

void
PagerFlingGesture::animateRemainder(bool fulfill)
{
    auto duration = mActionable->getRootConfig().getDefaultPagerAnimationDuration();
    auto remainder = fulfill ? 1.0f - mAmount : -mAmount;

    std::weak_ptr<PagerFlingGesture> weak_ptr(std::static_pointer_cast<PagerFlingGesture>(shared_from_this()));
    mAnimateAction = Action::makeAnimation(mActionable->getRootConfig().getTimeManager(), duration,
       [weak_ptr, duration, remainder](apl_duration_t offset) {
           auto self = weak_ptr.lock();
           if (self) {
               float alpha = offset/duration;
               // Float numbers could be flaky. Limit it to extremes
               alpha = std::max(0.0f, std::min(1.0f, alpha));

               auto amount = self->mAmount + alpha * remainder;
               self->mLastAnimationAmount = amount;
               std::dynamic_pointer_cast<PagerComponent>(self->mActionable)->executePageMove(amount);
           }
       });

    if (mAnimateAction && mAnimateAction->isPending()) {
        mAnimateAction->then([weak_ptr, fulfill](const ActionPtr& actionPtr){
            auto self = weak_ptr.lock();
            if (self) {
                self->awaitForFinish(weak_ptr, fulfill);
            }
        });
    } else {
        reset();
    }
}

void
PagerFlingGesture::awaitForFinish(const std::weak_ptr<PagerFlingGesture>& weak_ptr, bool fulfill)
{
    mAnimateAction = Action::make(mActionable->getRootConfig().getTimeManager(),
      [weak_ptr, fulfill](ActionRef ref) {
          auto self = weak_ptr.lock();
          if (self) {
              std::dynamic_pointer_cast<PagerComponent>(self->mActionable)->endPageMove(fulfill, ref, false);
          }
      });

    if (mAnimateAction && mAnimateAction->isPending()) {
        mAnimateAction->then([weak_ptr](const ActionPtr& actionPtr){
            auto self = weak_ptr.lock();
            if (self) {
                self->reset();
            }
        });
    } else {
        reset();
    }
}

bool
PagerFlingGesture::onUp(const PointerEvent& event, apl_time_t timestamp)
{
    if (!FlingGesture::onUp(event, timestamp))
        return false;

    mAmount = getTranslationAmount(mActionable, getDistance(mActionable, mStartPosition, event.pointerEventPosition));
    finishUp();
    return true;
}

bool
PagerFlingGesture::onCancel(const PointerEvent& event, apl_time_t timestamp)
{
    // We explicitly ignore cancel position - it should not be necessary and can be outside of Pager.
    return finishUp();
}

bool
PagerFlingGesture::finishUp()
{
    auto horizontal = mActionable->isHorizontal();
    auto globalToLocal = mActionable->getGlobalToLocalTransform();
    auto scaleFactor = (horizontal ? globalToLocal.getXScaling() : globalToLocal.getYScaling());
    if (std::isnan(scaleFactor)) {
        CONSOLE_CTP(mActionable->getContext())
            << "Singular transform encountered during page switch. Animation impossible, resetting.";
        // Reset the state of the component
        auto pager = std::dynamic_pointer_cast<PagerComponent>(mActionable);
        pager->executePageMove(0.0f);
        pager->endPageMove(false);
        // And gesture.
        reset();
        return false;
    }

    auto velocities = toLocalVector(mVelocityTracker->getEstimatedVelocity());
    auto velocity = horizontal ? velocities.getX() : velocities.getY();
    auto minFlingVelocity = std::abs(mActionable->getRootConfig().getMinimumFlingVelocity() * scaleFactor);
    auto fulfill = true;
    auto direction = (mActionable->isHorizontal() && mLayoutDirection == kLayoutDirectionRTL)
                     ? (velocity < 0 ? kPageDirectionBack    : kPageDirectionForward)
                     : (velocity < 0 ? kPageDirectionForward : kPageDirectionBack);


    direction = velocity == 0 ? mPageDirection : direction;

    // In case if we don't get enough fling or distance or fling in opposite direction - snap back.
    if ((mAmount < 0.5f && std::abs(velocity) < (minFlingVelocity / time::MS_PER_SECOND))
        || direction != mPageDirection) {
        LOG(LogLevel::kDebug) << "Do not fling with velocity: " << velocity << ", amount :" << mAmount
            << ", expected direction: " << mPageDirection << ", direction: " << direction;
        fulfill = false;
    }

    animateRemainder(fulfill);
    return true;
}

} // namespace apl
