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
#include "apl/engine/context.h"

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

    if ((mFlags & kConfigurationChangeDisallowVideo) != 0)
        rootConfig.set(RootProperty::kDisallowVideo, mDisallowVideo);

    if ((mFlags & kConfigurationChangeEnvironment) != 0) {
        for (const auto &prop : mEnvironment) {
            if (rootConfig.getEnvironmentValues().count(prop.first) > 0)
                rootConfig.setEnvironmentValue(prop.first, prop.second);
        }
    }

    return rootConfig;
}

void
ConfigurationChange::mergeConfigurationChange(const ConfigurationChange& other)
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

    if ((other.mFlags & kConfigurationChangeDisallowVideo) != 0)
        mDisallowVideo = other.mDisallowVideo;

    if ((other.mFlags & kConfigurationChangeEnvironment) != 0) {
        for (const auto &prop : other.mEnvironment)
            mEnvironment[prop.first] = prop.second;
    }
}

Size
ConfigurationChange::mergeSize(const Size& oldSize) const
{
    auto size = oldSize;

    if ((mFlags & kConfigurationChangeSize))
        size = { static_cast<float>(mPixelWidth), static_cast<float>(mPixelHeight) };

    return size;
}

ConfigurationChange&
ConfigurationChange::environmentValue(const std::string &name, const Object &newValue) {
    mFlags |= kConfigurationChangeEnvironment;
    mEnvironment[name] = newValue;
    return *this;
}

ObjectMap
ConfigurationChange::asEventProperties(const RootConfig& rootConfig, const Metrics& metrics) const
{
    auto sizeChanged = mPixelHeight != metrics.getPixelHeight() || mPixelWidth != metrics.getPixelWidth();
    auto rotated = sizeChanged && mPixelWidth == metrics.getPixelHeight() && mPixelHeight == metrics.getPixelWidth();

    // Populate the event with any custom env properties from the root config. If a property
    // was overridden in this config change instance, use the new value.
    auto env = std::make_shared<ObjectMap>();
    for (const auto &prop : rootConfig.getEnvironmentValues()) {
        const auto &propName = prop.first;
        const auto it = mEnvironment.find(propName);
        if (it != mEnvironment.end()) {
            env->emplace(propName, it->second);
        } else {
            env->emplace(propName, prop.second);
        }
    }

    return {
        {"height",        mPixelHeight},
        {"width",         mPixelWidth},
        {"theme",         mTheme},
        {"viewportMode",  sViewportModeBimap.at(mViewportMode)},
        {"disallowVideo", mDisallowVideo},
        {"fontScale",     mFontScale},
        {"screenMode",    sScreenModeBimap.at(mScreenMode)},
        {"screenReader",  mScreenReaderEnabled},
        {"environment",   env},
        // Synthesized properties - update getSynthesizedPropertyNames when adding a new one
        {"sizeChanged",   sizeChanged},
        {"rotated",       rotated},
    };
}

const std::set<std::string> &
ConfigurationChange::getSynthesizedPropertyNames() {
    static std::set<std::string> sNames = {
        "rotated",
        "sizeChanged"
    };
    return sNames;
}

} // namespace apl