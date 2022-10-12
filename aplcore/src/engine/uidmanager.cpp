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

#include "apl/engine/uidmanager.h"

#include <cassert>

namespace apl {

std::string
UIDManager::create(UIDObject*element)
{
    assert(element != nullptr);
    static int sIdGenerator = 1000;
    auto id = ':'+std::to_string(sIdGenerator++);
    mMap.emplace(id, element);
    return id;
}

void
UIDManager::remove(const std::string& id, UIDObject* element)
{
    assert(!id.empty());
    assert(element != nullptr);

    auto it = mMap.find(id);
    assert(it != mMap.end());
    assert(it->second == element);

    mMap.erase(it);
}

UIDObject*
UIDManager::find(const std::string& id) {
    auto it = mMap.find(id);
    if (it != mMap.end())
        return it->second;
    return nullptr;
}


}