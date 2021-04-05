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

#include "apl/command/selectcommand.h"
#include "apl/command/arraycommand.h"
#include "apl/command/commandfactory.h"
#include "apl/action/delayaction.h"
#include "apl/time/sequencer.h"

namespace apl {

const CommandPropDefSet&
SelectCommand::propDefSet() const {
    static CommandPropDefSet sSelectCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyCommands,    Object::EMPTY_ARRAY(), asArray },
            {kCommandPropertyData,        Object::EMPTY_ARRAY(), asArray },
            {kCommandPropertyOtherwise,   Object::EMPTY_ARRAY(), asArray }
    });

    return sSelectCommandProperties;
}

ActionPtr
SelectCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    auto commands = mValues.at(kCommandPropertyCommands).getArray();
    if (!commands.empty()) {
        auto data = mValues.at(kCommandPropertyData).getArray();

        // If there is no data, we look for the first valid command
        if (data.empty()) {
            for (const auto& command : commands) {
                auto ptr = CommandFactory::instance().inflate(context(), command, mBase);
                if (ptr)
                    return DelayAction::make(timers, ptr, fastMode);
            }
        } else {  // Iterate through the data items
            int index = 0;
            auto dataLength = data.size();
            for (const auto& datum : data) {
                auto childContext = Context::createFromParent(context());
                childContext->putConstant("data", datum);
                childContext->putConstant("index", index);
                childContext->putConstant("length", dataLength);

                // Look for an executable command
                for (const auto& command : commands) {
                    auto ptr = CommandFactory::instance().inflate(childContext, command, mBase);
                    if (ptr)
                        return DelayAction::make(timers, ptr, fastMode);
                }
                index++;
            }
        }
    }

    // If we get here, we need to execute the "otherwise" commands
    auto otherwise = mValues.at(kCommandPropertyOtherwise);
    if (otherwise.empty())
        return nullptr;

    auto arrayCommand = ArrayCommand::create(context(), otherwise, base(), Properties(), sequencer());
    return arrayCommand->execute(timers, fastMode);
}

} // namespace apl