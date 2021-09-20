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
#include "apl/command/displaystatechangecommand.h"
#include "apl/content/content.h"
#include "apl/document/documentproperties.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/rootcontext.h"

namespace apl {

const char * DisplayStateChangeCommand::SEQUENCER = "__DISPLAY_STATE_CHANGE_SEQUENCER";

ActionPtr
DisplayStateChangeCommand::execute(const TimersPtr& timers, bool fastMode)
{
    auto root = mRootContext.lock();
    if (!root)
        return nullptr;

    // Extract the event handler commands, if provided in the document
    auto& json = root->content()->getDocument()->json();
    auto it = json.FindMember(sDocumentPropertyBimap.at(kDocumentPropertyOnDisplayStateChange).c_str());
    if (it == json.MemberEnd()) {
        return nullptr;
    }

    auto context = root->createDocumentContext("DisplayStateChange", mProperties);
    auto commands = asCommand(*context, evaluate(*context, it->value));
    auto cmd = ArrayCommand::create(context, commands, nullptr, Properties(), "", true);

    // The subcommands of the DisplayStateChangeCommand always run in fast mode
    auto action = cmd->execute(timers, true);

    return action;
}

} // namespace apl
