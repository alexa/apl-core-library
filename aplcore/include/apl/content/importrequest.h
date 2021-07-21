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

#ifndef _APL_IMPORT_REQUEST_H
#define _APL_IMPORT_REQUEST_H

#include <string>

#include "importref.h"
#include "rapidjson/document.h"

namespace apl {

class Object;

/**
 * An outstanding request to load a particular version of a package.  Contains the reference (name and version),
 * as well as an optional source URL.
 */
class ImportRequest {
public:
    ImportRequest(const rapidjson::Value& value);

    ImportRequest(const ImportRequest&) = default;
    ImportRequest(ImportRequest&&) = default;
    ImportRequest& operator=(const ImportRequest&) = default;
    ImportRequest& operator=(ImportRequest&&) = default;

    bool isValid() const { return mValid; }
    const ImportRef& reference() const { return mReference; }

    bool operator==(const ImportRequest& other) const { return this->compare(other) == 0; }
    bool operator!=(const ImportRequest& other) const { return this->compare(other) != 0; }
    bool operator<(const ImportRequest& other) const { return this->compare(other) < 0; }

    int compare(const ImportRequest& other) const {
        int result = mReference.compare(other.reference());
        return result == 0 ? mSource.compare(other.mSource) : result;
    }
    uint32_t getUniqueId() const { return mUniqueId; }
    const std::string& source() const { return mSource; }
private:
    ImportRef mReference;
    std::string mSource;
    bool mValid;
    uint32_t mUniqueId;
    static uint32_t sNextId;
};

} // namespace apl
#endif //_APL_IMPORT_REQUEST_H
