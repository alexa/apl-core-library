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

#include "apl/component/componentproperties.h"

namespace apl {

Bimap<int, std::string> sAlignMap = {
    {kImageAlignBottom,      "bottom"},
    {kImageAlignBottomLeft,  "bottom-left"},
    {kImageAlignBottomLeft,  "bottomLeft"},
    {kImageAlignBottomRight, "bottom-right"},
    {kImageAlignBottomRight, "bottomRight"},
    {kImageAlignCenter,      "center"},
    {kImageAlignLeft,        "left"},
    {kImageAlignRight,       "right"},
    {kImageAlignTop,         "top"},
    {kImageAlignTopLeft,     "top-left"},
    {kImageAlignTopLeft,     "topLeft"},
    {kImageAlignTopRight,    "top-right"},
    {kImageAlignTopRight,    "topRight"}
};

Bimap<int, std::string> sScaleMap = {
    {kImageScaleNone,        "none"},
    {kImageScaleFill,        "fill"},
    {kImageScaleBestFill,    "best-fill"},
    {kImageScaleBestFill,    "bestFill"},
    {kImageScaleBestFit,     "best-fit"},
    {kImageScaleBestFit,     "bestFit"},
    {kImageScaleBestFitDown, "best-fit-down"},
    {kImageScaleBestFitDown, "bestFitDown"}
};

Bimap<int, std::string> sFontStyleMap = {
    {kFontStyleNormal, "normal"},
    {kFontStyleItalic, "italic"}
};

Bimap<int, std::string> sFontWeightMap = {
    {400, "normal"},
    {700, "bold"},
    {100, "100"},
    {200, "200"},
    {300, "300"},
    {400, "400"},
    {500, "500"},
    {600, "600"},
    {700, "700"},
    {800, "800"},
    {900, "900"}
};

Bimap<int, std::string> sTextAlignMap = {
    {kTextAlignAuto,   "auto"},
    {kTextAlignLeft,   "left"},
    {kTextAlignCenter, "center"},
    {kTextAlignRight,  "right"}
};

Bimap<int, std::string> sTextAlignVerticalMap = {
    {kTextAlignVerticalAuto,   "auto"},
    {kTextAlignVerticalTop,    "top"},
    {kTextAlignVerticalCenter, "center"},
    {kTextAlignVerticalBottom, "bottom"}
};

Bimap<int, std::string> sFlexboxAlignMap = {
    {kFlexboxAlignStretch,  "stretch"},
    {kFlexboxAlignCenter,   "center"},
    {kFlexboxAlignStart,    "start"},
    {kFlexboxAlignEnd,      "end"},
    {kFlexboxAlignBaseline, "baseline"},
    {kFlexboxAlignAuto,     "auto"}
};

Bimap<int, std::string> sContainerDirectionMap = {
    {kContainerDirectionColumn, "column"},
    {kContainerDirectionRow,    "row"}
};

Bimap<int, std::string> sFlexboxJustifyContentMap = {
    {kFlexboxJustifyContentStart,        "start"},
    {kFlexboxJustifyContentEnd,          "end"},
    {kFlexboxJustifyContentCenter,       "center"},
    {kFlexboxJustifyContentSpaceBetween, "spaceBetween"},
    {kFlexboxJustifyContentSpaceBetween, "space-between"},
    {kFlexboxJustifyContentSpaceAround,  "spaceAround"},
    {kFlexboxJustifyContentSpaceAround,  "space-around"}
};

Bimap<int, std::string> sNumberingMap = {
    {kNumberingNormal, "normal"},
    {kNumberingSkip,   "skip"},
    {kNumberingReset,  "reset"}
};

Bimap<int, std::string> sPositionMap = {
    {kPositionAbsolute, "absolute"},
    {kPositionRelative, "relative"}
};

Bimap<int, std::string> sScrollDirectionMap = {
    {kScrollDirectionHorizontal, "horizontal"},
    {kScrollDirectionVertical,   "vertical"}
};

Bimap<int, std::string> sNavigationMap = {
    {kNavigationNormal,      "normal"},
    {kNavigationNone,        "none"},
    {kNavigationWrap,        "wrap"},
    {kNavigationForwardOnly, "forward-only"},
    {kNavigationForwardOnly, "forwardOnly"}
};

Bimap<int, std::string> sAudioTrackMap = {
    {kAudioTrackForeground, "foreground"},
    {kAudioTrackBackground, "background"},
    {kAudioTrackNone,       "none"}
};

Bimap<int, std::string> sVectorGraphicAlignMap = {
    {kVectorGraphicAlignBottom,      "bottom"},
    {kVectorGraphicAlignBottomLeft,  "bottom-left"},
    {kVectorGraphicAlignBottomLeft,  "bottomLeft"},
    {kVectorGraphicAlignBottomRight, "bottom-right"},
    {kVectorGraphicAlignBottomRight, "bottomRight"},
    {kVectorGraphicAlignCenter,      "center"},
    {kVectorGraphicAlignLeft,        "left"},
    {kVectorGraphicAlignRight,       "right"},
    {kVectorGraphicAlignTop,         "top"},
    {kVectorGraphicAlignTopLeft,     "top-left"},
    {kVectorGraphicAlignTopLeft,     "topLeft"},
    {kVectorGraphicAlignTopRight,    "top-right"},
    {kVectorGraphicAlignTopRight,    "topRight"}
};

Bimap<int, std::string> sVectorGraphicScaleMap = {
    {kVectorGraphicScaleNone,        "none"},
    {kVectorGraphicScaleFill,        "fill"},
    {kVectorGraphicScaleBestFill,    "best-fill"},
    {kVectorGraphicScaleBestFill,    "bestFill"},
    {kVectorGraphicScaleBestFit,     "best-fit"},
    {kVectorGraphicScaleBestFit,     "bestFit"},
};


Bimap<int, std::string> sVideoScaleMap = {
    {kVideoScaleBestFill, "best-fill"},
    {kVideoScaleBestFit,  "best-fit"}
};

Bimap<int, std::string> sDisplayMap = {
    {kDisplayNormal,    "normal"},
    {kDisplayInvisible, "invisible"},
    {kDisplayNone,      "none"}
};

Bimap<int, std::string> sSnapMap = {
    {kSnapNone,   "none"},
    {kSnapStart,  "start"},
    {kSnapCenter, "center"},
    {kSnapEnd,    "end"}
};

/**
 * Map between property names and property constants.
 *
 * The naming convention is camel-case, starting with a lower-case
 * letter.  The naming convention is:
 *
 * 1. User-settable properties start with a lower-case letter (e.g., "borderWidth")
 * 2. System-calculated properties start with an underscore (e.g., "_bounds").  A system-calculated
 *    property may not be set by the user, but may (and should) be read.
 * 3. Experimental properties start with a hyphen (e.g., "-fastScrollScale").  Experimental properties
 *    are not part of the public documentation and may change at any time.
 */
Bimap<int, std::string> sComponentPropertyBimap = {
    {kPropertyAccessibilityLabel,      "accessibilityLabel"},
    {kPropertyAlign,                   "align"},
    {kPropertyAlignItems,              "alignItems"},
    {kPropertyAlignSelf,               "alignSelf"},
    {kPropertyAudioTrack,              "audioTrack"},
    {kPropertyAutoplay,                "autoplay"},
    {kPropertyBackgroundColor,         "backgroundColor"},
    {kPropertyBorderBottomLeftRadius,  "borderBottomLeftRadius"},
    {kPropertyBorderBottomRightRadius, "borderBottomRightRadius"},
    {kPropertyBorderColor,             "borderColor"},
    {kPropertyBorderRadius,            "borderRadius"},
    {kPropertyBorderRadii,             "_borderRadii"},
    {kPropertyBorderTopLeftRadius,     "borderTopLeftRadius"},
    {kPropertyBorderTopRightRadius,    "borderTopRightRadius"},
    {kPropertyBorderWidth,             "borderWidth"},
    {kPropertyBottom,                  "bottom"},
    {kPropertyBounds,                  "_bounds"},
    {kPropertyChecked,                 "checked"},
    {kPropertyColor,                   "color"},
    {kPropertyColorKaraokeTarget,      "_colorKaraokeTarget"},
    {kPropertyColorNonKaraoke,         "_colorNonKaraoke"},
    {kPropertyCurrentPage,             "_currentPage"},
    {kPropertyDescription,             "description"},
    {kPropertyDirection,               "direction"},
    {kPropertyDisabled,                "disabled"},
    {kPropertyDisplay,                 "display"},
    {kPropertyEntities,                "entities"},
    {kPropertyFastScrollScale,         "-fastScrollScale"},
    {kPropertyFilters,                 "filters"},
    {kPropertyFontFamily,              "fontFamily"},
    {kPropertyFontSize,                "fontSize"},
    {kPropertyFontStyle,               "fontStyle"},
    {kPropertyFontWeight,              "fontWeight"},
    {kPropertyGraphic,                 "graphic"},
    {kPropertyGrow,                    "grow"},
    {kPropertyHandleKeyDown,           "handleKeyDown"},
    {kPropertyHandleKeyUp,             "handleKeyUp"},
    {kPropertyHeight,                  "height"},
    {kPropertyId,                      "id"},
    {kPropertyInitialPage,             "initialPage"},
    {kPropertyInnerBounds,             "_innerBounds"},
    {kPropertyJustifyContent,          "justifyContent"},
    {kPropertyLeft,                    "left"},
    {kPropertyLetterSpacing,           "letterSpacing"},
    {kPropertyLineHeight,              "lineHeight"},
    {kPropertyMaxHeight,               "maxHeight"},
    {kPropertyMaxLines,                "maxLines"},
    {kPropertyMaxWidth,                "maxWidth"},
    {kPropertyMediaBounds,             "mediaBounds"},
    {kPropertyMinHeight,               "minHeight"},
    {kPropertyMinWidth,                "minWidth"},
    {kPropertyNavigation,              "navigation"},
    {kPropertyNotifyChildrenChanged,   "_notify_childrenChanged"},
    {kPropertyNumbered,                "numbered"},
    {kPropertyNumbering,               "numbering"},
    {kPropertyOnBlur,                  "onBlur"},
    {kPropertyOnEnd,                   "onEnd"},
    {kPropertyOnFocus,                 "onFocus"},
    {kPropertyOnMount,                 "onMount"},
    {kPropertyOnScroll,                "onScroll"},
    {kPropertyOnPageChanged,           "onPageChanged"},
    {kPropertyOnPause,                 "onPause"},
    {kPropertyOnPlay,                  "onPlay"},
    {kPropertyOnPress,                 "onPress"},
    {kPropertyOnTimeUpdate,            "onTimeUpdate"},
    {kPropertyOnTrackUpdate,           "onTrackUpdate"},
    {kPropertyOpacity,                 "opacity"},
    {kPropertyOverlayColor,            "overlayColor"},
    {kPropertyOverlayGradient,         "overlayGradient"},
    {kPropertyPaddingBottom,           "paddingBottom"},
    {kPropertyPaddingLeft,             "paddingLeft"},
    {kPropertyPaddingRight,            "paddingRight"},
    {kPropertyPaddingTop,              "paddingTop"},
    {kPropertyPosition,                "position"},
    {kPropertyRight,                   "right"},
    {kPropertyScale,                   "scale"},
    {kPropertyScrollDirection,         "scrollDirection"},
    {kPropertyScrollPosition,         "_scrollPosition"},
    {kPropertyShadowColor,             "shadowColor"},
    {kPropertyShadowHorizontalOffset,  "shadowHorizontalOffset"},
    {kPropertyShadowRadius,            "shadowRadius"},
    {kPropertyShadowVerticalOffset,    "shadowVerticalOffset"},
    {kPropertyShrink,                  "shrink"},
    {kPropertySnap,                    "snap"},
    {kPropertySource,                  "source"},
    {kPropertySpacing,                 "spacing"},
    {kPropertySpeech,                  "speech"},
    {kPropertyText,                    "text"},
    {kPropertyTextAlign,               "textAlign"},
    {kPropertyTextAlignVertical,       "textAlignVertical"},
    {kPropertyTop,                     "top"},
    {kPropertyTrackCount,              "_trackCount"},
    {kPropertyTrackCurrentTime,        "_trackCurrentTime"},
    {kPropertyTrackDuration,           "_trackDuration"},
    {kPropertyTrackEnded,              "_trackEnded"},
    {kPropertyTrackIndex,              "_trackIndex"},
    {kPropertyTrackPaused,             "_trackPaused"},
    {kPropertyTransformAssigned,       "transform"},
    {kPropertyTransform,               "_transform"},
    {kPropertyUser,                    "_user"},
    {kPropertyWidth,                   "width"},
    {kPropertyOnCursorEnter,           "onCursorEnter"},
    {kPropertyOnCursorExit,            "onCursorExit"},
    {kPropertyLaidOut,                 "_laidOut"}
};

Bimap<int, std::string> sComponentTypeBimap = {
    {kComponentTypeContainer,     "Container"},
    {kComponentTypeFrame,         "Frame"},
    {kComponentTypeImage,         "Image"},
    {kComponentTypePager,         "Pager"},
    {kComponentTypeScrollView,    "ScrollView"},
    {kComponentTypeSequence,      "Sequence"},
    {kComponentTypeText,          "Text"},
    {kComponentTypeTouchWrapper,  "TouchWrapper"},
    {kComponentTypeVectorGraphic, "VectorGraphic"},
    {kComponentTypeVideo,         "Video"}
};


}  // namespace apl