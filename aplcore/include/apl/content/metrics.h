/*
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

#ifndef _APL_METRICS_H
#define _APL_METRICS_H

#include <cassert>

#include "apl/primitives/size.h"
#include "apl/utils/streamer.h"
#include "apl/utils/bimap.h"
#include "apl/utils/log.h"
#include "apl/utils/userdata.h"

namespace apl {


/**
 * Standard screen shapes.
 */
enum ScreenShape {
    RECTANGLE,
    ROUND
};

extern Bimap<int, std::string> sScreenShapeBimap;

/**
 * Standard viewport device modes
 */
enum ViewportMode {
    /// Automotive or vehicle
    kViewportModeAuto,
    /// Desktop or countertop
    kViewportModeHub,
    /// Mobile phone or tablet
    kViewportModeMobile,
    /// Desktop or laptop PC
    kViewportModePC,
    /// Television
    kViewportModeTV,
};

extern Bimap<int, std::string> sViewportModeBimap;

struct ViewportSize {
    float width, minWidth, maxWidth;
    float height, minHeight, maxHeight;

    bool isFixed() const { return minWidth == maxWidth && minHeight == maxHeight; }
    bool isAutoWidth() const { return minWidth != maxWidth; }
    bool isAutoHeight() const { return minHeight != maxHeight; }
    Size nominalSize() const { return { width, height }; }
    Size layoutSize() const {
        return {minWidth == maxWidth ? width : -1, minHeight == maxHeight ? height : -1};
    }
};

/**
 * Store information about the viewport
 */
class Metrics : public UserData<Metrics> {
public:
    static constexpr float CORE_DPI = 160.0f;

    /**
     * Construct default metrics.
     */
    Metrics() = default;

    /**
     * Set the color theme.
     * @param theme The color theme.
     * @return This object for chaining.
     */
    Metrics& theme(const char *theme) {
        mTheme = theme;
        return *this;
    }

    /**
     * Set the pixel dimensions of the screen or view.  When using auto-sizing, this
     * should be set to the nominal or target dimension of the view.
     * @param pixelWidth The width of the viewport, in pixels.
     * @param pixelHeight The height of the viewport, in pixels.
     * @return This object for chaining.
     */
    Metrics& size(int pixelWidth, int pixelHeight) {
        assert(pixelWidth > 0 && pixelHeight > 0);
        mPixelWidth = pixelWidth;
        mPixelHeight = pixelHeight;
        return *this;
    }

    /**
     * Set the minimum and maximum pixel width of the viewport.
     * @param minPixelWidth The minimum width of the viewport, in pixels
     * @param maxPixelWidth The maximum width of the viewport, in pixels
     * @return This object for chaining
     */
    Metrics& minAndMaxWidth(int minPixelWidth, int maxPixelWidth) {
        assert(minPixelWidth > 0 && minPixelWidth <= maxPixelWidth);
        mMinPixelWidth = minPixelWidth;
        mMaxPixelWidth = maxPixelWidth;
        mFlags |= kMinMaxPixelWidth;
        return *this;
    }

    /**
     * Set the minimum and maximum pixel height of the viewport.
     * @param minPixelHeight The minimum height of the viewport, in pixels
     * @param maxPixelHeight The maximum height of the viewport, in pixels
     * @return This object for chaining
     */
    Metrics& minAndMaxHeight(int minPixelHeight, int maxPixelHeight) {
        assert(minPixelHeight > 0 && minPixelHeight <= maxPixelHeight);
        mMinPixelHeight = minPixelHeight;
        mMaxPixelHeight = maxPixelHeight;
        mFlags |= kMinMaxPixelHeight;
        return *this;
    }

    /**
     * Set the dpi of the screen (display-independent pixel resolution).
     * @param dpi The dpi to set.
     * @return This object for chaining.
     */
    Metrics& dpi(int dpi) {
        assert(dpi > 0);
        mDpi = dpi;
        return *this;
    }

    /**
     * Set the shape of the screen.
     * @param shape The screen shape.
     * @return This object, for chaining.
     */
    Metrics& shape(ScreenShape shape) {
        mShape = shape;
        return *this;
    }

    /**
     * Set the shape of the screen.
     * @param screenShape The screen shape.
     * @return This object, for chaining.
     */
    Metrics& shape(const std::string& screenShape) {
        auto it = sScreenShapeBimap.find(screenShape);
        if (it != sScreenShapeBimap.endBtoA()) {
            return shape(static_cast<ScreenShape>(it->second));
        }

        LOG(LogLevel::kWarn) << "Ignoring invalid screen shape for metrics: " << screenShape;
        return *this;
    }

    /**
     * Set the operating mode of the viewport
     * @param mode The viewport mode.
     * @return This object, for chaining.
     */
    Metrics& mode(ViewportMode mode) {
        mMode = mode;
        return *this;
    }

    /**
     * Set the operating mode of the viewport
     * @param viewportMode The viewport mode.
     * @return This object, for chaining.
     */
    Metrics& mode(const std::string& viewportMode) {
        auto it = sViewportModeBimap.find(viewportMode);
        if (it != sViewportModeBimap.endBtoA()) {
            return mode(static_cast<ViewportMode>(it->second));
        }

        LOG(LogLevel::kWarn) << "Ignoring invalid viewport mode for metrics: " << viewportMode;
        return *this;
    }

    /**
     * @return The dpi of the viewport.
     */
    int getDpi() const { return mDpi; }

    /**
     * @return The complete viewport information needed for layout
     */
    ViewportSize getViewportSize() const {
        return {
            getWidth(),
            getMinWidth(),
            getMaxWidth(),
            getHeight(),
            getMinHeight(),
            getMaxHeight()
        };
    }

    /**
     * @return The height of the viewport in dp
     */
    float getHeight() const { return pxToDp(mPixelHeight); }

    /**
     * @return The width of the viewport in dp
     */
    float getWidth() const { return pxToDp(mPixelWidth); }

    /**
     * @return The minimum height of the viewport in dp
     */
    float getMinHeight() const {
        return pxToDp((mFlags & kMinMaxPixelHeight) != 0 ? mMinPixelHeight : mPixelHeight);
    }

    /**
     * @return The maximum height of the viewport in dp
     */
    float getMaxHeight() const {
        return pxToDp((mFlags & kMinMaxPixelHeight) != 0 ? mMaxPixelHeight : mPixelHeight);
    }

    /**
     * @return The minimum width of the viewport in dp
     */
    float getMinWidth() const {
        return pxToDp((mFlags & kMinMaxPixelWidth) != 0 ? mMinPixelWidth : mPixelWidth);
    }

    /**
     * @return The maximum height of the viewport in dp
     */
    float getMaxWidth() const {
        return pxToDp((mFlags & kMinMaxPixelWidth) != 0 ? mMaxPixelWidth : mPixelWidth);
    }

    /**
     * @return True if the width should auto-size
     */
    bool getAutoWidth() const {
        return (mFlags & kMinMaxPixelWidth) != 0 && mMinPixelWidth < mMaxPixelWidth;
    }

    /**
     * @return True if the height should auto-size
     */
    bool getAutoHeight() const {
        return (mFlags & kMinMaxPixelHeight) != 0 && mMinPixelHeight < mMaxPixelHeight;
    }

    /**
     * Convert Display Pixels to Pixels
     * @param dp Display Pixels
     * @return Pixels
     */
    float dpToPx(float dp) const { return dp * static_cast<float>(mDpi) / CORE_DPI; }

    /**
     * Convert Pixels to Display Pixels
     * @param px Pixels
     * @return Display Pixels
     */
    float pxToDp(float px) const { return px * CORE_DPI / static_cast<float>(mDpi); }

    /**
     * Convert pixels to display pixels
     * @param px Pixels
     * @return Display pixels
     */
    float pxToDp(int px) const { return static_cast<float>(px) * CORE_DPI / static_cast<float>(mDpi); }

    /**
     * @return The human-readable shape of the screen (either "rectangle" or "round")
     */
     const std::string& getShape() const { return sScreenShapeBimap.at(mShape); }

    /**
     * @return The screen shape
     */
    ScreenShape getScreenShape() { return mShape; }

    /**
     * @return The width of the screen, in pixels.
     */
    int getPixelWidth() const { return mPixelWidth; }

    /**
     * @return The height of the screen, in pixels.
     */
    int getPixelHeight() const { return mPixelHeight; }

    /**
     * @return The assigned color theme.
     */
    const std::string& getTheme() const { return mTheme; }

    /**
     * @return The human-readable mode of the viewport
     */
    const std::string& getMode() const { return sViewportModeBimap.at(mMode); }

    /**
     * @return The viewport mode
     */
    ViewportMode getViewportMode() const { return mMode; }

    std::string toDebugString() const;

private:

    enum SetFlags : unsigned int {
        kMinMaxPixelWidth = 1u << 0,
        kMinMaxPixelHeight = 1u << 1,
    };

    std::string mTheme = "dark";
    int mPixelWidth = 1024;
    int mPixelHeight = 800;
    int mDpi = CORE_DPI;
    ScreenShape mShape = RECTANGLE;
    ViewportMode mMode = kViewportModeHub;

    int mMinPixelWidth = 1024;
    int mMaxPixelWidth = 1024;
    int mMinPixelHeight = 800;
    int mMaxPixelHeight = 800;

    unsigned int mFlags = 0;
};

}  // namespace apl

#endif // _APL_METRICS_H
