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

#include "apl/component/textmeasurement.h"

#include "apl/scenegraph/textlayout.h"
#include "apl/scenegraph/edittextbox.h"

namespace apl {

class DummyTextMeasurement : public TextMeasurement {
public:
    class DummyTextLayout : public sg::TextLayout {
    public:
        explicit DummyTextLayout(float height): mHeight(height) {}
        bool empty() const override { return false; }
        Size getSize() const override { return {10, 10}; }
        float getBaseline() const override { return mHeight * 0.5f; }
        int getLineCount() const override { return 1; }
        std::string toDebugString() const override { return ""; }
        unsigned int getByteLength() const override { return 1; }
        Range getLineRangeFromByteRange(Range byteRange) const override { return {}; }
        Rect getBoundingBoxForLines(Range lineRange) const override { return {}; }
        std::string getLaidOutText() const override { return ""; }
        bool isTruncated() const override { return false; }

    private:
        float mHeight;
    };

    class DummyTextBox : public sg::EditTextBox {
    public:
        explicit DummyTextBox(float height): mHeight(height) {}
        Size getSize() const override { return {10, 10}; }
        float getBaseline() const override { return mHeight * 0.5f; }

    private:
        float mHeight;
    };

    sg::TextLayoutPtr layout(Component *component,
                             const sg::TextChunkPtr& chunk,
                             const sg::TextPropertiesPtr& textProperties,
                             float width,
                             MeasureMode widthMode,
                             float height,
                             MeasureMode heightMode) override {
        return std::make_shared<DummyTextLayout>(height);
    }

    sg::EditTextBoxPtr box(Component *component,
                           int size,
                           const sg::TextPropertiesPtr& textProperties,
                           float width,
                           MeasureMode widthMode,
                           float height,
                           MeasureMode heightMode) override {
        return std::make_shared<DummyTextBox>(height);
    }
};

/**
 * By design, this is a global. We expect a single TextMeasurement to be shared
 * across all root contexts.
 */
static TextMeasurementPtr sTextMeasurement = std::make_shared<DummyTextMeasurement>();

void
TextMeasurement::install(const TextMeasurementPtr& textMeasurement)
{
    sTextMeasurement = textMeasurement;
}

const TextMeasurementPtr&
TextMeasurement::instance()
{
    return sTextMeasurement;
}

} // namespace apl
