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

#ifndef _APL_GRAPHIC_DEPENDANT_H
#define _APL_GRAPHIC_DEPENDANT_H

#include "apl/common.h"
#include "apl/engine/dependant.h"
#include "apl/engine/binding.h"
#include "apl/graphic/graphicproperties.h"
#include "apl/primitives/object.h"

namespace apl {

/**
 * A dependant relationship where a change in the upstream context results in a recalculation
 * of a downstream graphic element property.
 *
 * The dependant stores the parsed Node expression.  When the upstream context changes,
 * the Node expression is recalculated and the new value is stored in the downstream
 * GraphicElement.  The GraphicElement will set dirty flags to indicate that it needs
 * to be reloaded and redrawn.
 */
class GraphicDependant : public Dependant {
public:
    static void create(const ContextPtr& upstreamContext,
                       const std::string& upstreamName,
                       const GraphicElementPtr& downstreamGraphicElement,
                       GraphicPropertyKey downstreamKey,
                       const Object& node,
                       BindingFunction func);

    GraphicDependant(const ContextPtr& upstreamContext,
                     const GraphicElementPtr& downstreamGraphicElement,
                     GraphicPropertyKey downstreamKey,
                     const Object& node,
                     BindingFunction func)
        : mUpstreamContext(upstreamContext),
          mDownstreamGraphicElement(downstreamGraphicElement),
          mDownstreamKey(downstreamKey),
          mNode(node),
          mEval(func)
    {}

    void removeFromSource() override;
    void recalculate(bool useDirtyFlag) const override;

private:
    std::weak_ptr<Context> mUpstreamContext;
    std::weak_ptr<GraphicElement> mDownstreamGraphicElement;
    GraphicPropertyKey mDownstreamKey;
    Object mNode;
    BindingFunction mEval;
};

} // namespace apl

#endif // _APL_GRAPHIC_DEPENDANT_H
