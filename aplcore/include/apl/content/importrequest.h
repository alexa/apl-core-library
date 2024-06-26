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

#include "apl/versioning/semanticpattern.h"
#include "apl/versioning/semanticversion.h"

namespace apl {

class Object;

/**
 * An outstanding request to load a particular version of a package.  Contains the reference (name and version),
 * as well as an optional source URL.
 */
class ImportRequest {
public:
    ImportRequest();

    /**
     * Creates an ImportRequest reusing existing requests that satisfy the accept criteria.
     * @param value JSON with package import specification.
     * @param context data binding context.
     * @param commonName name to be used if none specified in *value*.
     * @param commonVersion version to be used if none specified in *value*.
     * @param commonLoadAfter loadAfter to be used if none specified in *value*.
     * @param commonAccept accept to be used if none specified in *value*.
     * @param session session for reporting errors parsing version and accept
     * @return ImportRequest. May be invalid, use #isValid() to check.
     */
    static ImportRequest create(const rapidjson::Value& value,
                                const ContextPtr& context,
                                const SessionPtr& session,
                                const std::string& commonName = "",
                                const std::string& commonVersion = "",
                                const std::set<std::string>& commonLoadAfter = {},
                                const std::string& commonAccept = "");

    ImportRequest(const std::string& name,
                  const std::string& version,
                  const std::string& source,
                  const std::set<std::string>& loadAfter,
                  const SemanticVersionPtr& semanticVersion,
                  const SemanticPatternPtr& acceptPattern);

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

    /**
     * Determines if this import is an acceptable replacement for the other import.
     *
     * Runtimes will want to keep a registry of cached imports by name to determine if a given cached
     * import satisfies a new import request to prevent a network call. See the APL specification
     * regarding import requests and accept.
     *
     * @param other the import request that may accept this one as a replacement.
     * @return true if this import reference is acceptable as a replacement for the other import.
     */
    bool isAcceptableReplacementFor(const ImportRequest& other) const { return mReference.isAcceptableReplacementFor(other.reference()); }

    static std::pair<std::string, std::string> extractNameAndVersion(const rapidjson::Value& value, const ContextPtr& context);
    static std::set<std::string> extractLoadAfter(const rapidjson::Value& value, const ContextPtr& context);
    static std::string extractAccept(const rapidjson::Value& value, const ContextPtr& context);

private:
    static std::string extractString(const std::string& key, const rapidjson::Value& value, const ContextPtr& context);

private:
    ImportRef mReference;
    bool mValid;
    uint32_t mUniqueId;
    static uint32_t sNextId;
};

} // namespace apl
#endif //_APL_IMPORT_REQUEST_H
