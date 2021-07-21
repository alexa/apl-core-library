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

#ifndef _APL_BOUND_SYMBOL_H
#define _APL_BOUND_SYMBOL_H

#include "apl/primitives/objectdata.h"
#include "apl/primitives/symbolreferencemap.h"

namespace apl {
namespace datagrammar {

/**
 * A reference to a symbol in a specific context.  Bound symbols are used in equations
 * to retrieve the current value of a symbol.  They hold a weak pointer to the bound
 * context to avoid referential loops.  Bounds symboles are normallly only used for mutable
 * values (immutable values should be directly referenced).
 */
class BoundSymbol : public ObjectData
{
public:
    BoundSymbol(const ContextPtr& context, std::string name)
        : mContext(context), mName(std::move(name))
    {}

    /**
     * @return The newly evaluated value of the symbol
     */
    Object eval() const override;

    SymbolReference getSymbol() const { return SymbolReference(mName + "/", mContext.lock()); }

    std::string toDebugString() const override;

    bool operator==(const BoundSymbol& rhs) const;

    friend streamer& operator<<(streamer&, const BoundSymbol&);

private:
    std::weak_ptr<Context> mContext;
    std::string mName;
};

} // namespace datagrammar
} // namespace apl

#endif // _APL_BOUND_SYMBOL_H
