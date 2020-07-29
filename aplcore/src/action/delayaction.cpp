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

#include "apl/action/delayaction.h"
#include "apl/command/command.h"

namespace apl {

DelayAction::DelayAction(const TimersPtr& timers, const CommandPtr& command, bool fastMode)
        : Action(timers), mCommand(command), mFastMode(fastMode)
{
    addTerminateCallback([this](const TimersPtr&) {
        if (mCurrentAction) {
            mCurrentAction->terminate();
            mCurrentAction = nullptr;
        }

        mCommand->complete();
    });
}

std::shared_ptr<DelayAction>
DelayAction::make(const TimersPtr& timers, const CommandPtr& command, bool fastMode) {
    if (!command)
        return nullptr;

    command->prepare();
    auto ptr = std::make_shared<DelayAction>(timers, command, fastMode);
    ptr->start();
    return ptr;
}

void
DelayAction::start() {
    if (checkDelay() || checkCommand())
        return;

    resolveInternal();
}

/**
 * Check for a valid delay.
 * @return True if the current action has been set to a delay
 */
bool
DelayAction::checkDelay() {
    unsigned long delay = mFastMode ? 0 : mCommand->delay();
    if (delay == 0)
        return false;

    auto sptr = std::static_pointer_cast<DelayAction>(shared_from_this());
    std::weak_ptr<DelayAction> weak_ptr(sptr);

    mCurrentAction = Action::makeDelayed(timers(), delay);
    mCurrentAction->then([weak_ptr](const ActionPtr& ptr) {
        auto self = weak_ptr.lock();
        if (self) {
            self->mCurrentAction = nullptr;
            if (!self->isTerminated() && !self->checkCommand()) {
                self->resolveInternal();
            }
        }
    });
    return true;
}

/**
 * Check for a valid command
 * @return True if the current action has been set to the commanded action
 */
bool
DelayAction::checkCommand() {
    mCurrentAction = mCommand->execute(timers(), mFastMode);
    if (!mCurrentAction || mCurrentAction->isResolved())
        return false;

    auto sptr = std::static_pointer_cast<DelayAction>(shared_from_this());
    std::weak_ptr<DelayAction> weak_ptr(sptr);

    mCurrentAction->then([weak_ptr](const ActionPtr &) {
        auto self = weak_ptr.lock();
        if (self) {
            self->mCurrentAction = nullptr;
            self->resolveInternal();
        }
    });

    mCurrentAction->addTerminateCallback([weak_ptr](const TimersPtr&) {
        auto self = weak_ptr.lock();
        if (self) {
            self->mCurrentAction = nullptr;
            self->resolveInternal();
        }
    });

    return true;
}

void
DelayAction::resolveInternal() {
    if (mCommand && !isTerminated())
        mCommand->complete();

    resolve();
}

} // namespace apl