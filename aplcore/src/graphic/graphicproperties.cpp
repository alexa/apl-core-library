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
    {kGraphicPropertyFill,                   "fill"},
    {kGraphicPropertyFillOpacity,            "fillOpacity"},
    {kGraphicPropertyHeightOriginal,         "height"},
    {kGraphicPropertyHeightActual,           "height_actual"},
    {kGraphicPropertyOpacity,                "opacity"},
    {kGraphicPropertyPathData,               "pathData"},
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
    {kGraphicPropertyStrokeOpacity,          "strokeOpacity"},
    {kGraphicPropertyStrokeWidth,            "strokeWidth"},
    {kGraphicPropertyTranslateX,             "translateX"},
    {kGraphicPropertyTranslateY,             "translateY"},
    {kGraphicPropertyVersion,                "version"},
    {kGraphicPropertyViewportHeightOriginal, "viewportHeight"},
    {kGraphicPropertyViewportWidthOriginal,  "viewportWidth"},
    {kGraphicPropertyWidthActual,            "width_actual"},
    {kGraphicPropertyWidthOriginal,          "width"}
};


} // namespace apl