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

#ifndef _APL_ACTION_DELAY_ACTION_H
#define _APL_ACTION_DELAY_ACTION_H

#include "apl/action/action.h"

namespace apl {
/**
 * An action that executes a command appropriately after a delay.
 * If the command has a delay and we're not in fast mode, delay appropriately first.
 * After the delay use the command to construct an action and execute that action.
 */
class DelayAction : public Action {
public:
    static std::shared_ptr<DelayAction> make(const TimersPtr& timers, const CommandPtr& command, bool fastMode);

    /**
     * Build the delay action. You must use std::make_shared<>
     * @param command The command to execute.
     * @param fastMode True if in fast mode.
     */
    DelayAction(const TimersPtr& timers, const CommandPtr& command, bool fastMode);

private:
    /**
     * This method must be called to start the action running.
     */
    void start();

    /**
     * Check for a valid delay.
     * @return True if the current action has been set to a delay
     */
    bool checkDelay();

    /**
     * Check for a valid command
     * @return True if the current action has been set to the commanded action
     */
    bool checkCommand();

private:
    void resolveInternal();

    const CommandPtr mCommand;
    const bool mFastMode;

    ActionPtr mCurrentAction;
};

} // namespace apl

#endif //_APL_ACTION_DELAY_ACTION_H
