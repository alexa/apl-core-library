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

namespace apl {

/**
 * The description of a particular import package.  Includes the
 * name and the version of the package to load.
 */
class ImportRef {
public:
    ImportRef() {}
    ImportRef(const std::string& name, const std::string& version)
        : mName(name), mVersion(version)
    {}

    ImportRef(const ImportRef&) = default;
    ImportRef(ImportRef&&) = default;
    ImportRef& operator=(const ImportRef&) = default;
    ImportRef& operator=(ImportRef&&) = default;

    const std::string& name() const { return mName; }
    const std::string& version() const { return mVersion; }
    std::string toString() const { return mName + ":" + mVersion; }

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
};

} // namespace apl

#endif //_APL_IMPORT_REF_H
