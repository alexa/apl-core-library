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
     * @param downstreamContext The downstream or target context.
     * @param downstreamName The name of the symbol in the downstream context which will be recalculated.
     * @param equation The expression which will be evaluated to recalculate downstream.
     * @param bindingContext The context where the equation will be bound
     * @param bindingFunction The binding function that will be applied after evaluating the equation
     */
    static void create(const ContextPtr& downstreamContext,
                       const std::string& downstreamName,
                       const Object& equation,
                       const ContextPtr& bindingContext,
                       BindingFunction bindingFunction);


    /**
     * Internal constructor - do not call. Use ContextDependant::create instead.
     * @param downstreamContext
     *
     * @param evaluationContext
     * @param name
     * @param node
     * @param func
     */
    ContextDependant(const ContextPtr& downstreamContext,
                     const std::string& downstreamName,
                     const Object& equation,
                     const ContextPtr& bindingContext,
                     BindingFunction bindingFunction)
        : Dependant(equation, bindingContext, bindingFunction),
          mDownstreamContext(downstreamContext),
          mDownstreamName(downstreamName)
    {}

    void recalculate(bool useDirtyFlag) const override;

private:
    std::weak_ptr<Context> mDownstreamContext;
    std::string mDownstreamName;
};

} // namespace apl

#endif //_APL_CONTEXT_DEPENDANT_H
