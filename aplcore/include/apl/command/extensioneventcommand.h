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

#ifndef _APL_EXTENSION_EVENT_COMMAND_H
#define _APL_EXTENSION_EVENT_COMMAND_H

#include "apl/command/corecommand.h"
#include "apl/action/extensioneventaction.h"
#include "apl/content/extensioncommanddefinition.h"
#include "apl/utils/session.h"

namespace apl {

/**
 * Run a command that the view host has registered using RootConfig.registerExtensionCommand()
 * This command will dispatch a Event::kEventTypeCustom to the view host when it executes.
 */
class ExtensionEventCommand : public CoreCommand {
public:
    static CommandPtr create(const ExtensionCommandDefinition& def,
                             const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base,
                             const std::string& parentSequencer) {
        return std::make_shared<ExtensionEventCommand>(def, context, std::move(properties), base, parentSequencer);
    }

    ExtensionEventCommand(const ExtensionCommandDefinition& def,
                          const ContextPtr& context,
                          Properties&& properties,
                          const CoreComponentPtr& base,
                          const std::string& parentSequencer)
        : CoreCommand(context, std::move(properties), base, parentSequencer),
          mDefinition(def) {}

    CommandType type() const override { return kCommandTypeCustomEvent; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override;

    std::string getCommandName() const { return mDefinition.getName(); }
    std::string getCommandURI() const { return mDefinition.getURI(); }

private:
    const ExtensionCommandDefinition& mDefinition;
};

} // namespace apl

#endif // _APL_EXTENSION_EVENT_COMMAND_H
