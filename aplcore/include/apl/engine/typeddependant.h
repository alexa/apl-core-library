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

#ifndef _APL_TYPED_DEPENDANT_H
#define _APL_TYPED_DEPENDANT_H

#include <memory>
#include <string>

#include "apl/engine/dependant.h"
#include "apl/engine/evaluate.h"

namespace apl {

/**
 * A template wrapper around Dependant that simplifies creating dependants for data-bound value
 * propagation. The Downstream class should support the "setValue" method.
 *
 * @tparam Downstream The type of the downstream class.
 * @tparam Key The type of key used by the downstream class
 */
template<typename Downstream, typename Key>
class TypedDependant : public Dependant {
public:
   static void create(const std::shared_ptr<Downstream>& downstream,
                      Key downstreamKey,
                      Object expression,
                      const ContextPtr& bindingContext,
                      BindingFunction bindingFunction,
                      BoundSymbolSet symbols) {
       assert(!symbols.empty());
       assert(downstream);

       auto dependant = std::make_shared<TypedDependant<Downstream, Key>>(downstream,
                                                                          downstreamKey,
                                                                          std::move(expression),
                                                                          bindingContext,
                                                                          std::move(bindingFunction),
                                                                          std::move(symbols));
       dependant->attach();
       downstream->addUpstream(downstreamKey, dependant);
   }

   TypedDependant(const std::shared_ptr<Downstream>& downstream,
                  Key downstreamKey,
                  Object expression,
                  const ContextPtr& bindingContext,
                  BindingFunction bindingFunction,
                  BoundSymbolSet symbols)
       : Dependant(std::move(expression),
                   bindingContext,
                   std::move(bindingFunction),
                   std::move(symbols)),
         mDownstream(downstream),
         mDownstreamKey(std::move(downstreamKey))
   {}

   void recalculate(bool useDirtyFlag) override {
       auto bindingContext = mBindingContext.lock();
       auto downstream = mDownstream.lock();

       if (bindingContext && downstream) {
           auto result = applyDataBinding(*bindingContext, mExpression, mBindingFunction);
           reattach(result.symbols);
           downstream->setValue(mDownstreamKey, result.value, useDirtyFlag);
       }
   }

private:
   std::weak_ptr<Downstream> mDownstream;
   Key mDownstreamKey;
};

using ContextDependant = TypedDependant<Context, std::string>;

}  // namespace apl

#endif //_APL_TYPED_DEPENDANT_H
