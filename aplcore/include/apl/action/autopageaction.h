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

#ifndef _APL_AUTO_PAGE_ACTION_H
#define _APL_AUTO_PAGE_ACTION_H

#include "apl/action/resourceholdingaction.h"
#include "apl/common.h"

namespace apl {

class CoreCommand;
class Component;

/**
 * Scroll or page to bring a target component into view.
 *
 * This action results in either a kEventScrollTo or kEventPageTo event being fired.
 * The following properties are passed with the event:
 *
 *    kCommandPropertyDirection        Set to forward/back (only for PageTo)
 *    kCommandPropertyPosition         The scroll position or the page number.
 */
class AutoPageAction : public ResourceHoldingAction {
public:
    static std::shared_ptr<AutoPageAction> make(const TimersPtr& timers,
                                                const std::shared_ptr<CoreCommand>& command);

    AutoPageAction(const TimersPtr& timers,
                   const std::shared_ptr<CoreCommand>& command,
                   const ComponentPtr& container,
                   int start,
                   int end,
                   apl_time_t duration);

private:
    void advance();

private:
    std::shared_ptr<CoreCommand> mCommand;
    ComponentPtr mContainer;
    ActionPtr mCurrentAction;
    size_t mNextIndex;
    size_t mEndIndex;
    apl_time_t mDuration;
};

} // namespace apl

#endif //_APL_AUTO_PAGE_ACTION_H
