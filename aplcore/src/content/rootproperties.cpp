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

#include "apl/content/rootproperties.h"

namespace apl {

Bimap<int, std::string> sRootPropertyBimap = {
        { RootProperty::kAgentName,                                   "agentName" },
        { RootProperty::kAgentVersion,                                "agentVersion" },
        { RootProperty::kAllowOpenUrl,                                "allowOpenUrl" },
        { RootProperty::kDisallowVideo,                               "disallowVideo" },
        { RootProperty::kAnimationQuality,                            "animationQuality" },
        { RootProperty::kDefaultIdleTimeout,                          "defaultIdleTimeout" },
        { RootProperty::kReportedVersion,                             "reportedAPLVersion" },
        { RootProperty::kEnforceTypeField,                            "enforceTypeField" },
        { RootProperty::kDefaultFontColor,                            "defaultFontColor" },
        { RootProperty::kDefaultHighlightColor,                       "defaultHighlightColor" },
        { RootProperty::kDefaultFontFamily,                           "defaultFontFamily" },
        { RootProperty::kTrackProvenance,                             "trackProvenance" },
        { RootProperty::kPagerChildCache,                             "pagerChildCache" },
        { RootProperty::kSequenceChildCache,                          "sequenceChildCache" },
        { RootProperty::kUTCTime,                                     "utcTime" },
        { RootProperty::kLang,                                        "lang" },
        { RootProperty::kLocalTimeAdjustment,                         "localTimeAdjustment" },
        { RootProperty::kDoublePressTimeout,                          "doublePressTimeout" },
        { RootProperty::kLongPressTimeout,                            "longPressTimeout" },
        { RootProperty::kPressedDuration,                             "pressedDuration" },
        { RootProperty::kTapOrScrollTimeout,                          "tapOrScrollTimeout" },
        { RootProperty::kSwipeAwayFulfillDistancePercentageThreshold, "swipeAway.fulfillDistancePercentageThreshold" },
        { RootProperty::kSwipeAwayAnimationEasing,                    "swipeAway.animationEasing" },
        { RootProperty::kSwipeVelocityThreshold,                      "swipeAway.swipeVelocityThreshold" },
        { RootProperty::kSwipeMaxVelocity,                            "swipeAway.swipeMaxVelocity" },
        { RootProperty::kSwipeAngleTolerance,                         "swipeAway.swipeAngleTolerance" },
        { RootProperty::kDefaultSwipeAnimationDuration,               "swipeAway.defaultAnimationDuration" },
        { RootProperty::kMaxSwipeAnimationDuration,                   "swipeAway.maxAnimationDuration" },
        { RootProperty::kMinimumFlingVelocity,                        "fling.minimumVelocity" },
        { RootProperty::kMaximumFlingVelocity,                        "fling.maxVelocity" },
        { RootProperty::kTickHandlerUpdateLimit,                      "tickHAndlerUpdateLimit" },
        { RootProperty::kFontScale,                                   "fontScale" },
        { RootProperty::kScreenMode,                                  "screenMode" },
        { RootProperty::kScreenReader,                                "screenReader" },
        { RootProperty::kPointerInactivityTimeout,                    "pointerInactivityTimeout" },
        { RootProperty::kPointerSlopThreshold,                        "pointerSlopThreshold" },
        { RootProperty::kScrollCommandDuration,                       "scroll.commandDuration" },
        { RootProperty::kScrollOnFocusDuration,                       "scroll.onFocusDuration" },
        { RootProperty::kScrollSnapDuration,                          "scroll.snapDuration" },
        { RootProperty::kDefaultPagerAnimationDuration,               "defaultPagerAnimationDuration" },
        { RootProperty::kDefaultPagerAnimationEasing,                 "defaultPagerAnimationEasing"},
        { RootProperty::kScrollAngleSlopeVertical,                    "scroll.angleSlopeVertical" },
        { RootProperty::kScrollAngleSlopeHorizontal,                  "scroll.angleSlopeHorizontal" },
        { RootProperty::kScrollFlingVelocityLimitEasingVertical,      "scroll.flingVelocityLimitEasingVertical" },
        { RootProperty::kScrollFlingVelocityLimitEasingHorizontal,    "scroll.flingVelocityLimitEasingHorizontal" },
        { RootProperty::kUEScrollerVelocityEasing,                    "scroller.ue.velocityEasing" },
        { RootProperty::kUEScrollerDurationEasing,                    "scroller.ue.durationEasing" },
        { RootProperty::kUEScrollerMaxDuration,                       "scroller.ue.maxDuration" },
        { RootProperty::kUEScrollerDeceleration,                      "scroller.ue.deceleration" },
        { RootProperty::kSendEventAdditionalFlags,                    "sendEvent.flags" },
};

}
