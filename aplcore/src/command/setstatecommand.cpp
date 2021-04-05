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

#include "apl/command/setstatecommand.h"
#include "apl/focus/focusmanager.h"

namespace apl {

const CommandPropDefSet&
SetStateCommand::propDefSet() const {
    static CommandPropDefSet sSetStateCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyComponentId, "",    asString,  kPropRequiredId},
            {kCommandPropertyState,       "",    asString,  kPropRequired},
            {kCommandPropertyValue,       false, asBoolean, kPropRequired}
    });

    return sSetStateCommandProperties;
}

ActionPtr
SetStateCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    std::string state = mValues.at(kCommandPropertyState).asString();
    bool value = mValues.at(kCommandPropertyValue).asBoolean();

    StateProperty stateProperty = State::stringToState(state);
    switch (stateProperty) {
        case kStateChecked:
            mTarget->setProperty(kPropertyChecked, value);
            break;
        case kStateDisabled:
            mTarget->setProperty(kPropertyDisabled, value);
            break;
        case kStateFocused: {
            auto &fm = mTarget->getContext()->focusManager();
            fm.setFocus(mTarget, true);
            break;
        }
        default:   // Ignore pressed, hover, and karaoke state
            break;
    }

    return nullptr;
}

} // namespace apl