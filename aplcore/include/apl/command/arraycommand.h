/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/command/command.h"
#include "apl/primitives/object.h"
#include "apl/engine/context.h"
#include "apl/engine/properties.h"
#include "apl/component/corecomponent.h"

namespace apl {

/**
 * Not a single command - instead, an array of commands
 */
class ArrayCommand : public Command {
public:
    static CommandPtr create(const ContextPtr& context,
                             const Object& commands,
                             const CoreComponentPtr& base,
                             const Properties& properties,
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
                 const Properties& properties,
                 bool finishAllOnTerminate)
        : mContext(context),
          mCommands(commands),
          mBase(base),
          mProperties(properties),
          mFinishAllOnTerminate(finishAllOnTerminate){}

    virtual unsigned long delay() const override { return 0; }
    virtual std::string name() const override { return "ArrayCommand"; }
    virtual ActionPtr execute(const TimersPtr& timers, bool fastMode) override;

    const std::vector<Object>& commands() const { return mCommands.getArray(); }
    ContextPtr context() const { return mContext; }
    CoreComponentPtr base() const { return mBase; }
    bool finishAllOnTerminate() const { return mFinishAllOnTerminate; }

private:
    const ContextPtr mContext;
    const Object mCommands;
    const CoreComponentPtr mBase;
    Properties mProperties;
    bool mFinishAllOnTerminate;
};

using ArrayCommandPtr = std::shared_ptr<ArrayCommand>;

} // namespace apl

#endif //_APL_COMMAND_ARRAY_COMMAND_H
