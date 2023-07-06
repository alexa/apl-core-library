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

#ifndef _APL_SET_STATE_COMMAND_H
#define _APL_SET_STATE_COMMAND_H

#include "apl/command/corecommand.h"

namespace apl {

class SetStateCommand : public TemplatedCommand<SetStateCommand> {
public:
    COMMAND_CONSTRUCTOR(SetStateCommand);

    const CommandPropDefSet& propDefSet() const override;

    CommandType type() const override { return kCommandTypeSetState; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override;
};

} // namespace apl

#endif // _APL_SET_STATE_COMMAND_H
