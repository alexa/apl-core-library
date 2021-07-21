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

#include "apl/component/actionablecomponent.h"
#include "apl/component/componentpropdef.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/focus/focusmanager.h"
#include "apl/primitives/keyboard.h"
#include "apl/time/sequencer.h"
#include "apl/touch/gesture.h"

namespace apl {

const ComponentPropDefSet&
ActionableComponent::propDefSet() const {
    static ComponentPropDefSet sActionableComponentProperties(CoreComponent::propDefSet(), {
            {kPropertyHandleKeyDown,    Object::EMPTY_ARRAY(), asArray,   kPropIn},
            {kPropertyHandleKeyUp,      Object::EMPTY_ARRAY(), asArray,   kPropIn},
            {kPropertyOnBlur,           Object::EMPTY_ARRAY(), asCommand, kPropIn},
            {kPropertyOnFocus,          Object::EMPTY_ARRAY(), asCommand, kPropIn},
            {kPropertyNextFocusDown,    "",                    asString,  kPropIn},
            {kPropertyNextFocusForward, "",                    asString,  kPropIn},
            {kPropertyNextFocusLeft,    "",                    asString,  kPropIn},
            {kPropertyNextFocusRight,   "",                    asString,  kPropIn},
            {kPropertyNextFocusUp,      "",                    asString,  kPropIn},
    });
    return sActionableComponentProperties;
}

void
ActionableComponent::executeOnBlur() {
    auto command = getCalculated(kPropertyOnBlur);
    auto eventContext = createDefaultEventContext("Blur");
    mContext->sequencer().executeCommands(command, eventContext, shared_from_corecomponent(), true);
}

void
ActionableComponent::executeOnFocus() {
    auto command = getCalculated(kPropertyOnFocus);
    auto eventContext = createDefaultEventContext("Focus");
    mContext->sequencer().executeCommands(command, eventContext, shared_from_corecomponent(), true);
}


bool
ActionableComponent::executeKeyHandlers(KeyHandlerType type, const Keyboard& keyboard) {
    auto propertyKey = KeyboardManager::getHandlerPropertyKey(type);
    auto handlerId = KeyboardManager::getHandlerId(type);

    // return false if no handlers ( not consumed )
    auto handlers = getCalculated(propertyKey);
    if (!handlers.isArray())
        return false;

    ContextPtr eventContext = createKeyboardEventContext(handlerId, keyboard.serialize());

    for (const auto& handler: handlers.getArray()) {
        if (handler.isMap() && propertyAsBoolean(*eventContext, handler, "when", true)) {
            // We've identified commands to execute
            auto commands = Object(arrayifyProperty(*eventContext, handler, "commands"));
            if (!commands.empty())
                mContext->sequencer().executeCommands(commands, eventContext, shared_from_corecomponent(), false);

            // Return TRUE if the next key handler in the hierarchy should be run
            return !propertyAsBoolean(*eventContext, handler, "propagate", false);
        }
    }

    return false;
}

bool
ActionableComponent::executeIntrinsicKeyHandlers(KeyHandlerType type, const Keyboard& keyboard)
{
    // TODO: Do we need to allow SpatialNav to be disabled by runtime configuration?
    if (type == kKeyDown) {
        auto it = keyboardToFocusDirection().find(keyboard);
        if (it != keyboardToFocusDirection().end()) {
            // We consume the key, but don't perform any action as one is already in progress.
            if (mContext->sequencer().isRunning(FOCUS_SEQUENCER))
                return true;

            auto focusDirection = it->second;
            auto& fm = getContext()->focusManager();
            auto nextFocus = getUserSpecifiedNextFocus(focusDirection);
            if (!nextFocus)
                nextFocus = takeFocusFromChild(focusDirection, Rect());

            if (nextFocus) {
                // If returned self - it's already processed. Just ignore.
                if (nextFocus != shared_from_this()) {
                    fm.setFocus(nextFocus, true);
                }
            } else {
                // "Default" processing - means navigate out of component.
                fm.focus(focusDirection);
            }
            return true;
        }
    }

    return false;
}

void
ActionableComponent::release()
{
    // Avoiding reference loop.
    if (mActiveGesture) {
        mActiveGesture->release();
        mActiveGesture = nullptr;
    }
    mGestureHandlers.clear();
    CoreComponent::release();
}

bool
ActionableComponent::processGestures(const PointerEvent& event, apl_time_t timestamp) {
    if (mGesturesDisabled) return false;

    if (mActiveGesture) {
        if(mActiveGesture->isTriggered()) {
            mActiveGesture->consume(event, timestamp);
            if (!mActiveGesture->isTriggered())
                mActiveGesture = nullptr; // Consumed but reset afterwards.
            return true;
        }

        mActiveGesture = nullptr;
        return false;
    }

    for (auto& gesture : mGestureHandlers) {
        auto locked = gesture->consume(event, timestamp);
        if (gesture->isTriggered()) {
            // Triggered by event, so active now
            mActiveGesture = gesture;
            for (auto& g : mGestureHandlers) {
                if (g != mActiveGesture)
                    g->reset();
            }
        }
        if (locked) return true;
    }

    return false;
}

void
ActionableComponent::invokeStandardAccessibilityAction(const std::string& name)
{
    // Check attached gestures to see if one of them is a standard handler for this named accessibility action
    for (const auto& m : mGestureHandlers)
        if (m->invokeAccessibilityAction(name))
            return;

    CoreComponent::invokeStandardAccessibilityAction(name);
}

ObjectMapPtr
ActionableComponent::createTouchEventProperties(const Point &localPoint) const
{
    auto eventProps = std::make_shared<ObjectMap>();
    auto componentPropertyMap = std::make_shared<ObjectMap>();
    componentPropertyMap->emplace("x", localPoint.getX());
    componentPropertyMap->emplace("y", localPoint.getY());
    componentPropertyMap->emplace("width", YGNodeLayoutGetWidth(mYGNodeRef));
    componentPropertyMap->emplace("height", YGNodeLayoutGetHeight(mYGNodeRef));
    eventProps->emplace("component", componentPropertyMap);

    return eventProps;
}

void
ActionableComponent::enableGestures()
{
    if (!mGesturesDisabled) {
        return;
    }
    mGesturesDisabled = false;

    mActiveGesture = nullptr;
    for (auto& gesture : mGestureHandlers) {
        gesture->reset();
    }
}

CoreComponentPtr
ActionableComponent::getUserSpecifiedNextFocus(FocusDirection direction)
{
    auto it = focusDirectionToNextProperty().find(direction);
    if (it != focusDirectionToNextProperty().end()) {
        auto componentId = getCalculated(it->second).getString();
        if (!componentId.empty()) {
            return std::dynamic_pointer_cast<CoreComponent>(getContext()->findComponentById(componentId));
        }
    }
    return nullptr;
}

const std::map<Keyboard, FocusDirection, Keyboard::comparatorWithoutRepeat>&
ActionableComponent::keyboardToFocusDirection()
{
    static std::map<Keyboard, FocusDirection, Keyboard::comparatorWithoutRepeat> sKeyboardToFocusDirection = {
        { Keyboard::ARROW_DOWN_KEY(), kFocusDirectionDown },
        { Keyboard::ARROW_UP_KEY(), kFocusDirectionUp },
        { Keyboard::ARROW_LEFT_KEY(), kFocusDirectionLeft },
        { Keyboard::ARROW_RIGHT_KEY(), kFocusDirectionRight },
        { Keyboard::TAB_KEY(), kFocusDirectionForward },
        { Keyboard::SHIFT_TAB_KEY(), kFocusDirectionBackwards },
    };
    return sKeyboardToFocusDirection;
}

const std::map<FocusDirection, PropertyKey>&
ActionableComponent::focusDirectionToNextProperty()
{
    static std::map<FocusDirection, PropertyKey> sFocusDirectionToNextProperty = {
        { kFocusDirectionDown,      kPropertyNextFocusDown },
        { kFocusDirectionUp,        kPropertyNextFocusUp },
        { kFocusDirectionLeft,      kPropertyNextFocusLeft },
        { kFocusDirectionRight,     kPropertyNextFocusRight },
        { kFocusDirectionForward,   kPropertyNextFocusForward },
    };
    return sFocusDirectionToNextProperty;
}

} // namespace apl
