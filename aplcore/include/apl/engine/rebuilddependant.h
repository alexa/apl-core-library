/*
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
 *
 */

#ifndef _APL_REBUILD_DEPENDANT_H
#define _APL_REBUILD_DEPENDANT_H

#include <memory>
#include <string>

#include "apl/engine/dependant.h"
#include "apl/engine/evaluate.h"

namespace apl {

class RebuildDependant : public Dependant {
public:
    static void create(const std::shared_ptr<CoreComponent>& parentComponent,
                       const ContextPtr& downstream,
                       BoundSymbolSet symbols) {
        assert(!symbols.empty());
        assert(parentComponent);

        auto dependant = std::make_shared<RebuildDependant>(parentComponent,
                                                            downstream,
                                                            sBindingFunctions.at(kBindingTypeBoolean),
                                                            std::move(symbols));
        dependant->attach();
        // Register under "pseudo" upstream to disambiguate from any other upstreams.
        downstream->addUpstream("_SPECIAL_WHEN_CONDITIONAL", dependant);
    }

    RebuildDependant(const CoreComponentPtr& parentComponent,
                     const ContextPtr& downstream,
                     BindingFunction bindingFunction,
                     BoundSymbolSet symbols)
        : Dependant(Object::NULL_OBJECT(),
                    downstream,
                    std::move(bindingFunction),
                    std::move(symbols)),
          mParentComponent(parentComponent),
          mDownstreamContext(downstream)
    {}

    ~RebuildDependant() override = default;

    void recalculate(bool useDirtyFlag) override {
        auto parentComponent = mParentComponent.lock();
        auto downstreamCtx = mDownstreamContext.lock();
        if (parentComponent && downstreamCtx) {
            parentComponent->scheduleRebuildChange(downstreamCtx);
        }
    }

private:
   std::weak_ptr<CoreComponent> mParentComponent;
   std::weak_ptr<Context> mDownstreamContext;
};

}  // namespace apl

#endif //_APL_REBUILD_DEPENDANT_H
