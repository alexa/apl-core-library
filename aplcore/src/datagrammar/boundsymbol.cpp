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

#include "apl/datagrammar/boundsymbol.h"
#include "apl/engine/context.h"

namespace apl {
namespace datagrammar {

Object
BoundSymbol::eval() const
{
    auto context = mContext.lock();
    return context ? context->opt(mName) : Object::NULL_OBJECT();
}

std::string
BoundSymbol::toDebugString() const {
    return "BoundSymbol<" + mName + ">";
}

bool
BoundSymbol::operator==(const BoundSymbol& rhs) const
{
    return !mContext.owner_before(rhs.mContext) &&
               !rhs.mContext.owner_before(mContext) &&
               mName == rhs.mName;
}

streamer& operator<<(streamer& os, const BoundSymbol& boundSymbol) {
    os << boundSymbol.toDebugString();
    return os;
}

}  // namespace datagrammar
}  // namespace apl
