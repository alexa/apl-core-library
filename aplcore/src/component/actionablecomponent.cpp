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

#include "apl/component/actionablecomponent.h"
#include "apl/component/componentpropdef.h"
#include "apl/time/sequencer.h"
#include "apl/primitives/keyboard.h"
#include "apl/engine/keyboardmanager.h"


namespace apl {

const ComponentPropDefSet&
ActionableComponent::propDefSet() const {
    static ComponentPropDefSet sActionableComponentProperties(CoreComponent::propDefSet(), {
            {kPropertyHandleKeyDown, Object::EMPTY_ARRAY(), asArray,   kPropIn},
            {kPropertyHandleKeyUp,   Object::EMPTY_ARRAY(), asArray,   kPropIn},
            {kPropertyOnBlur,        Object::EMPTY_ARRAY(), asCommand, kPropIn},
            {kPropertyOnFocus,       Object::EMPTY_ARRAY(), asCommand, kPropIn}
    });
    return sActionableComponentProperties;
}

void
ActionableComponent::executeOnBlur() {
    auto command = getCalculated(kPropertyOnBlur);
    auto eventContext = createDefaultEventContext("Blur");
    mContext->sequencer().executeCommands(command, eventContext, shared_from_this(), true);
}

void
ActionableComponent::executeOnFocus() {
    auto command = getCalculated(kPropertyOnFocus);
    auto eventContext = createDefaultEventContext("Focus");
    mContext->sequencer().executeCommands(command, eventContext, shared_from_this(), true);
}


bool
ActionableComponent::executeKeyHandlers(KeyHandlerType type, const ObjectMapPtr& keyboard) {

    auto propertyKey = KeyboardManager::getHandlerPropertyKey(type);
    auto handlerId = KeyboardManager::getHandlerId(type);

    // return false if no handlers ( not consumed )
    auto handlers = getCalculated(propertyKey);
    if (!handlers.isArray())
        return false;

    ContextPtr eventContext = createKeyboardEventContext(handlerId, keyboard);

    for (const auto& handler: handlers.getArray()) {
        if (handler.isMap() && propertyAsBoolean(*eventContext, handler, "when", true)) {
            // We've identified commands to execute
            auto commands = Object(arrayifyProperty(*eventContext, handler, "commands"));
            if (!commands.empty())
                mContext->sequencer().executeCommands(commands, eventContext, shared_from_this(), false);

            // Return TRUE if the next key handler in the hierarchy should be run
            return !propertyAsBoolean(*eventContext, handler, "propagate", false);
        }
    }

    return false;
}

} // namespace apl