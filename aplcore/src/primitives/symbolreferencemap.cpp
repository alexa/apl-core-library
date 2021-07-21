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

#include "apl/primitives/symbolreferencemap.h"

namespace apl {

/**
 * Check the existing map.  There are two considerations we need to check.  First,
 * there may be keys in the map that are supersets of this key.  For example, if
 * the key is "alpha/0/" and the map contains "alpha/0/first", then the existing
 * key in the map should be removed.  Second, if there is a key in the map that
 * is a subset of this key, then the key should not be added.  For example, if
 * the key is "alpha/0/" and the map contains "alpha/", then we should not add
 * the new key.
 * @param name
 * @param map
 * @return
 */
static bool
checkExisting(const std::string& key, std::map<std::string, ContextPtr>& map)
{
    auto it = map.lower_bound(key);

    // First, check for a subset of this key.  It should be directly before the lower bound
    if (it != map.begin()) {
        auto beforeKey = std::prev(it, 1)->first;
        if (key.compare(0, beforeKey.size(), beforeKey) == 0)
            return false;
    }

    // Now look for keys that are a superset of this key
    while (it != map.end() && it->first.compare(0, key.size(), key) == 0)
        it = map.erase(it);

    return true;
}

void
SymbolReferenceMap::emplace(const std::string& key, const ContextPtr& value) {
    if (checkExisting(key, mMap))
        mMap.emplace(key, value);
}

void
SymbolReferenceMap::emplace(SymbolReference& ref) {
    if (checkExisting(ref.first, mMap))
        mMap.emplace(ref);
}

void
SymbolReferenceMap::emplace(SymbolReference&& ref) {
    if (checkExisting(ref.first, mMap))
        mMap.emplace(std::move(ref));
}


std::string
SymbolReferenceMap::toDebugString() const
{
    std::string result;
    for (const auto& m : mMap) {
        if (!result.empty())
            result += ", ";
        result += m.first;
    }
    return result;
}




} // namespace apl