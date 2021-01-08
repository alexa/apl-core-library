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

#ifndef _APL_SCROLL_GESTURE_H
#define _APL_SCROLL_GESTURE_H

#include "apl/common.h"
#include "apl/primitives/object.h"
#include "apl/touch/gesture.h"

namespace apl {

class AutoScroller;
class VelocityTracker;

class ScrollGesture : public Gesture, public std::enable_shared_from_this<ScrollGesture> {
public:
    static std::shared_ptr<ScrollGesture> create(const ActionablePtr& actionable);

    ScrollGesture(const ActionablePtr& actionable);
    virtual ~ScrollGesture() = default;

protected:
    void onMove(const PointerEvent& event, apl_time_t timestamp) override;
    void onDown(const PointerEvent& event, apl_time_t timestamp) override;
    void onUp(const PointerEvent& event, apl_time_t timestamp) override;
    void onTimeUpdate(const PointerEvent& event, apl_time_t timestamp) override;
    void reset() override;
    void scrollToSnap();

private:
    float toLocalThreshold(float threshold);

    Point mLastLocalPosition;
    std::unique_ptr<VelocityTracker> mVelocityTracker;
    std::shared_ptr<AutoScroller> mScroller;
};

} // namespace apl

#endif //_APL_SCROLL_GESTURE_H
