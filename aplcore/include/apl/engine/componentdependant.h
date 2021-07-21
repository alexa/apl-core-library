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

#ifndef _APL_COMPONENT_DEPENDANT_H
#define _APL_COMPONENT_DEPENDANT_H

#include <memory>

#include "apl/common.h"
#include "apl/engine/dependant.h"
#include "apl/component/componentproperties.h"

namespace apl {

class Context;
class CoreComponent;

/**
 * A dependant relationship where a change in the upstream context results in a change in
 * a downstream component property.
 *
 * The downstream component stores the data-binding expression that will be recalculated, so
 * all this object has to do is inform the downstream component that the specific property
 * should be recalculated.
 */
class ComponentDependant : public Dependant {
public:
    /**
     *
     * @param downstreamComponent The downstream or target component
     * @param downstreamKey The property that will be modified
     * @param equation The expression which will be evaluated to recalculate downstream.
     * @param bindingContext The context where the equation will be bound
     * @param bindingFunction The binding function that will be applied after evaluating the equation
     */
    static void create(const CoreComponentPtr& downstreamComponent,
                       PropertyKey downstreamKey,
                       const Object& equation,
                       const ContextPtr& bindingContext,
                       BindingFunction bindingFunction);

    /**
     * Internal constructor: do not call.  Use ComponentDependant::create instead.
     *
     * @param downstreamComponent
     * @param downstreamKey
     * @param equation
     * @param bindingContext
     * @param bindingFunction
     */
    ComponentDependant(const CoreComponentPtr& downstreamComponent,
                       PropertyKey downstreamKey,
                       const Object& equation,
                       const ContextPtr& bindingContext,
                       BindingFunction bindingFunction)
        : Dependant(equation, bindingContext, bindingFunction),
          mDownstreamComponent(downstreamComponent),
          mDownstreamKey(downstreamKey)
    {}

    void recalculate(bool useDirtyFlag) const override;

private:
    std::weak_ptr<CoreComponent> mDownstreamComponent;
    PropertyKey mDownstreamKey;
};


} // namespace apl

#endif //_APL_COMPONENT_DEPENDANT_H
