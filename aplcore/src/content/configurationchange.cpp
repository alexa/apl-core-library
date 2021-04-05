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

#include "apl/content/configurationchange.h"
#include "apl/content/metrics.h"

namespace apl {

Metrics
ConfigurationChange::mergeMetrics(const Metrics& oldMetrics) const
{
    auto metrics = oldMetrics;

    if ((mFlags & kConfigurationChangeSize) != 0)
        metrics.size(mPixelWidth, mPixelHeight);

    if ((mFlags & kConfigurationChangeTheme) != 0)
        metrics.theme(mTheme.c_str());

    if ((mFlags & kConfigurationChangeViewportMode) != 0)
        metrics.mode(mViewportMode);

    return metrics;
}

RootConfig
ConfigurationChange::mergeRootConfig(const RootConfig& oldRootConfig) const
{
    auto rootConfig = oldRootConfig;

    if ((mFlags & kConfigurationChangeScreenMode) != 0)
        rootConfig.screenMode(mScreenMode);

    if ((mFlags & kConfigurationChangeFontScale) != 0)
        rootConfig.fontScale(mFontScale);

    if ((mFlags & kConfigurationChangeScreenReader) != 0)
        rootConfig.screenReader(mScreenReaderEnabled);

    return rootConfig;
}

void
ConfigurationChange::mergeConfigurationChange(const apl::ConfigurationChange& other)
{
    mFlags |= other.mFlags;

    if ((other.mFlags & kConfigurationChangeSize) != 0) {
        mPixelWidth = other.mPixelWidth;
        mPixelHeight = other.mPixelHeight;
    }

    if ((other.mFlags & kConfigurationChangeTheme) != 0)
        mTheme = other.mTheme;

    if ((other.mFlags & kConfigurationChangeViewportMode) != 0)
        mViewportMode = other.mViewportMode;

    if ((other.mFlags & kConfigurationChangeScreenMode) != 0)
        mScreenMode = other.mScreenMode;

    if ((other.mFlags & kConfigurationChangeFontScale) != 0)
        mFontScale = other.mFontScale;

    if ((other.mFlags & kConfigurationChangeScreenReader) != 0)
        mScreenReaderEnabled = other.mScreenReaderEnabled;
}

Size
ConfigurationChange::mergeSize(const Size& oldSize) const
{
    auto size = oldSize;

    if ((mFlags & kConfigurationChangeSize))
        size = { static_cast<float>(mPixelWidth), static_cast<float>(mPixelHeight) };

    return size;
}

ObjectMap
ConfigurationChange::asEventProperties(const RootConfig& rootConfig, const Metrics& metrics) const
{
    auto sizeChanged = mPixelHeight != metrics.getPixelHeight() || mPixelWidth != metrics.getPixelWidth();
    auto rotated = sizeChanged && mPixelWidth == metrics.getPixelHeight() && mPixelHeight == metrics.getPixelWidth();

    return {
        {"height",       mPixelHeight},
        {"width",        mPixelWidth},
        {"theme",        mTheme},
        {"viewportMode", sViewportModeBimap.at(mViewportMode)},
        {"fontScale",    mFontScale},
        {"screenMode",   sScreenModeBimap.at(mScreenMode)},
        {"screenReader", mScreenReaderEnabled},
        {"sizeChanged",  sizeChanged},
        {"rotated",      rotated}
    };
}



} // namespace apl