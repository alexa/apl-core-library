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

#include "apl/content/importrequest.h"

#include "apl/engine/context.h"
#include "apl/engine/evaluate.h"

namespace apl {

static const char *IMPORT_NAME = "name";
static const char *IMPORT_VERSION = "version";
static const char *IMPORT_SOURCE = "source";
static const char *IMPORT_LOAD_AFTER = "loadAfter";

ImportRequest
ImportRequest::create(const rapidjson::Value& value,
                      const ContextPtr& context,
                      const std::string& commonName,
                      const std::string& commonVersion,
                      const std::set<std::string>& commonLoadAfter)
{
    if (value.IsObject()) {
        auto nameAndVersion = extractNameAndVersion(value, context);
        // Prefer specific name and version, use common if not provided
        auto name = nameAndVersion.first;
        name = name.empty() ? commonName : name;
        auto version = nameAndVersion.second;
        version = version.empty() ? commonVersion : version;

        if (name.empty() || version.empty()) return {};

        // Source is always specific, if exists
        std::string source;
        auto it_source = value.FindMember(IMPORT_SOURCE);
        if (it_source != value.MemberEnd()) {
            source = it_source->value.GetString();
            if (!source.empty() && context) source = evaluate(*context, source).asString();
        }

        // Load after can also be common
        auto loadAfter = extractLoadAfter(value, context);
        loadAfter = loadAfter.empty() ? commonLoadAfter : loadAfter;
        if (loadAfter.count(nameAndVersion.first)) return {};

        return {name, version, source, loadAfter};
    }

    return {};
}

ImportRequest::ImportRequest() : mValid(false), mUniqueId(ImportRequest::sNextId++) {}

ImportRequest::ImportRequest(const std::string& name,
                             const std::string& version,
                             const std::string& source,
                             const std::set<std::string>& loadAfter)
    : mReference(name, version, source, loadAfter), mValid(true), mUniqueId(ImportRequest::sNextId++)
{
}

std::pair<std::string, std::string>
ImportRequest::extractNameAndVersion(const rapidjson::Value& value, const ContextPtr& context)
{
    std::string name;
    std::string version;

    auto it_name = value.FindMember(IMPORT_NAME);
    if (it_name != value.MemberEnd()) {
        name = it_name->value.GetString();
        if (context) name = evaluate(*context, name).asString();
    }

    auto it_version = value.FindMember(IMPORT_VERSION);
    if (it_version != value.MemberEnd()) {
        version = it_version->value.GetString();
        if (context) version = evaluate(*context, version).asString();
    }
    return std::make_pair(name, version);
}

std::set<std::string>
ImportRequest::extractLoadAfter(const rapidjson::Value& value, const ContextPtr& context)
{
    std::set<std::string> result;

    auto it_load_after = value.FindMember(IMPORT_LOAD_AFTER);
    if (it_load_after != value.MemberEnd()) {
        auto& tmpValue = it_load_after->value;
        if (tmpValue.IsString()) {
            auto loadString = context ? evaluate(*context, tmpValue.GetString()).asString() : tmpValue.GetString();
            result.emplace(loadString);
        } else if (tmpValue.IsArray()) {
            for (const auto& load : tmpValue.GetArray()) {
                if (load.IsString()) {
                    auto depString = context ? evaluate(*context, load.GetString()).asString() : load.GetString();
                    result.emplace(depString);
                }
            }
        }
    }

    return result;
}

uint32_t ImportRequest::sNextId = 0;

} // namespace apl