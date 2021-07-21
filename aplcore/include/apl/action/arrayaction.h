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

#ifndef _APL_ACTION_ARRAY_ACTION_H
#define _APL_ACTION_ARRAY_ACTION_H

#include "apl/action/action.h"
#include "apl/command/command.h"
#include "apl/command/arraycommand.h"

namespace apl {

class ArrayAction : public Action {
public:
    static std::shared_ptr<ArrayAction> make(const TimersPtr& timers,
                                             std::shared_ptr<const ArrayCommand> command,
                                             bool fastMode) {
        auto ptr = std::make_shared<ArrayAction>(timers, command, fastMode);
        ptr->advance();
        return ptr;
    }

    ArrayAction(const TimersPtr& timers, std::shared_ptr<const ArrayCommand> command, bool fastMode);

private:
    void advance();

private:
    const std::shared_ptr<const ArrayCommand> mCommand;
    const bool mFastMode;

    int mNextIndex;

    CommandPtr mCurrentCommand;
    ActionPtr mCurrentAction;
};


} // namespace apl

#endif //_APL_ACTION_ARRAY_ACTION_H
