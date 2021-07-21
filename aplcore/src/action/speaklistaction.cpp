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

#include "apl/action/speaklistaction.h"
#include "apl/action/speakitemaction.h"
#include "apl/action/scrolltoaction.h"
#include "apl/command/corecommand.h"

namespace apl {

std::shared_ptr<SpeakListAction>
SpeakListAction::make(const TimersPtr& timers,
                      const std::shared_ptr<CoreCommand>& command)
{
    auto container = command->target();
    auto start = command->getValue(kCommandPropertyStart).asInt();
    auto count = command->getValue(kCommandPropertyCount).asInt();
    auto len = static_cast<int>(container->getChildCount());

    // Sanity checks
    if (len <= 0 || count <= 0 || start >= len)
        return nullptr;

    // A negative start value means from the end
    if (start < 0) {
        start += len;
        if (start < 0)
            start = 0;   // Just clip it to the start
    }

    // Clip the count to the number of items possible
    if (start + count > len)
        count = len - start;

    auto ptr = std::make_shared<SpeakListAction>(timers, command, container, start, start + count);
    ptr->advance();
    return ptr;
}

void
SpeakListAction::advance()
{
    while (mNextIndex < mEndIndex) {
        mCurrentAction = SpeakItemAction::make(timers(), mCommand, mContainer->getCoreChildAt(mNextIndex++));
        if (!mCurrentAction)
            continue;

        std::weak_ptr<SpeakListAction> weak_ptr(std::static_pointer_cast<SpeakListAction>(shared_from_this()));
        mCurrentAction->then([weak_ptr](const ActionPtr& actionPtr){
            auto self = weak_ptr.lock();
            if (!self)
                return;

            self->advance();
        });

        return;
    }

    resolve();
}


} // namespace apl