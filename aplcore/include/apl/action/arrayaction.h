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
#include "apl/command/arraycommand.h"
#include "apl/command/command.h"

namespace apl {

class ArrayAction : public Action {
public:
    static std::shared_ptr<ArrayAction> make(const TimersPtr& timers,
                                             const ContextPtr& context,
                                             std::shared_ptr<const CoreCommand>&& command,
                                             CommandData&& data,
                                             bool fastMode) {
        auto ptr = std::make_shared<ArrayAction>(
            timers, context, std::move(command), std::move(data), fastMode);
        ptr->advance();
        return ptr;
    }

    static std::shared_ptr<ArrayAction> make(const TimersPtr& timers,
                                             std::shared_ptr<const CoreCommand>&& command,
                                             bool fastMode) {
        return make(timers, command->context(), std::move(command),
                    CommandData(command->data()), fastMode);
    }

    ArrayAction(
        const TimersPtr& timers,
        const ContextPtr& context,
        std::shared_ptr<const CoreCommand>&& command,
        CommandData&& data,
        bool fastMode);

private:
    void advance();

private:
    const std::shared_ptr<const CoreCommand> mCommand;
    const bool mFastMode;
    ContextPtr mContext;
    CommandData mData;

    size_t mNextIndex;

    CommandPtr mCurrentCommand;
    ActionPtr mCurrentAction;
};


} // namespace apl

#endif //_APL_ACTION_ARRAY_ACTION_H
