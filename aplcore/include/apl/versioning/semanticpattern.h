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

#ifndef _APL_SEMANTIC_PATTERN_H
#define _APL_SEMANTIC_PATTERN_H

#include <string>
#include <vector>
#include <memory>

#include "apl/common.h"

namespace apl {

/**
 * A Semantic Pattern specifies a valid range or set of ranges of semantic version strings.
 * This is a subset of the traditional semantic patterns that only supports comparison operators
 * and Boolean OR statements.
 *
 * Valid examples include:
 *
 *   1.3.2
 *   =1.3.2
 *   >1.3.0
 *   >=1.3.0 <2.0.0
 *   >1.3.1-alpha || >1.3.2-beta <2.0
 */
class SemanticPattern {
public:
    enum OpType {
        kSemanticOpEquals,
        kSemanticOpGreaterThan,
        kSemanticOpGreaterThanOrEquals,
        kSemanticOpLessThan,
        kSemanticOpLessThanOrEquals,
        kSemanticOpOr,
    };

    /**
     * Static constructor for semantic patterns
     * @param session A session object for error reporting.
     * @param string The string version of the semantic pattern.
     * @return The semantic pattern or null if it fails to parse.
     */
    static SemanticPatternPtr create(const SessionPtr& session, const std::string& string);

    /**
     * Return true if this semantic version matches the semantic pattern.
     * @param version A semantic version of a package.
     * @return True if the version falls within the valid range.
     */
    bool match(const SemanticVersionPtr& version) const;

    /**
     * @return A debugging string showing the internal structure of the pattern.
     */
    std::string toDebugString() const;

    /**
     * Constructor. Generally you should use the SemanticPattern::create() method instead.
     * @param versions An array of the semantic versions in the pattern.
     * @param ops An array of operations
     */
    SemanticPattern(std::vector<SemanticVersionPtr> versions, std::vector<OpType> ops)
        : mVersions(std::move(versions)),
          mOperators(std::move(ops))
    {}

private:
    std::vector<SemanticVersionPtr> mVersions;
    std::vector<OpType> mOperators;
};

} // namespace apl

#endif // _APL_SEMANTIC_PATTERN_H