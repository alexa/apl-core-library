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

#include "apl/action/sequentialaction.h"
#include "apl/action/delayaction.h"
#include "apl/command/commandfactory.h"
#include "apl/time/sequencer.h"

namespace apl {

static const bool DEBUG_SEQUENTIAL = false;

SequentialAction::SequentialAction(const TimersPtr& timers,
                                   std::shared_ptr<const CoreCommand> command, bool fastMode)
    : Action(timers),
      mCommand(command),
      mFastMode(fastMode),
      mStateFinally(false),
      mNextIndex(0),
      mRepeatCounter(0)
{
    addTerminateCallback([this](const TimersPtr&) {
        LOG_IF(DEBUG_SEQUENTIAL) << "terminating " << *this;
        if (mCurrentAction) {
            mCurrentAction->terminate();
            mCurrentAction = nullptr;
        }

        // When we're terminated, we lump together all of the catch and finally commands remaining and send
        // them off to the sequencer.
        auto context = mCommand->context();
        std::vector<Object> commands;
        if (!mStateFinally) {
            auto catchCommands = mCommand->getValue(kCommandPropertyCatch);
            for (int i = 0; i < catchCommands.size(); i++)
                commands.push_back(catchCommands.at(i));
            auto finallyCommands = mCommand->getValue(kCommandPropertyFinally);
            for (int i = 0; i < finallyCommands.size() ; i++)
                commands.push_back(finallyCommands.at(i));
        }
        else {
            auto finallyCommands = mCommand->getValue(kCommandPropertyFinally);
            for (int i = mNextIndex; i < finallyCommands.size() ; i++)
                commands.push_back(finallyCommands.at(i));
        }

        context->sequencer().executeCommands(Object(std::move(commands)), context, mCommand->base(), true);
    });
}

/**
 * Advance through the commands.  This method gets called at start and once
 * each time an existing command action finishes.
 */
void
SequentialAction::advance() {
    LOG_IF(DEBUG_SEQUENTIAL) << *this << " state=" << mStateFinally;

    if (isTerminated())
        return;

    if (!mStateFinally) {
        auto commands = mCommand->getValue(kCommandPropertyCommands);
        auto repeatCount = mCommand->getValue(kCommandPropertyRepeatCount).asInt();

        while (mRepeatCounter <= repeatCount) {
            while (mNextIndex < commands.size()) {
                Object command = commands.at(mNextIndex++);
                if (doCommand(command))
                    return;  // Done advancing until the current action resolves
            }
            mRepeatCounter++;
            mNextIndex = 0;
        }
        mStateFinally = true;
    }

    auto commands = mCommand->getValue(kCommandPropertyFinally);
    while (mNextIndex < commands.size()) {
        Object command = commands.at(mNextIndex++);
        if (doCommand(command))
            return;
    }

    resolve();
}

bool
SequentialAction::doCommand(const Object& command)
{
    auto commandPtr = CommandFactory::instance().inflate(command, mCommand);
    if (commandPtr) {
        auto childSeq = commandPtr->sequencer();
        if (childSeq != mCommand->sequencer()) {
            mCommand->context()->sequencer().executeOnSequencer(commandPtr, childSeq);
            return false;
        }

        mCurrentCommand = commandPtr;
        mCurrentAction = DelayAction::make(timers(), mCurrentCommand, mFastMode);

        std::weak_ptr<SequentialAction> weak_ptr(std::static_pointer_cast<SequentialAction>(shared_from_this()));

        mCurrentAction->then([weak_ptr](const ActionPtr&) {
            auto self = weak_ptr.lock();
            if (self) {
                self->mCurrentAction = nullptr;
                if (!self->isTerminated())
                    self->advance();
            }
        });
        return true;  // Done advancing until the current action resolves
    }

    return false;
}


} // namespace apl