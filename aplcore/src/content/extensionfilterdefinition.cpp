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

#include "apl/content/extensionfilterdefinition.h"

#include "apl/utils/log.h"

namespace apl {

ExtensionFilterDefinition&
ExtensionFilterDefinition::property(const std::string& name, Property&& prop)
{
    if (name == "when" || name == "type" || name == "source" || name == "destination")
        LOG(LogLevel::kWarn) << "Unable to register property '" << name << "' in custom filter extension " << mName;
    else
        mPropertyMap.emplace(name, std::move(prop));
    return *this;
}

} // namespace apl
