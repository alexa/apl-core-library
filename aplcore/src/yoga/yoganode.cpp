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

#include "apl/yoga/yoganode.h"

#include <yoga/YGNode.h>

#include "apl/component/corecomponent.h"
#include "apl/utils/streamer.h"

namespace apl {

#define TO_YOGA_NODE(NODE) ((YGNodeRef)(NODE))

static const Bimap<LayoutDirection, YGDirection> sDirectionToYGDirection = {
    {kLayoutDirectionInherit, YGDirectionInherit},
    {kLayoutDirectionLTR,     YGDirectionLTR},
    {kLayoutDirectionRTL,     YGDirectionRTL}
};

static const Bimap<ContainerDirection, YGFlexDirection> sFlexDirectionToYGDirection = {
    {kContainerDirectionColumn,        YGFlexDirectionColumn},
    {kContainerDirectionRow,           YGFlexDirectionRow},
    {kContainerDirectionColumnReverse, YGFlexDirectionColumnReverse},
    {kContainerDirectionRowReverse,    YGFlexDirectionRowReverse}
};

/**
 * This inline function casts YGMeasureMode enum to MeasureMode enum.
 * @param ygMeasureMode Yoga definition of the measuring mode
 * @return APL definition of the measuring mode
 */
static inline MeasureMode toMeasureMode(YGMeasureMode ygMeasureMode)
{
    switch(ygMeasureMode){
        case YGMeasureModeExactly:
            return Exactly;
        case YGMeasureModeAtMost:
            return AtMost;
            //default case will execute when mode is YGMeasureModeUndefined as well as other than specified cases in case of Yoga lib update.
        default:
            return Undefined;
    }
}

YogaNode::YogaNode(const YogaConfig& config)
{
    mNode = YGNodeNewWithConfig((YGConfigRef)config.mConfig);
    YGNodeSetContext(TO_YOGA_NODE(mNode), this);
}

YogaNode::~YogaNode()
{
    YGNodeFree(TO_YOGA_NODE(mNode));
}

void
YogaNode::setComponent(CoreComponent *component)
{
    mComponent = component;
}

void
YogaNode::setNodeTypeDefault()
{
    YGNodeSetNodeType(TO_YOGA_NODE(mNode), YGNodeTypeDefault);
}

void
YogaNode::setNodeTypeText()
{
    YGNodeSetNodeType(TO_YOGA_NODE(mNode), YGNodeTypeText);
}

void
YogaNode::insertChild(const YogaNode& child, uint32_t index)
{
    YGNodeInsertChild(TO_YOGA_NODE(mNode), TO_YOGA_NODE(child.mNode), index);
}

void
YogaNode::removeChild(const YogaNode& child)
{
    YGNodeRemoveChild(TO_YOGA_NODE(mNode), TO_YOGA_NODE(child.mNode));
}

void
YogaNode::markDirty()
{
    YGNodeMarkDirty(TO_YOGA_NODE(mNode));
}

void
YogaNode::calculateLayout(float ownerWidth, float ownerHeight, LayoutDirection ownerDirection)
{
    YGNodeCalculateLayout(TO_YOGA_NODE(mNode), ownerWidth, ownerHeight, sDirectionToYGDirection.at(ownerDirection));
}

bool
YogaNode::isDirty() const
{
    return YGNodeIsDirty(TO_YOGA_NODE(mNode));
}

void
YogaNode::setDirtiedFunc(DirtiedFunc dirtiedFunc)
{
    mDirtiedFunc = dirtiedFunc;
    if (!mDirtiedFunc) {
        YGNodeSetDirtiedFunc(TO_YOGA_NODE(mNode), nullptr);
    } else {
        auto yogaDirtiedFunc = [](YGNodeRef node) -> void {
            auto yogaNode =  static_cast<YogaNode*>(node->getContext());
            yogaNode->mDirtiedFunc(yogaNode->getComponent());
        };

        YGNodeSetDirtiedFunc(TO_YOGA_NODE(mNode), yogaDirtiedFunc);
    }
}

void
YogaNode::setMeasureFunc()
{
    static auto textMeasureFunc = [](YGNodeRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) -> YGSize {
        auto yogaNode =  static_cast<YogaNode*>(node->getContext());
        auto *component = yogaNode->getComponent();
        assert(component);

        if (!component || !component->isValid())
            return YGSize{};

        auto size = component->textMeasure(width, toMeasureMode(widthMode), height, toMeasureMode(heightMode));
        return YGSize{size.getWidth(), size.getHeight()};
    };
    YGNodeSetMeasureFunc(TO_YOGA_NODE(mNode), textMeasureFunc);
}

void
YogaNode::setBaselineFunc()
{
    static auto baselineFunc = [](YGNodeRef node, float width, float height) -> float {
        auto yogaNode =  static_cast<YogaNode*>(node->getContext());
        auto *component = yogaNode->getComponent();
        assert(component);

        if (!component || !component->isValid())
            return 0;

        return component->textBaseline(width, height);
    };
    YGNodeSetBaselineFunc(TO_YOGA_NODE(mNode), baselineFunc);
}

bool
YogaNode::hasMeasureFunc() const
{
    return YGNodeHasMeasureFunc(TO_YOGA_NODE(mNode));
}

bool
YogaNode::hasDirtiedFunc() const
{
    return YGNodeGetDirtiedFunc(TO_YOGA_NODE(mNode));
}

bool
YogaNode::hasOwner() const
{
    return TO_YOGA_NODE(mNode)->getOwner() != nullptr;
}

YogaNode*
YogaNode::getParent() const
{
    auto parentNode = YGNodeGetParent(TO_YOGA_NODE(mNode));
    if (!parentNode) return nullptr;
    return static_cast<YogaNode*>(parentNode->getContext());
}

YogaNode*
YogaNode::getChild(uint32_t index) const
{
    auto childNode = YGNodeGetChild(TO_YOGA_NODE(mNode), index);
    if (!childNode) return nullptr;
    return static_cast<YogaNode*>(childNode->getContext());
}

CoreComponent*
YogaNode::getComponent() const
{
    return mComponent;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void
YogaNode::setPropertyGrow(float value)
{
    YGNodeStyleSetFlexGrow(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setPropertyShrink(float value)
{
    YGNodeStyleSetFlexShrink(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setPositionType(Position value)
{
    if (value == kPositionRelative || value == kPositionSticky)
        YGNodeStyleSetPositionType(TO_YOGA_NODE(mNode), YGPositionTypeRelative);
    else if (value == kPositionAbsolute)
        YGNodeStyleSetPositionType(TO_YOGA_NODE(mNode), YGPositionTypeAbsolute);
}

void
YogaNode::setWidth(float value)
{
    YGNodeStyleSetWidth(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setWidthPercent(float value)
{
    YGNodeStyleSetWidthPercent(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setWidthAuto()
{
    YGNodeStyleSetWidthAuto(TO_YOGA_NODE(mNode));
}

void
YogaNode::setMinWidth(float value)
{
    YGNodeStyleSetMinWidth(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setMinWidthPercent(float value)
{
    YGNodeStyleSetMinWidthPercent(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setMaxWidth(float value)
{
    YGNodeStyleSetMaxWidth(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setMaxWidthPercent(float value)
{
    YGNodeStyleSetMaxWidthPercent(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setHeight(float value)
{
    YGNodeStyleSetHeight(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setHeightPercent(float value)
{
    YGNodeStyleSetHeightPercent(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setHeightAuto()
{
    YGNodeStyleSetHeightAuto(TO_YOGA_NODE(mNode));
}

void
YogaNode::setMinHeight(float value)
{
    YGNodeStyleSetMinHeight(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setMinHeightPercent(float value)
{
    YGNodeStyleSetMinHeightPercent(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setMaxHeight(float value)
{
    YGNodeStyleSetMaxHeight(TO_YOGA_NODE(mNode), value);
}

void
YogaNode::setMaxHeightPercent(float value)
{
    YGNodeStyleSetMaxHeightPercent(TO_YOGA_NODE(mNode), value);
}

// This must be kept in synch with yoganode.h Edge
static const YGEdge EDGE_LOOKUP[] = {
    YGEdgeLeft,
    YGEdgeTop,
    YGEdgeRight,
    YGEdgeBottom,
    YGEdgeStart,
    YGEdgeEnd,
    YGEdgeHorizontal,
    YGEdgeVertical,
    YGEdgeAll
};

inline int edgeAsInt(Edge edge) {
    return static_cast<std::underlying_type<Edge>::type>(edge);
}

void
YogaNode::setPadding(Edge edge, float value)
{
    YGNodeStyleSetPadding(TO_YOGA_NODE(mNode), EDGE_LOOKUP[edgeAsInt(edge)], value);
}

void
YogaNode::setPaddingPercent(Edge edge, float value)
{
    YGNodeStyleSetPaddingPercent(TO_YOGA_NODE(mNode), EDGE_LOOKUP[edgeAsInt(edge)], value);
}

void
YogaNode::setBorder(Edge edge, float value)
{
    YGNodeStyleSetBorder(TO_YOGA_NODE(mNode), EDGE_LOOKUP[edgeAsInt(edge)], value);
}

void
YogaNode::setPosition(Edge edge, float value)
{
    YGNodeStyleSetPosition(TO_YOGA_NODE(mNode), EDGE_LOOKUP[edgeAsInt(edge)], value);
}

void
YogaNode::setPositionPercent(Edge edge, float value)
{
    YGNodeStyleSetPositionPercent(TO_YOGA_NODE(mNode), EDGE_LOOKUP[edgeAsInt(edge)], value);
}

void
YogaNode::setFlexDirection(ContainerDirection value)
{
    YGNodeStyleSetFlexDirection(TO_YOGA_NODE(mNode), sFlexDirectionToYGDirection.at(value));
}

// This must be kept in synch with componentproperties.h FlexboxJustifyContent
static const YGJustify JUSTIFY_LOOKUP[] = {
    YGJustifyFlexStart,
    YGJustifyFlexEnd,
    YGJustifyCenter,
    YGJustifySpaceBetween,
    YGJustifySpaceAround
};

void
YogaNode::setJustifyContent(FlexboxJustifyContent value)
{
    YGNodeStyleSetJustifyContent(TO_YOGA_NODE(mNode), JUSTIFY_LOOKUP[value]);
}

// This must be kept in synch with componentproperties.h FlexboxWrap
static const YGWrap WRAP_LOOKUP[] = {
    YGWrapNoWrap,
    YGWrapWrap,
    YGWrapWrapReverse,
};

void
YogaNode::setWrap(FlexboxWrap value)
{
    YGNodeStyleSetFlexWrap(TO_YOGA_NODE(mNode), WRAP_LOOKUP[value]);
}

// This must be kept in sync with componentproperties.h: FlexboxAlign
static const YGAlign ALIGN_LOOKUP[] = {
    YGAlignStretch,
    YGAlignCenter,
    YGAlignFlexStart,
    YGAlignFlexEnd,
    YGAlignBaseline,
    YGAlignAuto
};

void
YogaNode::setAlignSelf(FlexboxAlign value)
{
    if (value >= kFlexboxAlignStretch && value <= kFlexboxAlignAuto)
        YGNodeStyleSetAlignSelf(TO_YOGA_NODE(mNode), ALIGN_LOOKUP[value]);
}

void
YogaNode::setAlignItems(FlexboxAlign value)
{
    if (value >= kFlexboxAlignStretch && value <= kFlexboxAlignAuto)
        YGNodeStyleSetAlignItems(TO_YOGA_NODE(mNode), ALIGN_LOOKUP[value]);
}

inline YGFlexDirection
scrollDirectionLookup(ScrollDirection direction) {
    switch (direction) {
        case kScrollDirectionHorizontal:
            return YGFlexDirectionRow;

        case kScrollDirectionVertical:
            return YGFlexDirectionColumn;
    }
    // Should not happen.
    return YGFlexDirectionRow;
}

void
YogaNode::setScrollDirection(ScrollDirection value)
{
    YGNodeStyleSetFlexDirection(TO_YOGA_NODE(mNode), scrollDirectionLookup(value));
}

inline YGFlexDirection
gridScrollDirectionLookup(ScrollDirection direction) {
    switch (direction) {
        case kScrollDirectionHorizontal:
            return YGFlexDirectionColumn;

        case kScrollDirectionVertical:
            return YGFlexDirectionRow;
    }
    // Should not happen.
    return YGFlexDirectionRow;
}

void
YogaNode::setGridScrollDirection(ScrollDirection value)
{
    YGNodeStyleSetFlexDirection(TO_YOGA_NODE(mNode), gridScrollDirectionLookup(value));
}

void
YogaNode::setDisplay(Display value)
{
    YGNodeStyleSetDisplay(TO_YOGA_NODE(mNode), value == kDisplayNone ? YGDisplayNone : YGDisplayFlex);
}

void
YogaNode::setLayoutDirection(LayoutDirection value)
{
    switch (value) {
        case kLayoutDirectionLTR:
            YGNodeStyleSetDirection(TO_YOGA_NODE(mNode), YGDirection::YGDirectionLTR);
            break;
        case kLayoutDirectionRTL:
            YGNodeStyleSetDirection(TO_YOGA_NODE(mNode), YGDirection::YGDirectionRTL);
            break;
        case kLayoutDirectionInherit: // FALL_THROUGH
        default:
            YGNodeStyleSetDirection(TO_YOGA_NODE(mNode), YGDirection::YGDirectionInherit);
    }
}

void
YogaNode::setMargin(Edge edge, float value)
{
    YGNodeStyleSetMargin(TO_YOGA_NODE(mNode), EDGE_LOOKUP[edgeAsInt(edge)], value);
}

void
YogaNode::setOverflowScroll()
{
    YGNodeStyleSetOverflow(TO_YOGA_NODE(mNode), YGOverflowScroll);
}

void
YogaNode::setSpacing(float value, bool skip0)
{
    auto* component = getComponent();
    auto parentNode = getParent();
    auto parentComponent = component->getParentIfInDocument();
    if (!parentNode || !parentComponent)
        return;

    if (skip0 && parentNode->getChild(0)->mNode == mNode)
        return;

    auto parentLayoutDirection = parentComponent->getCalculated(kPropertyLayoutDirection);
    auto dir = parentNode->getFlexDirection();
    Edge edge = Edge::Left;
    switch (dir) {
        case kContainerDirectionColumn:        edge = Edge::Top;    break;
        case kContainerDirectionColumnReverse: edge = Edge::Bottom; break;
        case kContainerDirectionRow:
            edge = (parentLayoutDirection == kLayoutDirectionLTR) ? Edge::Left : Edge::Right;
            break;
        case kContainerDirectionRowReverse:
            edge = (parentLayoutDirection == kLayoutDirectionLTR) ? Edge::Right : Edge::Left;
            break;
    }

    float currentValue = getMargin(edge);
    if ((std::isnan(currentValue) && value != 0) || std::abs(currentValue - value) > 0.1) {
        setMargin(Edge::Top,    0);
        setMargin(Edge::Bottom, 0);
        setMargin(Edge::Left,   0);
        setMargin(Edge::Right,  0);
        setMargin(edge, value);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

float 
YogaNode::getBorder(Edge edge) const 
{
    return YGNodeLayoutGetBorder(TO_YOGA_NODE(mNode), EDGE_LOOKUP[edgeAsInt(edge)]);
}

float 
YogaNode::getPadding(Edge edge) const 
{
    return YGNodeLayoutGetPadding(TO_YOGA_NODE(mNode), EDGE_LOOKUP[edgeAsInt(edge)]);
}

float 
YogaNode::getMargin(Edge edge) const 
{
    return YGNodeStyleGetMargin(TO_YOGA_NODE(mNode), EDGE_LOOKUP[edgeAsInt(edge)]).value;
}

float 
YogaNode::getWidth() const 
{
    return YGNodeLayoutGetWidth(TO_YOGA_NODE(mNode));
}

bool 
YogaNode::isAutoWidth() const 
{
    return (YGNodeStyleGetWidth(TO_YOGA_NODE(mNode)) == YGValueAuto);
}

bool 
YogaNode::isAbsoluteWidth() const 
{
    return (YGNodeStyleGetWidth(TO_YOGA_NODE(mNode)).unit == YGUnit::YGUnitPoint);
}

float 
YogaNode::getMinWidth() const 
{
    return YGNodeStyleGetMinWidth(TO_YOGA_NODE(mNode)).value;
}

float 
YogaNode::getMaxWidth() const 
{
    return YGNodeStyleGetMaxWidth(TO_YOGA_NODE(mNode)).value;
}

float 
YogaNode::getHeight() const 
{
    return YGNodeLayoutGetHeight(TO_YOGA_NODE(mNode));
}

bool 
YogaNode::isAutoHeight() const 
{
    return (YGNodeStyleGetHeight(TO_YOGA_NODE(mNode)) == YGValueAuto);
}

bool 
YogaNode::isAbsoluteHeight() const 
{
    return (YGNodeStyleGetHeight(TO_YOGA_NODE(mNode)).unit == YGUnit::YGUnitPoint);
}

float 
YogaNode::getMinHeight() const 
{
    return YGNodeStyleGetMinHeight(TO_YOGA_NODE(mNode)).value;
}

float 
YogaNode::getMaxHeight() const 
{
    return YGNodeStyleGetMaxHeight(TO_YOGA_NODE(mNode)).value;
}

float 
YogaNode::getLeft() const 
{
    return YGNodeLayoutGetLeft(TO_YOGA_NODE(mNode));
}

float 
YogaNode::getTop() const 
{
    return YGNodeLayoutGetTop(TO_YOGA_NODE(mNode));
}

LayoutDirection 
YogaNode::getLayoutDirection() const 
{
    return sDirectionToYGDirection.at(YGNodeStyleGetDirection(TO_YOGA_NODE(mNode)));
}

ContainerDirection 
YogaNode::getFlexDirection() const 
{
    return sFlexDirectionToYGDirection.at(YGNodeStyleGetFlexDirection(TO_YOGA_NODE(mNode)));
}

streamer& operator<<(streamer& os, YGValue&& value) {
    switch (value.unit) {
        case YGUnitUndefined:
            os << "undefined";
            break;
        case YGUnitAuto:
            os << "auto";
            break;
        case YGUnitPercent:
            os << value.value<< "%";
            break;
        case YGUnitPoint:
            os << value.value;
            break;
    }
    return os;
}

inline void
dumpYogaInternal(streamer& out, YGNodeRef node, int indent=0) {
    auto i = std::string(indent, ' ');
    out << i << "##### Type #### " << YGNodeTypeToString(YGNodeGetNodeType(node)) << "\n";
    out << i << "Is Dirty........" << YGNodeIsDirty(node) << "\n";
    out << i << "Direction......." << YGDirectionToString(YGNodeStyleGetDirection(node)) << "\n";
    out << i << "Flex Direction.." << YGFlexDirectionToString(YGNodeStyleGetFlexDirection(node)) << "\n";
    out << i << "Justify Content." << YGJustifyToString(YGNodeStyleGetJustifyContent(node)) << "\n";
    out << i << "Align Content..." << YGAlignToString(YGNodeStyleGetAlignContent(node)) << "\n";
    out << i << "Align Items....." << YGAlignToString(YGNodeStyleGetAlignItems(node)) << "\n";
    out << i << "Align Self......" << YGAlignToString(YGNodeStyleGetAlignSelf(node)) << "\n";
    out << i << "Position Type..." << YGPositionTypeToString(YGNodeStyleGetPositionType(node)) << "\n";
    out << i << "Flex Wrap......." << YGWrapToString(YGNodeStyleGetFlexWrap(node)) << "\n";
    out << i << "Overflow........" << YGOverflowToString(YGNodeStyleGetOverflow(node)) << "\n";
    out << i << "Display........." << YGDisplayToString(YGNodeStyleGetDisplay(node)) << "\n";
    out << i << "Flex............" << YGNodeStyleGetFlex(node) << "\n";
    out << i << "Flex Grow......." << YGNodeStyleGetFlexGrow(node) << "\n";
    out << i << "Flex Shrink....." << YGNodeStyleGetFlexShrink(node) << "\n";
    out << i << "Width..........." << YGNodeStyleGetWidth(node) << "\n";
    out << i << "Height.........." << YGNodeStyleGetHeight(node) << "\n";
    out << i << "MaxWidth........" << YGNodeStyleGetMaxWidth(node) << "\n";
    out << i << "MaxHeight......." << YGNodeStyleGetMaxHeight(node) << "\n";
    out << i << "MinWidth........" << YGNodeStyleGetMinWidth(node) << "\n";
    out << i << "MinHeight......." << YGNodeStyleGetMinHeight(node) << "\n";

    auto child_count = YGNodeGetChildCount(node);
    for (size_t index = 0; index < child_count; index++)
        dumpYogaInternal(out, YGNodeGetChild(node, index), indent + 4);
}

std::string
YogaNode::toDebugString() const
{
    auto out = streamer();
    dumpYogaInternal(out, TO_YOGA_NODE(mNode));
    return out.str();
}

streamer&
operator<<(streamer& os, const YogaNode& node)
{
    os << node.mNode;
    return os;
}

} // namespace apl
