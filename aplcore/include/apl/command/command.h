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

#ifndef _APL_COMMAND_H
#define _APL_COMMAND_H

#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"
#include "apl/action/action.h"
#include "apl/command/commandproperties.h"

namespace apl {

/**
 * Generic command definition.  A command is an abstract entity that contains
 * one or more APL primitive commands to be executed in some order with delays.
 *
 * The tricky part to get right is that those primitive commands execute in a
 * constructed Context which has bindings for source and target properties. These
 * bindings are evaluated right before the command is executed so that they
 * reflect the current state of the components.  For example, a command could
 * contain a sequence of primitive "SetValue" commands each of which toggle
 * the checked state of a component between checked and unchecked.
 *
 * The net result is that each command generally holds the raw data and defers
 * constructing the detailed action to be performed until it is explicitly
 * required.
 */
class Command : public std::enable_shared_from_this<Command>,
                public NonCopyable,
                public Counter<Command> {

public:
    virtual ~Command() {}

    /**
     * @return The time in milliseconds to delay before running this command.
     */
    virtual unsigned long delay() const = 0;

    /**
     * @return The human-readable name of this command.
     */
    virtual std::string name() const = 0;

    /**
     * Execute the command.
     *
     * The execution of the command ignores any delay() value; it is
     * assumed that the delay has already been processed.
     * @param fastMode True if we're in fast mode.
     * @return An action or nullptr if there is nothing to do.
     */
    virtual ActionPtr execute(const TimersPtr& timers, bool fastMode) = 0;

    /**
     * Called before the command is executed and before any delay with the command
     * is started.
     */
    virtual void prepare() {};

    /**
     * Called after the command has been completed or terminated.
     */
    virtual void complete() {};

    /**
     * @return Sequencer name for command to be executed on.
     */
    virtual std::string sequencer() const { return ""; }
};

} // namespace apl

#endif //_APL_COMMAND_H
