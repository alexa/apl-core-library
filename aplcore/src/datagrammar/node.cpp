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

namespace apl {
namespace datagrammar {

std::string
Node::toDebugString() const {
    return "Node<" + mName + ">";
}

streamer& operator<<(streamer& os, const Node& node) {
    os << node.toDebugString();
    return os;
}

std::string
Node::getSuffix() const
{
    if (mOp != ApplyArrayAccess && mOp != ApplyFieldAccess)
        return {};

    assert(mArgs.size() == 2);
    assert(mArgs.at(0).isEvaluable());

    // If the RHS attribute/array access value is not pure, then each time we read it we may
    // get a new value.  We can't extend the LHS path with this information, so stash the LHS
    // path into the driving symbols.
    if (!mArgs.at(1).isPure())
        return {};

    // A pure RHS attribute/array access extends the LHS path
    return mArgs.at(1).eval().asString();
}

}  // namespace datagrammar
}  // namespace apl
