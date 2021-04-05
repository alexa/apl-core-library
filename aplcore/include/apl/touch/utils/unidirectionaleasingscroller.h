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

#ifndef _APL_UNIDIRECTIONAL_EASING_SCROLLER_H
#define _APL_UNIDIRECTIONAL_EASING_SCROLLER_H

#include "apl/touch/utils/autoscroller.h"

namespace apl {

/**
 * Simple scroller that supports only vertical OR horizontal scrolling that uses ease-out to simulate
 * velocity slowing down.
 */
class UnidirectionalEasingScroller : public AutoScroller {
public:
    /**
     * Make scroller from starting velocity. Usually touch movement driven.
     * @param scrollable target component.
     * @param finish callback to call when scrolling finished.
     * @param velocity starting velocity.
     * @return shared pointer to scroller object.
     */
    static std::shared_ptr<UnidirectionalEasingScroller>
    make(const std::shared_ptr<ScrollableComponent>& scrollable, FinishFunc&& finish, const Point& velocity);

    /**
     * Make scroller from target position and duration. Usually programmatically defined.
     * @param scrollable target component.
     * @param finish callback to call when scrolling finished.
     * @param target target scrolling shift.
     * @param duration animation duration.
     * @return shared pointer to scroller object.
     */
    static std::shared_ptr<UnidirectionalEasingScroller>
    make(const std::shared_ptr<ScrollableComponent>& scrollable, FinishFunc&& finish, const Point& target,
            apl_duration_t duration);

    /**
     * Constructor. Do not use directly, see UnidirectionalEasingScroller::make
     */
    UnidirectionalEasingScroller(const std::shared_ptr<ScrollableComponent>& scrollable, const EasingPtr& easing,
        FinishFunc&& finish, const Point& target, apl_duration_t duration);

    /**
     * @return Calculated or provided animation duration.
     */
    apl_duration_t getDuration() const override { return mDuration; }

protected:
    void update(const std::shared_ptr<ScrollableComponent>& scrollable, apl_duration_t offset) override;

private:
    void fixFlingStartPosition(const std::shared_ptr<ScrollableComponent>& scrollable);

private:
    EasingPtr mAnimationEasing;
    Point mScrollStartPosition;
    Point mLastScrollPosition;
    Point mEndTarget;
    apl_duration_t mDuration;
};

} // namespace apl

#endif //_APL_UNIDIRECTIONAL_EASING_SCROLLER_H
