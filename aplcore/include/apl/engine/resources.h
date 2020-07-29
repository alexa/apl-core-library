/*
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

#ifndef _APL_RESOURCES_H
#define _APL_RESOURCES_H

#include "rapidjson/document.h"
#include <map>
#include <functional>
#include <string>

namespace apl {

class Context;
class Path;
class Object;

using ConversionFunc = std::function<const Object(const Context&, const Object&)>;
using ResourceOperators = std::map<std::string, ConversionFunc>;

extern ResourceOperators sDefaultResourceOperators;

void addOrderedResources(Context& context, const rapidjson::Value& array, const Path& path,
        const ResourceOperators& resourceOperators);

void addNamedResourcesBlock(Context &context, const rapidjson::Value &json, const Path &path,
        const std::string &resourceBlockName, const ResourceOperators& resourceOperators = sDefaultResourceOperators);

} // namespace apl

#endif // _APL_RESOURCES_H
