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

#ifndef _APL_GRAPHIC_PROPERTIES_H
#define _APL_GRAPHIC_PROPERTIES_H

#include "apl/utils/bimap.h"

namespace apl {

enum GraphicScale {
    kGraphicScaleNone = 0,
    kGraphicScaleGrow = 1,
    kGraphicScaleShrink = 2,
    kGraphicScaleStretch = 3
};

enum GraphicPropertyKey {
    kGraphicPropertyFill,
    kGraphicPropertyFillOpacity,
    kGraphicPropertyHeightActual,
    kGraphicPropertyHeightOriginal,
    kGraphicPropertyOpacity,
    kGraphicPropertyPathData,
    kGraphicPropertyPivotX,
    kGraphicPropertyPivotY,
    kGraphicPropertyRotation,
    kGraphicPropertyScaleX,
    kGraphicPropertyScaleY,
    kGraphicPropertyViewportHeightActual,
    kGraphicPropertyViewportWidthActual,
    kGraphicPropertyScaleTypeHeight,
    kGraphicPropertyScaleTypeWidth,
    kGraphicPropertyStroke,
    kGraphicPropertyStrokeOpacity,
    kGraphicPropertyStrokeWidth,
    kGraphicPropertyTranslateX,
    kGraphicPropertyTranslateY,
    kGraphicPropertyVersion,
    kGraphicPropertyViewportHeightOriginal,
    kGraphicPropertyViewportWidthOriginal,
    kGraphicPropertyWidthActual,
    kGraphicPropertyWidthOriginal
};

enum GraphicElementType {
    kGraphicElementTypeContainer,
    kGraphicElementTypeGroup,
    kGraphicElementTypePath
};

extern Bimap<int, std::string> sGraphicScaleBimap;
extern Bimap<int, std::string> sGraphicPropertyBimap;

} // namespace apl

#endif //_APL_GRAPHIC_PROPERTIES_H
