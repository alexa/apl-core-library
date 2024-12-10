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

#ifndef _APL_SET_PAGE_ACTION_H
#define _APL_SET_PAGE_ACTION_H

#include "apl/action/resourceholdingaction.h"

namespace apl {

class CoreCommand;
class Component;

/**
 * Change the displayed page in a Pager component.
 */
class SetPageAction : public ResourceHoldingAction {
public:
    static std::shared_ptr<SetPageAction> make(const TimersPtr& timers,
                                               const std::shared_ptr<CoreCommand>& command);

    SetPageAction(const TimersPtr& timers,
                  const std::shared_ptr<CoreCommand>& command,
                  const CoreComponentPtr& target,
                  apl_duration_t transitionDuration);

    void freeze() override;
    bool rehydrate(const CoreDocumentContext& context) override;
    ActionData getActionData() override;

private:
    void start();

private:
    std::shared_ptr<CoreCommand> mCommand;
    CoreComponentPtr mTarget;
    int mTargetIndex;
    apl_duration_t mTransitionDuration;
};

} // namespace apl

#endif //_APL_SET_PAGE_ACTION_H
