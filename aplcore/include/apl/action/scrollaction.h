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

#include "apl/action/animatedscrollaction.h"
#include "apl/primitives/object.h"

namespace apl {

class CoreCommand;
class AnimatedProperty;

/**
 * Scroll to position in scrollable component.
 *
 * This action results in a kEventTypeScrollTo event being fired.
 * The following properties are passed with the event:
 *
 *    kEventPropertyPosition  The scroll position.
 */
class ScrollAction : public AnimatedScrollAction {
public:
    /**
     * @param timers Timer reference.
     * @param command Command that spawned this action.
     * @return
     */
    static std::shared_ptr<ScrollAction> make(const TimersPtr& timers,
                                              const std::shared_ptr<CoreCommand>& command);

    /**
     * @param timers Timer reference.
     * @param context context ot run this action in.
     * @param target component to perform action on.
     * @param targetDistance Object containing Dimension representing distance to be scrolled.
     * @param duration scrolling duration.
     * @return
     */
    static std::shared_ptr<ScrollAction> make(const TimersPtr& timers,
                                              const ContextPtr& context,
                                              const CoreComponentPtr& target,
                                              const Object& targetDistance,
                                              apl_duration_t duration = 0);

    ScrollAction(const TimersPtr& timers,
                 const ContextPtr& context,
                 const CoreComponentPtr& target,
                 const Object& targetDistance,
                 apl_duration_t duration);

private:
    void start();

    std::shared_ptr<CoreCommand> mCommand;
    Object mTargetDistance;
};


} // namespace apl

#endif //_APL_SCROLL_ACTION_H
