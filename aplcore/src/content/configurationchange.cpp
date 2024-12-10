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
        metrics.size(mPixelWidth, mPixelHeight)
            .minAndMaxWidth(mMinPixelWidth, mMaxPixelWidth)
            .minAndMaxHeight(mMinPixelHeight, mMaxPixelHeight);

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

    applyToRootConfig(rootConfig);

    return rootConfig;
}

void
ConfigurationChange::applyToRootConfig(RootConfig& rootConfig) const
{
    if ((mFlags & kConfigurationChangeScreenMode) != 0)
        rootConfig.set(RootProperty::kScreenMode, mScreenMode);

    if ((mFlags & kConfigurationChangeFontScale) != 0)
        rootConfig.set(RootProperty::kFontScale, mFontScale);

    if ((mFlags & kConfigurationChangeScreenReader) != 0)
        rootConfig.set(RootProperty::kScreenReader, mScreenReaderEnabled);

    if ((mFlags & kConfigurationChangeDisallowVideo) != 0)
        rootConfig.set(RootProperty::kDisallowVideo, mDisallowVideo);

    if ((mFlags & kConfigurationChangeEnvironment) != 0) {
        for (const auto &prop : mEnvironment) {
            rootConfig.setEnvironmentValue(prop.first, prop.second);
        }
    }
}

ObjectMap
ConfigurationChange::mergeEnvironment(const ObjectMap& oldEnvironment) const
{
    auto environment = oldEnvironment;

    environment["reason"] = "reinflation";

    if ((mFlags & kConfigurationChangeScreenMode) != 0)
        environment["screenMode"] = mScreenMode;

    if ((mFlags & kConfigurationChangeFontScale) != 0)
        environment["fontScale"] = mFontScale;

    if ((mFlags & kConfigurationChangeScreenReader) != 0)
        environment["screenReader"] = mScreenReaderEnabled;

    if ((mFlags & kConfigurationChangeDisallowVideo) != 0)
        environment["disallowVideo"] = mDisallowVideo;

    if ((mFlags & kConfigurationChangeEnvironment) != 0) {
        for (const auto &prop : mEnvironment) {
            environment[prop.first] = prop.second;
        }
    }

    return environment;
}

void
ConfigurationChange::mergeConfigurationChange(const ConfigurationChange& other)
{
    mFlags |= other.mFlags;

    if ((other.mFlags & kConfigurationChangeSize) != 0) {
        mPixelWidth = other.mPixelWidth;
        mPixelHeight = other.mPixelHeight;
        mMinPixelWidth = other.mMinPixelWidth;
        mMinPixelHeight = other.mMinPixelHeight;
        mMaxPixelWidth = other.mMaxPixelWidth;
        mMaxPixelHeight = other.mMaxPixelHeight;
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

ConfigurationChange&
ConfigurationChange::environmentValue(const std::string &name, const Object &newValue)
{
    mFlags |= kConfigurationChangeEnvironment;
    mEnvironment[name] = newValue;
    return *this;
}

ObjectMap
ConfigurationChange::asEventProperties(const RootConfig& rootConfig, const Metrics& metrics) const
{
    auto sizeChanged = hasSizeChange() && (mPixelHeight != metrics.getPixelHeight() || mPixelWidth != metrics.getPixelWidth());
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
        {"height",        metrics.pxToDp(mPixelHeight)},
        {"width",         metrics.pxToDp(mPixelWidth)},
        {"maxHeight",     metrics.pxToDp(mMaxPixelHeight)},
        {"maxWidth",      metrics.pxToDp(mMaxPixelWidth)},
        {"minHeight",     metrics.pxToDp(mMinPixelHeight)},
        {"minWidth",      metrics.pxToDp(mMinPixelWidth)},
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

ConfigurationChange
ConfigurationChange::embeddedDocumentChange() const
{
    // Only specific parameters allowed to propagate to embedded change. Everything else is Host-driven.
    auto embeddedConfigChange = ConfigurationChange();
    if ((mFlags & kConfigurationChangeTheme) != 0) embeddedConfigChange.theme(mTheme.c_str());
    if ((mFlags & kConfigurationChangeViewportMode) != 0) embeddedConfigChange.mode(mViewportMode);
    if ((mFlags & kConfigurationChangeFontScale) != 0) embeddedConfigChange.fontScale(mFontScale);
    if ((mFlags & kConfigurationChangeScreenMode) != 0) embeddedConfigChange.screenMode(mScreenMode);
    if ((mFlags & kConfigurationChangeScreenReader) != 0) embeddedConfigChange.screenReader(mScreenReaderEnabled);

    return embeddedConfigChange;
}

const std::set<std::string> &
ConfigurationChange::getSynthesizedPropertyNames()
{
    static std::set<std::string> sNames = {
        "rotated",
        "sizeChanged"
    };
    return sNames;
}

} // namespace apl
