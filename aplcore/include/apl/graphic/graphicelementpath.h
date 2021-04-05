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

#ifndef _APL_GRAPHIC_ELEMENT_PATH_H
#define _APL_GRAPHIC_ELEMENT_PATH_H

#include "apl/graphic/graphicelement.h"

namespace apl {

class GraphicElementPath : public GraphicElement,
                           public Counter<GraphicElementPath> {
public:
    static GraphicElementPtr create(const GraphicPtr& graphic, const ContextPtr& context, const Object& json);

    GraphicElementPath(const GraphicPtr& graphic, const ContextPtr& context) : GraphicElement(graphic, context) {}
    GraphicElementType getType() const override { return kGraphicElementTypePath; }
    std::string toDebugString() const override { return "GraphicElementPath<>"; }

protected:
    const GraphicPropDefSet& propDefSet() const override;
    bool initialize(const GraphicPtr& graphic, const Object& json) override;
};

} // namespace apl

#endif // _APL_GRAPHIC_ELEMENT_PATH_H
