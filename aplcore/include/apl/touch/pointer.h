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

#ifndef APL_CURSOR_H
#define APL_CURSOR_H

#include "apl/common.h"
#include "apl/primitives/point.h"
#include "apl/touch/pointerevent.h"

namespace apl {

class ActionableComponent;
using ActionableComponentPtr = std::shared_ptr<ActionableComponent>;

/**
 * The Pointer class encapsulates the relationship between a given pointer, identified by its ID and an associated target
 * component if one exists.  The target can change, but the ID is immutable.  It additionally holds properties assocciated
 * with the pointer.
 */
class Pointer {

public:
    Pointer(PointerType pointerType, id_type id = 0) : mPointerType(pointerType), mId(id) {}

    /**
     * Gets the target of this pointer.  For pointers involved in an ongoing interaction: post kCursorDown and pre
     * kCursorUp.  If a target is set for a pointer, future events will be routed to that target.
     *
     * @return The cursor target
     */
    ActionableComponentPtr getTarget() const { return mTarget.lock(); }

    void setTarget(const ActionableComponentPtr& target) {
        mTarget = target;
    }

    /**
     * @return true if pointer captured by current target, false otherwise.
     */
    bool isCaptured() const { return mCaptured; }

    /**
     * Capture pointer by provided target.
     * @param target Component that captured this pointer.
     */
    void setCapture(const ActionableComponentPtr& target) {
        assert(!mCaptured);
        setTarget(target);
        mCaptured = true;
    }

    /**
     * @return Get last known pointer position.
     */
    Point getPosition() const { return mPosition; }

    /**
     * Set pointer position.
     * @param position Pointer position.
     */
    void setPosition(const Point& position) {
        mPosition = position;
    }

    /**
     * @return Gets the id associated with this pointer.  There must never be two pointer with the same id active
     * at the same time, but id's may be reused.  For instance, once a touch ends, that pointer's id may be reused.
     */
     id_type getId() const { return mId; }

     /**
      * @return pointer type.
      */
    PointerType getPointerType() const { return mPointerType; }

private:
    const PointerType mPointerType;
    const id_type mId;
    Point mPosition;
    bool mCaptured = false;

    std::weak_ptr<ActionableComponent> mTarget;
};
}

#endif //APL_CURSOR_H
