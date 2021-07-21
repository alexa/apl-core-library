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

#include "apl/content/package.h"
#include "apl/utils/session.h"

namespace apl {

const char *DOCUMENT_TYPE = "type";
const char *DOCUMENT_VERSION = "version";

PackagePtr
Package::create(const SessionPtr& session, const std::string& name, JsonData&& json)
{
    if (!json) {
        CONSOLE_S(session).log("Package %s parse error offset=%u: %s", name.c_str(),
                   json.offset(), json.error());
        return nullptr;
    }

    const auto& value = json.get();
    if (!value.IsObject()) {
        CONSOLE_S(session).log("Package %s: not a valid JSON object", name.c_str());
        return nullptr;
    }

    auto it_type = value.FindMember(DOCUMENT_TYPE);
    if (it_type == value.MemberEnd()) {
        CONSOLE_S(session).log("Package %s does not contain a type field", name.c_str());
        return nullptr;
    }

    auto it_version = value.FindMember(DOCUMENT_VERSION);
    if (it_version == value.MemberEnd() || !it_version->value.IsString()) {
        CONSOLE_S(session).log("Package %s does not contain a valid version field", name.c_str());
        return nullptr;
    }

    return std::make_shared<Package>(name, std::move(json));
}

const std::string
Package::version()
{
    auto it_version = mJson.get().FindMember(DOCUMENT_VERSION);
    return std::string(it_version->value.GetString());
}

const std::string
Package::type()
{
    auto it_type = mJson.get().FindMember(DOCUMENT_TYPE);
    return std::string(it_type->value.GetString());
}

} // namespace apl