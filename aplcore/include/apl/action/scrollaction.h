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

#ifndef _APL_SCROLL_ACTION_H
#define _APL_SCROLL_ACTION_H

#include "apl/action/resourceholdingaction.h"

namespace apl {

class CoreCommand;

/**
 * Scroll to position in scrollable component.
 *
 * This action results in a kEventTypeScrollTo event being fired.
 * The following properties are passed with the event:
 *
 *    kEventPropertyPosition  The scroll position.
 */
class ScrollAction : public ResourceHoldingAction {
public:
    static std::shared_ptr<ScrollAction> make(const TimersPtr& timers,
                                                const std::shared_ptr<CoreCommand>& command,
                                                const ComponentPtr& target = nullptr);

    ScrollAction(const TimersPtr& timers,
                 const std::shared_ptr<CoreCommand>& command,
                 const ComponentPtr& target);

private:
    void start();

private:
    std::shared_ptr<CoreCommand> mCommand;
    ComponentPtr mTarget;
};


} // namespace apl

#endif //_APL_SCROLL_ACTION_H
