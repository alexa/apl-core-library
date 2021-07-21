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

enum GraphicLayoutDirection {
    kGraphicLayoutDirectionLTR = 0,
    kGraphicLayoutDirectionRTL = 1
};

enum GraphicLineCap {
    kGraphicLineCapButt = 0,
    kGraphicLineCapRound = 1,
    kGraphicLineCapSquare = 2
};

enum GraphicLineJoin {
    kGraphicLineJoinBevel = 0,
    kGraphicLineJoinMiter = 1,
    kGraphicLineJoinRound = 2
};

enum GraphicTextAnchor {
    kGraphicTextAnchorEnd = 0,
    kGraphicTextAnchorMiddle = 1,
    kGraphicTextAnchorStart = 2
};

enum GraphicPropertyKey {
    kGraphicPropertyClipPath,
    kGraphicPropertyCoordinateX,
    kGraphicPropertyCoordinateY,
    kGraphicPropertyFill,
    kGraphicPropertyFillOpacity,
    kGraphicPropertyFillTransform,
    kGraphicPropertyFillTransformAssigned,
    kGraphicPropertyFilters,
    kGraphicPropertyFontFamily,
    kGraphicPropertyFontSize,
    kGraphicPropertyFontStyle,
    kGraphicPropertyFontWeight,
    kGraphicPropertyHeightActual,
    kGraphicPropertyHeightOriginal,
    kGraphicPropertyLang,
    kGraphicPropertyLayoutDirection,
    kGraphicPropertyLetterSpacing,
    kGraphicPropertyOpacity,
    kGraphicPropertyPathData,
    kGraphicPropertyPathLength,
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
    kGraphicPropertyStrokeDashArray,
    kGraphicPropertyStrokeDashOffset,
    kGraphicPropertyStrokeLineCap,
    kGraphicPropertyStrokeLineJoin,
    kGraphicPropertyStrokeMiterLimit,
    kGraphicPropertyStrokeOpacity,
    kGraphicPropertyStrokeTransform,
    kGraphicPropertyStrokeTransformAssigned,
    kGraphicPropertyStrokeWidth,
    kGraphicPropertyText,
    kGraphicPropertyTextAnchor,
    kGraphicPropertyTransform,
    kGraphicPropertyTransformAssigned,
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
    kGraphicElementTypePath,
    kGraphicElementTypeText
};

enum GraphicVersions {
    kGraphicVersion10,
    kGraphicVersion11,
    kGraphicVersion12
};

extern Bimap<int, std::string> sGraphicScaleBimap;
extern Bimap<int, std::string> sGraphicPropertyBimap;
extern Bimap<int, std::string> sGraphicLayoutDirectionBimap;
extern Bimap<int, std::string> sGraphicLineCapBimap;
extern Bimap<int, std::string> sGraphicLineJoinBimap;
extern Bimap<int, std::string> sGraphicTextAnchorBimap;
extern Bimap<int, std::string> sGraphicVersionBimap;

} // namespace apl

#endif //_APL_GRAPHIC_PROPERTIES_H
