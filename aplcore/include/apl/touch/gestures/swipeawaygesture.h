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
#include "apl/touch/gesture.h"

namespace apl {

class InterpolatedTransformation;

/**
 * Enumeration of swipe actions
 */
enum SwipeAwayActionType {
    kSwipeAwayActionReveal,
    kSwipeAwayActionSlide,
    kSwipeAwayActionCover,
};

/**
 * Enumeration of swipe directions
 */
enum SwipeAwayDirection {
    kSwipeAwayDirectionLeft,
    kSwipeAwayDirectionRight,
    kSwipeAwayDirectionUp,
    kSwipeAwayDirectionDown
};

class SwipeAwayGesture : public Gesture, public std::enable_shared_from_this<SwipeAwayGesture> {
public:
    static std::shared_ptr<SwipeAwayGesture> create(const TouchablePtr& touchable, const Context& context, const Object& object);

    SwipeAwayGesture(const TouchablePtr& touchable, SwipeAwayActionType action, SwipeAwayDirection direction,
                     Object&& onSwipeMove, Object&& onSwipeDone, Object&& items);
    virtual ~SwipeAwayGesture() = default;

    void reset() override;

    GestureType getType() const override { return kGestureTypeSwipeAway; };

protected:
    void onMove(const PointerEvent& event, apl_time_t timestamp) override;
    void onDown(const PointerEvent& event, apl_time_t timestamp) override;
    void onUp(const PointerEvent& event, apl_time_t timestamp) override;

private:
    float getMove(SwipeAwayDirection direction, Point pos);
    std::shared_ptr<InterpolatedTransformation> getTransformation(bool above);
    void processTransformChange(float alpha);
    void animateRemainder(bool fulfilled, float velocity);
    void updateVelocity(float distanceDiff, apl_time_t timestamp);

    SwipeAwayActionType mAction;
    SwipeAwayDirection mDirection;
    Object mOnSwipeMove;
    Object mOnSwipeDone;
    Object mItems;
    float mDistance;
    Point mPosition;
    float mTraveledDistance;
    CoreComponentPtr mReplacedComponent;
    CoreComponentPtr mSwipeComponent;
    std::shared_ptr<InterpolatedTransformation> mReplaceTransformation;
    std::shared_ptr<InterpolatedTransformation> mSwipeTransformation;
    ActionPtr mAnimateAction;
    std::shared_ptr<Easing> mAnimationEasing;
    apl_time_t mTimestamp;
    float mInitialMove;
    float mVelocity;
    float mTriggerDistanceThreshold;
    float mFulfillDistanceThreshold;
    float mVelocityThreshold;
};

} // namespace apl

#endif //_APL_SWIPE_AWAY_GESTURE_H
