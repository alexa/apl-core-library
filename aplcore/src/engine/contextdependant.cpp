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

#include "apl/engine/contextdependant.h"
#include "apl/engine/evaluate.h"
#include "apl/primitives/symbolreferencemap.h"

namespace apl {

const static bool DEBUG_CONTEXT_DEP = false;

void
ContextDependant::create(const ContextPtr& downstreamContext,
                         const std::string& downstreamName,
                         const Object& equation,
                         const ContextPtr& bindingContext,
                         BindingFunction bindingFunction)
{
    LOG_IF(DEBUG_CONTEXT_DEP) << "to '" << downstreamName << "' (" << downstreamContext.get() << ")";

    SymbolReferenceMap symbols;
    equation.symbols(symbols);
    if (symbols.empty())
        return;

    auto dependant = std::make_shared<ContextDependant>(downstreamContext, downstreamName, equation,
                                                        bindingContext, bindingFunction);

    for (const auto& symbol : symbols.get())
        symbol.second->addDownstream(symbol.first, dependant);

    downstreamContext->addUpstream(downstreamName, dependant);
}

/**
 * One context is dependant upon another.
 */
void
ContextDependant::recalculate(bool useDirtyFlag) const
{
    auto downstream = mDownstreamContext.lock();
    auto bindingContext = mBindingContext.lock();
    if (downstream && bindingContext) {
        auto value = mBindingFunction(*bindingContext, reevaluate(*bindingContext, mEquation));
        downstream->propagate(mDownstreamName, value, useDirtyFlag);
    }
}

} // namespace apl
