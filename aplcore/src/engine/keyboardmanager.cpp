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

#include "apl/common.h"
#include "apl/command/documentcommand.h"
#include "apl/component/corecomponent.h"
#include "apl/content/content.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/arrayify.h"
#include "apl/engine/event.h"
#include "apl/engine/rootcontext.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/focus/focusmanager.h"
#include "apl/primitives/keyboard.h"
#include "apl/time/sequencer.h"

namespace apl {

static const bool DEBUG_KEYBOARD_MANAGER = false;


static std::map<int, std::string> sHandlerIds = { // NOLINT(cert-err58-cpp)
        {kKeyUp,   "KeyUp"},
        {kKeyDown, "KeyDown"}
};

static std::map<int, PropertyKey> sHandlerProperty = { // NOLINT(cert-err58-cpp)
        {kKeyUp,   kPropertyHandleKeyUp},
        {kKeyDown, kPropertyHandleKeyDown}
};

std::string
KeyboardManager::getHandlerId(KeyHandlerType type) {
    return sHandlerIds.at(type);
}

PropertyKey
KeyboardManager::getHandlerPropertyKey(KeyHandlerType type) {
    return sHandlerProperty.at(type);
}

bool
KeyboardManager::handleKeyboard(KeyHandlerType type, const CoreComponentPtr& component,
                                const Keyboard& keyboard, const RootContextPtr& rootContext) {
    LOG_IF(DEBUG_KEYBOARD_MANAGER) << "type:" << type << ", keyboard:" << keyboard.toDebugString();

    if (keyboard.isReservedKey()) {
        // do not process handlers when is key reserved for future use by APL
        return false;
    }

    bool consumed = false;
    auto target = component;
    const bool isIntrinsic = keyboard.isIntrinsicKey();

    while (!consumed && target) {
        consumed = target->processKeyPress(type, keyboard);
        if (consumed) {
            LOG_IF(DEBUG_KEYBOARD_MANAGER) << target->getUniqueId() << " " << type << " consumed.";
        } else {
            // propagate
            target = std::static_pointer_cast<CoreComponent>(target->getParent());
        }
    }

    // TODO:Having intrinsic handler does not really mean blocking from "document handling". Should split those.
    if (!consumed && !isIntrinsic) {
        consumed = executeDocumentKeyHandlers(rootContext, type, keyboard);
    }

    return consumed;
}

bool
KeyboardManager::executeDocumentKeyHandlers(const RootContextPtr& rootContext,
                                            KeyHandlerType type,
                                            const Keyboard& keyboard)
{
    const auto& property = sComponentPropertyBimap.at(getHandlerPropertyKey(type));
    auto handlerId = getHandlerId(type);

    const auto& json = rootContext->content()->getDocument()->json();
    auto handlers = arrayifyProperty(json, property.c_str());
    if (handlers.empty())
        return false;

    ContextPtr eventContext = rootContext->createKeyboardDocumentContext(handlerId, keyboard.serialize());

    for (const auto& handler : handlers) {
        if (propertyAsBoolean(*eventContext, handler, "when", true)) {
            auto commands = Object(arrayifyProperty(*eventContext, handler, "commands"));
            if (!commands.empty())
                eventContext->sequencer().executeCommands(commands, eventContext, nullptr, false);

            // NOTE: Checking for propagation at the document level is useless, except for debugging
            return !propertyAsBoolean(*eventContext, handler, "propagate", false);
        }
    }

    return false;
}

} // namespace apl
