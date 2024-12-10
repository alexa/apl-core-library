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
static const char *IMPORT_DOMAIN = "domain";
static const char *IMPORT_LOAD_AFTER = "loadAfter";
static const char *IMPORT_ACCEPT = "accept";

ImportRequest
ImportRequest::create(const rapidjson::Value& value,
                      const ContextPtr& context,
                      const SessionPtr& session,
                      const std::string& commonName,
                      const std::string& commonVersion,
                      const std::string& commonDomain,
                      const std::set<std::string>& commonLoadAfter,
                      const std::string& commonAccept)
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

        // Domain can be common
        std::string domain = extractDomain(value, context);
        domain = domain.empty() ? commonDomain : domain;

        // Load after can also be common
        auto loadAfter = extractLoadAfter(value, context);
        loadAfter = loadAfter.empty() ? commonLoadAfter : loadAfter;
        if (loadAfter.count(nameAndVersion.first)) return {};

        std::string accept;
        auto it = value.FindMember(IMPORT_ACCEPT);
        if (it != value.MemberEnd()) {
            accept = it->value.GetString();
            if (context) accept = evaluate(*context, accept).asString();
        }
        accept = accept.empty() ? commonAccept : accept;
        auto semanticVersion = SemanticVersion::create(session, version);
        auto acceptPattern = accept.empty() ? nullptr : SemanticPattern::create(session, accept);

        return {name, version, source, domain, loadAfter, semanticVersion, acceptPattern};
    }

    return {};
}

ImportRequest::ImportRequest() : mValid(false), mUniqueId(ImportRequest::sNextId++) {}

ImportRequest::ImportRequest(const std::string& name,
                             const std::string& version,
                             const std::string& source,
                             const std::set<std::string>& loadAfter,
                             const SemanticVersionPtr& semanticVersion,
                             const SemanticPatternPtr& acceptPattern)
    : ImportRequest(name, version, source, "", loadAfter, semanticVersion, acceptPattern)
{
}

ImportRequest::ImportRequest(const std::string& name,
                             const std::string& version,
                             const std::string& source,
                             const std::string& domain,
                             const std::set<std::string>& loadAfter,
                             const SemanticVersionPtr& semanticVersion,
                             const SemanticPatternPtr& acceptPattern)
    : mReference(name, version, source, domain, loadAfter, semanticVersion, acceptPattern),
      mValid(true),
      mUniqueId(ImportRequest::sNextId++)
{
}

std::pair<std::string, std::string>
ImportRequest::extractNameAndVersion(const rapidjson::Value& value, const ContextPtr& context)
{
    return std::make_pair(extractString(IMPORT_NAME, value, context), extractString(IMPORT_VERSION, value, context));
}

std::string
ImportRequest::extractDomain(const rapidjson::Value& value, const ContextPtr& context)
{
    return extractString(IMPORT_DOMAIN, value, context);
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

std::string
ImportRequest::extractAccept(const rapidjson::Value& value, const ContextPtr& context)
{
    return extractString(IMPORT_ACCEPT, value, context);
}

std::string
ImportRequest::extractString(const std::string& key, const rapidjson::Value& value, const ContextPtr& context)
{
    std::string result;
    auto it_name = value.FindMember(key.c_str());
    if (it_name != value.MemberEnd()) {
        result = it_name->value.GetString();
        if (context) result = evaluate(*context, result).asString();
    }

    return result;
}

uint32_t ImportRequest::sNextId = 0;

} // namespace apl