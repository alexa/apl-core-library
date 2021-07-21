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

#include "apl/command/sequentialcommand.h"
#include "apl/action/sequentialaction.h"

namespace apl {

const CommandPropDefSet&
SequentialCommand::propDefSet() const {
    static CommandPropDefSet sSequentialCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyCommands,    Object::EMPTY_ARRAY(), asArray },
            {kCommandPropertyRepeatCount, 0,                     asNonNegativeInteger },
            {kCommandPropertyCatch,       Object::EMPTY_ARRAY(), asArray },
            {kCommandPropertyFinally,     Object::EMPTY_ARRAY(), asArray }
    });

    return sSequentialCommandProperties;
}

ActionPtr
SequentialCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    auto shared = std::static_pointer_cast<const CoreCommand>(shared_from_this());
    return SequentialAction::make(timers, shared, fastMode);
}

} // namespace apl