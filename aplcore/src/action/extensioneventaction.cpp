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

#include "apl/action/extensioneventaction.h"
#include "apl/command/extensioneventcommand.h"
#include "apl/content/extensioncommanddefinition.h"

namespace apl {

std::shared_ptr<ExtensionEventAction>
ExtensionEventAction::make(const TimersPtr& timers,
                           const std::shared_ptr<ExtensionEventCommand>& command,
                           bool requireResolution)
{
    auto ptr = std::make_shared<ExtensionEventAction>(timers, command);
    ptr->start(requireResolution);
    return requireResolution ? ptr : nullptr;
}

void
ExtensionEventAction::start(bool requireResolution)
{
    // Freeze the "event.source" property as a JSON object
    rapidjson::Document source;
    auto serializedSource = mCommand->context()->opt("event").get("source").serialize(source.GetAllocator());
    source.CopyFrom(serializedSource, source.GetAllocator());

    EventBag bag;
    bag.emplace(kEventPropertyName, mCommand->getCommandName());
    bag.emplace(kEventPropertyExtensionURI, mCommand->getCommandURI());
    bag.emplace(kEventPropertySource, Object(std::move(source)));
    bag.emplace(kEventPropertyExtension, mCommand->getValue(kCommandPropertyExtension));

    auto self = requireResolution ? shared_from_this() : nullptr;
    mCommand->context()->pushEvent(Event(kEventTypeExtension, std::move(bag), nullptr, self));
}

}