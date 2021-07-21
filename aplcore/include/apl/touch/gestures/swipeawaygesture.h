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

#ifndef _APL_SWIPE_AWAY_GESTURE_H
#define _APL_SWIPE_AWAY_GESTURE_H

#include "apl/common.h"
#include "apl/primitives/object.h"
#include "apl/touch/gestures/flinggesture.h"

namespace apl {

class InterpolatedTransformation;
class VelocityTracker;

/**
 * Enumeration of swipe actions
 */
enum SwipeAwayActionType {
    kSwipeAwayActionReveal,
    kSwipeAwayActionSlide,
    kSwipeAwayActionCover,
};

class SwipeAwayGesture : public FlingGesture, public std::enable_shared_from_this<SwipeAwayGesture> {
public:
    static std::shared_ptr<SwipeAwayGesture> create(const ActionablePtr& actionable, const Context& context, const Object& object);

    SwipeAwayGesture(const ActionablePtr& actionable, SwipeAwayActionType action, SwipeDirection direction,
                     Object&& onSwipeMove, Object&& onSwipeDone, Object&& items);
    virtual ~SwipeAwayGesture() = default;

    void reset() override;
    bool invokeAccessibilityAction(const std::string& name) override;

protected:
    bool onMove(const PointerEvent& event, apl_time_t timestamp) override;
    bool onDown(const PointerEvent& event, apl_time_t timestamp) override;
    bool onUp(const PointerEvent& event, apl_time_t timestamp) override;

private:
    float getMove(SwipeDirection direction, Point localPos);
    int getFulfillMoveDirection();
    std::shared_ptr<InterpolatedTransformation> getTransformation(bool above);
    void processTransformChange(float alpha);
    void animateRemainder(bool fulfilled, float velocity);
    void sendSwipeMove(float travelPercentage);
    float toLocalThreshold(float threshold);
    float getCurrentVelocity();
    bool swipedFarEnough();
    bool swipedFastEnough(float velocity);

    SwipeAwayActionType mAction;
    SwipeDirection mDirection;
    SwipeDirection mDirectionAssigned;
    Object mOnSwipeMove;
    Object mOnSwipeDone;
    Object mItems;
    float mLocalDistance;
    float mTraveledDistance;
    CoreComponentPtr mReplacedComponent;
    CoreComponentPtr mSwipeComponent;
    std::shared_ptr<InterpolatedTransformation> mReplaceTransformation;
    std::shared_ptr<InterpolatedTransformation> mSwipeTransformation;
    ActionPtr mAnimateAction;
    EasingPtr mAnimationEasing;
    float mInitialMove;
};

} // namespace apl

#endif //_APL_SWIPE_AWAY_GESTURE_H
