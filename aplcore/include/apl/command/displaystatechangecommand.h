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

#ifndef _APL_DISPLAY_STATE_CHANGE_COMMAND_H
#define _APL_DISPLAY_STATE_CHANGE_COMMAND_H

#include "apl/command/command.h"

namespace apl {

/**
 * This command explicitly handles just the onDisplayStateChange event handler
 * in the APL document. The handler is special because all of the actions
 * execute in fast mode and the commands are assigned to a named sequencer.
 */
class DisplayStateChangeCommand : public Command {
public:
    // Sequencer reserved for executing the onDisplayStateChange command
    static const char *SEQUENCER;

    static CommandPtr create(const RootContextPtr& rootContext,
                             ObjectMap&& properties) {
        return std::make_shared<DisplayStateChangeCommand>(rootContext, std::move(properties));
    }

    DisplayStateChangeCommand(const RootContextPtr& rootContext,
                        ObjectMap&& properties)
    : mRootContext(rootContext),
      mProperties(std::move(properties))
    {
    }

    unsigned long delay() const override { return 0; }
    std::string name() const override { return "DisplayStateChangeCommand"; }
    ActionPtr execute(const TimersPtr& timers, bool fastMode) override;

private:
    const std::weak_ptr<RootContext> mRootContext;
    ObjectMap mProperties;
};

} // namespace apl

#endif // _APL_DISPLAY_STATE_CHANGE_COMMAND_H
