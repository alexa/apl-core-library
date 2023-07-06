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

#ifndef _APL_CONFIGURATION_CHANGE_H
#define _APL_CONFIGURATION_CHANGE_H

#include <set>

#include "apl/content/rootconfig.h"
#include "apl/content/metrics.h"
#include "apl/primitives/size.h"
#include "apl/utils/log.h"

namespace apl {

class Metrics;

/**
 * The RootContext may be re-inflated at runtime when certain RootConfig and/or Metric properties
 * are changed.  This data structure is used to define the new properties.  You must correctly initialize
 * all of the fields in this data structure; they are not copied from the existing RootConfig or Metrics.
 */
class ConfigurationChange {
public:
    ConfigurationChange() = default;

    /**
     * Convenience constructor that sets the pixel width and height immediately.
     * @param pixelWidth The pixel width of the screen
     * @param pixelHeight The pixel height of the screen
     */
    ConfigurationChange(int pixelWidth, int pixelHeight)
        : mFlags(kConfigurationChangeSize),
          mPixelWidth(pixelWidth),
          mPixelHeight(pixelHeight)
    {}

    /**
     * Update the size
     * @param pixelWidth The pixel width of the screen
     * @param pixelHeight The pixel height of the screen
     * @return This object for chaining
     */
    ConfigurationChange& size(int pixelWidth, int pixelHeight) {
        mFlags |= kConfigurationChangeSize;
        mPixelWidth = pixelWidth;
        mPixelHeight = pixelHeight;
        return *this;
    }

    /**
     * Set the color theme.
     * @param theme The color theme.
     * @return This object for chaining.
     */
    ConfigurationChange& theme(const char *theme) {
        mFlags |= kConfigurationChangeTheme;
        mTheme = theme;
        return *this;
    }

    /**
     * Set the viewport mode.
     * @param viewportMode The viewport mode
     * @return This object for chaining
     */
    ConfigurationChange& mode(ViewportMode viewportMode)
    {
        mFlags |= kConfigurationChangeViewportMode;
        mViewportMode = viewportMode;
        return *this;
    }

    /**
     * Set the viewport mode.
     * @param viewportMode The viewport mode
     * @return This object for chaining
     */
    ConfigurationChange& mode(const std::string &viewportMode)
    {
        auto it = sViewportModeBimap.find(viewportMode);
        if (it != sViewportModeBimap.endBtoA()) {
            return mode(static_cast<ViewportMode>(it->second));
        }

        LOG(LogLevel::kWarn) << "Ignoring invalid viewport mode for configuration change: " << viewportMode;
        return *this;
    }

    /**
     * Set the requested font scaling factor for the document.
     * @param scale The scaling factor.  Default is 1.0
     * @return This object for chaining
     */
    ConfigurationChange& fontScale(float scale) {
        mFlags |= kConfigurationChangeFontScale;
        mFontScale = scale;
        return *this;
    }

    /**
     * Set if video is supported
     * @param disallowed True if video is disallowed
     * @return This object for chaining
     */
    ConfigurationChange& disallowVideo(bool disallowed) {
        mFlags |= kConfigurationChangeDisallowVideo;
        mDisallowVideo = disallowed;
        return *this;
    }

    /**
     * Set the screen display mode for accessibility (normal or high-contrast)
     * @param screenMode The screen display mode
     * @return This object for chaining
     */
    ConfigurationChange& screenMode(RootConfig::ScreenMode screenMode) {
        mFlags |= kConfigurationChangeScreenMode;
        mScreenMode = screenMode;
        return *this;
    }

    /**
     * Set the screen display mode for accessibility (normal or high-contrast)
     * @param mode The screen display mode
     * @return This object for chaining
     */
    ConfigurationChange& screenMode(const std::string &mode) {
        auto it = sScreenModeBimap.find(mode);
        if (it != sScreenModeBimap.endBtoA()) {
            return screenMode(static_cast<RootConfig::ScreenMode>(it->second));
        }

        LOG(LogLevel::kWarn) << "Ignoring invalid screen mode for configuration change: " << mode;
        return *this;
    }

    /**
     * Inform that a screen reader is turned on.
     * @param enabled True if the screen reader is enabled
     * @return This object for chaining
     */
    ConfigurationChange& screenReader(bool enabled) {
        mFlags |= kConfigurationChangeScreenReader;
        mScreenReaderEnabled = enabled;
        return *this;
    }

    /**
     * Inform that a custom environment property has been modified.
     * Only additional properties not present in the initial data-binding context can be modified
     * with this method. These properties are typically provided by APL runtimes for their specific
     * platform.
     *
     * This method can be invoked multiple times to set different properties. Calling this
     * method for a property that was previously set overwrites the previous value.
     *
     * @param name The environment property name
     * @param value The updated property value
     * @return This object for chaining
     */
    ConfigurationChange& environmentValue(const std::string &name, const Object &value);

    /**
     * Merge this configuration change into a metrics object.
     * @param oldMetrics The old metrics to merge with this change
     * @return An new metrics object with these changes
     */
    Metrics mergeMetrics(const Metrics& oldMetrics) const;

    /**
     * Merge this configuration change into a root config object
     * @param oldRootConfig The old root config to merge with this change
     * @return A new root config with these changes
     */
    RootConfig mergeRootConfig(const RootConfig& oldRootConfig) const;

    /**
     * Merge a new configuration change into this one.
     * @param other The old configuration change to merge with this change
     */
    void mergeConfigurationChange(const ConfigurationChange& other);

    /**
     * Create a map of properties to add include in the onConfigChange event handler
     * @param rootConfig The current root config
     * @param metrics The current metrics
     * @return A key-value map of properties
     */
    ObjectMap asEventProperties(const RootConfig& rootConfig, const Metrics& metrics) const;

    /**
     * @return True if configuration change contains size change, false otherwise.
     */
    bool hasSizeChange() const { return (mFlags & kConfigurationChangeSize); }

    /**
     * @return New pixel size from this change.
     */
    Size getSize() const { return { static_cast<float>(mPixelWidth), static_cast<float>(mPixelHeight) }; }

    /**
     * @return True if the configuration change is empty
     */
    bool empty() const { return mFlags == 0; }

    /**
     * Clear the configuration change
     */
    void clear() { mFlags = 0; }

    /**
     * @return the full set of synthesized property names that can be added to events (e.g. "rotated").
     */
    static const std::set<std::string>& getSynthesizedPropertyNames();

private:
    enum SetFlags : unsigned int {
        kConfigurationChangeSize = 1u << 0,
        kConfigurationChangeTheme = 1u << 1,
        kConfigurationChangeViewportMode = 1u << 2,
        kConfigurationChangeScreenMode = 1u << 3,
        kConfigurationChangeFontScale = 1u << 4,
        kConfigurationChangeScreenReader = 1u << 5,
        kConfigurationChangeDisallowVideo = 1u << 6,
        kConfigurationChangeEnvironment = 1u << 7,
    };

    unsigned int mFlags = 0;

    // Metrics properties
    int mPixelWidth = 100;
    int mPixelHeight = 100;
    std::string mTheme = "dark";
    ViewportMode mViewportMode = kViewportModeHub;

    // RootConfig properties
    bool mDisallowVideo = false;
    RootConfig::ScreenMode mScreenMode = RootConfig::kScreenModeNormal;
    float mFontScale = 1.0;
    bool mScreenReaderEnabled = false;
    ObjectMap mEnvironment;
};

} // namespace apl

#endif // _APL_CONFIGURATION_CHANGE_H
