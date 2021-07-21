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

#ifndef _APL_SPEAK_LIST_ACTION_H
#define _APL_SPEAK_LIST_ACTION_H

#include "apl/action/action.h"

namespace apl {

class CoreCommand;
class CoreComponent;

/**
 * Process a SpeakListCommand by executing SpeakItemAction on each component in turn.
 */
class SpeakListAction : public Action {
public:
    static std::shared_ptr<SpeakListAction> make(const TimersPtr& timers,
                                                 const std::shared_ptr<CoreCommand>& command);

    SpeakListAction(const TimersPtr& timers,
                    const std::shared_ptr<CoreCommand>& command,
                    CoreComponentPtr& container,
                    size_t startIndex, size_t endIndex)
        : Action(timers),
          mCommand(command),
          mContainer(container),
          mNextIndex(startIndex),
          mEndIndex(endIndex)
    {
        addTerminateCallback([this](const TimersPtr&) {
            if (mCurrentAction) {
                mCurrentAction->terminate();
                mCurrentAction = nullptr;
            }
        });
    }

private:
    void advance();

private:
    std::shared_ptr<CoreCommand> mCommand;
    CoreComponentPtr mContainer;
    ActionPtr mCurrentAction;
    size_t mNextIndex;
    size_t mEndIndex;
};

} // namespace apl

#endif //_APL_SPEAK_LIST_ACTION_H
