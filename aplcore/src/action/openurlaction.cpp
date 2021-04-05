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

#include "apl/action/openurlaction.h"
#include "apl/command/arraycommand.h"
#include "apl/command/corecommand.h"

namespace apl {

/*
 * This is called at the start of the open URL.  Watch for the view host response to
 * the open request.  If it is positive, we resolve the action.  If not, we ask the OpenURLCommand
 * to execute the failure commands.
 */
void
OpenURLAction::start()
{
    std::weak_ptr<OpenURLAction> weak_ptr(std::static_pointer_cast<OpenURLAction>(shared_from_this()));
    mCurrentAction->then([weak_ptr](const ActionPtr& actionPtr){
        auto self = weak_ptr.lock();
        if (!self)
            return;

        auto arg = actionPtr->getIntegerArgument();
        if (arg == 0)  // Value of 0 represents success
            self->resolve();
        else  // Failure - run fail commands
            self->handleFailure(arg);
    });
}

/*
 * Ask the OpenURLCommand to execute the failure commands.
 */
void
OpenURLAction::handleFailure(int argument)
{
    // Configure an appropriate event.source.X model.
    auto source = std::make_shared<ObjectMap>();
    source->emplace("source", "OpenURL");
    source->emplace("type", "OpenURL");
    source->emplace("handler", "Fail");
    source->emplace("value", argument);

    auto event = std::make_shared<ObjectMap>();
    event->emplace("source", source);

    ContextPtr context = Context::createFromParent(mCommand->context());
    context->putConstant("event", event);

    auto array = ArrayCommand::create(context,
                                      mCommand->getValue(kCommandPropertyOnFail),
                                      mCommand->target(),
                                      mCommand->properties(),
                                      mCommand->sequencer());
    // Run these in normal mode
    mCurrentAction = array->execute(timers(), false);
    if (!mCurrentAction) {
        resolve();
        return;
    }

    std::weak_ptr<OpenURLAction> weak_ptr(std::static_pointer_cast<OpenURLAction>(shared_from_this()));
    mCurrentAction->then([weak_ptr](const ActionPtr& actionPtr){
        auto self = weak_ptr.lock();
        if (!self)
            return;

        self->resolve();
    });
}



} // namespace apl
