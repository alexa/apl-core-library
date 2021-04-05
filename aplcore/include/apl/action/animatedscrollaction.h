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

#ifndef _APL_ANIMATED_SCROLL_ACTION_H
#define _APL_ANIMATED_SCROLL_ACTION_H

#include "apl/action/resourceholdingaction.h"

namespace apl {

class CoreCommand;
class AutoScroller;

class AnimatedScrollAction : public ResourceHoldingAction {
public:
    CoreComponentPtr getScrollableContainer() const { return mContainer; }

protected:
    AnimatedScrollAction(const TimersPtr& timers,
                         const ContextPtr& context,
                         const CoreComponentPtr& scrollable,
                         apl_duration_t duration = 0);

    void scroll(bool vertical, const Point& position);

    CoreComponentPtr mContainer;

private:
    void advance();

    std::shared_ptr<AutoScroller> mScroller;
    ActionPtr mCurrentAction;
    apl_duration_t mDuration;
};


} // namespace apl

#endif //_APL_ANIMATED_SCROLL_ACTION_H
