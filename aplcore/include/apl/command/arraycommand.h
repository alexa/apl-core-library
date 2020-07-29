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

#ifndef _APL_COMMAND_ARRAY_COMMAND_H
#define _APL_COMMAND_ARRAY_COMMAND_H

#include "apl/command/corecommand.h"
#include "apl/engine/properties.h"

namespace apl {

/**
 * Commodity class to keep internal representation of commands array.
 */
class ArrayCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             const Object& commands,
                             const CoreComponentPtr& base,
                             const Properties& properties,
                             const std::string& parentSequencer = "",
                             bool finishAllOnTerminate = false);


    /**
     * Stock constructor.  You should only build this with std::make_shared<>
     * @param context The data-binding context.
     * @param commands An array of commands to execute. These commands have not yet been evaluated.
     * @param base The base component that these commands started from.  May be nullptr.
     * @param properties Additional properties to apply to each command.
     */
    ArrayCommand(const ContextPtr& context,
                 const Object& commands,
                 const CoreComponentPtr& base,
                 Properties&& properties,
                 const std::string& parentSequencer,
                 bool finishAllOnTerminate);

    CommandType type() const override { return kCommandTypeArray; }
    unsigned long delay() const override { return 0; }
    std::string name() const override { return "ArrayCommand"; }
    ActionPtr execute(const TimersPtr& timers, bool fastMode) override;

    const std::vector<Object>& commands() const { return mCommands.getArray(); }
    bool finishAllOnTerminate() const { return mFinishAllOnTerminate; }

private:
    const Object mCommands;
    bool mFinishAllOnTerminate;
};

using ArrayCommandPtr = std::shared_ptr<ArrayCommand>;

} // namespace apl

#endif //_APL_COMMAND_ARRAY_COMMAND_H
