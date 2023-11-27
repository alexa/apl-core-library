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

ContextPtr
InsertItemCommand::buildBaseChildContext(int insertIndex) const
{
    auto length = target()->getChildCount() + 1;
    auto childContext = Context::createFromParent(target()->getContext());
    childContext->putSystemWriteable("index", insertIndex);
    childContext->putSystemWriteable("length", length);
    return childContext;
}

ActionPtr
InsertItemCommand::execute(const TimersPtr& timers, bool fastMode) {

    if (!calculateProperties())
        return nullptr;

    const int index = getClampedIndex(
        (int) target()->getChildCount(),
        getValue(kCommandPropertyAt).asInt());

    auto childContext = target()->multiChild() ?
                                               buildBaseChildContext(index) :
                                               Context::createFromParent(target()->getContext());
    auto child = Builder(nullptr)
                     .expandSingleComponentFromArray(childContext,
                                                     arrayify(*childContext, getValue(kCommandPropertyItem)),
                                                     Properties(),
                                                     target(),
                                                     target()->getPathObject().addIndex(index),
                                                     // Force to inflate component's children as rebuilder
                                                     // not involved and nothing will be able to inflate
                                                     // lazily.
                                                     true,
                                                     true);

    if (!child || !child->isValid())
        CONSOLE(mContext) << "Could not inflate item to be inserted";
    else if (target()->insertChild(child, index)) {
        // Allow lazy components to process new children layout (if any).
        target()->processLayoutChanges(true, false);
        // And allow for full DOM to adjust any changed relative sizes
        CoreComponent::cast(mContext->topComponent())->processLayoutChanges(true, false);
    } else
        CONSOLE(mContext) << "Could not insert child into '" << target()->getId() << "'";

    return nullptr;
}

} // namespace apl