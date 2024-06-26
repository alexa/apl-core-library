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

#ifndef _APL_TEST_SG_TEXTMEASURE_H
#define _APL_TEST_SG_TEXTMEASURE_H

#include "gtest/gtest.h"
#include "apl/component/component.h"
#include "apl/component/componentproperties.h"
#include "apl/scenegraph/edittextbox.h"
#include "apl/scenegraph/textlayout.h"
#include "apl/scenegraph/textmeasurement.h"

using namespace apl;

class MyTestLayout : public sg::TextLayout {
    struct Line {
        std::string text;
        Rect rect;
        Range range;
    };

public:
    ~MyTestLayout() override = default;

    explicit MyTestLayout(float baseline)
        : mBaseline(baseline),
          mTruncated(false)
    {}

    // Add an additional line of text with a given size.
    // The next line is placed below the previous lines
    void addLine(const std::string& text, Size size) {
        auto start = static_cast<int>(mText.size());
        auto end = start + static_cast<int>(text.size()) - 1;
        mLines.emplace_back(
            Line{text, {0, mSize.getHeight(), size.getWidth(), size.getHeight()}, {start, end}});

        auto width = std::max(mSize.getWidth(), size.getWidth());
        auto height = mSize.getHeight() + size.getHeight();
        mSize = {width, height};
        mText += text;
    }

    // Update the width of the layout and adjust the horizontal alignment of each line
    void setWidth(float width, TextAlign align) {
        switch (align) {
            case kTextAlignAuto:  // We'll assume LTR languages for this testing
            case kTextAlignLeft:  // The text boxes are by default aligned left, so there is nothing to do
            case kTextAlignStart:
                break;
            case kTextAlignCenter:
                for (auto& line : mLines) {
                    auto dx = width / 2 - line.rect.getCenterX();
                    line.rect.offset(dx, 0);
                }
                break;
            case kTextAlignRight:
            case kTextAlignEnd:
                for (auto& line : mLines) {
                    auto dx = width - line.rect.getRight();
                    line.rect.offset(dx, 0);
                }
                break;
        }

        mSize = { width, mSize.getHeight() };
    }

    // Update the height of the layout and adjust the vertical alignment of each line
    void setHeight(float height, TextAlignVertical align) {
        switch (align) {
            case kTextAlignVerticalAuto:  // Assume default top alignment
            case kTextAlignVerticalTop:   // Nothing to do here
                break;
            case kTextAlignVerticalCenter:
                if (mLines.size() > 1) {
                    auto coveringRect = mLines.end()->rect.extend(mLines.begin()->rect);
                    auto dy = height / 2 - coveringRect.getCenterY();
                    for (auto& line : mLines)
                        line.rect.offset(0, dy);
                }
                break;
            case kTextAlignVerticalBottom:
                if (mLines.size() > 1) {
                    auto coveringRect = mLines.end()->rect.extend(mLines.begin()->rect);
                    auto dy = height - coveringRect.getBottom();
                    for (auto& line : mLines)
                        line.rect.offset(0, dy);
                }
                break;
        }

        if (height < mSize.getHeight()) mTruncated = true;

        mSize = { mSize.getWidth(), height };
    }

    bool empty() const override { return mText.empty(); }
    Size getSize() const override { return mSize; }
    float getBaseline() const override { return mBaseline; }
    int getLineCount() const override { return static_cast<int>(mLines.size()); }
    std::string toDebugString() const override { return mText; }
    unsigned int getByteLength() const override { return mText.size(); }

    Range getLineRangeFromByteRange(Range byteRange) const override {
        if (byteRange.empty())
            return {};

        auto it_lower = std::lower_bound(
            mLines.begin(), mLines.end(), byteRange.lowerBound(),
            [](const Line& line, int bottom) { return line.range.upperBound() < bottom; });

        if (it_lower == mLines.end())
            return {};

        auto it_upper = std::upper_bound(
            it_lower, mLines.end(), byteRange.upperBound(),
            [](int top, const Line& line) { return top < line.range.lowerBound(); });

        return {static_cast<int>(std::distance(mLines.begin(), it_lower)),
                static_cast<int>(std::distance(mLines.begin(), it_upper)) - 1};
    }

    Rect getBoundingBoxForLines(Range lineRange) const override {
        if (mLines.empty())
            return {};

        auto range = Range(0, mLines.size() - 1);
        if (!lineRange.empty())
            range = range.intersectWith(lineRange);

        Rect result;
        for (auto line : range)
            result = result.extend(mLines.at(line).rect);

        return result;
    }

    std::string getLaidOutText() const override {
        std::string result;
        for (auto& line : mLines) {
            if (line.rect.getY() + line.rect.getHeight() > mSize.getHeight()) break;
            result.append(line.text);
        }

        return result;
    }
    bool isTruncated() const override { return mTruncated; }

private:
    std::string mText;
    Size mSize;
    float mBaseline;
    std::vector<Line> mLines;
    bool mTruncated;
};

class MyTestBox : public sg::EditTextBox {
public:
    MyTestBox(Size size, float baseline)
        : mSize(size), mBaseline(baseline) {}

    Size getSize() const override { return mSize; }
    float getBaseline() const override { return mBaseline; }

private:
    Size mSize;
    float mBaseline;
};

/**
 * Fake text measurement logic.  Each character is assumed to be a square the size of the font
 */
class MyTestMeasurement : public sg::TextMeasurement {
public:
    ~MyTestMeasurement() override = default;

    sg::TextLayoutPtr layout(const sg::TextChunkPtr& textChunk,
                             const sg::TextPropertiesPtr& textProperties, float width,
                             MeasureMode widthMode, float height, MeasureMode heightMode) override;

    sg::EditTextBoxPtr box(int size, const sg::TextPropertiesPtr& textProperties, float width,
                            MeasureMode widthMode, float height, MeasureMode heightMode) override;

    /**
     * @return Number of times layout was requested
     */
    int getLayoutCount() const { return mLayoutCounter; }

private:
    int mLayoutCounter = 0;
};

/**
 * Mimics a viewhost that wants to store Layout objects in the Component UserData. This is a
 * demonstration of how viewhosts may want to behave while this API is available but scenegraph
 * migration has not been completed.
 */
class LayoutReuseMeasurement : public MyTestMeasurement {
    using MyTestMeasurement::layout;
    using MyTestMeasurement::box;

    sg::TextLayoutPtr layout(Component *component,
                              const sg::TextChunkPtr& chunk,
                              const sg::TextPropertiesPtr& textProperties,
                              float width,
                              MeasureMode widthMode,
                              float height,
                              MeasureMode heightMode) override {
        auto textLayout = layout(chunk, textProperties, width, widthMode, height, heightMode);
        component->setUserData(textLayout.get());
        return textLayout;
    }

    sg::EditTextBoxPtr box(Component *component,
                            int size,
                            const sg::TextPropertiesPtr& textProperties,
                            float width,
                            MeasureMode widthMode,
                            float height,
                            MeasureMode heightMode) override {
        auto editBox = box(size, textProperties, width, widthMode, height, heightMode);
        component->setUserData(editBox.get());
        return editBox;
    }
};

#endif // _APL_TEST_SG_TEXTMEASURE_H
