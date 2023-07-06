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
UIDManager::create(UIDObject* element)
{
    if (mTerminated) {
        LOG(LogLevel::kError).session(mSession) << "Trying to create new object on terminated state.";
        return "";
    }

    assert(element != nullptr);
    auto id = mGenerator.get();
    mMap.emplace(id, element);
    return id;
}

void
UIDManager::remove(const std::string& id, UIDObject* element)
{
    if (mTerminated) {
        LOG(LogLevel::kError).session(mSession) << "Trying to remove an object on terminated state.";
        // Handled by map clear, nothing to do.
        return;
    }

    assert(!id.empty());
    assert(element != nullptr);

    auto it = mMap.find(id);
    if (it == mMap.end()) {
        LOG(LogLevel::kError).session(mSession)
            << "Should not happen. Check for double destruction of UIDObject based class. ID: " << id;

        assert(false);
        return;
    }

    if(it->second != element) {
        LOG(LogLevel::kError).session(mSession)
            << "Should not happen. UIDObject ID should not be reused. ID: " << id;

        assert(false);
        return;
    }

    mMap.erase(it);
}

UIDObject*
UIDManager::find(const std::string& id) {
    if (mTerminated) {
        LOG(LogLevel::kError).session(mSession) << "Trying to find an object on terminated state.";
        return nullptr;
    }

    auto it = mMap.find(id);
    if (it != mMap.end())
        return it->second;
    return nullptr;
}

}