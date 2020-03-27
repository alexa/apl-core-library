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

#ifndef _APL_FINISH_COMMAND_H
#define _APL_FINISH_COMMAND_H

#include "apl/command/corecommand.h"
#include "apl/time/sequencer.h"
#include "apl/utils/log.h"
#include "apl/utils/dump_object.h"

namespace apl {

/**
 * This handles the Finish command execution.
 *
 * The finish command closes the current APL document and exits.
 *
 * Property      | Type          | Default       | Description
 * ------------- | ------------- | ------------- | -------------
 * reason        | back / exit   | exit          | The reason the activity is finishing.
 *
 * Executing the finish command stops all other processing in APL, including any commands that are still executing.
 * The finish command executes in both normal and fast mode.
 */
class FinishCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base) {
        auto ptr = std::make_shared<FinishCommand>(context, std::move(properties), base);
        return ptr->validate() ? ptr : nullptr;
    }

    FinishCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base)
            : CoreCommand(context, std::move(properties), base)
    {}

    const CommandPropDefSet& propDefSet() const override {
        static CommandPropDefSet sFinishCommandProperties(CoreCommand::propDefSet(), {
                {kCommandPropertyReason, kCommandReasonExit, sCommandReasonMap},
        });

        return sFinishCommandProperties;
    };

    CommandType type() const override { return kCommandTypeFinish; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override {
        if (!calculateProperties())
            return nullptr;

        EventBag bag;
        bag.emplace(kEventPropertyReason, getValue(kCommandPropertyReason));

        mContext->pushEvent(Event(kEventTypeFinish, std::move(bag)));
        mContext->sequencer().reset();
        return nullptr;
    }
};

} // namespace apl

#endif // _APL_FINISH_COMMAND_H
