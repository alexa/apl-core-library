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
#include "apl/versioning/semanticpattern.h"
#include "apl/utils/session.h"

namespace apl {

SemanticPatternPtr
SemanticPattern::create(const SessionPtr& session, const std::string& string)
{
    if (string.size() == 0) {
        CONSOLE(session) << "Empty pattern string";
        return nullptr;
    }

    svgrammar::semvar_pattern_state state;
    pegtl::memory_input<> in(string, "");

    if (!pegtl::parse<svgrammar::pattern, svgrammar::sp_action, svgrammar::sv_control>(in, state) || state.failed) {
        CONSOLE(session) << "Error parsing semantic pattern '" << string << "', " << state.what();
        return nullptr;
    }

    return std::make_shared<SemanticPattern>(state.versions, state.operators);
}

/**
 * In a comparison operation for pattern matching, the presence or absence of a prerelease is important.
 * The examples in this table have already passed the match() criteria where the semantic version is
 * in the correct range for the pattern.  However semantic versioning has the additional criteria that
 * if the version to be checked has prerelease elements, then the MAJOR.MINOR.PATCH numbers must match as well.
 *
 * This table summaries the conditions:
 *
 * Pattern     Version to check
 * =======     ================
 * >1.2.0      1.3.0             -> TRUE    The checked version has no prerelease elements.
 * >1.2-alpha  1.3.0             -> TRUE    The checked version has no prerelease elements.
 * >1.2.0      1.2.1-alpha       -> FALSE   The pattern has no prerelease, and the version does.
 * >1.2-alpha  1.2.1-alpha       -> FALSE   Both have prerelease elements, but MAJOR.MINOR.PATCH doesn't match
 * >1.2-alpha  1.2.0-alpha.2     -> TRUE    Both have prerelease elements, and MAJOR.MINOR.PATCH matches (1.2.0)
 */

static bool
checkForValidPrerelease(const SemanticVersion& pattern, const SemanticVersion& version)
{
    if (version.simple())
        return true;

    if (pattern.simple())
        return false;

    return pattern.versionMatch(version);
}


bool
SemanticPattern::match(const SemanticVersionPtr& version) const
{
    if (version == nullptr)
        return false;

    bool matched = true;  // Assume matched to start
    auto iter = mVersions.begin();

    for (const auto& op : mOperators) {
        switch (op) {
            case kSemanticOpEquals: {
                const auto operand = *iter++;
                matched = matched && version->compare(*operand) == 0;
            }
                break;
            case kSemanticOpGreaterThan: {
                const auto operand = *iter++;
                matched = matched && version->compare(*operand) > 0 && checkForValidPrerelease(*operand, *version);
            }
                break;
            case kSemanticOpGreaterThanOrEquals: {
                const auto operand = *iter++;
                matched = matched && version->compare(*operand) >= 0 && checkForValidPrerelease(*operand, *version);
            }
                break;
            case kSemanticOpLessThan: {
                const auto operand = *iter++;
                matched = matched && version->compare(*operand) < 0 && checkForValidPrerelease(*operand, *version);
            }
                break;
            case kSemanticOpLessThanOrEquals: {
                const auto operand = *iter++;
                matched = matched && version->compare(*operand) <= 0 && checkForValidPrerelease(*operand, *version);
            }
                break;
            case kSemanticOpOr:
                if (matched)
                    return true;
                matched = true;  // Start over again assuming a true match
                break;
        }
    }

    return matched;
}

std::string
SemanticPattern::toDebugString() const
{
    static std::vector<std::string> OPS_STRINGS = { "=", ">", ">=", "<", "<=", "||" };
    std::string result;
    auto iterOperands = mVersions.begin();

    for (const auto& op : mOperators) {
        if (!result.empty())
            result += ' ';
        result += OPS_STRINGS[op];
        if (op != kSemanticOpOr) {
            result += (*iterOperands)->toDebugString();
            iterOperands++;
        }
    }

    return result;
}


} // namespace apl