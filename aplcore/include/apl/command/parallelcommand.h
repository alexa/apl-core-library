/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef _APL_PARALLEL_COMMAND_H
#define _APL_PARALLEL_COMMAND_H

#include "apl/command/corecommand.h"
#include "apl/command/commandfactory.h"
#include "apl/action/delayaction.h"

namespace apl {

class ParallelCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base) {
        auto ptr = std::make_shared<ParallelCommand>(context, std::move(properties), base);
        return ptr->validate() ? ptr : nullptr;
    }

    ParallelCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base)
            : CoreCommand(context, std::move(properties), base)
    {}

    const CommandPropDefSet& propDefSet() const override {
        static CommandPropDefSet sParallelCommandProperties(CoreCommand::propDefSet(), {
                {kCommandPropertyCommands, Object::EMPTY_ARRAY(), asArray, kPropRequired}
        });
        return sParallelCommandProperties;
    };

    CommandType type() const override { return kCommandTypeParallel; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override {
        if (!calculateProperties())
            return nullptr;

        auto commands = mValues.at(kCommandPropertyCommands);
        ActionList actions;

        for (auto& command : commands.getArray()) {
            auto ptr = CommandFactory::instance().inflate(context(), command, mBase);
            if (ptr) {
                auto action = DelayAction::make(timers, ptr, fastMode);
                if (action)
                    actions.push_back(action);
            }
        }

        if (actions.size())
            return Action::makeAll(timers, actions);

        return nullptr;
    }
};

} // namespace apl

#endif // _APL_PARALLEL_COMMAND_H
