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

#include "apl/touch/pointermanager.h"

#include "apl/component/corecomponent.h"
#include "apl/component/touchablecomponent.h"
#include "apl/engine/corerootcontext.h"
#include "apl/engine/hovermanager.h"
#include "apl/time/sequencer.h"
#include "apl/utils/searchvisitor.h"

namespace apl {

static inline void
sendEventToComponent(PointerEventType type,
                     const std::shared_ptr<Pointer>& pointer,
                     const CoreComponentPtr& component,
                     apl_time_t timestamp)
{
    if (!pointer || !component) {
        return;
    }

    PointerEvent event(type, pointer->getPosition(), pointer->getId(), pointer->getPointerType());

    component->processPointerEvent(event, timestamp, false);
}

static inline void
sendEventToTarget(PointerEventType type,
                  const std::shared_ptr<Pointer>& pointer,
                  apl_time_t timestamp)
{
    if (!pointer) {
        return;
    }

    auto target = pointer->getTarget();
    if (!target) {
        return;
    }

    sendEventToComponent(type, pointer, target, timestamp);
}

/*
 * Maps input pointer events to the output events that are generated.
 */
Bimap<PointerEventType, PropertyKey> sEventHandlers = {{kPointerCancel, kPropertyOnCancel},
                                                       {kPointerDown, kPropertyOnDown},
                                                       {kPointerMove, kPropertyOnMove},
                                                       {kPointerUp, kPropertyOnUp}};

PointerManager::PointerManager(const CoreRootContext& core, HoverManager& hover) :
      mCore(core), mHoverManager(hover)
{}

/**
 * Walk the hit list - list of actionable components that should receive pointer event, based on component hierarchy,
 * unless it captured by a particular target starting from deepest up. Generally consists of 1 touchable component
 * (which may or may not be top target, currently TouchWrapper or VectorGraphic), and any number of components that
 * may natively support touch processing (Pager and Scrollables).
 */
class HitListIterator {
public:
    HitListIterator(const CoreComponentPtr& target) : mTarget(target) {
        reset();
    }

    struct HitTarget {
        CoreComponentPtr ptr;
        bool onlyProcessGestures;
    };

    void reset() {
        mCurrent = mTarget;
        mFirstTouchable = (mCurrent && mCurrent->isTouchable()) ? mCurrent : nullptr;
    }

    HitTarget next() {
        HitTarget result = {mCurrent, mCurrent && mCurrent->isTouchable() && mCurrent != mFirstTouchable};
        advance();
        return result;
    }

private:
    void advance() {
        while (mCurrent) {
            mCurrent = CoreComponent::cast(mCurrent->getParent());
            if (mCurrent && mCurrent->isActionable()) {
                if (mFirstTouchable == nullptr && mCurrent->isTouchable()) {
                     mFirstTouchable = mCurrent;
                }
                // mCurrent is pointing to an actionable component
                return;
            }
        }
    }

    CoreComponentPtr mTarget;
    CoreComponentPtr mCurrent;
    CoreComponentPtr mFirstTouchable;
};

bool
PointerManager::handlePointerEvent(const PointerEvent& pointerEvent, apl_time_t timestamp)
{
    auto pointer = getOrCreatePointer(pointerEvent);
    ActionableComponentPtr target = nullptr;
    // save the last active pointer before it gets updated
    auto previousPointer = mLastActivePointer;
    mLastestPointerTimeStamp = timestamp;

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
            LOG(LogLevel::kWarn) << "Unknown pointer event type ignored"
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

    if (pointer->isCaptured()) {
        target->processPointerEvent(pointerEvent, timestamp, false);
    } else {
        auto hitListIt = HitListIterator(target);
        // Each component in the list in turn receives the event.
        // The component can silently process the event or can
        // claim it by returning one of kPointerStatusCaptured or kPointerStatusPendingCapture.
        // In both cases, the component that returned this value will be set as the capturing
        // component. In the case of kPointerStatusCaptured, processing is immediately interrupted
        // and subsequent hit targets will not be given a chance to process the event. In the
        // case of kPointerStatusPendingCapture, the event will be allowed to keep bubbling up
        // through the component hierarchy.
        while (true) {
            auto hitTarget = hitListIt.next();
            if (!hitTarget.ptr) break;
            PointerCaptureStatus pointerStatus = hitTarget.ptr->processPointerEvent(
                pointerEvent, timestamp, hitTarget.onlyProcessGestures);
            // If component claims the event - pointer should be captured by it.
            if (pointerStatus != kPointerStatusNotCaptured) {
                if (!pointer->isCaptured())
                    pointer->setCapture(ActionableComponent::cast(hitTarget.ptr));
                if (pointerStatus == kPointerStatusCaptured)
                    break;
            }
        }

        // If pointer was captured - cancel events to all the other components on the list.
        if (pointer->isCaptured()) {
            hitListIt.reset();
            while (true) {
                auto hitTarget = hitListIt.next();
                if (!hitTarget.ptr) break;
                if (hitTarget.ptr != pointer->getTarget()) {
                    sendEventToComponent(kPointerCancel, mActivePointer, hitTarget.ptr, timestamp);
                }
            }
        }
    }
    handleNewTarget(previousPointer, pointer->getTarget(), timestamp);

    return pointer->isCaptured();
}

void
PointerManager::handleTimeUpdate(apl_time_t timestamp)
{
    if (mLastestPointerTimeStamp > timestamp) {
        LOG(LogLevel::kWarn) << "TimeUpdate pointer proccessed later than expected. Latest Pointer TimeStamp: "
                             << mLastestPointerTimeStamp << " TimeUpdate timestamp: " << timestamp;
    }
    sendEventToTarget(kPointerTimeUpdate, mLastActivePointer, timestamp);
}

void
PointerManager::handleNewTarget(const std::shared_ptr<apl::Pointer>& pointer,
                                const CoreComponentPtr &newTarget,
                                apl::apl_time_t timestamp)
{
    if (pointer && pointer->getTarget() != newTarget) {
        sendEventToTarget(kPointerTargetChanged, pointer, timestamp);
    }
}

std::shared_ptr<Pointer>
PointerManager::getOrCreatePointer(const PointerEvent& pointerEvent)
{
    if (!mActivePointer || pointerEvent.pointerId != mActivePointer->getId()) {
        return std::make_shared<Pointer>(Pointer(pointerEvent.pointerType, pointerEvent.pointerId));
    }
    return mActivePointer;
}

ActionableComponentPtr
PointerManager::handlePointerStart(const std::shared_ptr<Pointer>& pointer,
                                   const PointerEvent& pointerEvent)
{
    if (mActivePointer != nullptr)
        return nullptr;

    auto top = CoreComponent::cast(mCore.topComponent());
    if (!top)
        return nullptr;

    auto visitor = TouchableAtPosition(pointerEvent.pointerEventPosition);
    top->raccept(visitor);
    auto target = ActionableComponent::cast(visitor.getResult());

    pointer->setTarget(target);
    pointer->setPosition(pointerEvent.pointerEventPosition);
    mActivePointer = pointer;
    mLastActivePointer = mActivePointer;
    return target;
}

ActionableComponentPtr
PointerManager::handlePointerUpdate(const std::shared_ptr<Pointer>& pointer,
                                    const PointerEvent& pointerEvent)
{
    auto target = pointer->getTarget();
    if (pointer->getPointerType() == kMousePointer && !target) {
        mHoverManager.setCursorPosition(pointerEvent.pointerEventPosition);
    }
    pointer->setPosition(pointerEvent.pointerEventPosition);
    return target;
}

ActionableComponentPtr
PointerManager::handlePointerEnd(const std::shared_ptr<Pointer>& pointer,
                                 const PointerEvent& pointerEvent)
{
    if (mActivePointer && pointer->getId() == mActivePointer->getId()) {
        switch (pointer->getPointerType()) {
            case kTouchPointer:
                break;
            case kMousePointer:
                mHoverManager.setCursorPosition(pointerEvent.pointerEventPosition);
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
