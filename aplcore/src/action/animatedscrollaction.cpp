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

#include "apl/action/animatedscrollaction.h"
#include "apl/command/corecommand.h"
#include "apl/time/sequencer.h"
#include "apl/animation/coreeasing.h"
#include "apl/animation/animatedproperty.h"
#include "apl/content/rootconfig.h"

namespace apl {

static const bool DEBUG_SCROLL = false;

AnimatedScrollAction::AnimatedScrollAction(const TimersPtr& timers,
                                           const ContextPtr& context,
                                           const CoreComponentPtr& container)
        : ResourceHoldingAction(timers, context),
          mContainer(container),
          // TODO: Following is really specific to simplified scroll animation we have ATM. Replace
          //  with something more configurable similarly to VelocityTracking.
          mEasing(CoreEasing::bezier(0.42, 0, 0.58, 1))
{}

void
AnimatedScrollAction::scroll(bool vertical, const Point& position) {
    if (mContext->getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureHandleScrollingAndPagingInCore)) {
        auto from = vertical ? mContainer->scrollPosition().getY() : mContainer->scrollPosition().getX();
        auto to = vertical ? position.getY() : position.getX();
        advance(from, to);
    } else {
        EventBag bag;
        bag.emplace(kEventPropertyPosition, Dimension(vertical ? position.getY() : position.getX()));

        LOG_IF(DEBUG_SCROLL) << "Pushing scroll event position=" << position;
        mContext->pushEvent(Event(kEventTypeScrollTo, std::move(bag), mContainer, shared_from_this()));
    }
}

void
AnimatedScrollAction::advance(float from, float to) {
    if (isTerminated())
        return;

    std::weak_ptr<AnimatedScrollAction> weak_ptr(std::static_pointer_cast<AnimatedScrollAction>(shared_from_this()));
    auto duration = mContext->getRootConfig().getScrollCommandDuration();
    mCurrentAction = Action::makeAnimation(timers(), duration,
       [weak_ptr, from, to, duration](apl_duration_t offset) {
           auto self = weak_ptr.lock();
           if (self && !self->isTerminated()) {
               float alpha = offset / duration;
               alpha = self->mEasing->calc(alpha);

               double value = from * (1 - alpha) + to * alpha;
               self->mContainer->update(kUpdateScrollPosition, value);
           }
       });

    mCurrentAction->then([weak_ptr](const ActionPtr& ptr) {
        auto self = weak_ptr.lock();
        if (self) {
            self->mCurrentAction = nullptr;
            if (!self->isTerminated())
                self->resolve();
        }
    });
}

} // namespace apl