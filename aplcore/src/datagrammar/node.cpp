/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <cassert>
#include <cmath>

#include "apl/datagrammar/functions.h"
#include "apl/datagrammar/node.h"
#include "apl/engine/context.h"
#include "apl/primitives/dimension.h"
#include "apl/utils/log.h"

namespace apl {
namespace datagrammar {

void
Node::symbols(std::set<std::string>& symbols) const
{
    if (mOp == SymbolAccess)
        symbols.emplace(mArgs.at(0).asString());

    for (auto& m : mArgs)
        m.symbols(symbols);
}

std::string
Node::toDebugString() const {
    return "Node<" + (mOp == SymbolAccess ? mArgs.at(0).asString() : mName) + ">";
}

streamer& operator<<(streamer& os, const Node& node) {
    os << node.toDebugString();
    return os;
}

}  // namespace datagrammar
}  // namespace apl
