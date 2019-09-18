/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/command/arraycommand.h"
#include "apl/action/arrayaction.h"

namespace apl {

CommandPtr
ArrayCommand::create(const ContextPtr& context,
                     const Object& commands,
                     const CoreComponentPtr& base,
                     const Properties& properties,
                     bool finishAllOnTerminate)
{
    return std::make_shared<ArrayCommand>(context, commands, base, properties, finishAllOnTerminate);
}

ActionPtr
ArrayCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (mCommands.size() <= 0)
        return nullptr;

    auto self = std::static_pointer_cast<const ArrayCommand>(shared_from_this());
    return ArrayAction::make(timers, self, fastMode);
}

} // namespace apl