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
#include "apl/primitives/timefunctions.h"

namespace apl {

/// Our base deceleration is 20% of starting speed per second.
static const float BASE_DECELERATION_PERCENTAGE = 0.2;

inline float
getDeceleration(float velocity)
{
    return -velocity * BASE_DECELERATION_PERCENTAGE;
}

std::shared_ptr<UnidirectionalEasingScroller>
UnidirectionalEasingScroller::make(const std::shared_ptr<ScrollableComponent>& scrollable,
    FinishFunc finish, const Point& velocity)
{
    // Acting on "significant" direction.
    auto directionalVelocity = scrollable->isVertical() ?
                               velocity.getY() : velocity.getX();

    if (directionalVelocity == 0) {
        LOG(LogLevel::WARN) << "Can't create a scroller with 0 velocity";
        return nullptr;
    }

    auto deceleration = getDeceleration(directionalVelocity);

    // S = v^2 / 2*a
    auto distance = (directionalVelocity * directionalVelocity) / (2 * deceleration) * time::MS_PER_SECOND;
    return make(scrollable, std::move(finish), Point(distance, distance), std::abs(distance/directionalVelocity));
}

std::shared_ptr<UnidirectionalEasingScroller>
UnidirectionalEasingScroller::make(const std::shared_ptr<ScrollableComponent>& scrollable,
    FinishFunc finish, const Point& target, apl_duration_t duration)
{
    assert(duration > 0);
    return std::make_shared<UnidirectionalEasingScroller>(scrollable, finish,
        target, duration);
}

UnidirectionalEasingScroller::UnidirectionalEasingScroller(
        const std::shared_ptr<ScrollableComponent>& scrollable,
        FinishFunc finish,
        const Point& target,
        apl_duration_t duration)
        : AutoScroller(scrollable, std::move(finish)),
          mAnimationEasing(CoreEasing::bezier(0,0,0.58,1)),
          mScrollStartPosition(scrollable->scrollPosition()),
          mLastScrollPosition(scrollable->scrollPosition()),
          mEndTarget(target),
          mDuration(duration)
{}

void
UnidirectionalEasingScroller::update(
        const ScrollablePtr& scrollable,
        apl_duration_t offset)
{
    float alpha = mAnimationEasing->calc(offset / mDuration);
    auto delta = Point(alpha * mEndTarget.getX(), alpha * mEndTarget.getY());
    Point end = mEndTarget;

    // This could lead to more loading, so fix fling and re-adjust
    auto availablePosition = scrollable->trimScroll(
        Point::bottomRightBound(Point::bottomRightBound(mScrollStartPosition + end, Point()),
            mScrollStartPosition + delta));
    fixFlingStartPosition(scrollable);

    // Direction dependent.
    auto vertical = scrollable->isVertical();
    auto targetPoint = mScrollStartPosition + delta;
    auto endTargetPoint = mScrollStartPosition + end;
    auto target = vertical ? targetPoint.getY() : targetPoint.getX();
    auto endTarget = vertical ? endTargetPoint.getY() : endTargetPoint.getX();
    auto availableTarget = vertical ? availablePosition.getY() : availablePosition.getX();

    float resultingPosition = target <= endTarget ? std::min(availableTarget, target)
                                                  : std::max(availableTarget, target);

    scrollable->update(UpdateType::kUpdateScrollPosition, resultingPosition);
    mLastScrollPosition = scrollable->scrollPosition();

    // TODO: If update will bring us over maximum position we should switch to "overscroll" instead
    //  of stopping. It's not default behavior though. Can be resolved with "Overscroller" mixin?.
    if (target <= 0 || (endTarget > target && availableTarget <= target) || offset == mDuration) {
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
        mScrollStartPosition += (currentPosition - mLastScrollPosition);
        mLastScrollPosition = currentPosition;
    }
}

} // namespace apl