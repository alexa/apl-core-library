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

#include "apl/engine/styleinstance.h"
#include "apl/utils/path.h"

namespace apl {

StyleInstance::StyleInstance(const Path& styleProvenance)
    : mStyleProvenance(styleProvenance.toString())
{
}

void
StyleInstance::put(const std::string& key, const Object& value, const std::string& provenance)
{
    mValue[key] = value;
    if (!provenance.empty())
        mProvenance[key] = provenance;
}

Object
StyleInstance::at(const std::string& key) const
{
    auto it = mValue.find(key);
    if (it != mValue.end())
        return it->second;

    return Object::NULL_OBJECT();
}

std::string
StyleInstance::provenance(const std::string& key) const
{
    auto it = mProvenance.find(key);
    if (it != mProvenance.end())
        return it->second;

    return "";
}


} // namespace apl

