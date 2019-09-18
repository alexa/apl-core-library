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

#ifndef _APL_SEQUENCER_H
#define _APL_SEQUENCER_H

#include "apl/action/action.h"
#include "apl/engine/context.h"
#include "apl/component/corecomponent.h"
#include "apl/command/command.h"

namespace apl {

class TimeManager;

class Sequencer {
public:
    Sequencer(const std::shared_ptr<TimeManager>& timeManager) : mTerminated(false), mTimeManager(timeManager) {}

    /**
     * Execute a single command as the master action.  If a master action is in progress, it will
     * be terminated and replace with this command sequence.
     *
     * @param commandPtr The command to execute
     * @param fastMode True if in fast mode
     * @return An action pointer that will resolve when the command finishes.  A nullptr
     *         will be return in fast mode.
     */
    ActionPtr execute(const CommandPtr& commandPtr, bool fastMode);

    /**
     * Convenience routine that takes an array object of commands and a data-binding context,
     * inflates an ArrayCommand, and then executes it.
     *
     * The caller *must* hold onto the ActionPtr when running in fastMode or only the first
     * command in the sequence will execute.
     *
     * @param commands An array of commands to execute.
     * @param context The data-binding context.
     * @param baseComponent The base component that these commands execute from.
     * @param fastMode True if the commands should run in fast mode.
     * @return The action pointer of the command or nullptr if there is nothing to execute.
     *         A nullptr will be returned in fast mode.
     */
    ActionPtr executeCommands(const Object& commands,
                              const ContextPtr& context,
                              const CoreComponentPtr& baseComponent,
                              bool fastMode);

    /**
     * Terminate and clear out the sequencer.  After calling this, no more commands will be accepted.
     */
    void terminate();

    /**
     * Reset the sequencer to clear out any currently executing master command.
     */
    void reset();

private:
    ActionPtr mMasterActionPtr;
    bool mTerminated;
    const std::shared_ptr<TimeManager> mTimeManager;
    std::set<ActionPtr> mOneShotSet;
};

} // namespace apl

#endif //_APL_SEQUENCER_H
