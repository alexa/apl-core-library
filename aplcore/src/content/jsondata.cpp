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
#include "apl/content/jsondata.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

namespace apl {

std::string
JsonData::toDebugString() const {
    return "JsonData<" + toString() + ">";
}

std::string
JsonData::toString() const {
    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    mValue.Accept(writer);
    return buffer.GetString();
}

} // namespace apl