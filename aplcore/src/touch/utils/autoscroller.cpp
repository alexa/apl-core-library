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

#include "apl/touch/utils/autoscroller.h"
#include "apl/touch/utils/unidirectionaleasingscroller.h"

#include "apl/animation/coreeasing.h"
#include "apl/content/rootconfig.h"
#include "apl/component/scrollablecomponent.h"
#include "apl/primitives/timefunctions.h"
#include "apl/time/timemanager.h"

namespace apl {

std::shared_ptr<AutoScroller>
AutoScroller::make(
    const RootConfig& rootConfig,
    const ScrollablePtr& scrollable,
    FinishFunc finish,
    const Point& velocity)
{
    // TODO: We don't have any other scrollers ATM, so don't really use config.
    return UnidirectionalEasingScroller::make(scrollable, std::move(finish), velocity);
}

std::shared_ptr<AutoScroller>
AutoScroller::make(
    const RootConfig& rootConfig,
    const ScrollablePtr& scrollable,
    FinishFunc finish,
    const Point& target,
    apl_duration_t duration)
{
    return UnidirectionalEasingScroller::make(scrollable, std::move(finish), target, duration);
}

AutoScroller::AutoScroller(const ScrollablePtr& scrollable, FinishFunc finish) :
    mScrollable(scrollable), mOnFinish(std::move(finish)), mFinished(false),
    mStartTime(scrollable->getRootConfig().getTimeManager()->currentTime())
{}

void
AutoScroller::update(apl_time_t time)
{
    if (mFinished) return;

    // Make it safe in terms of limits.
    auto offset = time < mStartTime ? 0 : time - mStartTime;
    offset = offset >= getDuration() ? getDuration() : offset;

    updateOffset(offset);
}

void
AutoScroller::updateOffset(apl_duration_t offset)
{
    if (mFinished) return;

    auto scrollable = mScrollable.lock();
    if (!scrollable) {
        finish();
        return;
    }

    update(scrollable, offset);
}

} // namespace apl