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

#include "apl/touch/gestures/swipeawaygesture.h"

#include <cmath>

#include "apl/engine/evaluate.h"
#include "apl/component/touchwrappercomponent.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"
#include "apl/engine/propdef.h"

#include "apl/content/rootconfig.h"
#include "apl/primitives/timefunctions.h"
#include "apl/time/timemanager.h"
#include "apl/touch/utils/velocitytracker.h"

#include "apl/engine/builder.h"
#include "apl/action/action.h"

namespace apl {

static const Bimap<SwipeAwayActionType, std::string> sSwipeAwayActionTypeBimap = {
    {kSwipeAwayActionReveal, "reveal"},
    {kSwipeAwayActionSlide,  "slide"},
    {kSwipeAwayActionCover,  "cover"},
};

std::shared_ptr<SwipeAwayGesture>
SwipeAwayGesture::create(const ActionablePtr& actionable, const Context& context, const Object& object)
{
    // Only TouchWrapper supported ATM
    if (actionable->getType() != kComponentTypeTouchWrapper) {
        return nullptr;
    }

    auto action = propertyAsMapped<SwipeAwayActionType>(context, object, "action", kSwipeAwayActionSlide,
                                                        sSwipeAwayActionTypeBimap);
    if (action == static_cast<SwipeAwayActionType>(-1)) {
        CONSOLE_CTX(context) << "Unrecognized action field in SwipeAway gesture handler";
        return nullptr;
    }

    auto direction = propertyAsMapped<int>(context, object, "direction", -1, sSwipeDirectionMap);
    if (direction < 0) {
        CONSOLE_CTX(context) << "Unrecognized direction field in SwipeAway gesture handler";
        return nullptr;
    }

    Object onSwipeMove = arrayifyPropertyAsObject(context, object, "onSwipeMove");
    Object onSwipeDone = arrayifyPropertyAsObject(context, object, "onSwipeDone");
    Object items = arrayifyPropertyAsObject(context, object, "item", "items");

    return std::make_shared<SwipeAwayGesture>(actionable, action, static_cast<SwipeDirection>(direction),
        std::move(onSwipeMove), std::move(onSwipeDone), std::move(items));
}

SwipeAwayGesture::SwipeAwayGesture(const ActionablePtr& actionable, SwipeAwayActionType action,
                                   SwipeDirection direction, Object&& onSwipeMove, Object&& onSwipeDone,
                                   Object&& items) :
        FlingGesture(actionable),
        mAction(action),
        mDirection(direction),
        mDirectionAssigned(direction),
        mOnSwipeMove(std::move(onSwipeMove)),
        mOnSwipeDone(std::move(onSwipeDone)),
        mItems(std::move(items)),
        mLocalDistance(0),
        mTraveledDistance(0),
        mAnimationEasing(actionable->getRootConfig().getSwipeAwayAnimationEasing()),
        mInitialMove(0) {}

void
SwipeAwayGesture::reset()
{
    if (mAnimateAction) {
        mAnimateAction->terminate();
    }

    // Gesture in progress
    if (mSwipeComponent && mTriggered) {
        // Reset to initial state to mimic Android's behavior.
        processTransformChange(0);
        mSwipeComponent->remove();
        mSwipeComponent->release();
        mSwipeComponent = nullptr;
    }
    mTraveledDistance = 0;
    FlingGesture::reset();
}

bool
SwipeAwayGesture::invokeAccessibilityAction(const std::string& name)
{
    if (name == "swipeaway") {
        mActionable->executeEventHandler("SwipeDone", mOnSwipeDone, false,
                                         mActionable->createTouchEventProperties(Point()));
        return true;
    }
    return false;
}


float
SwipeAwayGesture::getMove(SwipeDirection direction, Point localPos)
{
    if (mStartPosition == localPos) {
        // No move happened
        return 0;
    }

    auto delta = mStartPosition - localPos;
    auto horizontal = delta.getX();
    auto vertical = delta.getY();

    switch (direction) {
        case SwipeDirection::kSwipeDirectionLeft: return horizontal >= 0 ? horizontal : 0;
        case SwipeDirection::kSwipeDirectionRight: return horizontal <= 0 ? -horizontal : 0;
        case SwipeDirection::kSwipeDirectionUp: return vertical >= 0 ? vertical : 0;
        case SwipeDirection::kSwipeDirectionDown: return vertical <= 0 ? -vertical : 0;
        default: return 0;
    }
}

int
SwipeAwayGesture::getFulfillMoveDirection()
{
    if (mDirection == SwipeDirection::kSwipeDirectionLeft
     || mDirection == SwipeDirection::kSwipeDirectionUp)
        return -1;

    return 1;
}

bool
SwipeAwayGesture::onDown(const PointerEvent& event, apl_time_t timestamp)
{
    if (!FlingGesture::onDown(event, timestamp))
        return false;

    auto touchableBounds = mActionable->getGlobalBounds();

    auto layoutDirection = static_cast<LayoutDirection>(mActionable->getCalculated(kPropertyLayoutDirection).asInt());
    if (mDirectionAssigned == kSwipeDirectionForward) {
        mDirection = (layoutDirection == kLayoutDirectionRTL ? kSwipeDirectionLeft  : kSwipeDirectionRight);
    } else if (mDirectionAssigned == kSwipeDirectionBackward) {
        mDirection = (layoutDirection == kLayoutDirectionRTL ? kSwipeDirectionRight : kSwipeDirectionLeft);
    }

    // Need just directional distance really.
    if (mDirection == SwipeDirection::kSwipeDirectionLeft ||
        mDirection == SwipeDirection::kSwipeDirectionRight) {
        mLocalDistance = touchableBounds.getWidth();
    } else {
        mLocalDistance = touchableBounds.getHeight();
    }

    mInitialMove = getMove(mDirection, mStartPosition);
    mTraveledDistance = mInitialMove/mLocalDistance;
    return true;
}

std::shared_ptr<InterpolatedTransformation>
SwipeAwayGesture::getTransformation(bool above)
{
    auto bounds = mActionable->getCalculated(kPropertyInnerBounds).getRect();
    auto from = std::make_shared<ObjectMap>();
    auto to = std::make_shared<ObjectMap>();

    if (above) {
        to->emplace("translateX", Object(0));
        to->emplace("translateY", Object(0));

        switch (mDirection) {
            case SwipeDirection::kSwipeDirectionLeft:
                from->emplace("translateX", Object(bounds.getWidth()));
                break;
            case SwipeDirection::kSwipeDirectionRight:
                from->emplace("translateX", Object(-bounds.getWidth()));
                break;
            case SwipeDirection::kSwipeDirectionUp:
                from->emplace("translateY", Object(bounds.getHeight()));
                break;
            case SwipeDirection::kSwipeDirectionDown:
                from->emplace("translateY", Object(-bounds.getHeight()));
                break;
            default:
                break;
        }
    } else {
        from->emplace("translateX", Object(0));
        from->emplace("translateY", Object(0));

        switch (mDirection) {
            case SwipeDirection::kSwipeDirectionLeft:
                to->emplace("translateX", Object(-bounds.getWidth()));
                break;
            case SwipeDirection::kSwipeDirectionRight:
                to->emplace("translateX", Object(bounds.getWidth()));
                break;
            case SwipeDirection::kSwipeDirectionUp:
                to->emplace("translateY", Object(-bounds.getHeight()));
                break;
            case SwipeDirection::kSwipeDirectionDown:
                to->emplace("translateY", Object(bounds.getHeight()));
                break;
            default:
                break;
        }
    }

    return InterpolatedTransformation::create(*mActionable->getContext(), {from}, {to});
}

void
SwipeAwayGesture::processTransformChange(float alpha)
{
    if (mAction == SwipeAwayActionType::kSwipeAwayActionReveal || mAction == SwipeAwayActionType::kSwipeAwayActionSlide) {
        mReplaceTransformation->interpolate(alpha);
        mReplacedComponent->markProperty(kPropertyTransformAssigned);
    }
    if (mAction == SwipeAwayActionType::kSwipeAwayActionCover || mAction == SwipeAwayActionType::kSwipeAwayActionSlide) {
        mSwipeTransformation->interpolate(alpha);
        mSwipeComponent->markProperty(kPropertyTransformAssigned);
    }
}

void
SwipeAwayGesture::sendSwipeMove(float travelPercentage)
{
    auto params = std::make_shared<ObjectMap>();
    params->emplace("position", travelPercentage);
    params->emplace("direction", sSwipeDirectionMap.at(mDirection));
    mActionable->executeEventHandler("SwipeMove", mOnSwipeMove, true, params);
}

bool
SwipeAwayGesture::onMove(const PointerEvent& event, apl_time_t timestamp)
{
    if (!FlingGesture::onMove(event, timestamp))
        return false;

    Point localPoint = mActionable->toLocalPoint(event.pointerEventPosition);
    float move;
    if (localPoint.isFinite()) {
        move = getMove(mDirection, localPoint);
    } else {
        // TODO: log singularity once to the session once this functionality is available
        move = 0;
    }

    // Limit move by distance. We don't need "overswiping"
    move = std::min(move, mLocalDistance);

    auto distanceDiff = move - mTraveledDistance;

    if (distanceDiff == 0) {
        return false;
    }

    mTraveledDistance = move;

    auto travelPercentage = mTraveledDistance/mLocalDistance;

    if (isTriggered()) {
        if (move >= 0) {
            processTransformChange(travelPercentage);
        }
    } else {
        if (move - mInitialMove > toLocalThreshold(mActionable->getRootConfig().getPointerSlopThreshold())) {
            if (!isSlopeWithinTolerance(localPoint, mDirection == kSwipeDirectionLeft || mDirection == kSwipeDirectionRight)) {
                reset();
                return false;
            }

            mTriggered = true;
            mReplacedComponent = mActionable->getCoreChildAt(0);

            mSwipeComponent = Builder().inflate(mActionable->getContext(), mItems);

            std::dynamic_pointer_cast<TouchWrapperComponent>(mActionable)->injectReplaceComponent(mSwipeComponent,
                    mAction != SwipeAwayActionType::kSwipeAwayActionReveal);

            if (mAction == SwipeAwayActionType::kSwipeAwayActionReveal ||
                mAction == SwipeAwayActionType::kSwipeAwayActionSlide) {
                mReplaceTransformation = getTransformation(false);
                mReplaceTransformation->interpolate(travelPercentage);
                mReplacedComponent->setProperty(kPropertyTransformAssigned, Object(mReplaceTransformation));
            }

            if (mAction == SwipeAwayActionType::kSwipeAwayActionCover ||
                mAction == SwipeAwayActionType::kSwipeAwayActionSlide) {
                mSwipeTransformation = getTransformation(true);
                mSwipeTransformation->interpolate(travelPercentage);
                mSwipeComponent->setProperty(kPropertyTransformAssigned, Object(mSwipeTransformation));
            }
            mActionable->executePointerEventHandler(kPropertyOnMove, localPoint);
            mActionable->executePointerEventHandler(kPropertyOnCancel, localPoint);
        } else {
            return false;
        }
    }
    sendSwipeMove(travelPercentage);

    return true;
}

void
SwipeAwayGesture::animateRemainder(bool fulfilled, float velocity)
{
    auto& rootConfig = mActionable->getRootConfig();
    auto timeManager = rootConfig.getTimeManager();
    float remainingDistance = fulfilled ? mLocalDistance - mTraveledDistance : -mTraveledDistance;
    auto travelPercentage = mTraveledDistance/mLocalDistance;
    float remainingPercentage = remainingDistance/mLocalDistance;

    apl_duration_t animationDuration = velocity > 0
        ? std::abs(remainingDistance) / velocity
        : rootConfig.getDefaultSwipeAnimationDuration();
    animationDuration = std::min(animationDuration, rootConfig.getMaxSwipeAnimationDuration());

    // Ensure "Early exit",
    if (remainingDistance == 0) {
        animationDuration = 0;
    }

    std::weak_ptr<SwipeAwayGesture> weak_ptr(std::dynamic_pointer_cast<SwipeAwayGesture>(shared_from_this()));
    mAnimateAction = Action::makeAnimation(timeManager, animationDuration,
       [this, weak_ptr, animationDuration, travelPercentage, remainingPercentage](apl_duration_t offset) {
           auto self = weak_ptr.lock();
           if (self) {
               auto timeDelta = offset / animationDuration;
               auto distanceDelta = self->mAnimationEasing->calc(timeDelta);
               float alpha = travelPercentage + (distanceDelta * remainingPercentage);

               // Float numbers could be flaky. Limit it to extremes
               alpha = std::max(0.0f, std::min(1.0f, alpha));

               self->processTransformChange(alpha);
               sendSwipeMove(alpha);
           }
       });

    // Prevent other gestures from happening if swipe will succeed
    // We are replacing original child so gestures does not mean same thing any more.
    if (fulfilled) mActionable->disableGestures();

    mAnimateAction->then([weak_ptr, fulfilled](const ActionPtr& ptr) {
        auto self = weak_ptr.lock();
        if (self) {
            if (fulfilled) {
                self->processTransformChange(1);
                self->mReplacedComponent->remove();
                self->mReplacedComponent->release();
                // Need that as absolute positioning does not play well when original child removed
                std::dynamic_pointer_cast<TouchWrapperComponent>(self->mActionable)->resetChildPositionType();
                auto params = std::make_shared<ObjectMap>();
                params->emplace("direction", sSwipeDirectionMap.at(self->mDirection));
                self->mActionable->executeEventHandler("SwipeDone", self->mOnSwipeDone, false, params);
            } else {
                self->processTransformChange(0);
                self->mSwipeComponent->remove();
                self->mSwipeComponent->release();
            }
            self->mReplacedComponent = nullptr;
            self->mSwipeComponent = nullptr;
            self->reset();
        }
    });
}

float
SwipeAwayGesture::getCurrentVelocity()
{
    auto velocities = toLocalVector(mVelocityTracker->getEstimatedVelocity());
    return (mDirection == SwipeDirection::kSwipeDirectionLeft ||
            mDirection == SwipeDirection::kSwipeDirectionRight) ?
            velocities.getX() : velocities.getY();
}

bool
SwipeAwayGesture::swipedFarEnough()
{
    return mTraveledDistance / mLocalDistance
        >= mActionable->getRootConfig().getSwipeAwayFulfillDistancePercentageThreshold();
}

bool
SwipeAwayGesture::swipedFastEnough(float velocity)
{
    auto swipeVelocityThreshold = mActionable->getRootConfig().getSwipeVelocityThreshold()/time::MS_PER_SECOND;
    auto velocityThreshold = toLocalThreshold(swipeVelocityThreshold);
    // Check for velocity condition
    return (getFulfillMoveDirection() * velocity > 0) && (std::abs(velocity) >= velocityThreshold);
}

bool
SwipeAwayGesture::onUp(const PointerEvent& event, apl_time_t timestamp) {
    if (!FlingGesture::onUp(event, timestamp))
        return false;

    auto velocity = getCurrentVelocity();
    if (std::isnan(velocity)) {
        CONSOLE_CTP(mActionable->getContext()) << "Singular transform encountered during "
                                                  "SwipeAway, aborting swipe";
        animateRemainder(false, 0);
    } else {
        // Check for velocity condition
        bool finishUp = swipedFastEnough(velocity);

        // If not sufficient - check for distance
        if (!finishUp) {
            finishUp = swipedFarEnough();
            velocity = 0;
        } else {
            // Limit to the configured maximum. We don't want speed of light here.
            velocity = std::min(std::abs(velocity),
                mActionable->getRootConfig().getSwipeMaxVelocity()/time::MS_PER_SECOND);
        }

        animateRemainder(finishUp, velocity);
    }

    return true;
}

float
SwipeAwayGesture::toLocalThreshold(float threshold)
{
    if (mDirection == kSwipeDirectionLeft || mDirection == kSwipeDirectionRight) {
        return std::abs(threshold * mActionable->getGlobalToLocalTransform().getXScaling());
    } else {
        return std::abs(threshold * mActionable->getGlobalToLocalTransform().getYScaling());
    }
}

} // namespace apl
