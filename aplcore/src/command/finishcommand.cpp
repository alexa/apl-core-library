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

#include "apl/command/finishcommand.h"
#include "apl/time/sequencer.h"

namespace apl {

const CommandPropDefSet&
FinishCommand::propDefSet() const {
    static CommandPropDefSet sFinishCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyReason, kCommandReasonExit, sCommandReasonMap},
    });

    return sFinishCommandProperties;
}

ActionPtr
FinishCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    EventBag bag;
    bag.emplace(kEventPropertyReason, getValue(kCommandPropertyReason));

    mContext->pushEvent(Event(kEventTypeFinish, std::move(bag)));
    mContext->sequencer().reset();
    return nullptr;
}

} // namespace apl