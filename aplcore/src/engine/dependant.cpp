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

#include "apl/engine/dependant.h"
#include "apl/engine/dependantmanager.h"
#include "apl/engine/context.h"
#include "apl/engine/evaluate.h"
#include "apl/primitives/boundsymbolset.h"

namespace apl {

Dependant::Dependant(Object expression,
                     const ContextPtr& bindingContext,
                     BindingFunction bindingFunction,
                     BoundSymbolSet symbols)
    : mExpression(std::move(expression)),
      mBindingContext(bindingContext),
      mBindingFunction(std::move(bindingFunction)),
      mSymbols(std::move(symbols)),
      mOrder(bindingContext->dependantManager().getNextSortOrder())
{
    assert(bindingContext);
}

bool
Dependant::enqueue()
{
    auto downstream = mBindingContext.lock();
    if (!downstream)
        return false;

    downstream->dependantManager().enqueueDependency(shared_from_this());
    return true;
}

void
Dependant::removeFromSource()
{
    detach();
};

void
Dependant::attach()
{
    for (const auto& m : mSymbols) {
        auto context = m.getContext();
        if (context)
            context->addDownstream(m.getName(), shared_from_this());
    }
}

void
Dependant::detach()
{
    for (const auto& m : mSymbols) {
        auto context = m.getContext();
        if (context)
            context->removeDownstream(shared_from_this());
    }

    mSymbols.clear();
}

void
Dependant::reattach(const BoundSymbolSet& symbols)
{
    if (mSymbols != symbols) {
        detach();
        mSymbols = symbols;
        attach();
    }
}


}  // namespace apl