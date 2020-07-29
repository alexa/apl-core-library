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

#include "apl/command/parallelcommand.h"
#include "apl/command/commandfactory.h"
#include "apl/action/delayaction.h"
#include "apl/time/sequencer.h"

namespace apl {

const CommandPropDefSet&
ParallelCommand::propDefSet() const
{
    static CommandPropDefSet sParallelCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyCommands, Object::EMPTY_ARRAY(), asArray, kPropRequired}
    });
    return sParallelCommandProperties;
}

ActionPtr
ParallelCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    auto commands = mValues.at(kCommandPropertyCommands);
    ActionList actions;

    for (auto& command : commands.getArray()) {
        auto self = std::static_pointer_cast<CoreCommand>(shared_from_this());
        auto commandPtr = CommandFactory::instance().inflate(command, self);
        if (commandPtr) {
            auto childSeq = commandPtr->sequencer();
            if (childSeq != sequencer()) {
                context()->sequencer().executeOnSequencer(commandPtr, childSeq);
                continue;
            }
            auto action = DelayAction::make(timers, commandPtr, fastMode);
            if (action)
                actions.push_back(action);
        }
    }

    if (!actions.empty())
        return Action::makeAll(timers, actions);

    return nullptr;
}

} // namespace apl