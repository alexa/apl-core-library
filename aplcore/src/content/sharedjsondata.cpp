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
#include "apl/content/sharedjsondata.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "apl/primitives/object.h"

namespace apl {

std::string
SharedJsonData::toDebugString() const
{
    return "SharedJsonData<" + toString() + ">";
}

std::string
SharedJsonData::toString() const
{
    if (!*this)
        return "INVALID";

    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    get().Accept(writer);
    return buffer.GetString();
}

} // namespace apl