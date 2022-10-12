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

#ifndef _APL_TEST_SG_H
#define _APL_TEST_SG_H

#include "gtest/gtest.h"
#include "apl/scenegraph/accessibility.h"
#include "apl/scenegraph/filter.h"
#include "apl/scenegraph/layer.h"
#include "apl/scenegraph/node.h"
#include "apl/scenegraph/paint.h"
#include "apl/scenegraph/path.h"
#include "apl/scenegraph/pathop.h"
#include "apl/scenegraph/scenegraph.h"
#include "apl/scenegraph/edittextbox.h"
#include "apl/scenegraph/textlayout.h"
#include "apl/scenegraph/textmeasurement.h"

using namespace apl;

using FilterTest = std::function<::testing::AssertionResult(sg::FilterPtr)>;
using LayerTest = std::function<::testing::AssertionResult(sg::LayerPtr)>;
using NodeTest = std::function<::testing::AssertionResult(sg::NodePtr)>;
using PaintTest = std::function<::testing::AssertionResult(sg::PaintPtr)>;
using PathTest = std::function<::testing::AssertionResult(sg::PathPtr)>;
using PathOpTest = std::function<::testing::AssertionResult(sg::PathOpPtr)>;
using ShadowTest = std::function<::testing::AssertionResult(sg::ShadowPtr)>;
using AccessibilityTest = std::function<::testing::AssertionResult(sg::AccessibilityPtr)>;


PaintTest IsColorPaint( Color color, float opacity=1.0f, const std::string& msg="" );
PaintTest IsLinearGradientPaint( std::vector<double> points,
                                 std::vector<Color> colors,
                                 Gradient::GradientSpreadMethod spreadMethod,
                                 bool useBoundingBox,
                                 Point start,
                                 Point end,
                                 float opacity=1.0f,
                                 Transform2D transform=Transform2D(),
                                 const std::string& msg="");
PaintTest IsRadialGradientPaint( std::vector<double> points,
                                 std::vector<Color> colors,
                                 Gradient::GradientSpreadMethod spreadMethod,
                                 bool useBoundingBox,
                                 Point center,
                                 float radius,
                                 float opacity=1.0f,
                                 Transform2D transform=Transform2D(),
                                 const std::string& msg="");
PaintTest IsPatternPaint( Size size,
                          NodeTest nodeTest,
                          float opacity=1.0f,
                          Transform2D transform=Transform2D(),
                          const std::string& msg="");

PathTest IsRectPath( Rect rect, const std::string& msg="" );
PathTest IsRectPath( float x, float y, float width, float height, const std::string& msg="" );
PathTest IsRoundRectPath( RoundedRect rrect, const std::string& msg="" );
PathTest IsRoundRectPath( float x, float y, float width, float height, float radius, const std::string& msg="" );
PathTest IsFramePath( RoundedRect rrect, float inset, const std::string& msg="" );
PathTest IsGeneralPath( std::string value, std::vector<float> points, const std::string& msg="");

PathOpTest IsStrokeOp( PaintTest paintTest, float strokeWidth, const std::string& msg="" );
PathOpTest IsStrokeOp( PaintTest paintTest,
           float strokeWidth, float miterLimit, float pathLength, float dashOffset,
           GraphicLineCap lineCap, GraphicLineJoin lineJoin, std::vector<float> dashes, const std::string& msg="");
PathOpTest IsFillOp( PaintTest paintTest, const std::string& msg="" );

ShadowTest IsShadow( Color color, Point offset, float radius, const std::string& msg="");

class IsAccessibility {
public:
    IsAccessibility(std::string msg="") : mMsg(std::move(msg)) {}
    IsAccessibility& label(std::string label) { mLabel = std::move(label); return *this; }
    IsAccessibility& role(Role role) { mRole = role; return *this; }

    IsAccessibility& action(const std::string& name, const std::string& label, bool enabled) {
        mActions.emplace_back(sg::Accessibility::Action{name, label, enabled});
        return *this;
    }

    ::testing::AssertionResult operator()(sg::AccessibilityPtr accessibility);

private:
    std::string mMsg;
    std::string mLabel;
    Role mRole = apl::kRoleNone;
    std::vector<sg::Accessibility::Action> mActions;
};

class IsNode {
public:
    IsNode(sg::Node::Type type, std::string msg) : mType(type), mMsg(std::move(msg)) {}
    ::testing::AssertionResult CheckBase(sg::NodePtr node);
    ::testing::AssertionResult CheckChildren(sg::NodePtr node);

protected:
    sg::Node::Type mType;
    std::string mMsg;
    std::vector<NodeTest> mChildTests;
};

template<class T>
class IsWrapper : public IsNode {
public:
    IsWrapper(sg::Node::Type type, std::string msg) : IsNode(type, std::move(msg)) {}
    T& child(NodeTest test) { mChildTests.emplace_back(std::move(test)); return static_cast<T&>(*this); }
    T& children(std::vector<NodeTest> nodeTests) { mChildTests = std::move(nodeTests); return static_cast<T&>(*this); }
};

class IsGenericNode : public IsWrapper<IsGenericNode> {
public:
    explicit IsGenericNode(std::string msg="") : IsWrapper(sg::Node::kGeneric, std::move(msg)) {}
    IsGenericNode(std::vector<NodeTest> nodeTests, std::string msg="")
        : IsWrapper(sg::Node::kGeneric, std::move(msg)) {
        mChildTests = std::move(nodeTests);
    }

    ::testing::AssertionResult operator()(sg::NodePtr node);
};

class IsClipNode : public IsWrapper<IsClipNode> {
public:
    explicit IsClipNode(std::string msg="") : IsWrapper(sg::Node::kClip, std::move(msg)) {}
    IsClipNode& path(PathTest pathTest) { mPathTest = pathTest; return *this; }

    ::testing::AssertionResult operator()(sg::NodePtr node);
private:
    PathTest mPathTest;
};

class IsOpacityNode : public IsWrapper<IsOpacityNode> {
public:
    explicit IsOpacityNode(std::string msg="") : IsWrapper(sg::Node::kOpacity, std::move(msg)) {}
    IsOpacityNode& opacity(float opacity) { mOpacity = opacity; return *this; }

    ::testing::AssertionResult operator()(sg::NodePtr node);
private:
    float mOpacity = 1.0f;
};

class IsTransformNode : public IsWrapper<IsTransformNode> {
public:
    explicit IsTransformNode(std::string msg="") : IsWrapper(sg::Node::kTransform, std::move(msg)) {}
    IsTransformNode& transform(Transform2D transform) { mTransform = transform; return *this; }
    IsTransformNode& translate(Point point) { mTransform = Transform2D::translate(point); return *this; }

    ::testing::AssertionResult operator()(sg::NodePtr node);
private:
    Transform2D mTransform;
};

class IsDrawNode : public IsWrapper<IsDrawNode> {
public:
    explicit IsDrawNode(std::string msg="") : IsWrapper(sg::Node::kDraw, std::move(msg)) {}

    IsDrawNode& path(PathTest pathTest) { mPathTest = pathTest; return *this; }
    IsDrawNode& pathOp(PathOpTest pathOpTest) { mPathOpTests.emplace_back(std::move(pathOpTest)); return *this; }
    IsDrawNode& pathOp(std::vector<PathOpTest> pathOpTests) { mPathOpTests = pathOpTests; return *this; }

    ::testing::AssertionResult operator()(sg::NodePtr node);
private:
    PathTest mPathTest;
    std::vector<PathOpTest> mPathOpTests;
};

class IsEditNode : public IsWrapper<IsEditNode> {
public:
    explicit IsEditNode(std::string msg="") : IsWrapper(sg::Node::kEditText, std::move(msg)) {}

    IsEditNode& text(std::string text) { mText = std::move(text); return *this; }
    IsEditNode& color(Color color) { mColor = color; return *this; }

    ::testing::AssertionResult operator()(sg::NodePtr node);

private:
    std::string mText;
    Color mColor;
};

class IsTextNode : public IsWrapper<IsTextNode> {
public:
    explicit IsTextNode(std::string msg="") : IsWrapper(sg::Node::kText, std::move(msg)) {}

    IsTextNode& text(std::string text) { mText = std::move(text); return *this; }
    IsTextNode& pathOp(PathOpTest pathOpTest) { mPathOpTests.emplace_back(std::move(pathOpTest)); return *this; }
    IsTextNode& pathOp(std::vector<PathOpTest> pathOpTests) { mPathOpTests = pathOpTests; return *this; }
    IsTextNode& range(Range range) { mRange = range; return *this; }
    IsTextNode& measuredSize(Size size) { mMeasuredSize = size; return *this; }

    ::testing::AssertionResult operator()(sg::NodePtr node);
private:
    std::string mText;
    std::vector<PathOpTest> mPathOpTests;
    Range mRange;
    Size mMeasuredSize;
};

class IsImageNode : public IsWrapper<IsImageNode> {
public:
    explicit IsImageNode(std::string msg="") : IsWrapper(sg::Node::kImage, std::move(msg)) {}

    IsImageNode& filterTest(FilterTest filterTest) { mFilterTest = std::move(filterTest); return *this; }
    IsImageNode& target(Rect target) { mTarget = target; return *this; }
    IsImageNode& source(Rect source) { mSource = source; return *this; }

    ::testing::AssertionResult operator()(sg::NodePtr node);
private:
    FilterTest mFilterTest;
    Rect mTarget;
    Rect mSource;
};

class IsVideoNode : public IsWrapper<IsVideoNode> {
public:
    explicit IsVideoNode(std::string msg="") : IsWrapper(sg::Node::kVideo, std::move(msg)) {}

    IsVideoNode& url(std::string url) { mURL = std::move(url); return *this; }
    IsVideoNode& scale(VideoScale scale) { mScale = scale; return *this; }
    IsVideoNode& target(Rect target) { mTarget = target; return *this; }

    ::testing::AssertionResult operator()(sg::NodePtr node);
private:
    std::string mURL;
    Rect mTarget;
    VideoScale mScale = apl::kVideoScaleBestFit;
};

class IsShadowNode : public IsWrapper<IsShadowNode> {
public:
    explicit IsShadowNode(std::string msg="") : IsWrapper(sg::Node::kShadow, std::move(msg)) {}

    IsShadowNode& shadowTest(ShadowTest shadowTest) { mShadowTest = std::move(shadowTest); return *this; }

    ::testing::AssertionResult operator()(sg::NodePtr node);
private:
    ShadowTest mShadowTest;
};


FilterTest IsBlendFilter( FilterTest backTest, FilterTest frontTest, BlendMode blendMode, const std::string& msg="");
FilterTest IsBlurFilter( FilterTest filterTest, float radius, const std::string& msg="");
FilterTest IsGrayscaleFilter( FilterTest filterTest, float amount, const std::string& msg="");
FilterTest IsMediaObjectFilter( std::string url, MediaObject::State state=MediaObject::State::kReady, const std::string& msg="");
FilterTest IsNoiseFilter( FilterTest filterTest, NoiseFilterKind kind, bool useColor, float sigma, const std::string& msg="");
FilterTest IsSaturateFilter( FilterTest filterTest, float amount, const std::string& msg="");
FilterTest IsSolidFilter( PaintTest paintTest, const std::string& msg="" );


class IsLayer {
public:
    IsLayer( Rect bounds, std::string msg="")
        : mBounds(bounds), mMsg(std::move(msg)) {}

    IsLayer& shadow(ShadowTest test) { mShadowTest = std::move(test); return *this; }
    IsLayer& outline(PathTest test) { mOutlineTest = std::move(test); return *this; }
    IsLayer& childClip(PathTest test) { mChildClipTest = std::move(test); return *this; }
    IsLayer& transform(Transform2D transform) { mTransform = std::move(transform); return *this; }
    IsLayer& childOffset(Point offset) { mChildOffset = std::move(offset); return *this; }
    IsLayer& opacity(float opacity) { mOpacity = opacity; return *this; }
    IsLayer& accessibility(AccessibilityTest test) { mAccessibilityTest = std::move(test); return *this; }

    IsLayer& disabled() { mInteraction |= sg::Layer::kInteractionDisabled; return *this; }
    IsLayer& checked() { mInteraction |= sg::Layer::kInteractionChecked; return *this; }
    IsLayer& pressable() { mInteraction |= sg::Layer::kInteractionPressable; return *this; }
    IsLayer& horizontal() { mInteraction |= sg::Layer::kInteractionScrollHorizontal; return *this; }
    IsLayer& vertical() { mInteraction |= sg::Layer::kInteractionScrollVertical; return *this; }

    IsLayer& content(NodeTest test) { mContentTests.emplace_back(std::move(test)); return *this; }
    IsLayer& child(LayerTest test) { mLayerTests.emplace_back(std::move(test)); return *this; }

    IsLayer& contents(std::vector<NodeTest> tests) { mContentTests = std::move(tests); return *this; }
    IsLayer& children(std::vector<LayerTest> tests) { mLayerTests = std::move(tests); return *this; }

    IsLayer& dirty(unsigned flags) { mDirtyFlags = flags; return *this; }

    ::testing::AssertionResult operator()(sg::LayerPtr layer);

private:
    Rect mBounds;
    ShadowTest mShadowTest;
    AccessibilityTest mAccessibilityTest;
    PathTest mOutlineTest;
    PathTest mChildClipTest;
    Transform2D mTransform;
    Point mChildOffset;
    float mOpacity = 1.0f;
    std::vector<NodeTest> mContentTests;
    std::vector<LayerTest> mLayerTests;
    std::string mMsg;
    unsigned mDirtyFlags = 0;
    unsigned mInteraction = 0;
};


::testing::AssertionResult CheckSceneGraph(const sg::SceneGraphPtr& sg, const LayerTest& layerTest);
::testing::AssertionResult CheckSceneGraph(sg::SceneGraphUpdates& updates,
                const sg::NodePtr& node, const NodeTest& nodeTest);

void DumpSceneGraph(const sg::PaintPtr& ptr, int inset=0);
void DumpSceneGraph(const sg::PathOpPtr& ptr, int inset=0);
void DumpSceneGraph(const sg::PathPtr& ptr, int inset=0);
void DumpSceneGraph(const sg::NodePtr& ptr, int inset=0);
void DumpSceneGraph(const sg::FilterPtr& ptr, int inset=0);
void DumpSceneGraph(const sg::SceneGraphPtr& ptr);

// Test classes for text layout

class MyTestLayout : public sg::TextLayout {
    struct Line {
        std::string text;
        Rect rect;
        Range range;
    };

public:
    ~MyTestLayout() override = default;

    explicit MyTestLayout(float baseline)
        : mBaseline(baseline)
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
        Rect result;
        for (auto line : lineRange)
            result = result.extend(mLines.at(line).rect);

        return result;
    }

private:
    std::string mText;
    Size mSize;
    float mBaseline;
    std::vector<Line> mLines;
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
};


#endif // _APL_TEST_SG_H
