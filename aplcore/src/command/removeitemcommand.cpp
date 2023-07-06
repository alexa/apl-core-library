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

#include "apl/command/removeitemcommand.h"
#include "apl/utils/session.h"

namespace apl {

const CommandPropDefSet&
RemoveItemCommand::propDefSet() const {
    static CommandPropDefSet sRemoveItemCommandProperties(CoreCommand::propDefSet(), {
        { kCommandPropertyComponentId, "", asString, kPropRequiredId },
    });

    return sRemoveItemCommandProperties;
}

ActionPtr
RemoveItemCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    if (auto comp = target()) {
        if (comp->remove()) {
            comp->release();
        } else {
            CONSOLE(mContext) << "Component '" << target()->getId() << "' cannot be removed";
        }
    }

    return nullptr;
}

} // namespace apl
