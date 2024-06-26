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

#include <tao/pegtl.hpp>

#include "apl/versioning/semanticgrammar.h"
#include "apl/versioning/semanticversion.h"
#include "apl/utils/session.h"

namespace apl {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

// Convenience function to extract a pointer to the start of an embedded string
static const char *encodedStart(const std::string& string, uint32_t value)
{
    assert(svgrammar::isEncodedString(value));
    return string.c_str() + svgrammar::encodedOffset(value);
}

// Convenience function to retrieve an embedded string
static std::string encodedToString(const std::string& string, uint32_t value)
{
    if (svgrammar::isEncodedString(value))
        return '\'' + string.substr(svgrammar::encodedOffset(value), svgrammar::encodedLen(value)) + '\'';
    else
        return std::to_string(value);
}

SemanticVersionPtr
SemanticVersion::create(const SessionPtr& session, const std::string& string)
{
    if (string.size() <= 0) {
        CONSOLE(session) << "Empty version string";
        return nullptr;
    }

    if (string.size() > std::numeric_limits<uint8_t>::max()) {
        CONSOLE(session) << "Version string too long";
        return nullptr;
    }

    pegtl::memory_input<> in(string, "");
    svgrammar::semvar_state state(in);

    if (!pegtl::parse<svgrammar::semver, svgrammar::sv_action, svgrammar::sv_control>(in, state) ||
        state.failed) {
        CONSOLE(session) << "Error parsing semantic version '" << string << "', " << state.what();
        return nullptr;
    }

    assert(state.elements.size() >= 3);
    return std::make_shared<SemanticVersion>(std::move(state.elements), std::move(state.string));
}

bool
SemanticVersion::versionMatch(const SemanticVersion& other) const
{
    assert(mElements.size() >= 3);
    assert(other.mElements.size() >= 3);
    return mElements.at(0) == other.mElements.at(0) &&
           mElements.at(1) == other.mElements.at(1) &&
           mElements.at(2) == other.mElements.at(2);
}

std::string
SemanticVersion::toDebugString() const
{
    std::string result;
    for (const auto& m : mElements) {
        if (!result.empty())
            result += ".";
        result += encodedToString(mString, m);
    }
    return result;
}

int
SemanticVersion::compare(const SemanticVersion& other) const
{
    const auto selfCount = mElements.size();
    const auto otherCount = other.mElements.size();
    const auto maxCount = std::min(selfCount, otherCount);
    for (auto i = 0; i < maxCount; i++) {
        const auto& a = mElements.at(i);
        const auto& b = other.mElements.at(i);
        if (!svgrammar::isEncodedString(a)) { // This is a number
            if (svgrammar::isEncodedString(b)) return -1;  // Numbers are less than strings
            // Numeric comparison
            if (a < b) return -1;
            if (a > b) return +1;
        } else {
            if (!svgrammar::isEncodedString(b)) return 1;  // Strings are bigger than numbers
            // String comparison
            const char *ptr_a = encodedStart(mString, a);
            const char *ptr_b = encodedStart(other.mString, b);
            const auto len_a = svgrammar::encodedLen(a);
            const auto len_b = svgrammar::encodedLen(b);
            const auto jmax = std::min(len_a, len_b);
            for (auto j = 0; j < jmax; j++) {
                if (*ptr_a < *ptr_b) return -1;
                if (*ptr_a > *ptr_b) return +1;
                ++ptr_a;
                ++ptr_b;
            }
            if (len_a < len_b) return -1;
            if (len_a > len_b) return +1;
        }
    }
    if (selfCount < otherCount) return (selfCount > 3 ? -1 : +1);  // "1.0.0-rc" < "1.0.0-rc.3" < "1.0.0"
    if (selfCount > otherCount) return (otherCount > 3 ? +1 : -1);
    return 0;
}

} // namespace apl