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
#include "apl/action/arrayaction.h"
#include "apl/action/delayaction.h"
#include "apl/command/commandfactory.h"
#include "apl/time/sequencer.h"

namespace apl {

const CommandPropDefSet&
ParallelCommand::propDefSet() const
{
    static CommandPropDefSet sParallelCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyCommands, Object::EMPTY_ARRAY(), asArray, kPropRequired},
            {kCommandPropertyData,     Object::EMPTY_ARRAY(), asArray },
    });
    return sParallelCommandProperties;
}

ActionPtr
ParallelCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    ActionList actions;

    auto commands = mValues.at(kCommandPropertyCommands);
    if (!commands.empty()) {
        auto data = mValues.at(kCommandPropertyData);
        auto self = std::static_pointer_cast<CoreCommand>(shared_from_this());

        // If there is no data, proceed through commands list
        if (data.empty()) {
            for (auto& command : commands.getArray()) {
                auto commandPtr = CommandFactory::instance().inflate({command, this->data()}, self);
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
        } else {  // Iterate through the data items, and assemble a sequence of commands for each
                  // data element
            int index = 0;
            auto dataLength = data.size();
            for (const auto& datum : data.getArray()) {
                auto childContext = Context::createFromParent(context());
                childContext->putConstant("data", datum);
                childContext->putConstant("index", index);
                childContext->putConstant("length", dataLength);


                auto shared = std::static_pointer_cast<CoreCommand>(shared_from_this());
                auto action = ArrayAction::make(timers, childContext, shared, CommandData(commands), fastMode);
                if (action)
                    actions.push_back(action);

                index++;
            }
        }
    }

    if (!actions.empty())
        return Action::makeAll(timers, actions);

    return nullptr;
}

} // namespace apl