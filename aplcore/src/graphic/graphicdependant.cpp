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

#include "apl/graphic/graphicdependant.h"
#include "apl/graphic/graphicelement.h"
#include "apl/graphic/graphic.h"
#include "apl/engine/context.h"
#include "apl/engine/evaluate.h"
#include "apl/primitives/symbolreferencemap.h"

namespace apl {

const static bool DEBUG_GRAPHIC_DEP = false;

void
GraphicDependant::create(const GraphicElementPtr& downstreamGraphicElement,
                         GraphicPropertyKey downstreamKey,
                         const Object& equation,
                         const ContextPtr& bindingContext,
                         BindingFunction bindingFunction)
{
    LOG_IF(DEBUG_GRAPHIC_DEP) << " to " << sGraphicPropertyBimap.at(downstreamKey)
                              << "(" << downstreamGraphicElement.get() << ")";

    SymbolReferenceMap symbols;
    equation.symbols(symbols);
    if (symbols.empty())
        return;

    auto dependant = std::make_shared<GraphicDependant>(downstreamGraphicElement, downstreamKey, equation,
                                                        bindingContext, bindingFunction);

    for (const auto& symbol : symbols.get())
        symbol.second->addDownstream(symbol.first, dependant);

    downstreamGraphicElement->addUpstream(downstreamKey, dependant);
}

void
GraphicDependant::recalculate(bool useDirtyFlag) const
{
    auto downstream = mDownstreamGraphicElement.lock();
    auto bindingContext = mBindingContext.lock();
    if (downstream && bindingContext) {
        auto value = mBindingFunction(*bindingContext, reevaluate(*bindingContext, mEquation));
        LOG_IF(DEBUG_GRAPHIC_DEP) << " new value " << value.toDebugString();
        downstream->setValue(mDownstreamKey, value, useDirtyFlag);
    }
}

}  // namespace apl
