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

#include "apl/command/configchangecommand.h"

#include "apl/command/arraycommand.h"
#include "apl/content/content.h"
#include "apl/document/documentproperties.h"
#include "apl/engine/corerootcontext.h"

namespace apl {

const char * ConfigChangeCommand::SEQUENCER = "__CONFIG_CHANGE_SEQUENCER";

ActionPtr
ConfigChangeCommand::execute(const TimersPtr& timers, bool fastMode)
{
    auto document = mDocumentContext.lock();
    if (!document)
        return nullptr;

    // Extract the event handler commands.  If none exist, we immediately execute the resize and return.
    auto& json = document->content()->getDocument()->json();
    auto it = json.FindMember(sDocumentPropertyBimap.at(kDocumentPropertyOnConfigChange).c_str());
    if (it == json.MemberEnd()) {
        document->resize();
        return nullptr;
    }

    auto context = document->createDocumentContext("ConfigChange", mProperties);
    auto commands = asCommand(*context, evaluate(*context, it->value));
    auto cmd = ArrayCommand::create(context, commands, nullptr, Properties(), "", true);

    // The subcommands of the ConfigChangeCommand always run in fast mode
    auto action = cmd->execute(timers, true);
    auto weak = mDocumentContext;

    // When all commands have finished executing call RootContext::resize().
    return Action::wrapWithCallback(timers, action, [weak](bool isResolved, const ActionPtr& actionPtr) {
        if (isResolved) {
            auto root = weak.lock();
            if (root)
                root->resize();
        }
    });
}

} // namespace apl
