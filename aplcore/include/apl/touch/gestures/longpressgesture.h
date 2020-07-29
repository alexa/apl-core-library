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

#ifndef _APL_LONG_PRESS_GESTURE_H
#define _APL_LONG_PRESS_GESTURE_H

#include "apl/common.h"
#include "apl/primitives/object.h"
#include "apl/touch/gesture.h"

namespace apl {

class LongPressGesture : public std::enable_shared_from_this<LongPressGesture>, public Gesture {
public:
    static std::shared_ptr<LongPressGesture> create(const TouchablePtr& touchable, const Context& context, const Object& object);

    LongPressGesture(const TouchablePtr& touchable, Object&& onLongPressStart, Object&& onLongPressEnd);
    virtual ~LongPressGesture() = default;

    void reset() override;

    GestureType getType() const override { return kGestureTypeLongPress; };

protected:
    void onMove(const PointerEvent& event, apl_time_t timestamp) override;
    void onDown(const PointerEvent& event, apl_time_t timestamp) override;
    void onUp(const PointerEvent& event, apl_time_t timestamp) override;

private:
    apl_time_t mStartTime;
    Object mOnLongPressStart;
    Object mOnLongPressEnd;
    apl_duration_t mLongPressTimeout;
};

} // namespace apl

#endif //_APL_LONG_PRESS_GESTURE_H