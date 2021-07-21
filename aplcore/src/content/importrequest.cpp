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

#include "apl/content/importrequest.h"

namespace apl {

static const char *IMPORT_NAME = "name";
static const char *IMPORT_VERSION = "version";
static const char *IMPORT_SOURCE = "source";


ImportRequest::ImportRequest(const rapidjson::Value& value)
    : mValid(false), mUniqueId(ImportRequest::sNextId++)
{
    if (value.IsObject()) {
        auto it_name = value.FindMember(IMPORT_NAME);
        auto it_version = value.FindMember(IMPORT_VERSION);

        if (it_name != value.MemberEnd() && it_version != value.MemberEnd()) {
            mReference = ImportRef(it_name->value.GetString(), it_version->value.GetString());
            mValid = true;
        }

        auto it_source = value.FindMember(IMPORT_SOURCE);
        if (it_source != value.MemberEnd())
            mSource = it_source->value.GetString();
    }
}

uint32_t ImportRequest::sNextId = 0;

} // namespace apl