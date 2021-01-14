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

#include "apl/action/arrayaction.h"
#include "apl/action/delayaction.h"
#include "apl/command/commandfactory.h"
#include "apl/command/arraycommand.h"
#include "apl/time/sequencer.h"

namespace apl {

ArrayAction::ArrayAction(const TimersPtr& timers, std::shared_ptr<const ArrayCommand> command, bool fastMode)
    : Action(timers),
      mCommand(command),
      mFastMode(fastMode),
      mNextIndex(0)
{
    addTerminateCallback([this](const TimersPtr&) {
        if (mCurrentAction) {
            mCurrentAction->terminate();
            mCurrentAction = nullptr;
        }

        if (mCommand->finishAllOnTerminate()) {
            auto& commands = mCommand->commands();
            std::vector<Object> remaining;
            for (int i = mNextIndex ; i < commands.size() ; i++)
                remaining.push_back(commands.at(i));

            auto context = mCommand->context();
            context->sequencer().executeCommands(Object(std::move(remaining)), context, mCommand->base(), true);
        }
    });
}

/**
 * Advance through the commands.  This method gets called at start and once
 * each time an existing command action finishes.
 */
void
ArrayAction::advance() {
    if (isTerminated())
        return;

    auto commands = mCommand->commands();

    while (mNextIndex < commands.size()) {
        Object command = commands.at(mNextIndex++);
        auto commandPtr = CommandFactory::instance().inflate(command,
                                                             mCommand);
        if (!commandPtr)
            continue;

        auto childSeq = commandPtr->sequencer();
        if (childSeq != mCommand->sequencer()) {
            mCommand->context()->sequencer().executeOnSequencer(commandPtr, childSeq);
            continue;
        }

        mCurrentCommand = commandPtr;
        mCurrentAction = DelayAction::make(timers(), mCurrentCommand, mFastMode);

        std::weak_ptr<ArrayAction> weak_ptr(std::static_pointer_cast<ArrayAction>(shared_from_this()));

        mCurrentAction->then([weak_ptr](const ActionPtr&) {
            auto self = weak_ptr.lock();

            if (self) {
                self->mCurrentAction = nullptr;
                if (!self->isTerminated())
                    self->advance();
            }
        });

        return;
    }

    resolve();
}


} // namespace apl
