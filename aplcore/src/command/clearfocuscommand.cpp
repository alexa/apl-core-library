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

#include "apl/command/clearfocuscommand.h"
#include "apl/focus/focusmanager.h"

namespace apl {

const CommandPropDefSet&
ClearFocusCommand::propDefSet() const {
    static CommandPropDefSet sClearFocusCommandProperties(CoreCommand::propDefSet(), {});
    return sClearFocusCommandProperties;
}

ActionPtr
ClearFocusCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    auto& fm = mContext->focusManager();
    fm.clearFocus(true, kFocusDirectionNone, true);
    return nullptr;
}

} // namespace apl