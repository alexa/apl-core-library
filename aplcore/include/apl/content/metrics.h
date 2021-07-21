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

#include "apl/utils/streamer.h"
#include "apl/utils/bimap.h"
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

/**
 * Store information about the viewport
 */
class Metrics : public UserData<Metrics> {
public:

    /**
     * Construct default metrics.
     */
    Metrics() :
        mTheme("dark"),
        mPixelWidth(1024),
        mPixelHeight(800),
        mDpi(160),
        mShape(RECTANGLE),
        mMode(kViewportModeHub)
    {
    }

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
     * Set the pixel dimensions of the screen.
     * @param pixelWidth The width of the screen, in pixels.
     * @param pixelHeight The height of the screen, in pixels.
     * @return This object for chaining.
     */
    Metrics& size(int pixelWidth, int pixelHeight) {
        mPixelWidth = pixelWidth;
        mPixelHeight = pixelHeight;
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
     * Set the operating mode of the viewport
     * @param mode The viewport mode.
     * @return This object, for chaining.
     */
    Metrics& mode(ViewportMode mode) {
        mMode = mode;
        return *this;
    }

    /**
     * @return The dpi of the screen.
     */
    int getDpi() const { return mDpi; }

    /**
     * @return The height of the screen in "dp"
     */
    float getHeight() const { return pxToDp(mPixelHeight); }

    /**
     * @return The width of the screen in "dp"
     */
    float getWidth() const { return pxToDp(mPixelWidth); }

    /**
     * Convert Display Pixels to Pixels
     * @param dp Display Pixels
     * @return Pixels
     */
    float dpToPx(float dp) const { return dp * mDpi / 160.0f; }

    /**
     * Convert Pixels to Display Pixels
     * @param px Pixels
     * @return Display Pixels
     */
    float pxToDp(float px) const { return px * 160.0f / mDpi; }

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
    std::string mTheme;
    int mPixelWidth;
    int mPixelHeight;
    int mDpi;
    ScreenShape mShape;
    ViewportMode mMode;
};

}  // namespace apl

#endif // _APL_METRICS_H
