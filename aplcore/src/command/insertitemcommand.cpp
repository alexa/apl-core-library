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

#include "apl/command/insertitemcommand.h"
#include "apl/engine/builder.h"
#include "apl/utils/session.h"

namespace {

int
getClampedIndex(const int childCount, const int requestedIndex) {
    return requestedIndex < 0
       ? std::max(0, childCount + requestedIndex)
       : std::min(requestedIndex, childCount);
}

} // namespace unnamed

namespace apl {

const CommandPropDefSet&
InsertItemCommand::propDefSet() const {
    static CommandPropDefSet sInsertItemCommandProperties(CoreCommand::propDefSet(), {
            { kCommandPropertyAt, INT_MAX, asInteger },
            { kCommandPropertyComponentId, "", asString , kPropRequiredId },
            { kCommandPropertyItem, Object::EMPTY_ARRAY(), asArray }
    });

    return sInsertItemCommandProperties;
}

ActionPtr
InsertItemCommand::execute(const TimersPtr& timers, bool fastMode) {

    if (!calculateProperties())
        return nullptr;

    const int index = getClampedIndex(
        (int) target()->getChildCount(),
        getValue(kCommandPropertyAt).asInt());

    auto child = Builder().inflate(
        target()->getContext(),
        getValue(kCommandPropertyItem));

    if (!child || !child->isValid())
        CONSOLE(mContext) << "Could not inflate item to be inserted";
    else if (!target()->insertChild(child, index))
        CONSOLE(mContext) << "Could not insert child into '" << target()->getId() << "'";

    return nullptr;
}

} // namespace apl