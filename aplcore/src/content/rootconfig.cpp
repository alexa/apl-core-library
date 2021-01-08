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

#include "apl/content/rootconfig.h"

#include <cmath>

#include "apl/animation/coreeasing.h"
#include "apl/component/textmeasurement.h"
#include "apl/time/coretimemanager.h"

namespace apl {

static float angleToSlope(float degrees) {
    return std::tan(degrees * M_PI / 180);
}

RootConfig::RootConfig()
    : mTextMeasurement( TextMeasurement::instance() ),
      mTimeManager(std::static_pointer_cast<TimeManager>(std::make_shared<CoreTimeManager>(0))),
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
      mSwipeAwayAnimationEasing(CoreEasing::bezier(0,0,0.58,1)), // Standard ease-out
      mSwipeAngleSlope(angleToSlope(40)) // tolerate up to 40 degree angles when swiping by default
{
}

RootConfig&
RootConfig::swipeAwayAnimationEasing(const std::string& easing) {
    mSwipeAwayAnimationEasing = Easing::parse(mSession, easing);
    return *this;
}

RootConfig& RootConfig::swipeAngleTolerance(float degrees) {
    mSwipeAngleSlope = angleToSlope(degrees);
    return *this;
}

} // namespace apl
