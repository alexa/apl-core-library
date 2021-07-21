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

#ifndef _APL_SYMBOL_REFERENCE_MAP_H
#define _APL_SYMBOL_REFERENCE_MAP_H

#include <map>

#include "apl/common.h"
#include "apl/utils/log.h"

namespace apl {

using SymbolReference = std::pair<std::string, ContextPtr>;

/**
 * Collect symbol references.  These are JSON paths of bound variables pointing
 * to the context where they are defined.  We store this information in a custom
 * map so that we can simplify the references as they are added.
 *
 * The path data for symbols are stored as strings with "/" characters separating
 * the path elements and terminating the path.  For example, the paths extracted
 * from the equation ${a.friends[2] + Math.min(b.height, b.weight)}" would be:
 * "a/friends/2", "b/height/", and "b/weight/".
 */
class SymbolReferenceMap {
public:
    void emplace(const std::string& key, const ContextPtr& value);
    void emplace(SymbolReference& ref);
    void emplace(SymbolReference&& ref);

    bool empty() const { return mMap.empty(); }

    bool operator==(const SymbolReferenceMap& other) const {
        return mMap == other.mMap;
    }

    std::string toDebugString() const;
    const std::map<std::string, ContextPtr>& get() const { return mMap; }

private:
    std::map<std::string, ContextPtr> mMap;
};

} // namespace apl

#endif // _APL_SYMBOL_REFERENCE_MAP_H
