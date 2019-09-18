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

#include "apl/engine/contextdependant.h"
#include "apl/engine/evaluate.h"

namespace apl {

const static bool DEBUG_CONTEXT_DEP = false;

void
ContextDependant::create(const ContextPtr& upstreamContext,
                         const std::string& upstreamName,
                         const ContextPtr& downstreamContext,
                         const std::string& downstreamName,
                         const ContextPtr& evaluationContext,
                         const Object& node,
                         BindingFunction func)
{
    LOG_IF(DEBUG_CONTEXT_DEP)
            << "from: " << upstreamName << "(" << upstreamContext.get()
            << ") to: " << downstreamName << " (" << downstreamContext.get() << ")";

    auto dependant = std::make_shared<ContextDependant>(upstreamContext,
                                                        downstreamContext,
                                                        evaluationContext,
                                                        downstreamName,
                                                        node,
                                                        func);
    upstreamContext->addDownstream(upstreamName, dependant);
    downstreamContext->addUpstream(downstreamName, dependant);
}

void
ContextDependant::removeFromSource()
{
    auto context = mUpstreamContext.lock();
    if (context)
        context->removeDownstream(shared_from_this());
}

/**
 * One context is dependant upon another.
 */
void
ContextDependant::recalculate(bool useDirtyFlag) const
{
    auto downstream = mDownstreamContext.lock();
    auto evaluation = mEvaluationContext.lock();
    if (downstream && evaluation) {
        auto value = mEval(*evaluation, evaluate(*evaluation, mNode));
        downstream->propagate(mName, value, useDirtyFlag);
    }
}

} // namespace apl
