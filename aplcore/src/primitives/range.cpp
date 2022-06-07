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

#include "apl/primitives/range.h"

namespace apl {

std::string
Range::toDebugString() const
{
    return std::string("Range<"+std::to_string(mLowerBound)+","+std::to_string(mUpperBound)+">");
}

rapidjson::Value
Range::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    using rapidjson::Value;
    using rapidjson::StringRef;

    Value span(rapidjson::kObjectType);
    span.AddMember("lowerBound", mLowerBound, allocator);
    span.AddMember("upperBound", mUpperBound, allocator);
    return span;
}


} // namespace apl