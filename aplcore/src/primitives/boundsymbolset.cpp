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

#include "apl/primitives/boundsymbolset.h"

namespace apl {

void
BoundSymbolSet::emplace(const BoundSymbol& boundSymbol)
{
    // Use an insertion sort
    // Find the first element greater than or equal to this symbol
    auto it = std::lower_bound(mSymbols.begin(),
                               mSymbols.end(),
                               boundSymbol);

    // If we find a match, don't add anything
    if (it != mSymbols.end() && *it == boundSymbol)
        return;

    mSymbols.insert(it, boundSymbol);
}


} // namespace apl
