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

#ifndef _APL_FLING_GESTURE_H
#define _APL_FLING_GESTURE_H

#include "apl/common.h"
#include "apl/primitives/object.h"
#include "apl/time/executionresourceholder.h"
#include "apl/touch/gesture.h"

namespace apl {

class VelocityTracker;

/**
 * Base implementation for any gesture that requires to track fling/scrolling velocity.
 */
class FlingGesture : public Gesture {
public:
    explicit FlingGesture(const ActionablePtr& actionable);
    virtual ~FlingGesture() = default;

    void reset() override;

protected:
    bool onMove(const PointerEvent& event, apl_time_t timestamp) override;
    bool onDown(const PointerEvent& event, apl_time_t timestamp) override;
    bool onUp(const PointerEvent& event, apl_time_t timestamp) override;

    /**
     * Check if sufficient time has passed to start the scrolling/swiping movement.
     * @return true if passed, false otherwise.
     */
    bool passedScrollOrTapTimeout(apl_time_t timestamp);

    bool isSlopeWithinTolerance(Point localPosition, bool horizontal);

    Point mStartPosition;
    apl_time_t mStartTime;
    std::unique_ptr<VelocityTracker> mVelocityTracker;
};

} // namespace apl

#endif //_APL_SCROLL_GESTURE_H
