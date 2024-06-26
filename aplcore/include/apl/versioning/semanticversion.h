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

#ifndef _APL_SEMANTIC_VERSION_H
#define _APL_SEMANTIC_VERSION_H

#include <memory>
#include <string>
#include <vector>

#include "apl/common.h"

namespace apl {

/**
 * A SemanticVersion object parses a <a href="https://semver.org/">Semantic Version</a>
 * string and stores it in a compact format suitable for comparison operations with other
 * semantic versions.
 *
 * The major, minor, and patch of a semantic version are numbers.  The optional prerelease
 * section ("-") contains an array of dot-separated numbers and/or strings.  The optional build
 * section ("+") is ignored for version comparisons.
 *
 * Internally we store each of the major, minor, patch, and prerelease elements in a single
 * array of uint32_t.  Numeric values are stored as simple unsigned integers.  String values
 * are encoded as a uint8_t offset from the start of the string and a uint8_t length of the
 * element. The high-order bit is used as a string flag.  The implications of this design
 * are (a) the original string is limited to 255 characters and (b) the largest supported numeric
 * value is 2^31.
 *
 * For convenience, if the minor and patch values are omitted they are assumed to be 0.
 * The toDebugString() method returns internal calculated array.
 *
 * Examples of valid strings:  1.0.0
 *                             2.13                 (resolves to 2.13.0)
 *                             10-alpha.2+build2234 (resolves to 10.0.0."alpha".2)
 */
class SemanticVersion {
public:
    /**
     * Call this to create a new semantic version object from a string.  If the semantic
     * version is invalid, a nullptr will be returned and an error message logged on the
     * session.
     *
     * @param session Console session for error logs.
     * @param string The string to parse as a semantic version.
     * @return The parsed object or nullptr.
     */
    static SemanticVersionPtr create(const SessionPtr& session, const std::string& string);

    /**
     * Internal constructor.  Do not use.
     */
    SemanticVersion(std::vector<uint32_t> elements, std::string string)
            : mElements(std::move(elements)), mString(std::move(string)) {}

    /**
     * @return True if this semantic version has no prerelease elements.
     */
    bool simple() const { return mElements.size() == 3; }

    /**
     * @return True if these semantic versions match on MAJOR.MINOR.PATCH.  The
     *         prerelease and build elements are ignored.
     */
    bool versionMatch(const SemanticVersion& other) const;

    /**
     * @return A debugging string showing the inner parts of the semantic version
     *         including major, minor, patch, and prerelease elements.  The build
     *         elements are not included.
     */
    std::string toDebugString() const;

    /**
     * Compare with another SemanticVersion. Return <0 if this version is ordered before,
     * 0 if they are equal (excluding build) and >0 if this version is ordered after.  Note
     * that versions with prerelease elements are ordered before the actual release.  For
     * example:   1.0.0-alpha < 1.0.0-alpha.2 < 1.0.0 < 1.0.1-beta.2 < 1.0.1
     *
     * @param other The version to compare with.
     * @return <0, 0, or >0.
     */
    int compare(const SemanticVersion& other) const;

    bool operator==(const SemanticVersion& other) const { return compare(other) == 0; }
    bool operator!=(const SemanticVersion& other) const { return compare(other) != 0; }
    bool operator<(const SemanticVersion& other) const { return compare(other) < 0; }
    bool operator<=(const SemanticVersion& other) const { return compare(other) <= 0; }
    bool operator>(const SemanticVersion& other) const { return compare(other) > 0; }
    bool operator>=(const SemanticVersion& other) const { return compare(other) >= 0; }

private:
    std::vector<uint32_t> mElements;
    std::string mString;
};

} // namespace apl

#endif // _APL_SEMANTIC_VERSION_H
