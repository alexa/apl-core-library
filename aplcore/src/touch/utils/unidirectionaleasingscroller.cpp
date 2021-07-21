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

#include "apl/touch/utils/unidirectionaleasingscroller.h"

#include "apl/animation/coreeasing.h"
#include "apl/component/scrollablecomponent.h"
#include "apl/content/rootconfig.h"
#include "apl/primitives/timefunctions.h"

namespace apl {

const bool DEBUG_SCROLLER = false;

std::shared_ptr<UnidirectionalEasingScroller>
UnidirectionalEasingScroller::make(
        const std::shared_ptr<ScrollableComponent>& scrollable,
        FinishFunc&& finish,
        const Point& velocity)
{
    // Acting on "significant" direction.
    auto directionalVelocity = scrollable->isVertical() ?
                               velocity.getY() : velocity.getX();

    if (directionalVelocity == 0) {
        LOG(LogLevel::kWarn) << "Can't create a scroller with 0 velocity";
        return nullptr;
    }

    auto& rootConfig = scrollable->getRootConfig();
    auto decelerationRate = rootConfig.getProperty(RootProperty::kUEScrollerDeceleration).getDouble();
    auto deceleration = -directionalVelocity * decelerationRate;

    LOG_IF(DEBUG_SCROLLER)
        << "Velocity: " << velocity.toString()
        << ", Deceleration" << decelerationRate;

    // S = v^2 / 2*a
    auto distance = (directionalVelocity * directionalVelocity) / (2 * deceleration) * time::MS_PER_SECOND;
    auto easing = rootConfig.getProperty(RootProperty::kUEScrollerVelocityEasing).getEasing();
    auto maxDuration = rootConfig.getProperty(RootProperty::kUEScrollerMaxDuration).getDouble();
    auto duration = std::min(static_cast<apl_duration_t>(std::abs(distance/directionalVelocity)), maxDuration);
    return std::make_shared<UnidirectionalEasingScroller>(
            scrollable, easing, std::move(finish), Point(distance, distance), duration);
}

std::shared_ptr<UnidirectionalEasingScroller>
UnidirectionalEasingScroller::make(
        const std::shared_ptr<ScrollableComponent>& scrollable,
        FinishFunc&& finish,
        const Point& target,
        apl_duration_t duration)
{
    assert(duration > 0);
    auto easing = scrollable->getRootConfig().getProperty(RootProperty::kUEScrollerDurationEasing).getEasing();
    return std::make_shared<UnidirectionalEasingScroller>(scrollable, easing, std::move(finish), target, duration);
}

UnidirectionalEasingScroller::UnidirectionalEasingScroller(
        const std::shared_ptr<ScrollableComponent>& scrollable,
        const EasingPtr& easing,
        FinishFunc&& finish,
        const Point& target,
        apl_duration_t duration)
        : AutoScroller(scrollable, std::move(finish)),
          mAnimationEasing(easing),
          mScrollStartPosition(scrollable->scrollPosition()),
          mLastScrollPosition(scrollable->scrollPosition()),
          mEndTarget(target),
          mDuration(duration)
{
    LOG_IF(DEBUG_SCROLLER)
        << "StartPos: " << mScrollStartPosition.toString()
        << ", LastPos: " << mLastScrollPosition.toString()
        << ", TargetDistance: " << mEndTarget.toString();
}

void
UnidirectionalEasingScroller::update(
        const ScrollablePtr& scrollable,
        apl_duration_t offset)
{
    float alpha = mAnimationEasing->calc(offset / mDuration);
    auto delta = Point(alpha * mEndTarget.getX(), alpha * mEndTarget.getY());
    Point end = mEndTarget;

    // May have got more items in the interim, or as a result of scroll position update
    fixFlingStartPosition(scrollable);
    auto availablePosition = scrollable->trimScroll(mScrollStartPosition + end);

    // This could lead to more loading, so fix fling and re-adjust
    fixFlingStartPosition(scrollable);

    // Direction dependent.
    auto vertical = scrollable->isVertical();
    auto horizontal = scrollable->isHorizontal();
    bool isRTL = scrollable->getCalculated(kPropertyLayoutDirection) == kLayoutDirectionRTL;
    auto targetPoint = mScrollStartPosition + delta;
    auto endTargetPoint = mScrollStartPosition + end;
    auto target = vertical ? targetPoint.getY() : targetPoint.getX();
    auto endTarget = vertical ? endTargetPoint.getY() : endTargetPoint.getX();
    auto availableTarget = vertical ? availablePosition.getY() : availablePosition.getX();

    float resultingPosition;
    if (horizontal && isRTL) {
        resultingPosition = target >= endTarget ? std::max(availableTarget, target)
                                                : std::min(availableTarget, target);
    } else {
        resultingPosition = target <= endTarget ? std::min(availableTarget, target)
                                                : std::max(availableTarget, target);
    }

    // Set it to what we expect it to be, because it may move afterwards
    mLastScrollPosition = vertical ? Point(0, resultingPosition) : Point(resultingPosition, 0);
    scrollable->update(UpdateType::kUpdateScrollPosition, resultingPosition);

    // TODO: If update will bring us over maximum position we should switch to "overscroll" instead
    //  of stopping. It's not default behavior though. Can be resolved with "Overscroller" mixin?.
    if (offset <= 0)
        return;
    bool isFinished = (horizontal && isRTL)
                          ? (target >= 0 || (endTarget < target && availableTarget >= target) || offset == mDuration)
                          : (target <= 0 || (endTarget > target && availableTarget <= target) || offset == mDuration);
    if (isFinished) {
       finish();
    }
}

void
UnidirectionalEasingScroller::fixFlingStartPosition(const std::shared_ptr<ScrollableComponent>& scrollable)
{
    auto currentPosition = scrollable->scrollPosition();

    // If position is not the same it was likely changed by dynamic changes, we need to
    // adjust for that.
    if (currentPosition != mLastScrollPosition) {
        auto diff = currentPosition - mLastScrollPosition;
        mScrollStartPosition += diff;
        mLastScrollPosition = currentPosition;
        LOG_IF(DEBUG_SCROLLER) << "Diff: " << diff.toString();
    }
}

} // namespace apl