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

#include "apl/command/autopagecommand.h"
#include "apl/action/autopageaction.h"
#include "apl/utils/session.h"

namespace apl {

const CommandPropDefSet&
AutoPageCommand::propDefSet() const {
    static CommandPropDefSet sAutoPageCommandProperties( CoreCommand::propDefSet(), {
            {kCommandPropertyComponentId, "",                              asString,  kPropRequiredId},
            {kCommandPropertyCount,       std::numeric_limits<int>::max(), asNonNegativeInteger },
            {kCommandPropertyDuration,    0,                               asNonNegativeInteger }
    });

    return sAutoPageCommandProperties;
}

ActionPtr
AutoPageCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (fastMode) {
        CONSOLE_CTP(mContext) << "Ignoring AutoPage in fast mode";
        return nullptr;
    }

    if (!calculateProperties())
        return nullptr;

    return AutoPageAction::make(timers, std::static_pointer_cast<CoreCommand>(shared_from_this()));
}

} // namespace apl