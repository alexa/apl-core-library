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

static const bool DEBUG_ACTIONABLE_COMPONENT = false;

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
    bool consumed = false;

    for (const auto& handler: handlers.getArray()) {

        if (!handler.isMap()) {
            continue;
        }

        auto when = propertyAsBoolean(*eventContext, handler, "when", false);
        if (!when)
            continue;

        auto commands = Object(arrayifyProperty(getContext(), handler, "commands"));
        if (!commands.empty()) {
            mContext->sequencer().executeCommands(commands, eventContext, shared_from_this(), false);
        }
        consumed = !propertyAsBoolean(getContext(), handler, "propagate", false);
        break;

    }

    return consumed;
}

} // namespace apl