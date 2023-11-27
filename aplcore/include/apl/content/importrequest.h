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

#include "apl/common.h"
#include "apl/content/importref.h"
#include "rapidjson/document.h"

namespace apl {

class Object;

/**
 * An outstanding request to load a particular version of a package.  Contains the reference (name and version),
 * as well as an optional source URL.
 */
class ImportRequest {
public:
    /**
     * Create ImportRequest.
     * @param value JSON with package import specification.
     * @param context data binding context.
     * @param commonName name to be used if none specified in *value*.
     * @param commonVersion version to be used if none specified in *value*.
     * @param commonLoadAfter loadAfter to be used if none specified in *value*.
     * @return ImportRequest. May be invalid, use #isValid() to check.
     */
    static ImportRequest create(const rapidjson::Value& value,
                                const ContextPtr& context,
                                const std::string& commonName,
                                const std::string& commonVersion,
                                const std::set<std::string>& commonLoadAfter);

    ImportRequest();
    ImportRequest(const std::string& name,
                  const std::string& version,
                  const std::string& source,
                  const std::set<std::string>& loadAfter);

    ImportRequest(const ImportRequest&) = default;
    ImportRequest(ImportRequest&&) = default;
    ImportRequest& operator=(const ImportRequest&) = default;
    ImportRequest& operator=(ImportRequest&&) = default;

    bool isValid() const { return mValid; }
    const ImportRef& reference() const { return mReference; }

    bool operator==(const ImportRequest& other) const { return this->compare(other) == 0; }
    bool operator!=(const ImportRequest& other) const { return this->compare(other) != 0; }
    bool operator<(const ImportRequest& other) const { return this->compare(other) < 0; }

    int compare(const ImportRequest& other) const { return mReference.compare(other.reference()); }
    uint32_t getUniqueId() const { return mUniqueId; }
    const std::string& source() const { return mReference.source(); }

    static std::pair<std::string, std::string> extractNameAndVersion(const rapidjson::Value& value, const ContextPtr& context);
    static std::set<std::string> extractLoadAfter(const rapidjson::Value& value, const ContextPtr& context);

private:
    ImportRef mReference;
    bool mValid;
    uint32_t mUniqueId;
    static uint32_t sNextId;
};

} // namespace apl
#endif //_APL_IMPORT_REQUEST_H
