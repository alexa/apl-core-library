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

#include "test_sg_textmeasure.h"
#include "apl/scenegraph/textchunk.h"
#include "apl/scenegraph/textproperties.h"

sg::TextLayoutPtr
MyTestMeasurement::layout(const sg::TextChunkPtr& textChunk,
                          const sg::TextPropertiesPtr& textProperties,
                          float width,
                          MeasureMode widthMode,
                          float height,
                          MeasureMode heightMode)
{
    mLayoutCounter++;
    auto text = textChunk->styledText().asString();

    // Assume all characters are squares
    float cw = textProperties->fontSize();
    float ch = cw;

    auto layout = std::make_shared<MyTestLayout>(ch * 0.8);  // Sets the baseline

    // Break up into lines. We don't do anything clever about spaces
    std::string::size_type charactersPerLine =
        (widthMode == MeasureMode::Undefined ? std::numeric_limits<std::string::size_type>::max()
                                             : static_cast<int>(width / cw));

    std::string::size_type position = 0;
    const auto maxLines = textProperties->maxLines();
    while (position < text.size() && (maxLines == 0 || layout->getLineCount() < maxLines)) {
        auto npos = std::min(charactersPerLine, text.size() - position);
        layout->addLine(text.substr(position, npos), {cw * npos, ch});
        position += npos;
    }

    // At this point the text layout has a "minimum" size that wraps the existing
    switch (widthMode) {
        case MeasureMode::Exactly:
            layout->setWidth(width, textProperties->textAlign());
            break;
        case MeasureMode::AtMost:
            layout->setWidth(layout->getSize().getWidth(), textProperties->textAlign());
            break;
        default:  // Don't do anything here
            break;
    }

    switch (heightMode) {
        case MeasureMode::Exactly:
            layout->setHeight(height, textProperties->textAlignVertical());
            break;
        case MeasureMode::AtMost:
            layout->setHeight(std::min(layout->getSize().getHeight(), height), textProperties->textAlignVertical());
            break;
        default:
            break;
    }

    return layout;
}

float fixMeasuredDimension(float target, float specified, MeasureMode mode)
{
    switch (mode) {
        case Exactly:
            return specified;
        case Undefined:
            return target;
        case AtMost:
            return std::min(specified, target);
    }
    return 0.0f;
}

sg::EditTextBoxPtr
MyTestMeasurement::box(int size,
                       const sg::TextPropertiesPtr& textProperties,
                       float width,
                       MeasureMode widthMode,
                       float height,
                       MeasureMode heightMode)
{
    // Assume all characters are squares
    float cw = textProperties->fontSize();
    float ch = cw;

    // At this point the text layout has a "minimum" size that wraps the existing
    return std::make_shared<MyTestBox>(Size(fixMeasuredDimension(cw * size, width, widthMode),
                                            fixMeasuredDimension(ch, height, heightMode)),
                                       ch * 0.8);
}