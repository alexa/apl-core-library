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

#ifndef _APL_PACKAGE_H
#define _APL_PACKAGE_H

#include <memory>
#include <string>
#include <vector>
#include "rapidjson/document.h"

#include "apl/common.h"
#include "apl/utils/counter.h"
#include "apl/content/importref.h"
#include "apl/content/jsondata.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/streamer.h"

namespace apl {

/**
 * Store the JSON information and dependency graph for a single downloaded package.
 */
class Package : public Counter<Package>,
                public NonCopyable {
public:
    /**
     * Build a named package with JSON data.
     * @param session A logging session
     * @param name The working name of the package
     * @param json The JSON package definition.
     * @return The package or nullptr if the package is not well-formed.
     */
     static PackagePtr create(const SessionPtr& session, const std::string& name, JsonData&& json);

    /**
     * @return The JSON package definition.
     */
    const rapidjson::Value& json() const { return mJson.get(); }

    /**
     * @return The debugging name of the package
     */
    const std::string& name() const { return mName; }

    /**
     * @return package APL spec version.
     */
    const std::string version();

    /**
     * @return package type field
     */
    const std::string type();

    Package(const std::string& name, JsonData&& json)
            : mName(name),
              mJson(std::move(json))
    {}

    friend streamer& operator<<(streamer& os, Package& package) {
        return os << package.name();
    }

private:
    friend class Content;
    friend class RootContext;

    void addDependency(const ImportRef& ref) { mDependencies.push_back(ref); }
    void addDependency(ImportRef&& ref) { mDependencies.push_back(ref); }

    const std::vector<ImportRef>& getDependencies() const { return mDependencies; }

private:
    std::string mName;
    const JsonData mJson;
    std::vector<ImportRef> mDependencies;
};

} // namespace apl

#endif //_APL_PACKAGE_H
