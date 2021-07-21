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

#ifndef _APL_DEPENDANT_H
#define _APL_DEPENDANT_H

#include <memory>

#include "apl/common.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"
#include "apl/primitives/object.h"
#include "apl/engine/binding.h"

namespace apl {

/**
 * A Dependant connects something that changes (like a data-binding) to something that needs to be informed
 * when a change occurs. The upstream object normally holds an array of dependants to be recalculated.  Each
 * dependant object is responsible for executing the appropriate recalculations on its downstream object.
 *
 * It's common for the target object to have its own list of dependants so that changes in one part of the
 * system will fan out and update many targets.  Loops in the dependant graph are not allowed.
 */
class Dependant : public Counter<Dependant>,
                  public NonCopyable,
                  public std::enable_shared_from_this<Dependant> {
public:
    Dependant(const Object& equation, const ContextPtr& bindingContext, BindingFunction bindingFunction)
        : mEquation(equation),
          mBindingContext(bindingContext),
          mBindingFunction(std::move(bindingFunction))
        {}
    virtual ~Dependant() = default;

    /**
     * Remove this dependant from the source or upstream. If you override this, call the base method.
     */
    virtual void removeFromSource();

    /**
     * Recalculate the values in the target object.
     * @param useDirtyFlag If true, mark downstream changes as dirty
     */
    virtual void recalculate(bool useDirtyFlag) const = 0;

protected:
    Object mEquation;                        // The equation or expression to be evaluated
    std::weak_ptr<Context> mBindingContext;  // The context the BindingFunction will be applied in
    BindingFunction mBindingFunction;        // The function to be applied after evaluation
};

}  // namespace apl

#endif //_APL_DEPENDANT_H
