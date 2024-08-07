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

#ifndef _APL_SG_TEXT_MEASUREMENT_H
#define _APL_SG_TEXT_MEASUREMENT_H

#include "apl/component/textmeasurement.h"
#include "apl/scenegraph/common.h"

namespace apl {
namespace sg {

/**
 * Override the older TextMeasurement class
 */
class TextMeasurement : public apl::TextMeasurement {
public:

    // Viewhost to implement one of these two definitions. The method with a component pointer
    // is a temporary definition to support usage of the new TextMeasurement API before
    // implementation is fully migrated to scenegraph.
    virtual sg::TextLayoutPtr layout( const TextChunkPtr& chunk,
                                      const TextPropertiesPtr& textProperties,
                                      float width,
                                      MeasureMode widthMode,
                                      float height,
                                      MeasureMode heightMode ) { return nullptr; };

    // Expect this definition to be deprecated when Scenegraph is available.
    virtual sg::TextLayoutPtr layout(Component *component,
                                     const TextChunkPtr& chunk,
                                     const TextPropertiesPtr& textProperties,
                                     float width,
                                     MeasureMode widthMode,
                                     float height,
                                     MeasureMode heightMode ) {
        return layout(chunk, textProperties, width, widthMode, height, heightMode);
    };

    // Viewhost to implement one of these two definitions. The method with a component pointer
    // is a temporary definition to support usage of the new TextMeasurement API before
    // implementation is fully migrated to scenegraph.
    virtual sg::EditTextBoxPtr box( int size,
                                    const TextPropertiesPtr& textProperties,
                                    float width,
                                    MeasureMode widthMode,
                                    float height,
                                    MeasureMode heightMode ) { return nullptr; }

    // Expect this definition to be deprecated when Scenegraph is available.
    virtual sg::EditTextBoxPtr box(Component *component,
                                   int size,
                                   const TextPropertiesPtr& textProperties,
                                   float width,
                                   MeasureMode widthMode,
                                   float height,
                                   MeasureMode heightMode ) {
        return box(size, textProperties, width, widthMode, height, heightMode);
    };

    // These functions are provided for backwards compatibility.  They should not be used
    LayoutSize measure(Component *component,
                       float width, MeasureMode widthMode,
                       float height, MeasureMode heightMode) override { return {0,0}; }

    float baseline(Component *component, float width, float height) override { return 0; }

    bool layoutCompatible() const override { return true; }
};

} // namespace sg
} // namespace apl

#endif // _APL_SG_TEXT_MEASUREMENT_H
