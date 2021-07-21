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

#ifndef _APL_PAGER_FLING_GESTURE_H
#define _APL_PAGER_FLING_GESTURE_H

#include "apl/common.h"
#include "apl/component/component.h"
#include "apl/primitives/object.h"
#include "apl/touch/gestures/flinggesture.h"

namespace apl {

class VelocityTracker;

class PagerFlingGesture : public FlingGesture, public std::enable_shared_from_this<PagerFlingGesture> {
public:
    static std::shared_ptr<PagerFlingGesture> create(const ActionablePtr& actionable);

    PagerFlingGesture(const ActionablePtr& actionable);
    virtual ~PagerFlingGesture() = default;

    void release() override;
    void reset() override;

protected:
    bool onMove(const PointerEvent& event, apl_time_t timestamp) override;
    bool onDown(const PointerEvent& event, apl_time_t timestamp) override;
    bool onUp(const PointerEvent& event, apl_time_t timestamp) override;
    bool onCancel(const PointerEvent& event, apl_time_t timestamp) override;


private:
    void animateRemainder(bool fulfill);
    void awaitForFinish(const std::weak_ptr<PagerFlingGesture>& weak_ptr, bool fulfill);
    bool finishUp();

    ActionPtr mAnimateAction;
    size_t mCurrentPage;
    size_t mTargetPage;
    PageDirection mPageDirection;
    float mLastAnimationAmount; // Last amount that was successfully applied to animation transition
    float mAmount;
    std::shared_ptr<ExecutionResourceHolder> mResourceHolder;
    LayoutDirection mLayoutDirection;
};

} // namespace apl

#endif //_APL_PAGER_FLING_GESTURE_H
