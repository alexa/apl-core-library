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

#ifndef _APL_IMPORT_REF_H
#define _APL_IMPORT_REF_H

#include <string>
#include <set>

#include "apl/versioning/semanticversion.h"
#include "apl/versioning/semanticpattern.h"

namespace apl {

/**
 * The description of a particular import package.  Includes the
 * name and the version of the package to load.
 */
class ImportRef {
public:
    ImportRef() {}

    ImportRef(
        const std::string& name,
        const std::string& version)
        : ImportRef(name, version, "", std::set<std::string>(), nullptr, nullptr)
    {}

    ImportRef(
        const std::string& name,
        const std::string& version,
        const std::string& source,
        const std::set<std::string>& loadAfter,
        const SemanticVersionPtr& semanticVersion,
        const SemanticPatternPtr& acceptPattern)
        : mName(name), mVersion(version), mSource(source), mLoadAfter(loadAfter), mSemanticVersion(semanticVersion), mAcceptPattern(acceptPattern)
    {}

    ImportRef(const ImportRef&) = default;
    ImportRef(ImportRef&&) = default;
    ImportRef& operator=(const ImportRef&) = default;
    ImportRef& operator=(ImportRef&&) = default;

    const std::string& name() const { return mName; }
    const std::string& version() const { return mVersion; }
    const std::string& source() const { return mSource; }
    const std::set<std::string>& loadAfter() const { return mLoadAfter; }
    const SemanticVersionPtr& semanticVersion() const { return mSemanticVersion; }
    const SemanticPatternPtr& acceptPattern() const { return mAcceptPattern; }
    std::string toString() const { return mName + ":" + mVersion; }

    /**
     * Determines if this import is an acceptable replacement for the other import.
     *
     * @param other the import reference that may accept this one as a replacement
     * @return true if this import reference is acceptable as a replacement for the other import.
     */
    bool isAcceptableReplacementFor(const ImportRef& other) const {
        // If names differ then other is not acceptable.
        if (mName != other.mName) return false;

        // If this has no semantic version or the other does not have an accept pattern, then versions
        // must match exactly.
        if (!mSemanticVersion || !other.mAcceptPattern) return mVersion == other.mVersion;

        return other.mAcceptPattern->match(mSemanticVersion);
    }

    bool operator==(const ImportRef& other) const { return this->compare(other) == 0; }
    bool operator!=(const ImportRef& other) const { return this->compare(other) != 0; }
    bool operator<(const ImportRef& other) const { return this->compare(other) < 0; }

    int compare(const ImportRef& other) const {
        int result = mName.compare(other.mName);
        return (result == 0 ? mVersion.compare(other.mVersion) : result);
    }

private:
    std::string mName;
    std::string mVersion;
    std::string mSource;
    std::set<std::string> mLoadAfter;
    SemanticVersionPtr mSemanticVersion;
    SemanticPatternPtr mAcceptPattern;
};

} // namespace apl

#endif //_APL_IMPORT_REF_H
