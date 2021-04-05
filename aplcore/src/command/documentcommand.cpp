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

#include "apl/action/documentaction.h"
#include "apl/command/documentcommand.h"
#include "apl/component/corecomponent.h"
#include "apl/command/arraycommand.h"
#include "apl/content/content.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/propdef.h"
#include "apl/engine/rootcontext.h"

namespace apl {

void
DocumentCommand::collectChildCommands(const ComponentPtr& base,
                                       std::vector<CommandPtr>& collection)
{
    auto commands = base->getCalculated(mPropertyKey);
    if (commands.isArray() && !commands.empty()) {
        auto core = std::static_pointer_cast<CoreComponent>(base);
        auto ctx = core->createDefaultEventContext(mHandler);
        collection.emplace_back(ArrayCommand::create(ctx, commands, core, Properties(), ""));
    }

    for (auto i = 0 ; i < base->getChildCount() ; i++)
        collectChildCommands(base->getChildAt(i), collection);
}

CommandPtr
DocumentCommand::getDocumentCommand()
{
    // NOTE: We make the large assumption that the name of the document property
    //       is the same name as the component property.
    auto root = mRootContext.lock();
    if (!root)
        return nullptr;

    auto& json = root->content()->getDocument()->json();
    auto it = json.FindMember(sComponentPropertyBimap.at(mPropertyKey).c_str());
    if (it == json.MemberEnd())
        return nullptr;

    auto context = root->createDocumentContext(mHandler);
    auto commands = asCommand(*context, evaluate(*context, it->value));
    auto cmd = ArrayCommand::create(context, commands, nullptr, Properties(), "", true);
    return cmd;
}

ActionPtr
DocumentCommand::getComponentActions(const TimersPtr& timers, bool fastMode)
{
    auto root = mRootContext.lock();
    if (!root)
        return nullptr;

    // Extract the commands from the components
    std::vector<CommandPtr> parallelCommands;
    collectChildCommands(root->topComponent(), parallelCommands);

    if (parallelCommands.empty())
        return nullptr;
    ActionList actions;

    for (const CommandPtr& command : parallelCommands) {
        auto action = command->execute(timers, fastMode);
        if (action)
            actions.push_back(action);
    }

    if (actions.empty())
        return nullptr;

    return Action::makeAll(timers, actions);
}

ContextPtr
DocumentCommand::context()
{
    auto root = mRootContext.lock();
    if (!root)
        return nullptr;

    return root->payloadContext();
}

/**
 * We have to store the list of commands to execute in parallel along with the
 * component that the command is associated with and the context.  These are all
 * stored in the CoreCommand, so we can use them.
 */
ActionPtr
DocumentCommand::execute(const TimersPtr& timers, bool fastMode)
{
    return DocumentAction::make(timers, std::static_pointer_cast<DocumentCommand>(shared_from_this()), fastMode);
}
} // namespace apl