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

#include "apl/component/componentpropdef.h"
#include "apl/component/touchablecomponent.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/context.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/propdef.h"
#include "apl/time/sequencer.h"
#include "apl/touch/gesture.h"

namespace apl {

static std::map<PropertyKey, std::string> sPropertyHandlers = {
    {kPropertyOnCancel, "Cancel"},
    {kPropertyOnDown,   "Down"},
    {kPropertyOnMove,   "Move"},
    {kPropertyOnPress,  "Press"},
    {kPropertyOnUp,     "Up"}
};

static std::map<PropertyKey, bool> sPropertyExecutesFast = {
    {kPropertyOnCancel, true},
    {kPropertyOnDown,   true},
    {kPropertyOnMove,   true},
    {kPropertyOnPress,  false},
    {kPropertyOnUp,     true}
};

void
TouchableComponent::setGestureHandlers()
{
    auto handlers = getCalculated(kPropertyGestures).getArray();
    for (auto& handler : handlers) {
        auto gesture = Gesture::create(std::dynamic_pointer_cast<TouchableComponent>(shared_from_this()), handler);
        if (gesture) {
            mGestureHandlers.emplace_back(gesture);
        }
    }
}

bool
TouchableComponent::processGestures(const PointerEvent& event, apl_time_t timestamp, bool topComponent) {
    // Touchable components is only process pointer events if initially designated as targets as per spec.
    // Could change in future.
    if (!topComponent) return false;
    return ActionableComponent::processGestures(event, timestamp, topComponent);
}

bool
TouchableComponent::shouldPropagatePointerEvent(const PointerEvent& event, apl_time_t timestamp) {
    if (!ActionableComponent::shouldPropagatePointerEvent(event, timestamp)) {
        return false;
    }

    // TODO: Will not handle multitouch properly.
    if (event.pointerEventType == kPointerDown) {
        mPointerInteractionStartTime = timestamp;
    }

    return timestamp < mPointerInteractionStartTime + mContext->getRootConfig().getTapOrScrollTimeout();
}

void
TouchableComponent::invokeStandardAccessibilityAction(const std::string& name)
{
    if (name == "activate")
        executeEventHandler(sPropertyHandlers.at(kPropertyOnPress),
                            getCalculated(kPropertyOnPress), false, createTouchEventProperties(Point()));
    else
        ActionableComponent::invokeStandardAccessibilityAction(name);
}

const ComponentPropDefSet &
TouchableComponent::propDefSet() const {
    static ComponentPropDefSet sTouchableComponentProperties(ActionableComponent::propDefSet(), {
        {kPropertyOnCancel, Object::EMPTY_ARRAY(), asCommand, kPropIn},
        {kPropertyOnDown,   Object::EMPTY_ARRAY(), asCommand, kPropIn},
        {kPropertyOnMove,   Object::EMPTY_ARRAY(), asCommand, kPropIn},
        {kPropertyOnPress,  Object::EMPTY_ARRAY(), asCommand, kPropIn},
        {kPropertyOnUp,     Object::EMPTY_ARRAY(), asCommand, kPropIn},
        {kPropertyGestures, Object::EMPTY_ARRAY(), asArray,   kPropIn}
    });
    return sTouchableComponentProperties;
}

void
TouchableComponent::executePreEventActions(PropertyKey handlerKey) {
    switch (handlerKey) {
        case kPropertyOnUp:
        case kPropertyOnCancel:
        case kPropertyOnPress:
        case kPropertyHandleKeyUp:
            setState(kStatePressed, false);
            break;
        case kPropertyOnDown:
        case kPropertyHandleKeyDown:
            setState(kStatePressed, true);
            break;
        case kPropertyOnMove:
        default:
            break;
    }
}

void
TouchableComponent::executePostEventActions(PropertyKey handlerKey, const Point &localPoint) {
    switch (handlerKey) {
        case kPropertyOnUp:
            if (containsLocalPosition(localPoint)) {
                executePointerEventHandler(kPropertyOnPress, localPoint);
            }
            break;
        case kPropertyOnCancel:
        case kPropertyOnDown:
        case kPropertyOnPress:
        case kPropertyOnMove:
        default:
            break;
    }
}

void
TouchableComponent::addHandlerEventProperties(PropertyKey handlerKey,
                                              const Point& localPoint,
                                              const ObjectMapPtr& eventProperties) const {
    switch (handlerKey) {
        case kPropertyOnMove:
        case kPropertyOnUp:
            eventProperties->emplace("inBounds", containsLocalPosition(localPoint));
            break;
        case kPropertyOnCancel:
        case kPropertyOnDown:
        case kPropertyOnPress:
        default:
            break;
    }
}

void
TouchableComponent::executePointerEventHandler(PropertyKey handlerKey, const Point& localPoint)
{
    if (mState.get(kStateDisabled)) return;

    executePreEventActions(handlerKey);
    auto& commands = getCalculated(handlerKey);
    if (!commands.empty()) {
        auto fastMode = sPropertyExecutesFast.at(handlerKey);
        ObjectMapPtr props = nullptr;
        if (handlerKey != kPropertyOnPress) {
            props = createTouchEventProperties(localPoint);
        }
        addHandlerEventProperties(handlerKey, localPoint, props);
        executeEventHandler(sPropertyHandlers.at(handlerKey), commands, fastMode, props);
    }
    executePostEventActions(handlerKey, localPoint);
}

bool
TouchableComponent::executeIntrinsicKeyHandlers(KeyHandlerType type, const ObjectMapPtr& keyboard) {
    auto key = keyboard->at("key").getString();
    if (!mState.get(kStateDisabled)) {
        auto handlerKey = KeyboardManager::getHandlerPropertyKey(type);
        if ((key == "Enter" || key == "NumpadEnter") && !getCalculated(kPropertyOnPress).empty()) {
            executePreEventActions(handlerKey);
            handleEnterKey(handlerKey);
            return true;
        } // add elseif for other keys
    }
    return ActionableComponent::executeKeyHandlers(type, keyboard);
}

void
TouchableComponent::handleEnterKey(PropertyKey handlerKey) {
    switch (handlerKey) {
        case kPropertyHandleKeyDown:
            break;
        case kPropertyHandleKeyUp:
            executeEventHandler(sPropertyHandlers.at(kPropertyOnPress), getCalculated(kPropertyOnPress), false);
            break;
        default:
         break;
    }
}


bool
TouchableComponent::getTags(rapidjson::Value &outMap, rapidjson::Document::AllocatorType &allocator) {
    CoreComponent::getTags(outMap, allocator);
    outMap.AddMember("clickable", true, allocator);
    return true;
}

Object
TouchableComponent::getValue() const {
    return mState.get(kStateChecked);
}

void
TouchableComponent::initialize() {
    CoreComponent::initialize();
    setGestureHandlers();
}

// TODO: remove once we support handing intrinsic/reserved keys
void
TouchableComponent::update(UpdateType type, float value) {
    if (type == kUpdatePressed) {
        // don't bother with setting pressed state and unsetting it here, although this should only
        // be triggered from the enter key/dpad center, we've lost the timings of key up/down.  Will
        // be rectified once intrinsic key handling is complete. This is not spec compliant, but meets
        // parity with the old touchwrapper impl
        executePointerEventHandler(kPropertyOnPress, {-1, -1});
    } else
        CoreComponent::update(type, value);
}

} // namespace apl
