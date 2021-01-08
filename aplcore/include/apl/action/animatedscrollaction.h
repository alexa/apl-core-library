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
class Easing;

class AnimatedScrollAction : public ResourceHoldingAction {
protected:
    AnimatedScrollAction(const TimersPtr& timers,
                         const ContextPtr& context,
                         const CoreComponentPtr& scrollable);

    void scroll(bool vertical, const Point& position);

    CoreComponentPtr mContainer;

private:
    void advance(float from, float to);

    EasingPtr mEasing;
    ActionPtr mCurrentAction;
};


} // namespace apl

#endif //_APL_ANIMATED_SCROLL_ACTION_H
