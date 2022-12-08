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

#ifndef _APL_TEXT_MEASUREMENT_H
#define _APL_TEXT_MEASUREMENT_H

#include <memory>

#include "apl/apl_config.h"
#include "apl/common.h"

namespace apl {

struct LayoutSize {
    float width;
    float height;
};

/**
 * Modes to measure layout size in TextMeasurement class
 */
enum MeasureMode {
    Undefined,
    Exactly,
    AtMost
};

/**
 * Convenience structure for storing and passing around measurement requests
 */
class MeasureRequest {
public:
    MeasureRequest() = default;
    MeasureRequest(float width,
                   MeasureMode widthMode,
                   float height,
                   MeasureMode heightMode)
        : mWidth(width),
          mWidthMode(widthMode),
          mHeight(height),
          mHeightMode(heightMode) {}

    bool isExact() const { return mWidthMode == Exactly && mHeightMode == Exactly; }

    float width() const { return mWidth; }
    float height() const { return mHeight; }

    MeasureMode widthMode() const { return mWidthMode; }
    MeasureMode heightMode() const { return mHeightMode; }

    friend bool operator==(const MeasureRequest& lhs, const MeasureRequest& rhs)  {
        return lhs.mWidth == rhs.mWidth && lhs.mWidthMode == rhs.mWidthMode &&
        lhs.mHeight == rhs.mHeight && lhs.mHeightMode == rhs.mHeightMode;
    }

    friend bool operator!=(const MeasureRequest& lhs, const MeasureRequest& rhs)  {
        return lhs.mWidth != rhs.mWidth || lhs.mWidthMode != rhs.mWidthMode ||
               lhs.mHeight != rhs.mHeight || lhs.mHeightMode != rhs.mHeightMode;
    }

private:
    float mWidth = 0;
    MeasureMode mWidthMode = Undefined;
    float mHeight = 0;
    MeasureMode mHeightMode = Undefined;
};

/**
 * Abstract class for measuring text.  Override this in your platform-specific
 * runtime and install your custom class.
 *
 * Note: To prevent odd breakages, we require the user to pass a shared_ptr
 * to the TextMeasurement object. This will be copied into the root context
 * when inflating a layout, so you can't change the measurement tool for an
 * inflated layout.
 */
class TextMeasurement {
public:
    /**
     * Install a TextMeasurement object.  This will be used for all future
     * layout calculations.
     * @param textMeasurement The TextMeasurement object
     */
    static void install(const TextMeasurementPtr& textMeasurement);
    static const TextMeasurementPtr& instance();

    virtual ~TextMeasurement() = default;

    virtual LayoutSize measure( Component *component,
                                float width,
                                MeasureMode widthMode,
                                float height,
                                MeasureMode heightMode ) = 0;

    virtual float baseline( Component *component,
                            float width,
                            float height ) = 0;

#ifdef SCENEGRAPH
    virtual bool sceneGraphCompatible() const { return false; }
#endif // SCENEGRAPH
};

} // namespace apl
#endif //_APL_TEXT_MEASUREMENT_H
