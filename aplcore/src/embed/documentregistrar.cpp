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

#include "apl/embed/documentregistrar.h"

#include <cassert>

#include "apl/utils/log.h"

namespace apl {

const std::map<int, CoreDocumentContextPtr>&
DocumentRegistrar::list() const
{
    return mDocumentMap;
}

CoreDocumentContextPtr
DocumentRegistrar::get(int id) const
{
    auto it = mDocumentMap.find(id);
    return (it != mDocumentMap.end())
        ? it->second
        : nullptr;
}

int
DocumentRegistrar::registerDocument(const CoreDocumentContextPtr& document)
{
    auto id = mIdGenerator++;
    // We never expect it to overflow
    assert(id > 0);
    mDocumentMap.emplace(id, document);
    return id;
}

void
DocumentRegistrar::deregisterDocument(int id)
{
    if (!mDocumentMap.erase(id)) LOG(LogLevel::kWarn) << "Can't de-register document " << id;
}

} // namespace apl
