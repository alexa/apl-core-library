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

#include "apl/animation/coreeasing.h"
#include "apl/content/rootconfig.h"
#include "apl/component/textmeasurement.h"
#include "apl/time/coretimemanager.h"

namespace apl {

RootConfig::RootConfig()
    : mAgentName("Default agent"),
      mAgentVersion("1.0"),
      mTextMeasurement( TextMeasurement::instance() ),
      mTimeManager(std::static_pointer_cast<TimeManager>(std::make_shared<CoreTimeManager>(0))),
      mUTCTime(0),
      mLocalTimeAdjustment(0),
      mAnimationQuality(kAnimationQualityNormal),
      mAllowOpenUrl(false),
      mDisallowVideo(false),
      mDefaultIdleTimeout(30000),
      mEnforcedAPLVersion(APLVersion::kAPLVersionIgnore),
      mReportedAPLVersion("1.4"),
      mEnforceTypeField(false),
      mDefaultFontColor(0xfafafaff),
      mDefaultFontFamily("sans-serif"),
      mDefaultThemeFontColor({{"light", 0x1e2222ff}, {"dark", 0xfafafaff}}),
      mDefaultThemeHighlightColor({{"light", 0x0070ba4d}, {"dark",  0x00caff4d}}),
      mTrackProvenance(true),
      mDefaultComponentSize({
          // Set default sizes for components that aren't "auto" width and "auto" height.
        {{kComponentTypeImage, true}, {Dimension(100), Dimension(100)}},
        {{kComponentTypePager, true}, {Dimension(100), Dimension(100)}},
        {{kComponentTypeScrollView, true}, {Dimension(), Dimension(100)}},
        {{kComponentTypeSequence, true}, {Dimension(), Dimension(100)}},  // Vertical scrolling, height=100dp width=auto
        {{kComponentTypeSequence, false}, {Dimension(100), Dimension()}}, // Horizontal scrolling, height=auto width=100dp
        {{kComponentTypeGridSequence, true}, {Dimension(), Dimension(100)}},  // Vertical scrolling, height=100dp width=auto
        {{kComponentTypeGridSequence, false}, {Dimension(100), Dimension()}}, // Horizontal scrolling, height=auto width=100dp
        {{kComponentTypeVideo, true}, {Dimension(100), Dimension(100)}},
      }),
      mPagerChildCache(1),
      mSequenceChildCache(1),
      mDoublePressTimeout(500),
      mLongPressTimeout(1000),
      mSwipeAwayTriggerDistanceThreshold(10),
      mSwipeAwayFulfillDistancePercentageThreshold(0.5),
      mSwipeAwayAnimationEasing(CoreEasing::bezier(0,0,0.58,0.1)), // Standard ease-out
      mSwipeVelocityThreshold(200),
      mTickHandlerUpdateLimit(16)
{
}

RootConfig&
RootConfig::swipeAwayAnimationEasing(const std::string& easing) {
    mSwipeAwayAnimationEasing = Easing::parse(mSession, easing);
    return *this;
}

} // namespace apl
