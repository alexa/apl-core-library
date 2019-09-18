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

#ifndef _APL_SEQUENTIAL_COMMAND_H
#define _APL_SEQUENTIAL_COMMAND_H

#include "apl/command/corecommand.h"
#include "apl/action/sequentialaction.h"

namespace apl {


class SequentialCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base) {
        auto ptr = std::make_shared<SequentialCommand>(context, std::move(properties), base);
        return ptr->validate() ? ptr : nullptr;
    }

    SequentialCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base)
            : CoreCommand(context, std::move(properties), base)
    {}

    const CommandPropDefSet& propDefSet() const override {
        static CommandPropDefSet sSequentialCommandProperties(CoreCommand::propDefSet(), {
                {kCommandPropertyCommands,    Object::EMPTY_ARRAY(), asArray },
                {kCommandPropertyRepeatCount, 0,                     asNonNegativeInteger },
                {kCommandPropertyCatch,       Object::EMPTY_ARRAY(), asArray },
                {kCommandPropertyFinally,     Object::EMPTY_ARRAY(), asArray }
        });

        return sSequentialCommandProperties;
    };

    CommandType type() const override { return kCommandTypeSequential; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override {
        if (!calculateProperties())
            return nullptr;

        auto shared = std::static_pointer_cast<const CoreCommand>(shared_from_this());
        return SequentialAction::make(timers, shared, fastMode);
    }
};


} // namespace apl

#endif // _APL_SEQUENTIAL_COMMAND_H
