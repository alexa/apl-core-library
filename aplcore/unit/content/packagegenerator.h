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

#ifndef APL_PACKAGEGENERATOR_H
#define APL_PACKAGEGENERATOR_H

#include "rapidjson/rapidjson.h"
#include <map>
#include <string>
#include <vector>

static std::string
makeTestPackage(const std::vector<const char *>& dependencies, std::map<const char *, const char *> stringMap)
{
    const char *FAKE_MAIN_TEMPLATE = R"apl({
     "item": {
       "type": "Text"
     }
    })apl";

    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    doc.AddMember("type", "APL", allocator);
    doc.AddMember("version", "1.1", allocator);

    // Add imports
    rapidjson::Value imports(rapidjson::kArrayType);
    for (const char *it : dependencies) {
        rapidjson::Value importBlock(rapidjson::kObjectType);
        importBlock.AddMember("name", rapidjson::StringRef(it), allocator);
        importBlock.AddMember("version", "1.0", allocator);
        imports.PushBack(importBlock, allocator);
    }
    doc.AddMember("import", imports, allocator);

    // Add resources
    rapidjson::Value resources(rapidjson::kArrayType);
    rapidjson::Value resourceBlock(rapidjson::kObjectType);
    rapidjson::Value resourceStrings(rapidjson::kObjectType);
    for (auto it : stringMap)
        resourceStrings.AddMember(rapidjson::StringRef(it.first), rapidjson::StringRef(it.second), allocator);
    resourceBlock.AddMember("strings", resourceStrings, allocator);
    resources.PushBack(resourceBlock, allocator);
    doc.AddMember("resources", resources, allocator);

    // Add a mainTemplate section (just in case)
    rapidjson::Document mainTemplate;
    mainTemplate.Parse(FAKE_MAIN_TEMPLATE);
    doc.AddMember("mainTemplate", mainTemplate, allocator);

    // Convert to a string
    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return {buffer.GetString()};
}

#endif // APL_PACKAGEGENERATOR_H
