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

#include "apl/component/scrollablecomponent.h"
#include "apl/component/pagercomponent.h"
#include "apl/content/rootconfig.h"
#include "apl/time/timemanager.h"

#include "apl/engine/builder.h"
#include "apl/action/action.h"

#include "apl/animation/coreeasing.h"
#include "apl/touch/utils/velocitytracker.h"
#include "apl/primitives/timefunctions.h"
#include "apl/utils/session.h"

namespace apl {

std::shared_ptr<PagerFlingGesture>
PagerFlingGesture::create(const ActionablePtr& actionable)
{
    return std::make_shared<PagerFlingGesture>(actionable);
}

PagerFlingGesture::PagerFlingGesture(const ActionablePtr& actionable) : Gesture(actionable),
    mVelocityTracker(new VelocityTracker(mActionable->getRootConfig())),
    mCurrentPage(0),
    mTargetPage(0),
    mPageDirection(kPageDirectionNone),
    mAmount(0)

{}

void
PagerFlingGesture::reset() {
    Gesture::reset();
    mVelocityTracker->reset();
    mActionable->enableGestures();
    mPageDirection = kPageDirectionNone;
}

void
PagerFlingGesture::onDown(const PointerEvent& event, apl_time_t timestamp)
{
    if (mTriggered) return;
    mStartPosition = mActionable->toLocalPoint(event.pointerEventPosition);
    mVelocityTracker->reset();
    mVelocityTracker->addPointerEvent(event, timestamp);
    mCurrentPage = mTargetPage = mActionable->pagePosition();
}

float
PagerFlingGesture::getDistance(const Point& currentPosition)
{
    auto shift = mActionable->toLocalPoint(currentPosition) - mStartPosition;
    return mActionable->isHorizontal() ? shift.getX() : shift.getY();
}

float
PagerFlingGesture::getAmount(float distance)
{
    auto parentBounds = mActionable->getCalculated(kPropertyBounds).getRect();
    return std::abs(distance) /
           (mActionable->isHorizontal() ? parentBounds.getWidth() : parentBounds.getHeight());
}

void
PagerFlingGesture::onMove(const PointerEvent& event, apl_time_t timestamp)
{
    mVelocityTracker->addPointerEvent(event, timestamp);

    auto pager = std::dynamic_pointer_cast<PagerComponent>(mActionable);
    auto distance = getDistance(event.pointerEventPosition);
    auto direction = distance < 0 ? kPageDirectionForward : kPageDirectionBack;

    if (mTriggered && direction != mPageDirection) {
        mTriggered = false;
        pager->resetPage(mCurrentPage);
        pager->resetPage(mTargetPage);
    }

    auto availableDirection = pager->pageDirection();
    auto globalToLocal = mActionable->getGlobalToLocalTransform();
    auto flingDistanceThreshold = mActionable->getRootConfig().getPointerSlopThreshold() *
        (mActionable->isHorizontal() ? globalToLocal.getXScaling() : globalToLocal.getYScaling());

    // Trigger only if distance is above the threshold ANG navigation direction available
    if (!mTriggered
        && (std::abs(distance) > std::abs(flingDistanceThreshold))
        && (availableDirection == direction || availableDirection == kPageDirectionBoth)) {
        mTriggered = true;
        mPageDirection = direction;

        const int childCount = mActionable->getChildCount();
        const int delta = mPageDirection == kPageDirectionForward ? 1 : childCount - 1;
        mTargetPage = (mCurrentPage + delta) % childCount;

        pager->startPageMove(mPageDirection, mCurrentPage, mTargetPage);
    }

    if (mTriggered) {
        mAmount = getAmount(distance);
        pager->executePageMove(mAmount);
    }
}

void
PagerFlingGesture::animateRemainder(bool fulfill)
{
    // Stop gestures from propagating to this component. It mekes no sense in between the pages.
    mActionable->disableGestures();

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

void
PagerFlingGesture::onUp(const PointerEvent& event, apl_time_t timestamp)
{
    if (!mTriggered) {
        reset();
        return;
    }

    mVelocityTracker->addPointerEvent(event, timestamp);
    mAmount = getAmount(getDistance(event.pointerEventPosition));
    finishUp();
}

void
PagerFlingGesture::onCancel(const PointerEvent& event, apl_time_t timestamp)
{
    // We explicitly ignore cancel position - it should not be necessary and can be outside of Pager.
    finishUp();
}

void
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
        return;
    }

    auto velocities = toLocalVector(mVelocityTracker->getEstimatedVelocity());
    auto velocity = horizontal ? velocities.getX() : velocities.getY();
    auto minFlingVelocity = std::abs(mActionable->getRootConfig().getMinimumFlingVelocity() * scaleFactor);
    auto fulfill = true;
    auto direction = velocity < 0 ? kPageDirectionForward : kPageDirectionBack;
    direction = velocity == 0 ? mPageDirection : direction;

    // In case if we don't get enough fling or distance or fling in opposite direction - snap back.
    if ((mAmount < 0.5f && std::abs(velocity) < (minFlingVelocity / time::MS_PER_SECOND))
        || direction != mPageDirection) {
        LOG(LogLevel::DEBUG) << "Do not fling with velocity: " << velocity << ", amount :" << mAmount
            << ", expected direction: " << mPageDirection << ", direction: " << direction;
        fulfill = false;
    }

    animateRemainder(fulfill);
}

} // namespace apl
