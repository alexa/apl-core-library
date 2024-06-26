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

#ifndef APL_BINDING_CHANGE_H
#define APL_BINDING_CHANGE_H

#include "apl/common.h"
#include "apl/primitives/object.h"

namespace apl {

/**
 * An abstract base class for the function to be executed when a data-binding has an "onChange"
 * handler. This base class takes care of ensuring that the function is not re-entrant.  Subclasses
 * should override the execute() method.
 */
class BindingChange {
public:
    explicit BindingChange(Object&& commands) : mCommands(std::move(commands)) {
        assert(!mCommands.empty());
    }

    virtual ~BindingChange() = default;

    /**
     * Override this in your subclass
     * @param value The new value assigned to the data-binding.
     * @param previous The old value assigned to the data-binding.
     */
    virtual void execute(const Object& value, const Object& previous) = 0;

    /**
     * Run this when the data-binding value changes.
     * @param value The new value assigned to the data binding.
     * @param previous The old value assigned to the data binding.
     */
    void run(const Object& value, const Object& previous) {
        if (!mInExecute) {
            mInExecute = true;
            execute(value, previous);
            mInExecute = false;
        }
    }

    /**
     * @return The commands associated with this binding change.
     */
    const Object& commands() { return mCommands; }

private:
    Object mCommands;
    bool mInExecute = false;
};

using BindingChangePtr = std::shared_ptr<BindingChange>;

/**
 * Check if binding with provided name valid in provided context.
 * @param context Context.
 * @param binding binding definition.
 * @param name Binding name.
 * @return true if valid, false otherwise.
 */
extern bool isValidBinding(const ContextPtr& context, const Object& binding, const std::string& name);

/**
 * Process the "bind" variable in a component or graphic element and add all bound values
 * to the data-binding context.
 *
 * @param context The data-binding context in which to evaluate the item
 * @param item The item that contains a "bind" property
 * @param func A function that takes an array of "onChange" commands and returns a BindingChangePtr.
 *             May be nullptr.
 * @return An array of BindingChangePtr.
 */
extern std::vector<BindingChangePtr>
attachBindings(const ContextPtr& context, const Object& item, std::function<BindingChangePtr(Object&&)> func);

} // namespace apl

#endif // APL_BINDING_CHANGE_H
