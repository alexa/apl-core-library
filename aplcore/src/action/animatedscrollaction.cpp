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

#include "apl/component/scrollablecomponent.h"
#include "apl/engine/rootcontext.h"
#include "apl/time/sequencer.h"
#include "apl/touch/utils/autoscroller.h"

namespace apl {

AnimatedScrollAction::AnimatedScrollAction(const TimersPtr& timers,
                                           const ContextPtr& context,
                                           const CoreComponentPtr& container,
                                           apl_duration_t duration)
        : ResourceHoldingAction(timers, context),
          mContainer(container),
          mDuration(duration)
{
    // Default to programmatic duration if not specified
    mDuration = mDuration >= 0 ? mDuration : context->getRootConfig().getScrollCommandDuration();
}

void
AnimatedScrollAction::scroll(bool vertical, const Point& position)
{
    if (isTerminated())
        return;

    // Ensure that it doesn't scroll if don't need to.
    if (mContainer->scrollPosition() == position) {
        resolve();
        return;
    }

    mScroller = AutoScroller::make(mContext->getRootConfig(),
        ScrollableComponent::cast(mContainer),
        []() {},
        position - mContainer->scrollPosition(),
        mDuration);
    advance();
}

void
AnimatedScrollAction::advance()
{
    if (!mScroller) {
        resolve();
        return;
    }

    std::weak_ptr<AnimatedScrollAction> weak_ptr(std::static_pointer_cast<AnimatedScrollAction>(shared_from_this()));
    mCurrentAction = Action::makeAnimation(timers(), mScroller->getDuration(),
       [weak_ptr](apl_duration_t offset) {
           auto self = weak_ptr.lock();
           if (self && !self->isTerminated()) {
               self->mScroller->updateOffset(offset);
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

void
AnimatedScrollAction::freeze()
{
    mCurrentAction->freeze();

    mFrozenContainerId = mContainer->getId();

    ResourceHoldingAction::freeze();
}

bool
AnimatedScrollAction::rehydrate(const RootContext& context)
{
    if (!mCurrentAction) {
        return true;
    }

    if (!ResourceHoldingAction::rehydrate(context)) return false;

    mContainer = CoreComponent::cast(context.findComponentById(mFrozenContainerId));
    if (!mContainer) return false;

    auto scrollable = ScrollableComponent::cast(mContainer);
    if (!scrollable) return false;

    mScroller->replaceTarget(scrollable);

    return mCurrentAction->rehydrate(context);
}

} // namespace apl