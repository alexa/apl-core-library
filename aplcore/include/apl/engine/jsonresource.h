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

#ifndef _APL_JSON_RESOURCE_H
#define _APL_JSON_RESOURCE_H

#include <string>

#include "rapidjson/document.h"
#include "apl/utils/path.h"

namespace apl {

/**
 * A resource loaded from the main document or from a package that includes path
 * data showing which package the resource was loaded from.  The path syntax is
 *
 *      PACKAGE/ELEMENT/ELEMENT/...
 *
 * The package name is given by the Package class.  It's "_main" for the main document (see Path)
 * and "PKGNAME:VERSION" for a loaded package.  For example, a layout named "Header" loaded
 * from the "Base" package with version 1.4 would have the path "Base:1.4/layouts/Header"
 */
class JsonResource {
public:
    JsonResource() : mJson(nullptr) {}
    JsonResource(const rapidjson::Value* json, const Path& path) : mJson(json), mPath(path) {}

    /**
     * @return True if this element is empty; that is, there is no data
     */
    bool empty() const { return mJson == nullptr; }

    /**
     * @return The JSON data associate with this element
     */
    const rapidjson::Value& json() const { return *mJson; }

    /**
     * @return The load path for where the element was loaded.
     */
    const Path& path() const { return mPath; }

private:
    const rapidjson::Value* mJson;
    Path mPath;
};

} // namespace apl

#endif //_APL_JSON_RESOURCE_H
