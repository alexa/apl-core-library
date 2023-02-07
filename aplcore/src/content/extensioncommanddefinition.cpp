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

#include "apl/content/extensioncommanddefinition.h"

#include "apl/utils/log.h"

namespace apl {

ExtensionCommandDefinition&
ExtensionCommandDefinition::property(const std::string& name, ExtensionProperty&& prop)
{
    if (name == "when" || name == "type")
        LOG(LogLevel::kWarn) << "Unable to register property '" << name << "' in custom command " << mName;
    else
        mPropertyMap.emplace(name, std::move(prop));
    return *this;
}

ExtensionCommandDefinition&
ExtensionCommandDefinition::arrayProperty(std::string property, bool required)
{
    if (property == "when" || property == "type")
        LOG(LogLevel::kWarn) << "Unable to register array-ified property '" << property << "' in custom command " << mName;
    else
        mPropertyMap.emplace(property, ExtensionProperty{kBindingTypeArray, Object::EMPTY_ARRAY(), required});
    return *this;
}

} // namespace apl
