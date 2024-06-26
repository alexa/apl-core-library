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

#include "apl/action/importpackageaction.h"

#include "apl/command/arraycommand.h"

namespace apl {

ImportPackageAction::ImportPackageAction(const TimersPtr& timers,
                                         const std::shared_ptr<CoreCommand>& command,
                                         ActionPtr&& startAction)
    : Action(timers),
      mCommand(command),
      mCurrentAction(std::move(startAction)) {}

std::shared_ptr<ImportPackageAction>
ImportPackageAction::make(const TimersPtr& timers,
                          const std::shared_ptr<CoreCommand>& command,
                          ActionPtr&& startAction)
{
    auto ptr = std::make_shared<ImportPackageAction>(timers, command, std::move(startAction));
    return ptr;
}

void
ImportPackageAction::onLoad(const std::string& version)
{
    resolve();
    auto event = std::make_shared<ObjectMap>();
    event->emplace("version", version);
    auto eventMap = mCommand->context()->opt("event").getMap();
    for (const auto& existing: eventMap) {
        event->emplace(existing.first, existing.second);
    }

    ContextPtr context = Context::createFromParent(mCommand->context());
    context->putConstant("event", event);

    ArrayCommand::create(context,
                         {mCommand->getValue(kCommandPropertyOnLoad), mCommand->data()},
                         mCommand->base(),
                         Properties(mCommand->properties()),
                         mCommand->sequencer())->execute(timers(), true);
}

void
ImportPackageAction::onFail(const std::string& nameVersionSource, const std::string& errorMessage, int code)
{
    resolve();
    auto event = std::make_shared<ObjectMap>();
    event->emplace("value", nameVersionSource);
    event->emplace("error", errorMessage);
    event->emplace("errorCode", code);
    auto eventMap = mCommand->context()->opt("event").getMap();
    for (const auto& existing: eventMap) {
        event->emplace(existing.first, existing.second);
    }

    ContextPtr context = Context::createFromParent(mCommand->context());
    context->putConstant("event", event);

    ArrayCommand::create(context,
                         {mCommand->getValue(kCommandPropertyOnFail), mCommand->data()},
                         mCommand->base(),
                         Properties(mCommand->properties()),
                         mCommand->sequencer())->execute(timers(), true);
}

} // namespace apl