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

#include "apl/action/animateitemaction.h"
#include "apl/animation/animatedproperty.h"
#include "apl/command/corecommand.h"
#include "apl/content/rootconfig.h"
#include "apl/time/sequencer.h"

namespace apl {

std::shared_ptr<AnimateItemAction>
AnimateItemAction::make(const TimersPtr& timers,
                        const std::shared_ptr<CoreCommand>& command,
                        bool fastMode)
{
    auto ptr = std::make_shared<AnimateItemAction>(timers, command, fastMode);
    ptr->start();
    return ptr;
}

AnimateItemAction::AnimateItemAction(const TimersPtr& timers,
                                     const std::shared_ptr<CoreCommand>& command,
                                     bool fastMode)
    : ResourceHoldingAction(timers, command->context()),
      mCommand(command),
      mRepeatCounter(0),
      mReversed(false),
      mDuration(command->getValue(kCommandPropertyDuration).asNumber()),
      mRepeatCount(command->getValue(kCommandPropertyRepeatCount).asInt()),
      mRepeatMode(command->getValue(kCommandPropertyRepeatMode).asInt()),
      mFastMode(fastMode),
      mEasing(command->getValue(kCommandPropertyEasing).getEasing())
{}

void
AnimateItemAction::start()
{
    // Walk the array of values and create animated properties
    for (const auto& m : mCommand->getValue(kCommandPropertyValue).getArray()) {
        auto ptr = AnimatedProperty::create(mCommand->context(), mCommand->target(), m);
        if (ptr) {
            // Claim all requested resources.
            mContext->sequencer().claimResource({kExecutionResourceProperty, mCommand->target(), ptr->key()},
                    shared_from_this());
            mAnimators.push_back(std::move(ptr));
        }
    }

    auto mode = mCommand->context()->getRootConfig().getAnimationQuality();

    // If the duration is 0, we are fast mode or animation quality set to none,
    // just set the final position and resolve()
    if (mDuration <= 0 ||
        mFastMode ||
        mAnimators.size() == 0 ||
        mode == RootConfig::AnimationQuality::kAnimationQualityNone) {
        finalize();
        resolve();
        return;
    }

    // On termination we jump to the end state
    addTerminateCallback([this](const TimersPtr&) {
        if (mCurrentAction)
            mCurrentAction->terminate();
        finalize();
    });

    advance();
}

/**
 * Advance the repeat counter.  We restart the animation, making sure that we have the
 * correct starting values.
 */
void
AnimateItemAction::advance() {
    if (isTerminated())
        return;

    if (mRepeatCounter > mRepeatCount) {
        resolve();
        return;
    }

    mReversed = (mRepeatMode == kCommandRepeatModeReverse && mRepeatCounter % 2 == 1);
    for (auto& m : mAnimators)
        m->update(mCommand->target(), mReversed ? 1 : 0);

    std::weak_ptr<AnimateItemAction> weak_ptr(std::static_pointer_cast<AnimateItemAction>(shared_from_this()));
    mCurrentAction = Action::makeAnimation(timers(), mDuration,
                                           [weak_ptr](apl_duration_t offset) {
                                               auto self = weak_ptr.lock();
                                               if (self && !self->isTerminated()) {
                                                   float alpha = offset / self->mDuration;
                                                   if (self->mReversed)
                                                       alpha = 1 - alpha;
                                                   alpha = self->mEasing->calc(alpha);
                                                   for (auto& m : self->mAnimators)
                                                       m->update(self->mCommand->target(), alpha);
                                               }
                                           });

    mCurrentAction->then([weak_ptr](const ActionPtr& ptr) {
        auto self = weak_ptr.lock();
        if (self) {
            self->mCurrentAction = nullptr;
            if (!self->isTerminated())
                self->advance();
        }
    });

    mRepeatCounter++;
}

void
AnimateItemAction::finalize()
{
    bool reverse = (mRepeatMode == kCommandRepeatModeReverse && mRepeatCount % 2 == 1);
    float alpha = reverse ? 0 : 1;
    for (auto& m : mAnimators)
        m->update(mCommand->target(), alpha);
}



} // namespace apl