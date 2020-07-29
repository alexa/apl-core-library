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

#include "apl/command/controlmediacommand.h"
#include "apl/action/controlmediaaction.h"
#include "apl/utils/session.h"

namespace apl {

const CommandPropDefSet&
ControlMediaCommand::propDefSet() const {
    static CommandPropDefSet sControlMediaCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyCommand,     kCommandControlMediaPlay, sControlMediaMap,   kPropRequired},
            {kCommandPropertyComponentId, "",                       asString,           kPropRequiredId},
            {kCommandPropertyValue,       0,                        asInteger }
    });

    return sControlMediaCommandProperties;
}

ActionPtr
ControlMediaCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    // All commands except "Play" are allowed in fast mode
    auto command = getValue(kCommandPropertyCommand);
    if (fastMode && command.getInteger() == kCommandControlMediaPlay) {
        CONSOLE_CTP(mContext) << "Ignoring ControlMedia.play in fast mode";
        return nullptr;
    }

    return ControlMediaAction::make(timers, std::static_pointer_cast<CoreCommand>(shared_from_this()));
}

} // namespace apl