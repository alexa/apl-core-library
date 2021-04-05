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

#ifndef _APL_GRAPHIC_BUILDER_H
#define _APL_GRAPHIC_BUILDER_H

#include "apl/graphic/graphicelement.h"


namespace apl {

/**
 * Static methods for inflating AVG objects.
 */
class GraphicBuilder {
public:
    /**
     * Inflate graphic elements inside a Graphic object
     * @param graphic The container Graphic
     * @param json The raw JSON wrapped in object
     * @return The top of the GraphicElement tree of graphic content.
     */
    static GraphicElementPtr build(const GraphicPtr& graphic, const Object& json);

    /**
     * Inflate graphic elements in a graphic pattern (does not have a container)
     * @param context The working context
     * @param json The raw JSON wrapped in object
     * @return GraphicElement.
     */
    static GraphicElementPtr build(const Context& context, const Object& json);

    /**
     * Internal constructor - don't use this
     * @param graphic
     */
    explicit GraphicBuilder(const GraphicPtr& graphic);

private:
    void addChildren(GraphicElement& element, const Object& json);
    GraphicElementPtr createChild(const ContextPtr& context, const Object& json);
    GraphicElementPtr createChildFromArray(const ContextPtr& context, const std::vector<Object>& items);

private:
    const GraphicPtr mGraphic;
    bool mMultichildSupport;
};


} // namespace apl

#endif // _APL_GRAPHIC_BUILDER_H
