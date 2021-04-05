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
#include "apl/touch/gestures/flinggesture.h"
#include "apl/touch/utils/autoscroller.h"
#include "apl/time/executionresourceholder.h"

namespace apl {

class VelocityTracker;

class ScrollGesture : public FlingGesture,
                      public std::enable_shared_from_this<ScrollGesture> {
public:
    static std::shared_ptr<ScrollGesture> create(const ActionablePtr& actionable);

    explicit ScrollGesture(const ActionablePtr& actionable);
    virtual ~ScrollGesture() = default;

    void release() override;
    void reset() override;

protected:
    bool onMove(const PointerEvent& event, apl_time_t timestamp) override;
    bool onDown(const PointerEvent& event, apl_time_t timestamp) override;
    bool onUp(const PointerEvent& event, apl_time_t timestamp) override;
    bool onTimeUpdate(const PointerEvent& event, apl_time_t timestamp) override;
    void scrollToSnap();

private:
    float toLocalThreshold(float threshold);
    bool isSlopeWithinTolerance(Point localPosition);
    ScrollablePtr getScrollable() const { return std::dynamic_pointer_cast<ScrollableComponent>(mActionable); }
    double getVelocityLimit(const Point& travel);

    Point mLastLocalPosition;
    std::shared_ptr<AutoScroller> mScroller;
    std::shared_ptr<ExecutionResourceHolder> mResourceHolder;
};

} // namespace apl

#endif //_APL_SCROLL_GESTURE_H
