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

#ifndef _APL_BOUND_SYMBOL_SET_H
#define _APL_BOUND_SYMBOL_SET_H

#include <string>
#include <vector>

#include "apl/common.h"
#include "apl/primitives/boundsymbol.h"

namespace apl {

/**
 * A sorted set of unique BoundSymbols.  Because the set is expected to be small, we internally
 * store this as a sorted std::vector rather than a std::set.
 */
class BoundSymbolSet {
public:
    BoundSymbolSet() = default;
    BoundSymbolSet(BoundSymbolSet&&) = default;

    BoundSymbolSet& operator=(const BoundSymbolSet& other) {
        if (&other == this)
            return *this;
        mSymbols = other.mSymbols;
        return *this;
    }

    void emplace(const BoundSymbol& boundSymbol);

    bool empty() const { return mSymbols.empty(); }
    size_t size() const { return mSymbols.size(); }
    void clear() { mSymbols.clear(); }

    bool operator==(const BoundSymbolSet& other) const { return mSymbols == other.mSymbols; }
    bool operator!=(const BoundSymbolSet& other) const { return mSymbols != other.mSymbols; }

private:
    std::vector<BoundSymbol> mSymbols;

public:
    auto begin() ->decltype(mSymbols)::iterator { return mSymbols.begin(); }
    auto end() -> decltype(mSymbols)::iterator { return mSymbols.end(); }
    auto begin() const ->decltype(mSymbols)::const_iterator { return mSymbols.begin(); }
    auto end() const -> decltype(mSymbols)::const_iterator { return mSymbols.end(); }
};

} // namespace apl

#endif // _APL_BOUND_SYMBOL_SET_H
