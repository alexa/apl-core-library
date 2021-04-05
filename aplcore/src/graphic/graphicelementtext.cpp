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

#include "apl/graphic/graphicelementtext.h"
#include "apl/graphic/graphicpropdef.h"

#include "apl/component/componentproperties.h"
#include "apl/content/rootconfig.h"

namespace apl {

GraphicElementPtr
GraphicElementText::create(const GraphicPtr &graphic,
                           const ContextPtr &context,
                           const Object &json)
{
    Properties properties;
    properties.emplace(json);

    auto text = std::make_shared<GraphicElementText>(graphic, context);
    if (!text->initialize(graphic, json))
        return nullptr;

    return text;
}

inline Object
defaultFontFamily(GraphicElement&, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultFontFamily());
}

const GraphicPropDefSet&
GraphicElementText::propDefSet() const
{
    static GraphicPropDefSet sTextProperties = GraphicPropDefSet()
        .add({
                 {kGraphicPropertyFill,                    Color(Color::BLACK),     asAvgFill,               kPropInOut | kPropDynamic | kPropEvaluated},
                 {kGraphicPropertyFillOpacity,             1,                       asOpacity,               kPropInOut | kPropDynamic},
                 {kGraphicPropertyFillTransform,           Object::IDENTITY_2D(),   nullptr,                 kPropOut},
                 {kGraphicPropertyFillTransformAssigned,   "",                      asString,                kPropIn    | kPropDynamic, fixFillTransform},
                 {kGraphicPropertyFilters,                 Object::EMPTY_ARRAY(),   asGraphicFilterArray,    kPropInOut },
                 {kGraphicPropertyFontFamily,              "",                      asString,                kPropInOut | kPropDynamic, defaultFontFamily},
                 {kGraphicPropertyFontSize,                40,                      asNonNegativeNumber,     kPropInOut | kPropDynamic},
                 {kGraphicPropertyFontStyle,               kFontStyleNormal,        sFontStyleMap,           kPropInOut | kPropDynamic},
                 {kGraphicPropertyFontWeight,              400,                     sFontWeightMap,          kPropInOut | kPropDynamic},
                 {kGraphicPropertyLetterSpacing,           0,                       asNumber,                kPropInOut | kPropDynamic},
                 {kGraphicPropertyStroke,                  Color(),                 asAvgFill,               kPropInOut | kPropDynamic | kPropEvaluated},
                 {kGraphicPropertyStrokeOpacity,           1,                       asOpacity,               kPropInOut | kPropDynamic},
                 {kGraphicPropertyStrokeTransform,         Object::IDENTITY_2D(),   nullptr,                 kPropOut},
                 {kGraphicPropertyStrokeTransformAssigned, "",                      asString,                kPropIn    | kPropDynamic, fixStrokeTransform},
                 {kGraphicPropertyStrokeWidth,             0,                       asNonNegativeNumber,     kPropInOut | kPropDynamic},
                 {kGraphicPropertyText,                    "",                      asFilteredText,          kPropInOut | kPropRequired | kPropDynamic},
                 {kGraphicPropertyTextAnchor,              kGraphicTextAnchorStart, sGraphicTextAnchorBimap, kPropInOut | kPropDynamic},
                 {kGraphicPropertyCoordinateX,             0,                       asNumber,                kPropInOut | kPropDynamic},
                 {kGraphicPropertyCoordinateY,             0,                       asNumber,                kPropInOut | kPropDynamic}
             });

    return sTextProperties;
}


bool
GraphicElementText::initialize(const GraphicPtr& graphic, const Object& json)
{
    if (!GraphicElement::initialize(graphic, json))
        return false;

    // Update output transforms
    updateTransform(*mContext, kGraphicPropertyFillTransformAssigned, kGraphicPropertyFillTransform, false);
    updateTransform(*mContext, kGraphicPropertyStrokeTransformAssigned, kGraphicPropertyStrokeTransform, false);

    return true;
}

} // namespace apl
