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

#include "apl/command/extensioneventcommand.h"

namespace apl {

ActionPtr
ExtensionEventCommand::execute(const TimersPtr& timers, bool fastMode)
{
    if (fastMode && !mDefinition.getAllowFastMode()) {
        CONSOLE_CTP(mContext) << "Ignoring extension " << mDefinition.getName() << " command in fast mode";
        return nullptr;
    }

    // Update the built-in properties
    if (!calculateProperties())
        return nullptr;

    // Calculate the properties and store them in mValues
    auto map = std::make_shared<ObjectMap>();
    for (const auto& m : mDefinition.getPropertyMap()) {
        auto it = mProperties.find(m.first);
        if (it == mProperties.end()) {
            if (m.second.required) {
                CONSOLE_CTP(mContext) << "Missing required property '" << m.first << "' for extension command '"
                                      << mDefinition.getName() << "': dropping command";
                return nullptr;
            }
            map->emplace(m.first, m.second.defvalue);
        } else {
            const auto& bfunc = sBindingFunctions.at(m.second.btype);
            auto raw = m.second.btype == kBindingTypeArray ? arrayify(*mContext, it->second) : it->second;
            map->emplace(m.first, bfunc(*mContext, evaluateRecursive(*mContext, raw)));
        }
    }

    mValues.emplace(kCommandPropertyExtension, map);

    auto requireResolution = mDefinition.getRequireResolution() && !fastMode;

    return ExtensionEventAction::make(timers,
                                      std::static_pointer_cast<ExtensionEventCommand>(shared_from_this()),
                                      requireResolution);
}

}  // namespace apl
