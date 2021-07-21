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

#ifndef _APL_ROOT_PROPERTIES_H
#define _APL_ROOT_PROPERTIES_H

#include <string>

#include "apl/utils/bimap.h"

namespace apl {

enum RootProperty {
    /// Agent name
    kAgentName,
    /// Agent version
    kAgentVersion,
    /// If the OpenURL command is supported
    kAllowOpenUrl,
    /// If video is supported
    kDisallowVideo,
    /// Quality of animation expected. If set to kAnimationQualityNone, all animation commands are disabled (include onMount).
    kAnimationQuality,
    /// Default idle timeout
    kDefaultIdleTimeout,
    /// APL version of the specification that is supported by this application. This value will be reported in the data-binding.
    kReportedVersion,
    /// Sets whether the "type" field of an APL document should be enforced
    kEnforceTypeField,
    /// Default font color. This is the fallback color for all themes
    kDefaultFontColor,
    /// Default highlight color. This is the fallback color for all themes
    kDefaultHighlightColor,
    /// Default font family. This is usually locale-based
    kDefaultFontFamily,
    ///  Enable or disable tracking of resource, style, and component provenance
    kTrackProvenance,
    /// Set pager layout cache in both directions
    kPagerChildCache,
    /// Set sequence layout cache in both directions
    kSequenceChildCache,
    /// Current UTC time in milliseconds since the epoch
    kUTCTime,
    /// A BCP-47 string (e.g., en-US) which affects the default font selection of Text or EditText components.
    /// For example to select the japanese characters of the "Noto Sans CJK" font family set this to "ja-JP"
    kLang,
    /// The document layout direction. Can be "RTL" or "LTR". Default is "LTR" (left to right)
    kLayoutDirection,
    /// Local time zone adjustment in milliseconds
    kLocalTimeAdjustment,
    /// Double press timeout
    kDoublePressTimeout,
    /// Long press timeout
    kLongPressTimeout,
    /// Duration to show the "pressed" state of a component when programmatically invoked
    kPressedDuration,
    /// This is the maximum amount of time that can pass before the system has to commit to this being a touch event instead of a scroll event.
    kTapOrScrollTimeout,
    /// SwipeAway gesture fulfill distance threshold in percents
    kSwipeAwayFulfillDistancePercentageThreshold,
    /// SwipeAway gesture animation easing
    kSwipeAwayAnimationEasing,
    /// SwipeAway (and any related gesture) gesture swipe speed threshold
    kSwipeVelocityThreshold,
    /// Maximum SwipeAway (and any related gesture) gesture swipe speed
    kSwipeMaxVelocity,
    /// SwipeAway gesture tolerance in degrees when determining whether a swipe was triggered
    kSwipeAngleTolerance,
    /// Default animation duration, in ms, for SwipeAway animations
    kDefaultSwipeAnimationDuration,
    /// Max animation duration, in ms, for SwipeAway animations
    kMaxSwipeAnimationDuration,
    /// Fling velocity threshold
    kMinimumFlingVelocity,
    /// Tick handler update limit in ms
    kTickHandlerUpdateLimit,
    /// Requested font scaling factor for the document
    kFontScale,
    /// Screen display mode for accessibility (normal or high-contrast)
    kScreenMode,
    /// Inform that a screen reader is turned on
    kScreenReader,
    /// Pointer inactivity timeout. Pointer considered stale after pointer was not updated for this time.
    kPointerInactivityTimeout,
    /// Fling gestures velocity limit
    kMaximumFlingVelocity,
    /// Gesture distance threshold in dp. Initial movement below this threshold does not trigger gestures.
    kPointerSlopThreshold,
    /// Scroll commands duration
    kScrollCommandDuration,
    /// Scroll into view on focus duration
    kScrollOnFocusDuration,
    /// Scroll snap duration
    kScrollSnapDuration,
    /// Default pager page switch animation duration
    kDefaultPagerAnimationDuration,
    /// Default pager animation easing
    kDefaultPagerAnimationEasing,
    /// Fling gesture tolerance in degrees when determining whether a swipe was triggered in vertical direction
    kScrollAngleSlopeVertical,
    /// Fling gesture tolerance in degrees when determining whether a swipe was triggered in horizontal direction
    kScrollAngleSlopeHorizontal,
    /// Scrolling velocity limit easing to apply based on scroll distance in vertical direction
    kScrollFlingVelocityLimitEasingVertical,
    /// Scrolling velocity limit easing to apply based on scroll distance in horizontal direction
    kScrollFlingVelocityLimitEasingHorizontal,
    /// UnidirectionalEasingScroller scrolling easing for velocity defined (touch produced) scrolling
    kUEScrollerVelocityEasing,
    /// UnidirectionalEasingScroller scrolling easing for duration defined (programmatic) scrolling
    kUEScrollerDurationEasing,
    /// UnidirectionalEasingScroller maximum scrolling animation duration
    kUEScrollerMaxDuration,
    /// UnidirectionalEasingScroller deceleration
    kUEScrollerDeceleration,
};

extern Bimap<int, std::string> sRootPropertyBimap;

} // namespace apl


#endif //_APL_ROOT_PROPERTIES_H
