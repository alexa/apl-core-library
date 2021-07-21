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

#ifndef _APL_OPEN_URL_ACTION_H
#define _APL_OPEN_URL_ACTION_H

#include "apl/action/action.h"

namespace apl {

class CoreCommand;

/**
 * Lightweight action class that handles the onFail case for OpenURL.
 */
class OpenURLAction : public Action {
public:
    static std::shared_ptr<OpenURLAction> make(const TimersPtr& timers,
                                               const std::shared_ptr<CoreCommand>& command,
                                               ActionPtr&& startAction)
    {
        if (!startAction)
            return nullptr;

        auto ptr = std::make_shared<OpenURLAction>(timers, command, std::move(startAction));
        ptr->start();
        return ptr;
    }

    static std::shared_ptr<OpenURLAction> makeFailed(const TimersPtr& timers,
                                                     const std::shared_ptr<CoreCommand>& command,
                                                     int argument)
    {
        auto ptr = std::make_shared<OpenURLAction>(timers, command, nullptr);
        ptr->handleFailure(argument);
        return ptr;
    }

    OpenURLAction(const TimersPtr& timers,
                  const std::shared_ptr<CoreCommand>& command,
                  ActionPtr&& startAction)
        : Action(timers),
          mCommand(command),
          mCurrentAction(std::move(startAction))
    {
        addTerminateCallback([this](const TimersPtr&) {
            if (mCurrentAction) {
                mCurrentAction->terminate();
                mCurrentAction = nullptr;
            }
        });
    }

private:
    void start();
    void handleFailure(int argument);

private:
    std::shared_ptr<CoreCommand> mCommand;
    ActionPtr mCurrentAction;
};


} // namespace apl

#endif //_APL_OPEN_URL_ACTION_H
