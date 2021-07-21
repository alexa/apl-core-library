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

#include "apl/action/scrollaction.h"
#include "apl/command/corecommand.h"
#include "apl/time/sequencer.h"

namespace apl {

ScrollAction::ScrollAction(const TimersPtr& timers,
                           const ContextPtr& context,
                           const CoreComponentPtr& target,
                           const Object& targetDistance,
                           apl_duration_t duration)
        : AnimatedScrollAction(timers, context, target, duration),
          mTargetDistance(targetDistance)
{}

std::shared_ptr<ScrollAction>
ScrollAction::make(const TimersPtr& timers,
                   const std::shared_ptr<CoreCommand>& command)
{
    return make(timers, command->context(), command->target(),  command->getValue(kCommandPropertyDistance));
}

std::shared_ptr<ScrollAction>
ScrollAction::make(const TimersPtr& timers,
                   const ContextPtr& context,
                   const CoreComponentPtr& target,
                   const Object& targetDistance,
                   apl_duration_t duration)
{
    // Ensure it's scrollable target.
    if (!(target->scrollType() == kScrollTypeVertical || target->scrollType() == kScrollTypeHorizontal))
        return nullptr;
    auto ptr = std::make_shared<ScrollAction>(timers, context, target, targetDistance, duration);
    context->sequencer().claimResource({kExecutionResourcePosition, target}, ptr);
    ptr->start();
    return ptr;
}

void
ScrollAction::start() {
    // Now set the position
    auto vertical = (mContainer->scrollType() == kScrollTypeVertical);
    auto innerBounds = mContainer->getCalculated(kPropertyInnerBounds).getRect();

    auto targetSize = (vertical ? innerBounds.getHeight() : innerBounds.getWidth());
    float distance = 0;

    if (mTargetDistance.isRelativeDimension())
        distance = mTargetDistance.getRelativeDimension() * targetSize / 100;
    else if (mTargetDistance.isAbsoluteDimension())
        distance = mTargetDistance.getAbsoluteDimension();

    bool isRTL = mContainer->getCalculated(kPropertyLayoutDirection) == kLayoutDirectionRTL;
    if (!vertical && isRTL) {
        distance *= -1.0f;
    }

    // Calculate the new position by trimming the old position plus the distance
    auto position = mContainer->trimScroll(mContainer->scrollPosition() + Point(distance, distance));

    scroll(vertical, position);
}

} // namespace apl