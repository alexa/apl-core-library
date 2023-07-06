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

#include "apl/primitives/objecttype.h"

namespace apl {

/**
 * A reference to a symbol in a specific context.  Bound symbols are used in equations
 * to retrieve the current value of a symbol.  They hold a weak pointer to the bound
 * context to avoid referential loops.  Bounds symbols are normally only used for mutable
 * values (immutable values should be directly referenced).
 */
class BoundSymbol
{
public:
    BoundSymbol(const ContextPtr& context, std::string name)
        : mContext(context), mName(std::move(name))
    {}

    ContextPtr getContext() const { return mContext.lock(); }
    std::string getName() const { return mName; }

    // Standard methods for a EvaluableReferenceHolderObjectType
    bool truthy() const;
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;
    std::string toDebugString() const;
    bool operator==(const BoundSymbol& rhs) const;
    bool operator<(const BoundSymbol& rhs) const;
    bool empty() const;
    Object eval() const;

    friend streamer& operator<<(streamer&, const BoundSymbol&);

    class ObjectType final : public EvaluableReferenceObjectType<BoundSymbol> {};

private:
    std::weak_ptr<Context> mContext;
    std::string mName;
};

} // namespace apl

#endif // _APL_BOUND_SYMBOL_H
