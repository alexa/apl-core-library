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

static const bool DEBUG_SCROLL = false;

ScrollAction::ScrollAction(const TimersPtr& timers,
                 const std::shared_ptr<CoreCommand>& command,
                 const ComponentPtr& target)
        : ResourceHoldingAction(timers, command->context()),
          mCommand(command),
          mTarget(target)
{}

std::shared_ptr<ScrollAction>
ScrollAction::make(const TimersPtr& timers,
                     const std::shared_ptr<CoreCommand>& command,
                     const ComponentPtr& target)
{
    auto ptr = std::make_shared<ScrollAction>(timers, command, command->target());
    command->context()->sequencer().claimResource({kCommandResourcePosition, command->target()}, ptr);
    ptr->start();
    return ptr;

}

void
ScrollAction::start() {
    EventBag bag;
//    for (const auto& it : mCommand->propDefSet()) {
//        if ((it.second.flags & kPropOut) != 0)
//            bag.emplace(it.second.key, mCommand->getValue(it.second.key));
//    }

    // Now set the position
    auto vertical = (mTarget->scrollType() == kScrollTypeVertical);
    auto innerBounds = mTarget->getCalculated(kPropertyInnerBounds).getRect();

    auto targetSize = (vertical ? innerBounds.getHeight() : innerBounds.getWidth());
    auto prop = mCommand->getValue(kCommandPropertyDistance);
    float distance = 0;

    if (prop.isRelativeDimension())
        distance = prop.getRelativeDimension() * targetSize / 100;
    else if (prop.isAbsoluteDimension())
        distance = prop.getAbsoluteDimension();

    // Calculate the new position by trimming the old position plus the distance
    auto position = mTarget->trimScroll(mTarget->scrollPosition() + Point(distance, distance));
    bag.emplace(kEventPropertyPosition, Dimension(vertical ? position.getY() : position.getX()));

    LOG_IF(DEBUG_SCROLL) << "Pushing scroll event position=" << position;
    mCommand->context()->pushEvent(Event(kEventTypeScrollTo, std::move(bag), mTarget, shared_from_this()));
}

} // namespace apl