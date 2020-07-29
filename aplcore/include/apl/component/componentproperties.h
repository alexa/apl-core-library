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

#ifndef _APL_COMPONENT_PROPERTIES_H
#define _APL_COMPONENT_PROPERTIES_H

#include <string>

#include "apl/utils/bimap.h"

namespace apl {

/**
 * Alignment of an image within an ImageComponent
 */
enum ImageAlign {
    kImageAlignBottom = 0,
    kImageAlignBottomLeft = 1,
    kImageAlignBottomRight = 2,
    kImageAlignCenter = 3,
    kImageAlignLeft = 4,
    kImageAlignRight = 5,
    kImageAlignTop = 6,
    kImageAlignTopLeft = 7,
    kImageAlignTopRight = 8
};

/**
 * Scaling of an image within an ImageComponent
 */
enum ImageScale {
    kImageScaleNone = 0,
    kImageScaleFill = 1,
    kImageScaleBestFill = 2,
    kImageScaleBestFit = 3,
    kImageScaleBestFitDown = 4
};

/**
 * Font style in a TextComponent
 */
enum FontStyle {
    kFontStyleNormal = 0,
    kFontStyleItalic = 1
};

/**
 * Horizontal alignment of text in a TextComponent
 */
enum TextAlign {
    kTextAlignAuto = 0,
    kTextAlignLeft = 1,
    kTextAlignCenter = 2,
    kTextAlignRight = 3
};

/**
 * Vertical alignment of text in a TextComponent
 */
enum TextAlignVertical {
    kTextAlignVerticalAuto = 0,
    kTextAlignVerticalTop = 1,
    kTextAlignVerticalCenter = 2,
    kTextAlignVerticalBottom = 3
};

/**
 * Used by alignItems and alignSelf in a ContainerComponent
 * Don't change these values without checking component.cpp
 */
enum FlexboxAlign {
    kFlexboxAlignStretch = 0,
    kFlexboxAlignCenter = 1,
    kFlexboxAlignStart = 2,
    kFlexboxAlignEnd = 3,
    kFlexboxAlignBaseline = 4,
    kFlexboxAlignAuto = 5
};

/**
 * Layout direction of a ContainerComponent
 */
enum ContainerDirection {
    kContainerDirectionColumn = 0,
    kContainerDirectionRow = 1,
    kContainerDirectionColumnReverse = 2,
    kContainerDirectionRowReverse = 3
};

/**
 * Justification of children of a ContainerComponent.
 * Don't change these values without checking component.cpp
 */
enum FlexboxJustifyContent {
    kFlexboxJustifyContentStart = 0,
    kFlexboxJustifyContentEnd = 1,
    kFlexboxJustifyContentCenter = 2,
    kFlexboxJustifyContentSpaceBetween = 3,
    kFlexboxJustifyContentSpaceAround = 4
};

/**
 * Wrap setting of the flexbox container
 */
enum FlexboxWrap {
    kFlexboxWrapNoWrap = 0,
    kFlexboxWrapWrap = 1,
    kFlexboxWrapWrapReverse = 2,
};

/**
 * Numbering of a child of a ContainerComponent or SequenceComponent
 */
enum Numbering {
    kNumberingNormal = 0,
    kNumberingSkip = 1,
    kNumberingReset = 2
};

/**
 * Relative or absolute positioning for a child of a ContainerComponent
 */
enum Position {
    kPositionAbsolute = 0,
    kPositionRelative = 1
};

/**
 * Scrolling direction for a SequenceComponent
 */
enum ScrollDirection {
    kScrollDirectionVertical = 0,
    kScrollDirectionHorizontal = 1
};

/**
 * Navigation options for a PagerComponent
 */
enum Navigation {
    kNavigationNormal = 0,
    kNavigationNone = 1,
    kNavigationWrap = 2,
    kNavigationForwardOnly = 3
};

/**
 * Selected audio track for a VideoComponent
 */
enum AudioTrack {
    kAudioTrackBackground = 0,
    kAudioTrackForeground = 1,
    kAudioTrackNone = 2
};

/**
 * Alignment of graphic within a VectorGraphicComponent
 */
enum VectorGraphicAlign {
    kVectorGraphicAlignBottom = 0,
    kVectorGraphicAlignBottomLeft = 1,
    kVectorGraphicAlignBottomRight = 2,
    kVectorGraphicAlignCenter = 3,
    kVectorGraphicAlignLeft = 4,
    kVectorGraphicAlignRight = 5,
    kVectorGraphicAlignTop = 6,
    kVectorGraphicAlignTopLeft = 7,
    kVectorGraphicAlignTopRight = 8
};

/**
 * Scaling of graphic within a VectorGraphicComponent
 */
enum VectorGraphicScale {
    kVectorGraphicScaleNone = 0,
    kVectorGraphicScaleFill = 1,
    kVectorGraphicScaleBestFill = 2,
    kVectorGraphicScaleBestFit = 3
};

/**
 * Scaling of video within a VideoComponent
 */
enum VideoScale {
    kVideoScaleBestFill = 0,
    kVideoScaleBestFit = 1
};

/**
 * Display options for a Component
 */
enum Display {
    kDisplayNormal = 0,
    kDisplayInvisible = 1,
    kDisplayNone = 2
};

/**
 * Snapping location for children of a Sequence
 */
enum Snap {
    kSnapNone = 0,
    kSnapStart = 1,
    kSnapCenter = 2,
    kSnapEnd = 3
};

/**
 * The type of keyboard to display for the EditText Component.
 */
enum KeyboardType {
    kKeyboardTypeDecimalPad = 0,
    kKeyboardTypeEmailAddress = 1,
    kKeyboardTypeNormal = 2,
    kKeyboardTypeNumberPad = 3,
    kKeyboardTypePhonePad = 4,
    kKeyboardTypeUrl = 5
};

enum SubmitKeyType {
    kSubmitKeyTypeDone = 0,
    kSubmitKeyTypeGo = 1,
    kSubmitKeyTypeNext = 2,
    kSubmitKeyTypeSearch = 3,
    kSubmitKeyTypeSend = 4
};

enum PropertyKey {
    // NOTE: ScrollDirection is placed early in the list so that it loads BEFORE height and width (see SequenceComponent.cpp)
    /// SequenceComponent scrolling direction (see #ScrollDirection)
    kPropertyScrollDirection,
    /// Component accessibility label
    kPropertyAccessibilityLabel,
    /// ImageComponent and VectorGraphicComponent alignment (see #ImageAlign, #VectorGraphicAlign)
    kPropertyAlign,
    /// ContainerComponent alignment of items (see #FlexboxAlign)
    kPropertyAlignItems,
    /// ContainerComponent child alignment (see #FlexboxAlign)
    kPropertyAlignSelf,
    /// VideoComponent audio track (see #AudioTrack)
    kPropertyAudioTrack,
    /// VideoComponent autoplay
    kPropertyAutoplay,
    /// FrameComponent background color
    kPropertyBackgroundColor,
    /// FrameComponent border bottom-left radius (input only)
    kPropertyBorderBottomLeftRadius,
    /// FrameComponent border bottom-right radius (input only)
    kPropertyBorderBottomRightRadius,
    /// FrameComponent | EditTextComponent border color
    kPropertyBorderColor,
    /// ImageComponent border radius; also used by FrameComponent to set overall border radius (input only)
    kPropertyBorderRadius,
    /// FrameComponent border radii (output only)
    kPropertyBorderRadii,
    /// FrameComponent | EditTextComponent width of the border stroke (input only)
    kPropertyBorderStrokeWidth,
    /// FrameComponent border top-left radius (input only)
    kPropertyBorderTopLeftRadius,
    /// FrameComponent border top-right radius (input only)
    kPropertyBorderTopRightRadius,
    /// FrameComponent | EditTextComponent border width
    kPropertyBorderWidth,
    /// ContainerComponent child absolute bottom position
    kPropertyBottom,
    /// Component bounding rectangle (output only)
    kPropertyBounds,
    /// GridSequenceComponent child height(s)
    kPropertyChildHeight,
    /// GridSequenceComponent child width(s)
    kPropertyChildWidth,
    /// Component checked state
    kPropertyChecked,
    /// TextComponent | EditTextComponent color
    kPropertyColor,
    /// TextComponent color for karaoke target
    kPropertyColorKaraokeTarget,
    /// TextComponent color for text that isn't subject to Karaoke
    kPropertyColorNonKaraoke,
    /// PagerComponent current page display
    kPropertyCurrentPage,
    /// Component description
    kPropertyDescription,
    /// ContainerComponent layout direction (see #ContainerDirection)
    kPropertyDirection,
    /// Component disabled state
    kPropertyDisabled,
    /// Component general display (see #Display)
    kPropertyDisplay,
    /// FrameComponent | EditTextComponent drawn border width (output only)
    kPropertyDrawnBorderWidth,
    /// Component array of opaque entity data
    kPropertyEntities,
    /// SequenceComponent fast scroll scaling setting
    kPropertyFastScrollScale,
    /// ImageComponent array of filters
    kPropertyFilters,
    /// Property that identifies that component is focusable and as a result of it navigable
    kPropertyFocusable,
    /// TextComponent | EditTextComponent valid font families
    kPropertyFontFamily,
    /// TextComponent font size
    kPropertyFontSize,
    /// TextComponent font style (see #FontStyle)
    kPropertyFontStyle,
    /// TextComponent | EditTextComponent font weight
    kPropertyFontWeight,
    /// Component handler for tick
    kPropertyHandleTick,
    /// EditTextComponent highlight color behind selected text.
    kPropertyHighlightColor,
    /// EditTextComponent hint text,displayed when no text has been entered
    kPropertyHint,
    /// EditTextComponent color for hint text
    kPropertyHintColor,
    /// EditTextComponent style of the hint font
    kPropertyHintStyle,
    /// EditTextComponent weight of the hint font
    kPropertyHintWeight,
    /// Gesture handlers
    kPropertyGestures,
    /// VectorGraphicComponent calculated graphic data (output only)
    kPropertyGraphic,
    /// ContainerComponent child flexbox grow
    kPropertyGrow,
    /// Handlers to check on key down events.
    kPropertyHandleKeyDown,
    // Handlers to check on key up events.
    kPropertyHandleKeyUp,
    /// Component height
    kPropertyHeight,
    /// Component assigned ID
    kPropertyId,
    /// PagerComponent initial page to display
    kPropertyInitialPage,
    /// Component calculated inner bounds rectangle [applies padding and border] (output only)
    kPropertyInnerBounds,
    /// GridSequenceComponent number of Columns if vertical, number of Rows if horizontal
    kPropertyItemsPerCourse,
    /// ContainerComponent flexbox content justification (see #FlexboxJustifyContent)
    kPropertyJustifyContent,
    /// EditTextComponent keyboard type
    kPropertyKeyboardType,
    /// ContainerComponent child absolute left position
    kPropertyLeft,
    /// TextComponent letter spacing
    kPropertyLetterSpacing,
    /// TextComponent line height
    kPropertyLineHeight,
    /// Component maximum height
    kPropertyMaxHeight,
    /// EditTextComponent maximum number of characters that can be displayed
    kPropertyMaxLength,
    /// TextComponent maximum number of lines
    kPropertyMaxLines,
    /// Component maximum width
    kPropertyMaxWidth,
    /// VectorGraphicComponent bounding rectangle for displayed graphic (output only)
    kPropertyMediaBounds,
    /// Component minimum height
    kPropertyMinHeight,
    /// Component minimum width
    kPropertyMinWidth,
    /// PagerComponent valid navigation mode (see #Navigation)
    kPropertyNavigation,
    /// Synthetic notification for dirty flag that the children of a component have changed (dirty only)
    kPropertyNotifyChildrenChanged,
    /// SequenceComponent or ContainerComponent has ordinal numbers
    kPropertyNumbered,
    /// SequenceComponent or ContainerComponent child numbered marker (see #Numbering)
    kPropertyNumbering,
    /// ActionableComponent handler when focus is lost
    kPropertyOnBlur,
    /// TouchableComponent handler for cancel
    kPropertyOnCancel,
    /// TouchableComponent handler for down
    kPropertyOnDown,
    /// VideoComponent handler for video end
    kPropertyOnEnd,
    /// ActionableComponent handler when focus is gained
    kPropertyOnFocus,
    /// Component or Document handler for first display of the component or document
    kPropertyOnMount,
    /// TouchableComponent handler for move
    kPropertyOnMove,
    /// PagerComponent handler for when the page changes
    kPropertyOnPageChanged,
    /// VideoComponent handler for video pause
    kPropertyOnPause,
    /// VideoComponent handler for video play
    kPropertyOnPlay,
    /// TouchableComponent handler for press
    kPropertyOnPress,
    /// ScrollViewComponent or SequenceComponent handler for scroll events.
    kPropertyOnScroll,
    /// EditTextComponent commands to execute when the submit button is pressed.
    kPropertyOnSubmit,
    /// EditTextComponent Commands to execute when the text changes from a user event.
    kPropertyOnTextChange,
    /// TouchableComponent handler for up
    kPropertyOnUp,
    /// VideoComponent handler for video time updates
    kPropertyOnTimeUpdate,
    /// VideoComponent handler for video track updates
    kPropertyOnTrackUpdate,
    /// Component opacity (just the current opacity; not the cumulative)
    kPropertyOpacity,
    /// ImageComponent overlay color
    kPropertyOverlayColor,
    /// ImageComponent overlay gradient
    kPropertyOverlayGradient,
    /// Component bottom padding
    kPropertyPaddingBottom,
    /// Component left padding
    kPropertyPaddingLeft,
    /// Component right padding
    kPropertyPaddingRight,
    /// Component top padding
    kPropertyPaddingTop,
    /// ContainerComponent child absolute or relative position (see #Position)
    kPropertyPosition,
    /// ContainerComponent child absolute right position
    kPropertyRight,
    /// ImageComponent, VideoComponent, and VectorGraphicComponent scale property (see #ImageScale, #VectorGraphicScale, #VideoScale)
    kPropertyScale,
    /// Scroll position of the Scrollable component.
    kPropertyScrollPosition,
    /// EditTextComponent hide characters as typed if true
    kPropertySecureInput,
    /// EditTextComponent the text is selected on focus when true
    kPropertySelectOnFocus,
    /// Component shadow color
    kPropertyShadowColor,
    /// Component shadow horizontal offset
    kPropertyShadowHorizontalOffset,
    /// Component shadow radius
    kPropertyShadowRadius,
    /// Component shadow vertical offset
    kPropertyShadowVerticalOffset,
    /// ContainerComponent child flexbox shrink property
    kPropertyShrink,
    /// EditTextComponent specifies approximately howmany characters canbe displayed
    kPropertySize,
    /// SequenceComponent snap location (see #Snap)
    kPropertySnap,
    /// ImageComponent, VideoComponent, and VectorGraphic source URL(s)
    kPropertySource,
    /// ContainerComponent and SequenceComponent child spacing
    kPropertySpacing,
    /// Component opaque speech data
    kPropertySpeech,
    // EditTextComponent label of the return key
    kPropertySubmitKeyType,
    /// TextComponent | EditTextComponent assigned rich text
    kPropertyText,
    /// TextComponent horizontal alignment (see #TextAlign)
    kPropertyTextAlign,
    /// TextComponent vertical alignment (see #TextAlignVertical)
    kPropertyTextAlignVertical,
    /// Integer property giving the total number of tracks in the video component. This property will be updated
    /// whenever a new set of tracks is assigned to the video component.
    kPropertyTrackCount,
    /// Integer property giving the current time (in milliseconds) of the currently active track in the video component.
    /// This property will be updated whenever the current time of the active track changes.
    kPropertyTrackCurrentTime,
    /// Integer property denoting the total duration (in milliseconds) of the currently active track in the video
    /// component.
    kPropertyTrackDuration,
    /// Boolean property.  True if the final track in the video component has finished and the video component is
    /// not playing.
    kPropertyTrackEnded,
    /// Integer property denoting the index of the currently active track in the the video component. This property
    /// will be updated whenever there is a track change.
    kPropertyTrackIndex,
    /// Boolean property.  True if the current track in the video component is not playing.
    kPropertyTrackPaused,
    /// Component 2D graphics transformation
    kPropertyTransform,
    /// Calculated Component 2D graphics transformation (output-only)
    kPropertyTransformAssigned,
    /// ContainerComponent child absolute top position
    kPropertyTop,
    /// Component user-defined properties assembled into a single map.
    kPropertyUser,
    /// Component width
    kPropertyWidth,
    /// Component handler for cursor enter
    kPropertyOnCursorEnter,
    /// Component handler for cursor exit
    kPropertyOnCursorExit,
    /// Component attached to Yoga tree and has flexbox properties calculated.
    kPropertyLaidOut,
    /// EditTextComponent restrict the characters that can be entered
    kPropertyValidCharacters,
    /// Flexbox wrap
    kPropertyWrap,
};

// Be careful adding new items to this list or changing the order of the list.
enum ComponentType {
    kComponentTypeContainer,
    kComponentTypeEditText,
    kComponentTypeFrame,
    kComponentTypeGridSequence,
    kComponentTypeImage,
    kComponentTypePager,
    kComponentTypeScrollView,
    kComponentTypeSequence,
    kComponentTypeText,
    kComponentTypeTouchWrapper,
    kComponentTypeVectorGraphic,
    kComponentTypeVideo
};

/**
 * Modes to measure layout size in TextMeasurement class
 */
enum MeasureMode {
    Undefined,
    Exactly,
    AtMost
};

extern Bimap<int, std::string> sAlignMap;
extern Bimap<int, std::string> sScaleMap;
extern Bimap<int, std::string> sFontStyleMap;
extern Bimap<int, std::string> sFontWeightMap;
extern Bimap<int, std::string> sTextAlignMap;
extern Bimap<int, std::string> sTextAlignVerticalMap;
extern Bimap<int, std::string> sKeyboardTypeMap;
extern Bimap<int, std::string> sSubmitKeyTypeMap;
extern Bimap<int, std::string> sFlexboxAlignMap;
extern Bimap<int, std::string> sContainerDirectionMap;
extern Bimap<int, std::string> sFlexboxJustifyContentMap;
extern Bimap<int, std::string> sFlexboxWrapMap;
extern Bimap<int, std::string> sNumberingMap;
extern Bimap<int, std::string> sPositionMap;
extern Bimap<int, std::string> sScrollDirectionMap;
extern Bimap<int, std::string> sNavigationMap;
extern Bimap<int, std::string> sAudioTrackMap;
extern Bimap<int, std::string> sVectorGraphicAlignMap;
extern Bimap<int, std::string> sVectorGraphicScaleMap;
extern Bimap<int, std::string> sVideoScaleMap;
extern Bimap<int, std::string> sComponentPropertyBimap;
extern Bimap<int, std::string> sComponentTypeBimap;
extern Bimap<int, std::string> sDisplayMap;
extern Bimap<int, std::string> sSnapMap;

}  // namespace apl

#endif //_APL_COMPONENT_PROPERTIES_H
