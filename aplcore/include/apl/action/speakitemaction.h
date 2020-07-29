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

#ifndef _APL_SPEAK_ITEM_ACTION_H
#define _APL_SPEAK_ITEM_ACTION_H

#include "apl/action/resourceholdingaction.h"

namespace apl {

class CoreCommand;
class CoreComponent;
class ScrollToAction;

/**
 * Lightweight action that handles speaking.  This action will send out an EventType::kEventTypePreroll,
 * followed by a EventType::kEventtypeScrollTo to bring the item into view and a EventType::kEventTypeSpeak
 * to speak the item.
 */
class SpeakItemAction : public ResourceHoldingAction {
public:
    static std::shared_ptr<SpeakItemAction> make(const TimersPtr& timers,
                                                 const std::shared_ptr<CoreCommand>& command,
                                                 const CoreComponentPtr& target=nullptr);

    SpeakItemAction(const TimersPtr& timers,
                    const std::shared_ptr<CoreCommand>& command,
                    const CoreComponentPtr& target);

private:
    void start();
    void scroll(const std::shared_ptr<ScrollToAction>& action);
    void advance();

private:
    std::shared_ptr<CoreCommand> mCommand;
    CoreComponentPtr mTarget;
    ActionPtr mCurrentAction;
    std::string mSource;
};

}  // namespace apl

#endif //_APL_SPEAK_ITEM_ACTION_H
