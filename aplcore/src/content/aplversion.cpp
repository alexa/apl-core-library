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

#include <string>

#include "apl/content/aplversion.h"
#include "apl/utils/bimap.h"

namespace apl {

static Bimap<APLVersion::Value, std::string> sVersionMap = {
        { APLVersion::kAPLVersion10, "1.0" },
        { APLVersion::kAPLVersion11, "1.1" },
        { APLVersion::kAPLVersion12, "1.2" },
        { APLVersion::kAPLVersion13, "1.3" },
        { APLVersion::kAPLVersion14, "1.4" },
        { APLVersion::kAPLVersion15, "1.5" },
        { APLVersion::kAPLVersion16, "1.6" },
        { APLVersion::kAPLVersion17, "1.7" }
};

bool
APLVersion::isValid(Value other) const
{
    if(kAPLVersionIgnore == mValue)
        return true;
    return (static_cast<uint32_t>(mValue) & static_cast<uint32_t>(other)) != 0;
}

bool
APLVersion::isValid(const std::string& other) const
{
    if(kAPLVersionIgnore == mValue)
        return true;
    Value version = sVersionMap.get(other, kAPLVersionIgnore);
    return !(kAPLVersionIgnore == version) && isValid(version);
}

std::string
APLVersion::getDefaultReportedVersionString()
{
    return sVersionMap.at(kAPLVersionReported);
}

} // namespace apl
