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

#include "apl/content/rootconfig.h"
#include "apl/content/metrics.h"
#include "apl/primitives/size.h"

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
     * @param pixelWidth
     * @param pixelHeight
     */
    ConfigurationChange(int pixelWidth, int pixelHeight)
        : mFlags(kConfigurationChangeSize),
          mPixelWidth(pixelWidth),
          mPixelHeight(pixelHeight)
    {}

    /**
     * Update the size
     * @param pixelWidth
     * @param pixelHeight
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
     * Merge this configuration change into a metrics object.
     * @param oldMetrics
     * @return An new metrics object with these changes
     */
    Metrics mergeMetrics(const Metrics& oldMetrics) const;

    /**
     * Merge this configuration change into a root config object
     * @param oldRootConfig
     * @return A new root config with these changes
     */
    RootConfig mergeRootConfig(const RootConfig& oldRootConfig) const;

    /**
     * Merge this configuration change into a new size object
     * @param oldSize
     * @return A new size object with these changes
     */
    Size mergeSize(const Size& oldSize) const;

    /**
     * Merge a new configuration change into this one.
     * @param other
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
     * @return True if the configuration change is empty
     */
    bool empty() const { return mFlags == 0; }

    /**
     * Clear the configuration change
     */
    void clear() { mFlags = 0; }

private:
    enum SetFlags : unsigned int {
        kConfigurationChangeSize = 1u << 0,
        kConfigurationChangeTheme = 1u << 1,
        kConfigurationChangeViewportMode = 1u << 2,
        kConfigurationChangeScreenMode = 1u << 3,
        kConfigurationChangeFontScale = 1u << 4,
        kConfigurationChangeScreenReader = 1u << 5
    };

    unsigned int mFlags = 0;

    // Metrics properties
    int mPixelWidth = 100;
    int mPixelHeight = 100;
    std::string mTheme = "dark";
    ViewportMode mViewportMode = kViewportModeHub;

    // RootConfig properties
    RootConfig::ScreenMode mScreenMode = RootConfig::kScreenModeNormal;
    float mFontScale = 1.0;
    bool mScreenReaderEnabled = false;
};

} // namespace apl

#endif // _APL_CONFIGURATION_CHANGE_H
