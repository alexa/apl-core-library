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

#ifndef _APL_AUTO_SCROLLER_H
#define _APL_AUTO_SCROLLER_H

#include "apl/common.h"
#include "apl/primitives/point.h"

namespace apl {

class ScrollableComponent;

using ScrollablePtr = std::shared_ptr<ScrollableComponent>;
using FinishFunc = std::function<void()>;

/**
 * Time driven scrolling interface. This is a base class for autonomous functions that allow to scroll provided
 * ScrollableComponent according to rules defined in specific implementations.
 */
class AutoScroller {
public:
    /**
     * Constructor. Do not use directly, see AutoScroller::make.
     */
    AutoScroller(const ScrollablePtr& scrollable, FinishFunc finish);
    virtual ~AutoScroller() = default;

    /**
     * Make scroller from starting velocity.
     * @param rootConfig RootConfig for configuration.
     * @param scrollable target component.
     * @param finish callback to call when scrolling finished.
     * @param velocity starting velocity.
     * @return shared pointer to scroller object.
     */
    static std::shared_ptr<AutoScroller> make(const RootConfig& rootConfig,
        const ScrollablePtr& scrollable, FinishFunc finish, const Point& velocity);

    /**
     * Make scroller from target position and duration.
     * @param rootConfig RootConfig for configuration.
     * @param scrollable target component.
     * @param finish callback to call when scrolling finished.
     * @param target target scrolling shift.
     * @param duration animation duration.
     * @return shared pointer to scroller object.
     */
    static std::shared_ptr<AutoScroller> make(const RootConfig& rootConfig,
        const ScrollablePtr& scrollable, FinishFunc finish, const Point& target,
        apl_duration_t duration);

    /**
     * Update scrolling state.
     * @param time current time.
     */
    void update(apl_time_t time);

    /**
     * Update scrolling state.
     * @param offset time offset from the start of animation.
     */
    void updateOffset(apl_duration_t offset);

    /**
    * @return expected scroll duration.
    */
    virtual apl_duration_t getDuration() const = 0;

protected:
    /**
     * Update function to be overriden by particular scroller implementation.
     * @param scrollable Scrollable to move.
     * @param offset time offset in expected duration range.
     */
    virtual void update(const ScrollablePtr& scrollable, apl_duration_t offset) = 0;

    void finish()
    {
        mFinished = true;
        mOnFinish();
    }

private:
    std::weak_ptr<ScrollableComponent> mScrollable;
    FinishFunc mOnFinish;
    bool mFinished;
    apl_time_t mStartTime;
};

} // namespace apl

#endif //_APL_AUTO_SCROLLER_H
