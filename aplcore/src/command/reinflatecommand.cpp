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

#include "apl/command/reinflatecommand.h"

namespace apl {

const CommandPropDefSet&
ReinflateCommand::propDefSet() const
{
    static CommandPropDefSet sReinflateCommandProperties(CoreCommand::propDefSet(), {
        // This space reserved for properties to be added in the future
    });

    return sReinflateCommandProperties;
}

ActionPtr
ReinflateCommand::execute(const TimersPtr& timers, bool fastMode)
{
    if (!calculateProperties())
        return nullptr;

    // Return a simple action that pushes the event and does nothing else.  The view host must
    // resolve this event to allow further events in the sequencer to execute.
    return Action::make(timers, [this](ActionRef ref) {
        mContext->pushEvent(Event(kEventTypeReinflate, EventBag(), nullptr, ref));
    });
}

} // namespace apl