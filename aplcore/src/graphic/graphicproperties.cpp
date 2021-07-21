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

#include <string>

#include "apl/graphic/graphicproperties.h"

namespace apl {

Bimap<int, std::string> sGraphicScaleBimap = {
    {kGraphicScaleNone,    "none"},
    {kGraphicScaleGrow,    "grow"},
    {kGraphicScaleShrink,  "shrink"},
    {kGraphicScaleStretch, "stretch"},
};

Bimap<int, std::string> sGraphicPropertyBimap = {
    {kGraphicPropertyClipPath,               "clipPath"},
    {kGraphicPropertyCoordinateX,            "x"},
    {kGraphicPropertyCoordinateY,            "y"},
    {kGraphicPropertyFill,                   "fill"},
    {kGraphicPropertyFillOpacity,            "fillOpacity"},
    {kGraphicPropertyFillTransform,          "_fillTransform"},
    {kGraphicPropertyFillTransformAssigned,  "fillTransform"},
    {kGraphicPropertyFilters,                "filters"},
    {kGraphicPropertyFilters,                "filter"},
    {kGraphicPropertyFontFamily,             "fontFamily" },
    {kGraphicPropertyFontSize,               "fontSize" },
    {kGraphicPropertyFontStyle,              "fontStyle" },
    {kGraphicPropertyFontWeight,             "fontWeight"},
    {kGraphicPropertyHeightOriginal,         "height"},
    {kGraphicPropertyHeightActual,           "height_actual"},
    {kGraphicPropertyLang,                   "lang"},
    {kGraphicPropertyLayoutDirection,        "layoutDirection"},
    {kGraphicPropertyLetterSpacing,          "letterSpacing"},
    {kGraphicPropertyOpacity,                "opacity"},
    {kGraphicPropertyPathData,               "pathData"},
    {kGraphicPropertyPathLength,             "pathLength"},
    {kGraphicPropertyPivotX,                 "pivotX"},
    {kGraphicPropertyPivotY,                 "pivotY"},
    {kGraphicPropertyRotation,               "rotation"},
    {kGraphicPropertyScaleX,                 "scaleX"},
    {kGraphicPropertyScaleY,                 "scaleY"},
    {kGraphicPropertyViewportHeightActual,   "viewportHeight_actual"},
    {kGraphicPropertyViewportWidthActual,    "viewportWidth_actual"},
    {kGraphicPropertyScaleTypeHeight,        "scaleTypeHeight"},
    {kGraphicPropertyScaleTypeWidth,         "scaleTypeWidth"},
    {kGraphicPropertyStroke,                 "stroke"},
    {kGraphicPropertyStrokeDashArray,        "strokeDashArray"},
    {kGraphicPropertyStrokeDashOffset,       "strokeDashOffset"},
    {kGraphicPropertyStrokeLineCap,          "strokeLineCap"},
    {kGraphicPropertyStrokeLineJoin,         "strokeLineJoin"},
    {kGraphicPropertyStrokeMiterLimit,       "strokeMiterLimit"},
    {kGraphicPropertyStrokeOpacity,          "strokeOpacity"},
    {kGraphicPropertyStrokeTransform,        "_strokeTransform"},
    {kGraphicPropertyStrokeTransformAssigned,"strokeTransform"},
    {kGraphicPropertyStrokeWidth,            "strokeWidth"},
    {kGraphicPropertyText,                   "text"},
    {kGraphicPropertyTextAnchor,             "textAnchor"},
    {kGraphicPropertyTransform,              "_transform"},
    {kGraphicPropertyTransformAssigned,      "transform"},
    {kGraphicPropertyTranslateX,             "translateX"},
    {kGraphicPropertyTranslateY,             "translateY"},
    {kGraphicPropertyVersion,                "version"},
    {kGraphicPropertyViewportHeightOriginal, "viewportHeight"},
    {kGraphicPropertyViewportWidthOriginal,  "viewportWidth"},
    {kGraphicPropertyWidthActual,            "width_actual"},
    {kGraphicPropertyWidthOriginal,          "width"}
};

Bimap<int, std::string> sGraphicLayoutDirectionBimap = {
    {kGraphicLayoutDirectionLTR,    "LTR"},
    {kGraphicLayoutDirectionRTL,    "RTL"},
};

Bimap<int, std::string> sGraphicLineCapBimap = {
    {kGraphicLineCapButt,   "butt"},
    {kGraphicLineCapRound,  "round"},
    {kGraphicLineCapSquare, "square"}
};

Bimap<int, std::string> sGraphicLineJoinBimap = {
    {kGraphicLineJoinBevel, "bevel"},
    {kGraphicLineJoinMiter, "miter"},
    {kGraphicLineJoinRound, "round"}
};

Bimap<int, std::string> sGraphicTextAnchorBimap = {
    {kGraphicTextAnchorEnd,    "end"},
    {kGraphicTextAnchorMiddle, "middle"},
    {kGraphicTextAnchorStart,  "start"}
};

Bimap<int, std::string> sGraphicVersionBimap = {
    {kGraphicVersion10, "1.0"},
    {kGraphicVersion11, "1.1"},
    {kGraphicVersion12, "1.2"},
};

} // namespace apl
