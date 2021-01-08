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

#include "apl/component/corecomponent.h"
#include "apl/component/touchablecomponent.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/touch/pointer.h"
#include "apl/touch/pointermanager.h"
#include "apl/utils/log.h"
#include "apl/utils/searchvisitor.h"

namespace apl {

static inline bool
sendEventToTarget(PointerEventType type,
                  const std::shared_ptr<Pointer>& pointer,
                  apl_time_t timestamp) {
    if (!pointer) {
        return false;
    }

    auto target = pointer->getTarget();
    if (!target) {
        return false;
    }

    PointerEvent event(type, pointer->getPosition(), pointer->getId(), pointer->getPointerType());

    // We ignore the return value here as this path should never trigger gestures
    auto gestureTarget = target->processPointerEvent(event, timestamp);
    if (gestureTarget != nullptr && target != gestureTarget) {
        LOG(LogLevel::WARN) << "Ignoring non-null gesture target";
    }
    return true;
}

/*
 * Maps input pointer events to the output events that are generated.
 */
Bimap<PointerEventType, PropertyKey> sEventHandlers = {{kPointerCancel, kPropertyOnCancel},
                                                       {kPointerDown, kPropertyOnDown},
                                                       {kPointerMove, kPropertyOnMove},
                                                       {kPointerUp, kPropertyOnUp}};

PointerManager::PointerManager(const RootContextData& core) : mCore(core) {}

bool
PointerManager::handlePointerEvent(const PointerEvent& pointerEvent, apl_time_t timestamp) {
    auto pointer = getOrCreatePointer(pointerEvent);
    ActionableComponentPtr target = nullptr;
    // save the last active pointer before it gets updated
    auto previousPointer = mLastActivePointer;

    switch (pointerEvent.pointerEventType) {
        case kPointerDown:
            mCore.sequencer().reset();
            target = handlePointerStart(pointer, pointerEvent);
            break;
        case kPointerMove:
            target = handlePointerUpdate(pointer, pointerEvent);
            break;
        case kPointerCancel:
        case kPointerUp:
            target = handlePointerEnd(pointer, pointerEvent);
            break;
        case kPointerTargetChanged:
        default:
            LOG(LogLevel::WARN) << "Unknown pointer event type ignored"
                                << pointerEvent.pointerEventType;
            return false;
    }

    if (!target) {
        // Target was lost, notify the previous target, if it's active
        if (mActivePointer) {
            handleNewTarget(previousPointer, nullptr, timestamp);
        }
        return false;
    }

    // If locked in component defined the gesture - do not send "usual" event. It's up to gesture to decide now on
    //  what to send out.
    auto gestureTarget = target->processPointerEvent(pointerEvent, timestamp);
    if (gestureTarget) {
        if (gestureTarget != target) {
            pointer->setTarget(std::dynamic_pointer_cast<ActionableComponent>(gestureTarget));
        }
        handleNewTarget(previousPointer, gestureTarget, timestamp);
        return true;
    } else {
        handleNewTarget(previousPointer, target, timestamp);
    }


    auto pointInCurrent = target->toLocalPoint(pointerEvent.pointerEventPosition);
    auto handler = sEventHandlers.at(pointerEvent.pointerEventType);
    target->executePointerEventHandler(handler, pointInCurrent);
    return false;
}

void
PointerManager::handleTimeUpdate(apl_time_t timestamp) {
    sendEventToTarget(kPointerTimeUpdate, mLastActivePointer, timestamp);
}

void
PointerManager::handleNewTarget(const std::shared_ptr<apl::Pointer>& pointer,
                                const CoreComponentPtr &newTarget,
                                apl::apl_time_t timestamp) {
    if (pointer && pointer->getTarget() != newTarget) {
        sendEventToTarget(kPointerTargetChanged, pointer, timestamp);
    }
}

std::shared_ptr<Pointer>
PointerManager::getOrCreatePointer(const PointerEvent& pointerEvent) {
    if (!mActivePointer || pointerEvent.pointerId != mActivePointer->getId()) {
        return std::make_shared<Pointer>(Pointer(pointerEvent.pointerType, pointerEvent.pointerId));
    }
    return mActivePointer;
}

ActionableComponentPtr
PointerManager::handlePointerStart(const std::shared_ptr<Pointer>& pointer,
                                   const PointerEvent& pointerEvent) {
    if (mActivePointer != nullptr)
        return nullptr;

    auto top = std::dynamic_pointer_cast<CoreComponent>(mCore.top());
    if (!top)
        return nullptr;

    auto visitor = TouchableAtPosition(pointerEvent.pointerEventPosition);
    top->raccept(visitor);
    auto target = std::dynamic_pointer_cast<ActionableComponent>(visitor.getResult());

    pointer->setTarget(target);
    pointer->setPosition(pointerEvent.pointerEventPosition);
    mActivePointer = pointer;
    mLastActivePointer = mActivePointer;
    return target;
}

ActionableComponentPtr
PointerManager::handlePointerUpdate(const std::shared_ptr<Pointer>& pointer,
                                    const PointerEvent& pointerEvent) {
    auto target = pointer->getTarget();
    if (pointer->getPointerType() == kMousePointer && !target) {
        mCore.hoverManager().setCursorPosition(pointerEvent.pointerEventPosition);
    }
    pointer->setPosition(pointerEvent.pointerEventPosition);
    return target;
}

ActionableComponentPtr
PointerManager::handlePointerEnd(const std::shared_ptr<Pointer>& pointer,
                                 const PointerEvent& pointerEvent) {
    if (mActivePointer && pointer->getId() == mActivePointer->getId()) {
        switch (pointer->getPointerType()) {
            case kTouchPointer:
                break;
            case kMousePointer:
                mCore.hoverManager().setCursorPosition(pointerEvent.pointerEventPosition);
                break;
        }

        auto target = pointer->getTarget();
        mLastActivePointer = mActivePointer;
        mActivePointer = nullptr;
        return target;
    }
    return nullptr;
}

}
