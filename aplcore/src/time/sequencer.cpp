/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/time/sequencer.h"
#include "apl/utils/log.h"
#include "apl/command/arraycommand.h"
#include "apl/time/timemanager.h"

namespace apl {

static const bool DEBUG_SEQUENCER = false;


ActionPtr
Sequencer::execute(const CommandPtr& commandPtr, bool fastMode)
{
    if (mTerminated || !commandPtr)
        return nullptr;

    if (!fastMode && mMasterActionPtr) {
        mMasterActionPtr->terminate();
        mMasterActionPtr = nullptr;
    }

    ActionPtr ptr = nullptr;

    if (commandPtr) {
        // Executing a command may end up telling the sequencer to reset().
        mResetInExecute = false;
        ptr = commandPtr->execute(mTimeManager, fastMode);
        if (mResetInExecute) {
            ptr->terminate();
            ptr = nullptr;
        }
    }

    if (fastMode) {
        if (ptr && ptr->isPending()) {
            mOneShotSet.emplace(ptr);
            ptr->then([this](const ActionPtr& ptr) {
                mOneShotSet.erase(ptr);
            });
        }
        ptr = nullptr;  // We never return the ActionPtr for fast mode
    }
    else {  // Normal mode
        mMasterActionPtr = ptr && ptr->isPending() ? ptr : nullptr;
    }

    return ptr;
}

ActionPtr
Sequencer::executeCommands(const Object& commands,
                           const ContextPtr& context,
                           const CoreComponentPtr& baseComponent,
                           bool fastMode)
{
    if (!commands.isArray()) {
        LOG(LogLevel::ERROR) << "executeCommands: invalid command list";
        return nullptr;
    }

    if (commands.empty())
        return nullptr;

    if (!context->has("event") && !fastMode)
        LOG(LogLevel::WARN) << "missing event in context";

    Properties props;
    auto commandPtr = ArrayCommand::create(context, commands, baseComponent, props);
    return execute(commandPtr, fastMode);
}

void
Sequencer::terminate()
{
    LOG_IF(DEBUG_SEQUENCER) << "Sequencer terminate";

    reset();
    mOneShotSet.clear();
    mTerminated = false;
}

void
Sequencer::reset()
{
    LOG_IF(DEBUG_SEQUENCER) << "reset start";
    mResetInExecute = true;
    if (mMasterActionPtr) {
        mMasterActionPtr->terminate();
        mMasterActionPtr = nullptr;
    }
}

} // namespace apl
