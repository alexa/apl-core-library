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

#ifndef _APL_CONTEXT_DEPENDANT_H
#define _APL_CONTEXT_DEPENDANT_H

#include <memory>

#include "apl/engine/builder.h"
#include "apl/engine/dependant.h"
#include "apl/primitives/object.h"

namespace apl {

class Context;

/**
 * A dependant relationship where a change in the source context results in a change
 * in the target context.  This occurs when a "bind" relationship in a Component refers
 * to a value defined in a data-binding context.
 *
 * The dependant stores the parsed Node in the child context.  When the source
 * context value changes, the dependant calculates the new target context value and
 * stores it in the target context.  This normally triggers additional dependants
 * to update their values.
 */
class ContextDependant : public Dependant {
public:
    /**
     * Construct a dependency between two contexts
     * @param upstreamContext The upstream or source context.
     * @param upstreamName The name of the symbol in the upstream context which drives a change downstream
     * @param downstreamContext The downstream or target context.
     * @param downstreamName The name of the symbol in the downstream context which will be recalculated.
     * @param evaluationContext The context where the node will be evaluated
     * @param node The Node expression which will be evaluated to recalculate downstream.
     * @param type The type of binding for the Node expression.
     */
    static void create(const ContextPtr& upstreamContext,
                       const std::string& upstreamName,
                       const ContextPtr& downstreamContext,
                       const std::string& downstreamName,
                       const ContextPtr& evaluationContext,
                       const Object& node,
                       BindingFunction func);

    /**
     * Internal constructor - do not call. Use ContextDependant::create instead.
     * @param upstreamContext
     * @param downstreamContext
     * @param name
     * @param node
     * @param type
     */
    ContextDependant(const ContextPtr& upstreamContext,
                     const ContextPtr& downstreamContext,
                     const ContextPtr& evaluationContext,
                     const std::string& name,
                     const Object& node,
                     BindingFunction func)
        : mUpstreamContext(upstreamContext),
          mDownstreamContext(downstreamContext),
          mEvaluationContext(evaluationContext),
          mName(name),
          mNode(node),
          mEval(func)
    {}

    void removeFromSource() override;

    void recalculate(bool useDirtyFlag) const override;

private:
    std::weak_ptr<Context> mUpstreamContext;
    std::weak_ptr<Context> mDownstreamContext;
    std::weak_ptr<Context> mEvaluationContext;
    std::string mName;
    Object mNode;
    BindingFunction mEval;
};

} // namespace apl

#endif //_APL_CONTEXT_DEPENDANT_H
