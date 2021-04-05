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

#include "apl/action/autopageaction.h"
#include "apl/command/corecommand.h"
#include "apl/time/sequencer.h"
#include "apl/content/rootconfig.h"
#include "apl/component/corecomponent.h"
#include "apl/component/pagercomponent.h"

namespace apl {

AutoPageAction::AutoPageAction(const TimersPtr& timers,
                               const std::shared_ptr<CoreCommand>& command,
                               const ComponentPtr& container,
                               int start,
                               int end,
                               apl_time_t duration)
        : ResourceHoldingAction(timers, command->context()),
          mCommand(command),
          mContainer(container),
          mNextIndex(start),
          mEndIndex(end),
          mDuration(duration)
{
    addTerminateCallback([this](const TimersPtr&) {
        if (mCurrentAction) {
            mCurrentAction->terminate();
            mCurrentAction = nullptr;
        }
    });
}

std::shared_ptr<AutoPageAction>
AutoPageAction::make(const TimersPtr& timers,
                     const std::shared_ptr<CoreCommand>& command)
{
    // Ensure there is a paging target with multiple children
    auto target = command->target();
    if (!target
        || (target->scrollType() != kScrollTypeHorizontalPager &&  target->scrollType() != kScrollTypeVerticalPager)
        || target->getChildCount() < 2)
        return nullptr;

    auto start = target->pagePosition() + 1;
    auto len = target->getChildCount();
    auto count = command->getValue(kCommandPropertyCount).asInt();
    auto duration = command->getValue(kCommandPropertyDuration).asInt();

    if (count <= 0 || start >= len)
        return nullptr;

    if (start + count > len)
        count = len - start;

    auto ptr = std::make_shared<AutoPageAction>(timers, command, target, start, start + count, duration);
    command->context()->sequencer().claimResource({kExecutionResourcePosition, target}, ptr);
    ptr->advance();
    return ptr;
}

void
AutoPageAction::advance() {
    if (mNextIndex > mEndIndex) {
        resolve();
        return;
    }

    if (mNextIndex == mEndIndex) {
        // No more pages to change.  We pause on the final page for mDuration
        mCurrentAction = Action::makeDelayed(timers(), mDuration);
        mNextIndex++;
    }
    else {
        // The first time through we need to skip the pause duration.  The "delay" for this command was already handled.
        bool firstTime = (mCurrentAction == nullptr);

        mCurrentAction = Action::makeDelayed(timers(), (firstTime ? 0 : mDuration), [this](ActionRef ref) {
            PagerComponent::setPageUtil(mContext, mContainer, mNextIndex++, kPageDirectionForward, ref,
                                        mContext->getRequestedAPLVersion().compare("1.6") < 0);
        });
    }

    std::weak_ptr<AutoPageAction> weak_ptr(std::static_pointer_cast<AutoPageAction>(shared_from_this()));
    mCurrentAction->then([weak_ptr](const ActionPtr& actionPtr) {
        auto self = weak_ptr.lock();
        if (!self)
            return;

        self->advance();
    });
}

} // namespace apl