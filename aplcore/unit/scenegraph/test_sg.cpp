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

#include "test_sg.h"
#include "../test_comparisons.h"

#include "apl/media/mediaobject.h"
#include "apl/scenegraph/edittextconfig.h"
#include "apl/scenegraph/edittext.h"
#include "apl/scenegraph/textchunk.h"
#include "apl/scenegraph/textproperties.h"

/**
 * Template to convert an array of objects into a single string, where each object
 * is converted into a string using the "convert" method.  This is useful for printing
 * debugging information
 */
template<typename T>
std::string asString(const std::vector<T>& array, std::function<std::string(T)> convert, std::string join = ",")
{
    std::string result;
    auto len = array.size();
    if (len)
        result = convert(array.at(0));

    for (int i = 1 ; i < len ; i++)
        result += join + convert(array.at(i));

    return result;
}

/**
 * Convert an array of objects into a single comma-separated string.  These objects must
 * be convertable into a std::string using the std::to_string<T>(x) function.
 */
template<typename T>
std::string asArray(const std::vector<T>& array) {
    return asString<T>(array, static_cast<std::string(*)(T)>(std::to_string));
}

/**
 * Convert an array of color values into a comma-separated string
 */
inline std::string asColorArray(const std::vector<Color>& array) {
    return asString<Color>(array, [](Color color) { return color.asString(); });
}


/*
 * Macro for testing a condition and returning the ::testing::AssertionFailure() from the
 * function if it, in fact, fails.  Use this inside of larger testing functions.
 */
#define SGASSERT(condition, msg) \
do {                             \
auto _result = (condition);  \
if (!_result)                \
return _result << (msg);   \
} while(0)

::testing::AssertionResult
CheckTrue(bool value, const char *name)
{
    if (!value)
        return ::testing::AssertionFailure() << name << " expected true";
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult
CheckFalse(bool value, const char *name)
{
    if (value)
        return ::testing::AssertionFailure() << name << " expected false";
    return ::testing::AssertionSuccess();
}

template<typename T>
::testing::AssertionResult
CheckNotNull(T actual, const char *name)
{
    if (!actual)
        return ::testing::AssertionFailure() << name << " is null";
    return ::testing::AssertionSuccess();
}


/**
 * Template to check a basic item that supports being written directly to the failure stream.
  */
template<typename T>
::testing::AssertionResult
CompareBasic(T actual, T expected, const char *name)
{
    if (actual != expected)
        return ::testing::AssertionFailure() << name << " mismatch; actual=" << actual << " expected=" << expected;
    return ::testing::AssertionSuccess();
}

/**
 * Template to check an item with a member function that converts it to a string
 */
template<typename T>
::testing::AssertionResult
CompareGeneral(T actual, T expected, const char *name, std::string (T::*f)() const)
{
    if (actual != expected)
        return ::testing::AssertionFailure() << name << " mismatch; actual=" << (actual.*f)()
                                               << " expected=" << (expected.*f)();
    return ::testing::AssertionSuccess();
}

/**
 * Template to check an item passing a function that converts it to a string
 */
template<typename T>
::testing::AssertionResult
CompareGeneral(T actual, T expected, const char *name, std::string (*f)(const T&))
{
    if (actual != expected)
        return ::testing::AssertionFailure() << name << " mismatch; actual=" << (*f)(actual)
                                             << " expected=" << (*f)(expected);
    return ::testing::AssertionSuccess();
}

template<typename T>
::testing::AssertionResult
CompareNumericArray(const std::vector<T>& actual, const std::vector<T>& expected,
                    const char *name, T epsilon=1e-3)
{
    if (actual.size() != expected.size())
        return ::testing::AssertionFailure() << name << " mismatch size; actual=" << asArray(actual)
                                             << " expected=" << asArray(expected);

    for (size_t i = 0; i < actual.size(); i++)
        if (std::abs(actual.at(i) - expected.at(i)) > epsilon)
            return ::testing::AssertionFailure()
                   << name << " mismatched elements at index=" << i << " actual=" << asArray(actual)
                   << " expected=" << asArray(expected);

    return ::testing::AssertionSuccess();
}

/**
 * Template to check an item with a "toDebugString()" method.
 */
template<typename T>
::testing::AssertionResult
CompareDebug(T actual, T expected, const char *name)
{
    return CompareGeneral(actual, expected, name, &T::toDebugString);
}

template<typename T>
::testing::AssertionResult
CompareDebug(const std::vector<T>& actual, const std::vector<T>& expected, const char *name)
{
    if (actual.size() != expected.size())
        return ::testing::AssertionFailure() << name << " count=" << actual.size()
                                             << " does not match expected count=" << expected.size();

    for (auto i = 0; i < actual.size(); i++) {
        auto result = CompareGeneral(actual.at(i), expected.at(i), name, &T::toDebugString);
        if (!result)
            return result << " mismatch " << name << " index=" << i;
    }

    return ::testing::AssertionSuccess();
}

/**
 * Template to check a vector of items with a "toDebugString" method
 */
template<class ItemType, class TestType>
::testing::AssertionResult
CompareDebug(const std::vector<ItemType>& items, const std::vector<TestType>& tests, const char *name) {
    if (items.size() != tests.size())
        return ::testing::AssertionFailure() << name << " count=" << items.size()
                                             << " does not match expected count=" << tests.size();

    for (auto i = 0; i < items.size(); i++) {
        auto result = tests.at(i)(items.at(i));
        if (!result)
            return result << " mismatch " << name << " index=" << i;
    }

    return ::testing::AssertionSuccess();
}

/**
 * Template to compare the equality of two enumerated items.
 */
template<typename T>
::testing::AssertionResult
CompareEnum(T actual, T expected, const Bimap<int, std::string>& bimap, const char *name)
{
    if (actual != expected)
        return ::testing::AssertionFailure() << name << " mismatch; actual=" << bimap.at(actual)
        << " expected=" << bimap.at(expected);
    return ::testing::AssertionSuccess();
}

/**
 * Template to check a vector of visible items
 */
template<class ItemType, class TestType>
::testing::AssertionResult
CompareVisible(const std::vector<ItemType>& items, const std::vector<TestType>& tests, const char *name) {
    std::vector<ItemType> visible;
    for (const auto& m : items)
        if (m->visible())
            visible.push_back(m);

    if (visible.size() != tests.size())
        return ::testing::AssertionFailure() << name << " visible count=" << visible.size()
                                             << " does not match expected count=" << tests.size();

    for (auto i = 0; i < visible.size(); i++) {
        auto result = tests.at(i)(visible.at(i));
        if (!result)
            return result << " mismatch " << name << " index=" << i;
    }

    return ::testing::AssertionSuccess();
}

/**
 * Template to check a single visible item
 */
template<class ItemType, class TestType>
::testing::AssertionResult
CompareVisible(ItemType item, TestType test, const char *name) {
    if (test != nullptr) {
        if (item == nullptr || !item->visible())
            return ::testing::AssertionFailure() << name << " expected to find a value";

        return test(item);
    }

    if (item != nullptr && item->visible())
        return ::testing::AssertionFailure() << name << " found a value when not expected";

    return ::testing::AssertionSuccess();
}

/**
 * Template to check an item that may or may not exist
 */

template<class ItemType, class TestType>
::testing::AssertionResult
CompareOptional(ItemType item, TestType test, const char *name) {
    if (test != nullptr) {
        if (item == nullptr)
            return ::testing::AssertionFailure() << name << " expected to find a value";
        return test(item);
    }

    if (item != nullptr)
        return ::testing::AssertionFailure() << name << " found a value when not expected";

    // Both null
    return ::testing::AssertionSuccess();
}

/**
 * Convert a boolean value into the string "true" or "false".
 */
inline std::string asBoolean(bool value) { return value ? "true": "false"; }

::testing::AssertionResult
CheckNode(const sg::NodePtr& node, const NodeTest& nodeTest)
{
    if (!nodeTest) {
        if (!node || !node->visible())
            return ::testing::AssertionSuccess();
        else
            return ::testing::AssertionFailure() << "Found a node where no node expected";
    }

    return nodeTest(node);
}

::testing::AssertionResult
checkPathOps(const sg::NodePtr& node, const std::vector<PathOpTest>& pathTests)
{
    if (!sg::DrawNode::is_type(node) && !sg::TextNode::is_type(node))
        return ::testing::AssertionFailure()
               << "Cannot check path operations on node type=" << node->type();

    auto pathOp = (sg::DrawNode::is_type(node) ? sg::DrawNode::cast(node)->getOp()
                                               : sg::TextNode::cast(node)->getOp());

    while (pathOp && !pathOp->visible())
        pathOp = pathOp->nextSibling;

    for (const auto& m : pathTests) {
        if (!pathOp)
            return ::testing::AssertionFailure() << "More tests than path ops";

        auto result = m(pathOp);
        if (!result)
            return result;

        pathOp = pathOp->nextSibling;
        while (pathOp && !pathOp->visible())
            pathOp = pathOp->nextSibling;
    }

    if (pathOp)
        return ::testing::AssertionFailure() << "More path ops than tests";

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult
CheckPathOps(sg::PathOpPtr pathOp, const std::vector<PathOpTest>& pathTests)
{
    while (pathOp && !pathOp->visible())
        pathOp = pathOp->nextSibling;

    for (const auto& m : pathTests) {
        if (!pathOp)
            return ::testing::AssertionFailure() << "More tests than path ops";

        auto result = m(pathOp);
        if (!result)
            return result;

        pathOp = pathOp->nextSibling;
        while (pathOp && !pathOp->visible())
            pathOp = pathOp->nextSibling;
    }

    if (pathOp)
        return ::testing::AssertionFailure() << "More path ops than tests";

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult
CheckPath(sg::PathPtr path, const PathTest& pathTest) {
    if (!path) {
        if (pathTest)
            return ::testing::AssertionFailure() << "Path test without a path";

        return ::testing::AssertionSuccess();
    }

    if (!pathTest)
        return ::testing::AssertionFailure() << "Path provided but no path test";

    return pathTest(path);
}


::testing::AssertionResult
checkPaintProps( const sg::PaintPtr& paint, float opacity, Transform2D transform)
{
    SGASSERT(CheckNotNull(paint, "Missing paint object"), "");
    SGASSERT(CompareBasic(paint->getOpacity(), opacity, "Opacity"), "");

    // Only check the transform if we are NOT a ColorPaint
    if (paint->getTransform() != transform && !sg::ColorPaint::is_type(paint))
        return ::testing::AssertionFailure()
               << "Mismatched paint transform was=" << paint->getTransform().toDebugString()
               << " expected " << transform.toDebugString();

    return ::testing::AssertionSuccess();
}

PaintTest
IsColorPaint( Color color, float opacity, const std::string& msg )
{
    return [=](const sg::PaintPtr& paint) {
        SGASSERT(checkPaintProps(paint, opacity, Transform2D()), msg);
        SGASSERT(CheckTrue(sg::ColorPaint::is_type(paint), "ColorPaint"), msg);
        auto ptr = sg::ColorPaint::cast(paint);
        SGASSERT(CompareGeneral(ptr->getColor(), color, "Color", &Color::asString), msg);
        return ::testing::AssertionSuccess();
    };
}

::testing::AssertionResult
checkGradientProps( const sg::GradientPaint *paint,
                    std::vector<double> points,
                    std::vector<Color> colors,
                    Gradient::GradientSpreadMethod spreadMethod,
                    bool useBoundingBox )
{
    SGASSERT(CompareNumericArray(paint->getPoints(), points, "Points"), "");
    SGASSERT(CompareGeneral(paint->getColors(), colors, "Colors", asColorArray), "");
    SGASSERT(CompareBasic(paint->getSpreadMethod(), spreadMethod, "SpreadMethod"), "");
    SGASSERT(CompareBasic(paint->getUseBoundingBox(), useBoundingBox, "useBoundingBox"), "");
    return ::testing::AssertionSuccess();
}

PaintTest
IsLinearGradientPaint( std::vector<double> points,
                       std::vector<Color> colors,
                       Gradient::GradientSpreadMethod spreadMethod,
                       bool useBoundingBox,
                       Point start,
                       Point end,
                       float opacity,
                       Transform2D transform,
                       const std::string& msg)
{
    return [=](sg::PaintPtr paint) {
        SGASSERT(checkPaintProps(paint, opacity, transform), msg);
        SGASSERT(CheckTrue(sg::LinearGradientPaint::is_type(paint), "linear gradient paint"), msg);
        auto ptr = sg::LinearGradientPaint::cast(paint);
        SGASSERT(checkGradientProps(ptr, points, colors, spreadMethod, useBoundingBox), msg);
        SGASSERT(CompareGeneral(ptr->getStart(), start, "start", &Point::toString), msg);
        SGASSERT(CompareGeneral(ptr->getEnd(), end, "end", &Point::toString), msg);
        return ::testing::AssertionSuccess();
    };
}

PaintTest
IsRadialGradientPaint( std::vector<double> points,
                       std::vector<Color> colors,
                       Gradient::GradientSpreadMethod spreadMethod,
                       bool useBoundingBox,
                       Point center,
                       float radius,
                       float opacity,
                       Transform2D transform,
                       const std::string& msg)
{
    return [=](sg::PaintPtr paint) {
        SGASSERT(checkPaintProps(paint, opacity, transform), msg);
        SGASSERT(CheckTrue(sg::RadialGradientPaint::is_type(paint), "radial gradient paint"), msg);
        auto ptr = sg::RadialGradientPaint::cast(paint);
        SGASSERT(checkGradientProps(ptr, points, colors, spreadMethod, useBoundingBox), msg);
        SGASSERT(CompareGeneral(ptr->getCenter(), center, "center", &Point::toString), msg);

        if (std::abs(ptr->getRadius() - radius) > 0.01)
            return ::testing::AssertionFailure()
                   << "Mismatched radius, was=" << radius << " expected=" << radius << msg;

        return ::testing::AssertionSuccess();
    };
}

PaintTest
IsPatternPaint( Size size, NodeTest nodeTest, float opacity, Transform2D transform, const std::string& msg )
{
    return [=](sg::PaintPtr paint) {
        SGASSERT(checkPaintProps(paint, opacity, transform), msg);
        SGASSERT(CheckTrue(sg::PatternPaint::is_type(paint), "Pattern paint"), msg);
        auto ptr = sg::PatternPaint::cast(paint);
        SGASSERT(CompareGeneral(ptr->getSize(), size, "size", &Size::toString), msg);
        SGASSERT(nodeTest(ptr->getNode()), msg);
        return ::testing::AssertionSuccess();
    };
}

PathTest
IsRectPath( Rect rect, const std::string& msg)
{
    return [=](const sg::PathPtr& path) {
        SGASSERT(CheckNotNull(path, "Missing path"), msg);

        // A rounded-rect with zero radius is considered a rect path
        if (sg::RoundedRectPath::is_type(path)) {
            auto rptr = sg::RoundedRectPath::cast(path);
            SGASSERT(CheckTrue(rptr->getRoundedRect().radii().empty(), "radii empty"), msg);
            SGASSERT(CompareDebug(rptr->getRoundedRect().rect(), rect, "rectangle"), msg);
            return ::testing::AssertionSuccess();
        }

        if (!sg::RectPath::is_type(path))
            return ::testing::AssertionFailure() << "Not a RectPath; type=" << path->type() << msg;

        auto ptr = sg::RectPath::cast(path);
        SGASSERT(CompareDebug(ptr->getRect(), rect, "rectangle"), msg);
        return ::testing::AssertionSuccess();
    };
}

PathTest
IsRectPath( float x, float y, float width, float height, const std::string& msg )
{
    return IsRectPath(Rect{x,y,width,height}, msg);
}

PathTest
IsRoundRectPath( RoundedRect rrect, const std::string& msg )
{
    return [=](const sg::PathPtr& path) {
        SGASSERT(CheckNotNull(path, "Missing path"), msg);
        SGASSERT(CheckTrue(sg::RoundedRectPath::is_type(path), "rounded rect path"), msg);
        auto ptr = sg::RoundedRectPath::cast(path);
        SGASSERT(CompareDebug(ptr->getRoundedRect(), rrect, "roundedRect"), msg);
        return ::testing::AssertionSuccess();
    };
}

PathTest
IsRoundRectPath( float x, float y, float width, float height, float radius, const std::string& msg )
{
    return IsRoundRectPath(RoundedRect{ Rect{x,y,width,height}, Radii{radius}} );
}

PathTest
IsFramePath( RoundedRect rrect, float inset, const std::string& msg )
{
    return [=](const sg::PathPtr& path) {
        SGASSERT(CheckNotNull(path, "Missing path"), msg);
        SGASSERT(CheckTrue(sg::FramePath::is_type(path), "frame path"), msg);
        auto ptr = sg::FramePath::cast(path);
        SGASSERT(CompareDebug(ptr->getRoundedRect(), rrect, "roundedRect"), msg);
        SGASSERT(CompareBasic(ptr->getInset(), inset, "inset"), msg);
        return ::testing::AssertionSuccess();
    };
}

PathTest
IsGeneralPath( std::string value, std::vector<float> points, const std::string& msg)
{
    return [=](sg::PathPtr path) {
        SGASSERT(CheckNotNull(path, "Missing path"), msg);
        SGASSERT(CheckTrue(sg::GeneralPath::is_type(path), "general path"), msg);
        auto ptr = sg::GeneralPath::cast(path);
        SGASSERT(CompareBasic(ptr->getValue(), value, "value"), msg);
        SGASSERT(CompareNumericArray(ptr->getPoints(), points, "points"), msg);
        return ::testing::AssertionSuccess();
    };
}

PathOpTest
IsStrokeOp( PaintTest paintTest, float strokeWidth, const std::string& msg ) {
    return IsStrokeOp(paintTest, strokeWidth, 4.0f, 0.0f, 0.0f, kGraphicLineCapButt,
                      kGraphicLineJoinMiter, {}, msg);
}

PathOpTest IsStrokeOp( PaintTest paintTest,
                       float strokeWidth, float miterLimit, float pathLength, float dashOffset,
                       GraphicLineCap lineCap, GraphicLineJoin lineJoin, std::vector<float> dashes,
                       const std::string& msg)
{
    return [=](sg::PathOpPtr op) {
        SGASSERT(CheckNotNull(op, "Missing path op"), msg);
        SGASSERT(CheckTrue(sg::StrokePathOp::is_type(op), "stroke pathop"), msg);
        auto ptr = sg::StrokePathOp::cast(op);
        SGASSERT(paintTest(ptr->paint), msg);
        SGASSERT(CompareBasic(ptr->strokeWidth, strokeWidth, "strokeWidth"), msg);
        SGASSERT(CompareBasic(ptr->miterLimit, miterLimit, "miterLimit"), msg);
        SGASSERT(CompareBasic(ptr->pathLength, pathLength, "pathLength"), msg);
        SGASSERT(CompareBasic(ptr->dashOffset, dashOffset, "dashOffset"), msg);
        SGASSERT(CompareBasic(ptr->lineCap, lineCap, "lineCap"), msg);
        SGASSERT(CompareBasic(ptr->lineJoin, lineJoin, "lineJoin"), msg);
        SGASSERT(CompareNumericArray(ptr->dashes, dashes, "dashes"), msg);
        return ::testing::AssertionSuccess();
    };
}

PathOpTest
IsFillOp( PaintTest paintTest, const std::string& msg ) {
    return [=](sg::PathOpPtr op) {
        SGASSERT(CheckNotNull(op, "Missing path op"), msg);
        SGASSERT(CheckTrue(sg::FillPathOp::is_type(op), "fill pathop"), msg);
        auto ptr = sg::FillPathOp::cast(op);
        SGASSERT(paintTest(ptr->paint), msg);
        return ::testing::AssertionSuccess();
    };
}

ShadowTest
IsShadow( Color color, Point offset, float radius, const std::string& msg )
{
    return [=](sg::ShadowPtr shadow) {
        SGASSERT(CheckNotNull(shadow, "Missing shadow"), msg);
        SGASSERT(CompareGeneral(shadow->getColor(), color, "Color", &Color::asString), msg);
        SGASSERT(CompareGeneral(shadow->getOffset(), offset, "Offset", &Point::toString), msg);
        SGASSERT(CompareBasic(shadow->getRadius(), radius, "Radius"), msg);
        return ::testing::AssertionSuccess();
    };
}

::testing::AssertionResult
IsAccessibility::operator()(sg::AccessibilityPtr accessibility)
{
    SGASSERT(CompareBasic(accessibility->getLabel(), mLabel, "Label"), mMsg);
    SGASSERT(CompareBasic(accessibility->getRole(), mRole, "Role"), mMsg);
    SGASSERT(CompareDebug(accessibility->actions(), mActions, "Actions"), mMsg);
    return ::testing::AssertionSuccess();

}

::testing::AssertionResult
IsNode::CheckBase(sg::NodePtr node)
{
    SGASSERT(CheckNotNull(node, "Missing node"), mMsg);
    SGASSERT(CompareBasic(node->type(), mType, "Type"), mMsg);
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult
IsNode::CheckChildren(sg::NodePtr node)
{
    SGASSERT(CheckNode(node->child(), mChildTest), mMsg);
    return CheckNode(node->next(), mNextTest);
}

/**
 * To simplify writing unit tests, nodes that are not visible are ignored in the `CheckSceneGraph`
 * method.  This method advances the node pointer until it finds a visible node.
 */
sg::NodePtr
AdvanceToVisibleNode(sg::NodePtr node)
{
    while (node && !node->visible())
        node = node->next();
    return node;
}

::testing::AssertionResult
IsClipNode::operator()(sg::NodePtr node)
{
    node = AdvanceToVisibleNode(node);
    SGASSERT(CheckBase(node), mMsg);
    auto *clip = sg::ClipNode::cast(node);
    SGASSERT(mPathTest(clip->getPath()), mMsg);
    return CheckChildren(node);
}

::testing::AssertionResult
IsOpacityNode::operator()(sg::NodePtr node)
{
    node = AdvanceToVisibleNode(node);
    SGASSERT(CheckBase(node), mMsg);
    auto *opacity = sg::OpacityNode::cast(node);
    SGASSERT(CompareBasic(opacity->getOpacity(), mOpacity, "Opacity"), mMsg);
    return CheckChildren(node);
}

::testing::AssertionResult
IsTransformNode::operator()(sg::NodePtr node)
{
    node = AdvanceToVisibleNode(node);
    SGASSERT(CheckBase(node), mMsg);
    auto *tnode = sg::TransformNode::cast(node);
    SGASSERT(CompareDebug(tnode->getTransform(), mTransform, "Transform"), mMsg);
    return CheckChildren(node);
}

::testing::AssertionResult
IsDrawNode::operator()(sg::NodePtr node)
{
    node = AdvanceToVisibleNode(node);
    SGASSERT(CheckBase(node), mMsg);
    auto *dnode = sg::DrawNode::cast(node);
    if (dnode->visible() || mPathTest || !mPathOpTests.empty()) {
        SGASSERT(CheckPath(dnode->getPath(), mPathTest), mMsg);
        SGASSERT(CheckPathOps(dnode->getOp(), mPathOpTests), mMsg);
    }
    return CheckChildren(node);
}

::testing::AssertionResult
IsEditNode::operator()(sg::NodePtr node)
{
    node = AdvanceToVisibleNode(node);
    SGASSERT(CheckBase(node), mMsg);
    auto *enode = sg::EditTextNode::cast(node);
    SGASSERT(CompareBasic(enode->getText(), mText, "Text"), mMsg);
    SGASSERT(CompareGeneral(enode->getEditTextConfig()->textColor(), mColor, "Color", &Color::asString), mMsg);
    return CheckChildren(node);
}

::testing::AssertionResult
IsTextNode::operator()(sg::NodePtr node)
{
    node = AdvanceToVisibleNode(node);
    SGASSERT(CheckBase(node), mMsg);
    auto *tnode = sg::TextNode::cast(node);

    SGASSERT(CompareBasic(tnode->getTextLayout()->toDebugString(), mText, "Text"), mMsg);
    SGASSERT(CheckPathOps(tnode->getOp(), mPathOpTests), mMsg);
    SGASSERT(CompareDebug(tnode->getRange(), mRange, "Range"), mMsg);

    auto layout = tnode->getTextLayout();
    if (!layout)
        return ::testing::AssertionFailure() << "No text layout in TextNode" << mMsg;

    // Treat measured size checks as optional
    if (!mMeasuredSize.empty())
        SGASSERT(CompareDebug(layout->getSize(), mMeasuredSize, "Measured Size"), mMsg);

    return CheckChildren(node);
}

::testing::AssertionResult
IsImageNode::operator()(sg::NodePtr node)
{
    node = AdvanceToVisibleNode(node);
    SGASSERT(CheckBase(node), mMsg);
    auto *ptr = sg::ImageNode::cast(node);
    SGASSERT(mFilterTest(ptr->getImage()), mMsg);
    SGASSERT(CompareDebug(ptr->getTarget(), mTarget, "Target"), mMsg);
    SGASSERT(CompareDebug(ptr->getSource(), mSource, "Source"), mMsg);
    return CheckChildren(node);
}

::testing::AssertionResult
IsVideoNode::operator()(sg::NodePtr node)
{
    node = AdvanceToVisibleNode(node);
    SGASSERT(CheckBase(node), mMsg);
    auto *ptr = sg::VideoNode::cast(node);
    // TODO: Fix the URL
    SGASSERT(CompareBasic(ptr->getScale(), mScale, "VideoScale"), mMsg);
    SGASSERT(CompareDebug(ptr->getTarget(), mTarget, "Target"), mMsg);
    return CheckChildren(node);
}

::testing::AssertionResult
IsShadowNode::operator()(sg::NodePtr node)
{
    node = AdvanceToVisibleNode(node);
    SGASSERT(CheckBase(node), mMsg);
    auto *ptr = sg::ShadowNode::cast(node);
    SGASSERT(mShadowTest(ptr->getShadow()), mMsg);
    return CheckChildren(node);
}

::testing::AssertionResult
IsLayer::operator()(sg::LayerPtr layer)
{
    SGASSERT(CheckNotNull(layer, "Missing layer"), mMsg);

    SGASSERT(IsEqual(layer->getBounds(), mBounds) << " Layer Bounds", mMsg);
    SGASSERT(CompareVisible(layer->getShadow(), mShadowTest, "Layer Shadow"), mMsg);
    SGASSERT(CompareOptional(layer->getOutline(), mOutlineTest, "Layer Outline"), mMsg);
    SGASSERT(CompareOptional(layer->getChildClip(), mChildClipTest, "Layer ChildClip"), mMsg);
    SGASSERT(IsEqual(layer->getOpacity(), mOpacity) <<  " Layer Opacity", mMsg);
    SGASSERT(IsEqual(layer->getTransform(), mTransform) << " Layer Transform", mMsg);
    SGASSERT(CompareOptional(layer->getAccessibility(), mAccessibilityTest, "Layer Accessibility"), mMsg);
    if (!layer->children().empty())
        SGASSERT(IsEqual(layer->getChildOffset(), mChildOffset) << " Layer Child Offset", mMsg);
    if (layer->content())
        SGASSERT(IsEqual(layer->getContentOffset(), mContentOffset) << " Layer Content Offset",
                 mMsg);
    SGASSERT(CompareBasic(layer->getInteraction(), mInteraction, "Interaction"), mMsg);
    SGASSERT(CheckNode(layer->content(), mContentTest), std::string("Layer Content") + mMsg);
    SGASSERT(CompareDebug(layer->children(), mLayerTests, "Layer Children"), mMsg);
    SGASSERT(CompareBasic(layer->getCharacteristic(), mCharacteristics, "Layer Characteristics"), mMsg);

    // Check dirty flags
    SGASSERT(CompareBasic(layer->getAndClearFlags(), mDirtyFlags, "Layer Flags"), mMsg);

    return ::testing::AssertionSuccess();
}


FilterTest
IsBlendFilter( FilterTest backTest, FilterTest frontTest, BlendMode blendMode, const std::string& msg )
{
    return [=](sg::FilterPtr filter) {
        SGASSERT(CheckNotNull(filter, "Missing filter"), msg);
        SGASSERT(CheckTrue(sg::BlendFilter::is_type(filter), "blend filter"), msg);
        auto ptr = sg::BlendFilter::cast(filter);
        SGASSERT(backTest(ptr->back), msg);
        SGASSERT(frontTest(ptr->front), msg);
        SGASSERT(CompareBasic(ptr->blendMode, blendMode, "blend mode"), msg);
        return ::testing::AssertionSuccess();
    };
}

FilterTest
IsBlurFilter( FilterTest filterTest, float radius, const std::string& msg )
{
    return [=](sg::FilterPtr filter) {
        SGASSERT(CheckNotNull(filter, "Missing filter"), msg);
        SGASSERT(CheckTrue(sg::BlurFilter::is_type(filter), "blur filter"), msg);
        auto ptr = sg::BlurFilter::cast(filter);
        SGASSERT(filterTest(ptr->filter), msg);
        SGASSERT(CompareBasic(ptr->radius, radius, "radius"), msg);
        return ::testing::AssertionSuccess();
    };
}

FilterTest
IsGrayscaleFilter( FilterTest filterTest, float amount, const std::string& msg)
{
    return [=](sg::FilterPtr filter) {
        SGASSERT(CheckNotNull(filter, "Missing filter"), msg);
        SGASSERT(CheckTrue(sg::GrayscaleFilter::is_type(filter), "grayscale filter"), msg);
        auto ptr = sg::GrayscaleFilter::cast(filter);
        SGASSERT(filterTest(ptr->filter), msg);
        SGASSERT(CompareBasic(ptr->amount, amount, "amount"), msg);
        return ::testing::AssertionSuccess();
    };
}

FilterTest
IsMediaObjectFilter( std::string url, MediaObject::State state, const std::string& msg)
{
    return [=](sg::FilterPtr filter) {
        SGASSERT(CheckNotNull(filter, "Missing filter"), msg);
        SGASSERT(CheckTrue(sg::MediaObjectFilter::is_type(filter), "media object filter"), msg);
        auto ptr = sg::MediaObjectFilter::cast(filter);
        SGASSERT(CompareBasic(ptr->mediaObject->url(), url, "URL"), msg);
        SGASSERT(CompareBasic(ptr->mediaObject->state(), state, "State"), msg);
        return ::testing::AssertionSuccess();
    };
}

FilterTest
IsNoiseFilter( FilterTest filterTest, NoiseFilterKind kind, bool useColor, float sigma, const std::string& msg)
{
    return [=](sg::FilterPtr filter) {
        SGASSERT(CheckNotNull(filter, "Missing filter"), msg);
        SGASSERT(CheckTrue(sg::NoiseFilter::is_type(filter), "noise filter"), msg);
        auto ptr = sg::NoiseFilter::cast(filter);
        SGASSERT(filterTest(ptr->filter), msg);
        SGASSERT(CompareBasic(ptr->kind, kind, "Kind"), msg);
        SGASSERT(CompareBasic(ptr->useColor, useColor, "useColor"), msg);
        SGASSERT(CompareBasic(ptr->sigma, sigma, "sigma"), msg);
        return ::testing::AssertionSuccess();
    };
}

FilterTest
IsSaturateFilter( FilterTest filterTest, float amount, const std::string& msg)
{
    return [=](sg::FilterPtr filter) {
        SGASSERT(CheckNotNull(filter, "Missing filter"), msg);
        SGASSERT(CheckTrue(sg::SaturateFilter::is_type(filter), "saturate filter"), msg);
        auto ptr = sg::SaturateFilter::cast(filter);
        SGASSERT(filterTest(ptr->filter), msg);
        SGASSERT(CompareBasic(ptr->amount, amount, "Amount"), msg);
        return ::testing::AssertionSuccess();
    };
}


FilterTest
IsSolidFilter( PaintTest paintTest, const std::string& msg)
{
    return [=](sg::FilterPtr filter) {
        SGASSERT(CheckNotNull(filter, "Missing filter"), msg);
        SGASSERT(CheckTrue(sg::SolidFilter::is_type(filter), "solid filter"), msg);
        auto ptr = sg::SolidFilter::cast(filter);
        SGASSERT(paintTest(ptr->paint), msg);
        return ::testing::AssertionSuccess();
    };
}


::testing::AssertionResult
CheckSceneGraph(const sg::SceneGraphPtr& sg, const LayerTest& layerTest)
{
    return layerTest(sg->getLayer());
}

::testing::AssertionResult
CheckSceneGraph(sg::SceneGraphUpdates& updates, const sg::NodePtr& node, const NodeTest& nodeTest)
{
    auto result = nodeTest(node);
    if (!result)
        return result;

    updates.clear();
    return result;
}

::testing::AssertionResult
CheckSceneGraph(sg::SceneGraphUpdates& updates, const sg::LayerPtr& layer, const LayerTest& layerTest)
{
    auto result = layerTest(layer);
    if (!result)
        return result;
    updates.clear();
    return result;
}

void
DumpSceneGraph(const sg::PaintPtr& ptr, int inset)   // NOLINT(misc-no-recursion)
{
    auto p = std::string(inset, ' ');
    if (!ptr) {
        std::cout << p << "Null Paint" << std::endl;
        return;
    }

    std::cout << p << ptr->toDebugString() << std::endl;

    switch (ptr->type()) {
        case sg::Paint::kPattern:
            DumpSceneGraph(sg::PatternPaint::cast(ptr)->getNode(), inset + 2);
            break;

        default:
            break;
    }
}

void
DumpSceneGraph(const sg::PathOpPtr& ptr, int inset)   // NOLINT(misc-no-recursion)
{
    auto p = std::string(inset, ' ');
    if (!ptr->visible())
        p += "[NOT DRAWN] ";
    std::cout << p << ptr->toDebugString() << std::endl;
    DumpSceneGraph(ptr->paint, inset + 2);
}

void
DumpSceneGraph(const sg::PathPtr& ptr, int inset)     // NOLINT(misc-no-recursion)
{
    auto p = std::string(inset, ' ');
    std::cout << p << ptr->toDebugString() << std::endl;
}

void
DumpSceneGraph(const sg::NodePtr& ptr, int inset)     // NOLINT(misc-no-recursion)
{
    auto p = std::string(inset, ' ');
    if (!ptr) {
        std::cout << p << "ERROR: NULL NODE" << std::endl;
        return;
    }

    if (!ptr->visible())
        p += "[NOT_DRAWN] ";
    std::cout << p << ptr->toDebugString() << std::endl;

    p += "  ";

    switch (ptr->type()) {
        case sg::Node::kDraw: {
            auto node = sg::DrawNode::cast(ptr);
            DumpSceneGraph(node->getPath(), inset + 2);
            for (auto op = node->getOp() ; op ; op = op->nextSibling)
                DumpSceneGraph(op, inset + 2);
        } break;

        case sg::Node::kText: {
            auto node = sg::TextNode::cast(ptr);
            for (auto op = node->getOp() ; op ; op = op->nextSibling)
                DumpSceneGraph(op, inset + 2);
        } break;

        case sg::Node::kClip: {
            auto node = sg::ClipNode::cast(ptr);
            DumpSceneGraph(node->getPath(), inset + 2);
        } break;

        case sg::Node::kImage: {
            auto node = sg::ImageNode::cast(ptr);
            DumpSceneGraph(node->getImage(), inset + 2);
        } break;

        case sg::Node::kShadow: {
            auto node = sg::ShadowNode::cast(ptr);
            std::cout << p << "Shadow " << node->getShadow()->toDebugString() << std::endl;
        } break;

        default:
            break;
    }

    if (ptr->child())
        DumpSceneGraph(ptr->child(), inset + 2);

    if (ptr->next())
        DumpSceneGraph(ptr->next(), inset);
}

void
DumpSceneGraph(const sg::LayerPtr& ptr, int inset)     // NOLINT(misc-no-recursion)
{
    auto p = std::string(inset, ' ');
    if (!ptr) {
        std::cout << p << "ERROR: NULL NODE" << std::endl;
        return;
    }

    std::cout << p << ptr->toDebugString() << std::endl;
    p += "  ";
    std::cout << p << "Bounds " << ptr->getBounds().toDebugString() << std::endl;
    std::cout << p << "Opacity " << ptr->getOpacity() << std::endl;

    auto flags = ptr->debugFlagString();
    if (!flags.empty())
        std::cout << p << "Dirty flags " << flags << std::endl;

    auto fixed = ptr->debugCharacteristicString();
    if (!fixed.empty())
        std::cout << p << "Fixed flags " << fixed << std::endl;

    if (ptr->getOutline())
        std::cout << p << "Outline " << ptr->getOutline()->toDebugString() << std::endl;

    if (ptr->getChildClip())
        std::cout << p << "ChildClip " << ptr->getChildClip()->toDebugString() << std::endl;

    if (!ptr->getTransform().empty())
        std::cout << p << "Transform " << ptr->getTransform().toDebugString() << std::endl;

    if (ptr->getAccessibility()) {
        std::cout << p << "Accessibility" << std::endl;
        std::cout << p << "  Label " << ptr->getAccessibility()->getLabel() << std::endl;
        std::cout << p << "  Role " << sRoleMap.at(ptr->getAccessibility()->getRole()) << std::endl;
        for (const auto& m : ptr->getAccessibility()->actions())
            std::cout << p << "    Action " << m.name << " label=" << m.label
                      << " enabled=" << (m.enabled ? "true" : "false");
    }

    if (ptr->getInteraction())
        std::cout << p << "Interaction: " << ptr->debugInteractionString() << std::endl;

    if (ptr->getShadow())
        std::cout << p << "Shadow " << ptr->getShadow()->toDebugString() << std::endl;

    if (!ptr->getChildOffset().empty())
        std::cout << p << "ChildOffset " << ptr->getChildOffset().toString() << std::endl;

    if (!ptr->getContentOffset().empty())
        std::cout << p << "ContentOffset " << ptr->getContentOffset().toString() << std::endl;

    if (ptr->content()) {
        std::cout << p << "Content" << std::endl;
        DumpSceneGraph(ptr->content(), inset + 4);
    }

    if (!ptr->children().empty()) {
        std::cout << p << "Children" << std::endl;
        for (const auto& child : ptr->children())
            DumpSceneGraph(child, inset + 4);
    }
}

void
DumpSceneGraph(const sg::GraphicFragmentPtr& fragment, int inset)
{
    if (fragment->layer())
        DumpSceneGraph(fragment->layer(), inset);
    else if (fragment->node())
        DumpSceneGraph(fragment->node(), inset);
}

void
DumpSceneGraph(const sg::FilterPtr& ptr, int inset)     // NOLINT(misc-no-recursion)
{
    auto p = std::string(inset, ' ');

    if (!ptr) {
        std::cout << p << "ERROR: NULL NODE" << std::endl;
        return;
    }

    std::cout << p << ptr->toDebugString() << std::endl;

    switch (ptr->type) {
        case sg::Filter::kBlend: {
            auto filter = sg::BlendFilter::cast(ptr);
            DumpSceneGraph(filter->back, inset + 2);
            DumpSceneGraph(filter->front, inset + 2);
        }
            break;
        case sg::Filter::kBlur: {
            auto filter = sg::BlurFilter::cast(ptr);
            DumpSceneGraph(filter->filter, inset + 2);
        }
            break;
        case sg::Filter::kGrayscale: {
            auto filter = sg::GrayscaleFilter::cast(ptr);
            DumpSceneGraph(filter->filter, inset + 2);
        }
            break;
        case sg::Filter::kNoise: {
            auto filter = sg::NoiseFilter::cast(ptr);
            DumpSceneGraph(filter->filter, inset + 2);
        }
            break;
        case sg::Filter::kSaturate: {
            auto filter = sg::SaturateFilter::cast(ptr);
            DumpSceneGraph(filter->filter, inset + 2);
        }
            break;
        case sg::Filter::kSolid: {
            auto filter = sg::SolidFilter::cast(ptr);
            DumpSceneGraph(filter->paint, inset + 2);
        }
            break;

        default:
            break;
    }
}

void
DumpSceneGraph(const sg::SceneGraphPtr& ptr)
{
    std::cout << "__BEGIN_SCENE_GRAPH___" << std::endl;
    if (ptr && ptr->getLayer())
        DumpSceneGraph(ptr->getLayer(), 0);
    else
        std::cout << "Null scene graph" << std::endl;
    std::cout << "__END_SCENE_GRAPH__" << std::endl;
}
