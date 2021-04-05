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

#ifndef _APL_DOUBLE_PRESS_GESTURE_H
#define _APL_DOUBLE_PRESS_GESTURE_H

#include "apl/common.h"
#include "apl/primitives/object.h"
#include "apl/touch/gesture.h"

namespace apl {

/**
 * Simple implementation of SinglePress/DoublePress gesture. Built to pretty much mimic iOS/MacOS behavior.
 * In case if speed of first press (DOWN->UP) or in between presses (first UP->second DOWN) exceeds configured limit
 * gesture will trigger SinglePress, DoublePress triggered otherwise (even in case if second press is quite long).
 */
class DoublePressGesture : public std::enable_shared_from_this<DoublePressGesture>, public Gesture {
public:
    static std::shared_ptr<DoublePressGesture> create(const ActionablePtr& actionable, const Context& context, const Object& object);

    DoublePressGesture(const ActionablePtr& actionable, Object&& onDoublePress, Object&& onSinglePress);

    virtual ~DoublePressGesture() = default;

    void reset() override;
    bool invokeAccessibilityAction(const std::string& name) override;

protected:
    bool onTimeUpdate(const PointerEvent& event, apl_time_t timestamp) override;
    bool onDown(const PointerEvent& event, apl_time_t timestamp) override;
    bool onUp(const PointerEvent& event, apl_time_t timestamp) override;

private:
    void onFirstUpInternal(const PointerEvent& event, apl_time_t timestamp);
    void onSecondDownInternal(const PointerEvent& event, apl_time_t timestamp);
    void onSecondUpInternal(const PointerEvent& event, apl_time_t timestamp);

    Object mOnDoublePress;
    Object mOnSinglePress;
    apl_time_t mStartTime;
    bool mBetweenPresses;
    apl_duration_t mDoublePressTimeout;
};

} // namespace apl

#endif //_APL_DOUBLE_PRESS_GESTURE_H
