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

#include "apl/graphic/graphicelementpath.h"
#include "apl/graphic/graphicpropdef.h"

namespace apl {

GraphicElementPtr
GraphicElementPath::create(const GraphicPtr& graphic,
                           const ContextPtr& context,
                           const Object& json)
{
    Properties properties;
    properties.emplace(json);

    auto path = std::make_shared<GraphicElementPath>(graphic, context);
    if (!path->initialize(graphic, json))
        return nullptr;

    return path;
}


const GraphicPropDefSet&
GraphicElementPath::propDefSet() const
{
    static GraphicPropDefSet sPathProperties = GraphicPropDefSet()
        .add({
                 {kGraphicPropertyFill,                    Color(),               asAvgFill,             kPropInOut | kPropDynamic | kPropEvaluated},
                 {kGraphicPropertyFillOpacity,             1.0,                   asOpacity,             kPropInOut | kPropDynamic},
                 {kGraphicPropertyFillTransform,           Object::IDENTITY_2D(), nullptr,               kPropOut},
                 {kGraphicPropertyFillTransformAssigned,   "",                    asString,              kPropIn | kPropDynamic, fixFillTransform},
                 {kGraphicPropertyFilters,                 Object::EMPTY_ARRAY(), asGraphicFilterArray,  kPropInOut},
                 {kGraphicPropertyPathData,                "",                    asString,              kPropInOut | kPropRequired | kPropDynamic},
                 {kGraphicPropertyPathLength,              0,                     asNumber,              kPropInOut | kPropDynamic},
                 {kGraphicPropertyStroke,                  Color(),               asAvgFill,             kPropInOut | kPropDynamic | kPropEvaluated},
                 {kGraphicPropertyStrokeDashArray,         Object::EMPTY_ARRAY(), asDashArray,           kPropInOut | kPropDynamic | kPropEvaluated},
                 {kGraphicPropertyStrokeDashOffset,        0,                     asNumber,              kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeLineCap,           kGraphicLineCapButt,   sGraphicLineCapBimap,  kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeLineJoin,          kGraphicLineJoinMiter, sGraphicLineJoinBimap, kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeMiterLimit,        4,                     asNumber,              kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeOpacity,           1.0,                   asOpacity,             kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeTransform,         Object::IDENTITY_2D(), nullptr,               kPropOut},
                 {kGraphicPropertyStrokeTransformAssigned, "",                    asString,              kPropIn | kPropDynamic, fixStrokeTransform},
                 {kGraphicPropertyStrokeWidth,             1.0,                   asNonNegativeNumber,   kPropInOut | kPropDynamic}
             });

    return sPathProperties;
}

bool
GraphicElementPath::initialize(const GraphicPtr& graphic, const Object& json)
{
    if (!GraphicElement::initialize(graphic, json))
        return false;

    // Update output transforms
    updateTransform(*mContext, kGraphicPropertyFillTransformAssigned, kGraphicPropertyFillTransform, false);
    updateTransform(*mContext, kGraphicPropertyStrokeTransformAssigned, kGraphicPropertyStrokeTransform, false);

    return true;
}

}  // namespace apl
