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

#ifndef _APL_TAP_GESTURE_H
#define _APL_TAP_GESTURE_H

#include "apl/common.h"
#include "apl/primitives/object.h"
#include "apl/touch/gesture.h"

namespace apl {

/**
 * The Tap gesture detects a deliberate tap on a component, in a manner that is more restrictive than onPress. Tap
 * imposes the following additional restrictions with respect to the initial down and final pointer up events:
 *
 * - They must occur within a short distance (to rule out drag-like gestures).
 * - They must describe a vector of little or no velocity (to rule out swipe-like gestures).
 */
class TapGesture : public std::enable_shared_from_this<TapGesture>, public Gesture {
public:
    static std::shared_ptr<TapGesture> create(const ActionablePtr& actionable, const Context& context, const Object& object);

    TapGesture(const ActionablePtr& actionable, Object&& onTap);

    virtual ~TapGesture() = default;

    void reset() override;
    bool invokeAccessibilityAction(const std::string& name) override;

protected:
    bool onDown(const PointerEvent& event, apl_time_t timestamp) override;
    bool onUp(const PointerEvent& event, apl_time_t timestamp) override;

private:
    Object mOnTap;
    apl_time_t mStartTime;
    Point mStartPoint;
    int mMaximumTapVelocity;
    int mMaximumTapTravel;
};

} // namespace apl

#endif //_APL_TAP_GESTURE_H
