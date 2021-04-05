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

#include "apl/command/arraycommand.h"
#include "apl/command/configchangecommand.h"
#include "apl/content/content.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/propdef.h"
#include "apl/engine/rootcontext.h"

namespace apl {

const char * ConfigChangeCommand::SEQUENCER = "__CONFIG_CHANGE_SEQUENCER";

ActionPtr
ConfigChangeCommand::execute(const TimersPtr& timers, bool fastMode)
{
    auto root = mRootContext.lock();
    if (!root)
        return nullptr;

    // Extract the event handler commands.  If none exist, we immediately execute the resize and return.
    auto& json = root->content()->getDocument()->json();
    auto it = json.FindMember(sComponentPropertyBimap.at(kPropertyOnConfigChange).c_str());
    if (it == json.MemberEnd()) {
        root->resize();
        return nullptr;
    }

    auto context = root->createDocumentContext("ConfigChange", mProperties);
    auto commands = asCommand(*context, evaluate(*context, it->value));
    auto cmd = ArrayCommand::create(context, commands, nullptr, Properties(), "", true);

    // The subcommands of the ConfigChangeCommand always run in fast mode
    auto action = cmd->execute(timers, true);
    auto weak = mRootContext;

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