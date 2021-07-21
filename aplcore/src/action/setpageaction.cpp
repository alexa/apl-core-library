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

#include "apl/action/setpageaction.h"
#include "apl/command/corecommand.h"
#include "apl/time/sequencer.h"
#include "apl/component/pagercomponent.h"

namespace apl {

inline int clamp(int v, int minV, int maxV)
{
    if (v < minV) return minV;
    if (v > maxV) return maxV;
    return v;
}

inline int modulus(int a, int b)
{
    const int result = a % b;
    return result >= 0 ? result : result + b;
}

SetPageAction::SetPageAction(const TimersPtr& timers,
                             const std::shared_ptr<CoreCommand>& command,
                             const CoreComponentPtr& target)
        : ResourceHoldingAction(timers, command->context()),
          mCommand(command),
          mTarget(target)
{}

std::shared_ptr<SetPageAction>
SetPageAction::make(const TimersPtr& timers,
                    const std::shared_ptr<CoreCommand>& command)
{
    // Ensure there is a paging target with multiple children
    auto target = command->target();
    if (!target
        || (target->scrollType() != kScrollTypeHorizontalPager &&  target->scrollType() != kScrollTypeVerticalPager)
        || target->getChildCount() < 2)
        return nullptr;

    auto ptr = std::make_shared<SetPageAction>(timers, command, target);
    command->context()->sequencer().claimResource({kExecutionResourcePosition, target}, ptr);
    ptr->start();
    return ptr;
}

void
SetPageAction::start()
{
    auto position = static_cast<CommandPosition>(mCommand->getValue(kCommandPropertyPosition).asInt());
    auto value = mCommand->getValue(kCommandPropertyValue).asInt();
    int len = mTarget->getChildCount();
    int current = mTarget->pagePosition();
    int index = current;
    PageDirection direction;

    switch (position) {
        case kCommandPositionAbsolute:
            // Clamp to the valid range.  Note that a negative position is a measurement from the end
            index = clamp((value < 0 ? value + len : value), 0, len - 1);
            direction = (index < current ? kPageDirectionBack : kPageDirectionForward);
            break;

        case kCommandPositionRelative:
            // Offset from the current location
            index = current + value;

            // A non-wrapping pager doesn't support relative motion that wraps. Ignore if it is out of range.
            if (mTarget->getCalculated(kPropertyNavigation).asInt() != kNavigationWrap && (index < 0 || index >= len)) {
                resolve();
                return;
            }

            // Use modulus to ensure we're in the correct range
            index = modulus(index, len);
            direction = (value < 0 ? kPageDirectionBack : kPageDirectionForward);
            break;
        default:
            direction = kPageDirectionForward;
            break;
    }

    // Check to see if we haven't moved
    if (index == current) {
        resolve();
    }
    else {
        mTarget->ensureChildLayout(mTarget->getCoreChildAt(index), true);
        PagerComponent::setPageUtil(mContext, mTarget, index, direction, shared_from_this(),
            position == kCommandPositionAbsolute || mContext->getRequestedAPLVersion().compare("1.6") < 0);
    }
}

} // namespace apl