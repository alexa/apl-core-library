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

#include "apl/graphic/graphicelementcontainer.h"
#include "apl/graphic/graphicpropdef.h"

#include "apl/utils/session.h"

namespace apl {

GraphicElementPtr
GraphicElementContainer::create(const GraphicPtr& graphic,
                                const ContextPtr& context,
                                const Object& json)
{
    auto container = std::make_shared<GraphicElementContainer>(graphic, context);
    if (!container->initialize(graphic, json))
        return nullptr;

    return container;
}


const GraphicPropDefSet&
GraphicElementContainer::propDefSet() const
{
    static GraphicPropDefSet sContainerProperties = GraphicPropDefSet()
        .add({
                 {kGraphicPropertyHeightActual,           Dimension(0),                nullptr,              kPropOut},
                 {kGraphicPropertyHeightOriginal,         Dimension(100),              asAbsoluteDimension,  kPropIn | kPropRequired},
                 {kGraphicPropertyLang,                   "",                          asString,             kPropInOut},
                 {kGraphicPropertyLayoutDirection,        kGraphicLayoutDirectionLTR,  sGraphicLayoutDirectionBimap, kPropInOut},
                 {kGraphicPropertyScaleTypeHeight,        kGraphicScaleNone,           sGraphicScaleBimap,   kPropIn},
                 {kGraphicPropertyScaleTypeWidth,         kGraphicScaleNone,           sGraphicScaleBimap,   kPropIn},
                 {kGraphicPropertyViewportHeightActual,   0.0,                         nullptr,              kPropOut},
                 {kGraphicPropertyViewportWidthActual,    0.0,                         nullptr,              kPropOut},
                 {kGraphicPropertyVersion,                kGraphicVersion12,           sGraphicVersionBimap, kPropIn | kPropRequired},
                 {kGraphicPropertyViewportHeightOriginal, 0,                           asNonNegativeNumber,  kPropIn},
                 {kGraphicPropertyViewportWidthOriginal,  0,                           asNonNegativeNumber,  kPropIn},
                 {kGraphicPropertyWidthActual,            Dimension(0),                nullptr,              kPropOut},
                 {kGraphicPropertyWidthOriginal,          Dimension(100),              asAbsoluteDimension,  kPropIn | kPropRequired}
             });

    return sContainerProperties;
}


bool
GraphicElementContainer::initialize(const GraphicPtr& graphic, const Object& json)
{
    if (!GraphicElement::initialize(graphic, json))
        return false;

    auto height = mValues.get(kGraphicPropertyHeightOriginal).getAbsoluteDimension();
    if (height <= 0) {
        CONSOLE_CTP(mContext) << "Invalid graphic height - must be positive";
        return false;
    }

    auto width = mValues.get(kGraphicPropertyWidthOriginal).getAbsoluteDimension();
    if (width <= 0) {
        CONSOLE_CTP(mContext) << "Invalid graphic width - must be positive";
        return false;
    }

    // Configure the original output values.  A later "layout" pass may update these
    mValues.set(kGraphicPropertyHeightActual, Dimension(height));
    mValues.set(kGraphicPropertyWidthActual, Dimension(width));

    // Update the viewport values.  If the viewport original values weren't set, we use the width/height
    double viewportHeight = mValues.get(kGraphicPropertyViewportHeightOriginal).getDouble();
    double viewportWidth = mValues.get(kGraphicPropertyViewportWidthOriginal).getDouble();

    if (viewportHeight <= 0) {
        viewportHeight = height;
        mValues.set(kGraphicPropertyViewportHeightOriginal, viewportHeight);
    }

    if (viewportWidth <= 0) {
        viewportWidth = width;
        mValues.set(kGraphicPropertyViewportWidthOriginal, viewportWidth);
    }

    // Configure the viewport actuals to match the originals
    mValues.set(kGraphicPropertyViewportHeightActual, viewportHeight);
    mValues.set(kGraphicPropertyViewportWidthActual, viewportWidth);

    // Update the context to include width and height (these are the viewport width and height)
    mContext->systemUpdateAndRecalculate("width", viewportWidth, false);
    mContext->systemUpdateAndRecalculate("height", viewportHeight, false);

    return true;
}


} // namespace apl
