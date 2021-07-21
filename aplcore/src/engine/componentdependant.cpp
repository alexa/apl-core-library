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

#include "apl/engine/componentdependant.h"
#include "apl/engine/context.h"
#include "apl/engine/evaluate.h"
#include "apl/component/corecomponent.h"
#include "apl/primitives/symbolreferencemap.h"

namespace apl {

void ComponentDependant::create(const CoreComponentPtr& downstreamComponent,
                                PropertyKey downstreamKey,
                                const Object& equation,
                                const ContextPtr& bindingContext,
                                BindingFunction bindingFunction)
{
    SymbolReferenceMap symbols;
    equation.symbols(symbols);
    if (symbols.empty())
        return;

    auto dependant = std::make_shared<ComponentDependant>(downstreamComponent, downstreamKey, equation,
                                                          bindingContext, bindingFunction);

    for (const auto& symbol : symbols.get())
        symbol.second->addDownstream(symbol.first, dependant);

    downstreamComponent->addUpstream(downstreamKey, dependant);
}

void
ComponentDependant::recalculate(bool useDirtyFlag) const
{
    auto downstream = mDownstreamComponent.lock();
    auto bindingContext = mBindingContext.lock();
    if (downstream && bindingContext) {
        auto value = mBindingFunction(*bindingContext, reevaluate(*bindingContext, mEquation));
        downstream->updateProperty(mDownstreamKey, value);
    }
}

} // namespace apl
