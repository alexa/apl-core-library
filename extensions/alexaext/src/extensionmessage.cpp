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

#include "alexaext/extensionmessage.h"

namespace alexaext {

template<>
const char*
GetWithDefault<const char*>(const rapidjson::Pointer& path,
                            const rapidjson::Value& root,
                            const char* defaultValue)
{
    const rapidjson::Value* value = path.Get(root);
    if (!value || value->IsNull() || !value->IsString())
        return defaultValue;
    return value->GetString();
}

template<>
std::string
GetWithDefault<std::string>(const rapidjson::Pointer& path,
                            const rapidjson::Value& root,
                            std::string defaultValue)
{
    const rapidjson::Value* value = path.Get(root);
    if (!value || value->IsNull() || !value->IsString())
        return defaultValue;
    return value->GetString();
}

int GetAsIntWithDefault(const rapidjson::Pointer& path,
                            const rapidjson::Value& root,
                            int defaultValue)
{
    const rapidjson::Value* value = path.Get(root);
    if (!value || value->IsNull()) {
        return defaultValue;
    } else if (value->IsNumber()) {
        return static_cast<int>(value->GetDouble());
    } else if (value->IsString()) {
        return std::atoi(value->GetString());
    }
    return defaultValue;
}

} // namespace alexaext
