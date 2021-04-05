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

#include "apl/command/setfocuscommand.h"
#include "apl/focus/focusmanager.h"

namespace apl {

const CommandPropDefSet&
SetFocusCommand::propDefSet() const {
    static CommandPropDefSet sSetFocusCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyComponentId, "", asString, kPropRequiredId},
    });

    return sSetFocusCommandProperties;
}

ActionPtr
SetFocusCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    auto& fm = mTarget->getContext()->focusManager();
    fm.setFocus(mTarget, true);
    return nullptr;
}

} // namespace apl