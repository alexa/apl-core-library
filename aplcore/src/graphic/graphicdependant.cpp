/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/graphic/graphicdependant.h"
#include "apl/graphic/graphicelement.h"
#include "apl/graphic/graphic.h"
#include "apl/engine/context.h"
#include "apl/engine/evaluate.h"

namespace apl {

const static bool DEBUG_GRAPHIC_DEP = false;

void
GraphicDependant::create(const ContextPtr& upstreamContext,
                         const std::string& upstreamName,
                         const GraphicElementPtr& downstreamGraphicElement,
                         GraphicPropertyKey downstreamKey,
                         const Object& node,
                         BindingFunction func)
{
    LOG_IF(DEBUG_GRAPHIC_DEP) << "From " << upstreamName << "(" << upstreamContext.get() << ")"
                              << " to " << sGraphicPropertyBimap.at(downstreamKey)
                              << "(" << downstreamGraphicElement.get() << ")";

    auto dependant = std::make_shared<GraphicDependant>(upstreamContext,
                                                        downstreamGraphicElement,
                                                        downstreamKey,
                                                        node,
                                                        func);
    upstreamContext->addDownstream(upstreamName, dependant);
    downstreamGraphicElement->addUpstream(downstreamKey, dependant);
}

void
GraphicDependant::removeFromSource()
{
    auto context = mUpstreamContext.lock();
    if (context)
        context->removeDownstream(shared_from_this());
}

void
GraphicDependant::recalculate(bool useDirtyFlag) const
{
    auto context = mUpstreamContext.lock();
    auto element = mDownstreamGraphicElement.lock();
    if (context && element) {
        auto value = mEval(*context, evaluate(*context, mNode));
        element->setValue(mDownstreamKey, value, useDirtyFlag);
    }
}

}  // namespace apl