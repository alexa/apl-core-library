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
#include "apl/component/component.h"

namespace apl {

/**
 * struct LayoutSize is defined to pass width and height mode.
 */
struct LayoutSize {
    float width;
    float height;
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
     * @param textMeasurement
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
};

} // namespace apl
#endif //_APL_TEXT_MEASUREMENT_H
