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

#include "apl/action/speaklistaction.h"
#include "apl/action/speakitemaction.h"
#include "apl/action/scrolltoaction.h"
#include "apl/command/corecommand.h"
#include "apl/engine/rootcontext.h"

namespace apl {

std::shared_ptr<SpeakListAction>
SpeakListAction::make(const TimersPtr& timers,
                      const std::shared_ptr<CoreCommand>& command)
{
    auto container = command->target();
    auto start = command->getValue(kCommandPropertyStart).asInt();
    auto count = command->getValue(kCommandPropertyCount).asInt();
    auto len = static_cast<int>(container->getChildCount());

    // Sanity checks
    if (len <= 0 || count <= 0 || start >= len)
        return nullptr;

    // A negative start value means from the end
    if (start < 0) {
        start += len;
        if (start < 0)
            start = 0;   // Just clip it to the start
    }

    // Clip the count to the number of items possible
    if (start + count > len)
        count = len - start;

    auto ptr = std::make_shared<SpeakListAction>(timers, command, container, start, start + count);
    ptr->advance();
    return ptr;
}

SpeakListAction::SpeakListAction(const TimersPtr& timers,
                                 const std::shared_ptr<CoreCommand>& command,
                                 CoreComponentPtr& container,
                                 size_t startIndex, size_t endIndex)
    : Action(timers),
      mCommand(command),
      mContainer(container),
      mNextIndex(startIndex),
      mEndIndex(endIndex)
{
    addTerminateCallback([this](const TimersPtr&) {
        if (mCurrentAction) {
            mCurrentAction->terminate();
            mCurrentAction = nullptr;
        }
    });
}

void
SpeakListAction::advance()
{
    while (mNextIndex < mEndIndex) {
        mCurrentAction = SpeakItemAction::make(timers(), mCommand, mContainer->getCoreChildAt(mNextIndex++));
        if (!mCurrentAction)
            continue;

        std::weak_ptr<SpeakListAction> weak_ptr(std::static_pointer_cast<SpeakListAction>(shared_from_this()));
        mCurrentAction->then([weak_ptr](const ActionPtr& actionPtr){
            auto self = weak_ptr.lock();
            if (!self)
                return;

            self->advance();
        });

        return;
    }

    resolve();
}

void
SpeakListAction::freeze()
{
    if (mCurrentAction) {
        mCurrentAction->freeze();
    }
    if (mCommand) {
        mCommand->freeze();
    }

    Action::freeze();
}

bool
SpeakListAction::rehydrate(const RootContext& context)
{
    if (!Action::rehydrate(context)) return false;

    if (mCommand) {
        if (!mCommand->rehydrate(context)) return false;
    }

    mContainer = mCommand->target();
    if (!mContainer) return false;

    // Clip the count in case if changed
    auto start = mCommand->getValue(kCommandPropertyStart).asInt();
    auto count = mCommand->getValue(kCommandPropertyCount).asInt();
    auto len = static_cast<int>(mContainer->getChildCount());

    if (start + count > len)
        mEndIndex = start + (len - start);

    auto speakItem = std::dynamic_pointer_cast<SpeakItemAction>(mCurrentAction);
    if (speakItem) {
        speakItem->mTarget = mContainer->getCoreChildAt(mNextIndex - 1);
        if (!speakItem->rehydrate(context)) return false;
    }

    return true;
}

} // namespace apl