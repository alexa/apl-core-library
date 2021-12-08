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

#include "apl/primitives/urlrequest.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/arrayify.h"
#include "apl/engine/context.h"
#include "apl/engine/evaluate.h"
#include "apl/utils/session.h"
#include "apl/utils/stringfunctions.h"

namespace apl{

using FilterRule = std::pair<std::regex, bool>;
using FilterRuleArray = const std::vector<FilterRule>;

static bool
passFilter( const FilterRuleArray& filterRules, const std::string& key )
{
    for (const auto& rule : filterRules) {
        if (std::regex_match(key, rule.first))
            return rule.second;
    }
    return true;   // If no rules match, we pass the key
}

static HeaderArray
processHeaders( const std::vector<Object>& sourceHeaders, const FilterRuleArray& filterRules )
{
    HeaderArray result;
    for (const auto& individualHeader : sourceHeaders) {
        if (!individualHeader.isString())
            continue;
        auto header = individualHeader.asString();
        if (header.empty())
            continue;
        auto colon = header.find(':');
        if (colon == std::string::npos)
            continue;
        auto key = apl::trim(header.substr(0, colon));
        if ( !key.empty() && passFilter(filterRules, key) ) {
            auto value = apl::trim(header.substr(colon + 1));
            // Push the trimmed values instead of the original header
            result.emplace_back(key + ": " + value);
        }
    }
    return result;
}

URLRequest::URLRequest(const std::string url, const HeaderArray headers) :
    mUrl(std::move(url)),
    mHeaders(std::move(headers)) {
}

Object
URLRequest::create(const Context& context, const Object& object) {
    if (object.isURLRequest())
        return object;

    if (object.isString())
        return object.asURLRequest();

    if (!object.isMap())
        return Object::NULL_OBJECT();

    auto url = propertyAsString(context, object, "url");
    if(url.empty()) {
        CONSOLE_CTX(context) << "Source has no URL defined.";
        return Object::NULL_OBJECT();
    }
    return URLRequest{ url,
                       processHeaders( arrayifyProperty(context, object, "headers", "header"),
                                       context.getRootConfig().getHttpHeadersFilterRules() )};
}

bool
URLRequest::operator==(const URLRequest& other) const {
    return mUrl == other.mUrl &&
           mHeaders == other.mHeaders;
}

// LCOV_EXCL_START
// GCOV_EXCL_START
std::string
URLRequest::toDebugString() const {
    std::string result = "Source<url=" + getUrl() +
                         " headers=[";
    for (auto i = 0; i < mHeaders.size(); ++i) {
        if (i > 0)
            result += ",";
        result += ("value<val=" + mHeaders[i] + ">");
    }
    result += "]";
    result += ">";
    return result;
}
// LCOV_EXCL_STOP
// GCOV_EXCL_STOP

rapidjson::Value
URLRequest::serialize(rapidjson::Document::AllocatorType& allocator) const {
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);
    v.AddMember("url", Value(getUrl().c_str(), allocator).Move(), allocator);
    rapidjson::Value vHeaders(rapidjson::kArrayType);
    for(const auto& header : mHeaders)
        vHeaders.PushBack(Value(header.c_str(), allocator).Move(), allocator);
    v.AddMember("headers", vHeaders, allocator);
    return v;
}

}