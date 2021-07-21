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

#include "apl/action/documentaction.h"
#include "apl/command/documentcommand.h"
#include "apl/time/sequencer.h"
#include "apl/command/commandfactory.h"
#include "apl/action/delayaction.h"

namespace apl {

std::shared_ptr<DocumentAction>
DocumentAction::make(const TimersPtr& timers,
                     const std::shared_ptr<DocumentCommand>& command,
                     bool fastMode)
{
    if (!command)
        return nullptr;

    auto ptr = std::make_shared<DocumentAction>(timers, command, fastMode);
    ptr->start();
    return ptr;
}

DocumentAction::DocumentAction(const TimersPtr& timers,
                               const std::shared_ptr<DocumentCommand>& command,
                               bool fastMode)
    : Action(timers),
      mCommand(command),
      mFastMode(fastMode),
      mStateFinally(false)
{
    addTerminateCallback([this](const TimersPtr&) {
        if (mCurrentAction) {
            mCurrentAction->terminate();
            mCurrentAction = nullptr;
        }

        if (!mStateFinally) {
            auto cmd = mCommand->getDocumentCommand();
            auto context = mCommand->context();
            if (context)
                context->sequencer().execute(cmd, true);
        }
    });
}

void
DocumentAction::start()
{
    mCurrentAction = mCommand->getComponentActions(timers(), mFastMode);
    if (!mCurrentAction) {
        advance();
        return;
    }

    auto weak_ptr = std::weak_ptr<DocumentAction>(std::static_pointer_cast<DocumentAction>(shared_from_this()));
    mCurrentAction->then([weak_ptr](const ActionPtr&) {
        auto self = weak_ptr.lock();
        if (self) {
            self->mCurrentAction = nullptr;
            self->advance();
        }
    });
}

void
DocumentAction::advance() {
    mStateFinally = true;

    auto cmd = mCommand->getDocumentCommand();
    if (!cmd) {
        resolve();
        return;
    }

    mCurrentAction = cmd->execute(timers(), mFastMode);
    if (!mCurrentAction) {
        resolve();
        return;
    }

    auto weak_ptr = std::weak_ptr<DocumentAction>(std::static_pointer_cast<DocumentAction>(shared_from_this()));
    mCurrentAction->then([weak_ptr](const ActionPtr&) {
        auto self = weak_ptr.lock();
        if (self) {
            self->mCurrentAction = nullptr;
            self->resolve();
        }
    });
}

} // namespace apl