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

#include "apl/component/yogaproperties.h"
#include "apl/component/corecomponent.h"
#include "apl/primitives/dimension.h"
#include "apl/primitives/object.h"
#include "apl/utils/log.h"

namespace apl {
namespace yn {

const static bool DEBUG_FLEXBOX = false;

void
setPropertyGrow(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << value << " [" << nodeRef << "]";
    YGNodeStyleSetFlexGrow(nodeRef, value.asNumber());
}

void
setPropertyShrink(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << value << " [" << nodeRef << "]";
    YGNodeStyleSetFlexShrink(nodeRef, value.asNumber());
}

void
setPositionType(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << value << " [" << nodeRef << "]";
    auto positionType = static_cast<Position>(value.asInt());
    if (positionType == kPositionRelative || positionType == kPositionSticky)
        YGNodeStyleSetPositionType(nodeRef, YGPositionTypeRelative);
    else if (positionType == kPositionAbsolute)
        YGNodeStyleSetPositionType(nodeRef, YGPositionTypeAbsolute);
}

void
setWidth(YGNodeRef nodeRef, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX) << value << " [" << nodeRef << "]";
    if (value.isNull())
        return;

    Dimension width = value.asDimension(context);
    if (width.isRelative())
        YGNodeStyleSetWidthPercent(nodeRef, width.getValue());
    else if (width.isAuto())
        YGNodeStyleSetWidthAuto(nodeRef);
    else if (width.isAbsolute())
        YGNodeStyleSetWidth(nodeRef, width.getValue());
}

void
setMinWidth(YGNodeRef nodeRef, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX) << value << " [" << nodeRef << "]";
    if (value.isNull())
        return;

    Dimension width = value.asDimension(context);
    if (width.isRelative())
        YGNodeStyleSetMinWidthPercent(nodeRef, width.getValue());
    else if (width.isAbsolute())
        YGNodeStyleSetMinWidth(nodeRef, width.getValue());
}

void
setMaxWidth(YGNodeRef nodeRef, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX) << value << " [" << nodeRef << "]";
    if (value.isNull())
        return;

    Dimension width = value.asDimension(context);
    if (width.isRelative())
        YGNodeStyleSetMaxWidthPercent(nodeRef, width.getValue());
    else if (width.isAbsolute())
        YGNodeStyleSetMaxWidth(nodeRef, width.getValue());
}

void
setHeight(YGNodeRef nodeRef, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX) << value << " [" << nodeRef << "]";
    if (value.isNull())
        return;

    Dimension height = value.asDimension(context);
    if (height.isRelative())
        YGNodeStyleSetHeightPercent(nodeRef, height.getValue());
    else if (height.isAuto())
        YGNodeStyleSetHeightAuto(nodeRef);
    else if (height.isAbsolute())
        YGNodeStyleSetHeight(nodeRef, height.getValue());
}

void
setMinHeight(YGNodeRef nodeRef, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX) << value << " [" << nodeRef << "]";
    if (value.isNull())
        return;

    Dimension height = value.asDimension(context);
    if (height.isRelative())
        YGNodeStyleSetMinHeightPercent(nodeRef, height.getValue());
    else if (height.isAbsolute())
        YGNodeStyleSetMinHeight(nodeRef, height.getValue());
}

void
setMaxHeight(YGNodeRef nodeRef, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX) << value << " [" << nodeRef << "]";
    if (value.isNull())
        return;

    Dimension height = value.asDimension(context);
    if (height.isRelative())
        YGNodeStyleSetMaxHeightPercent(nodeRef, height.getValue());
    else if (height.isAbsolute())
        YGNodeStyleSetMaxHeight(nodeRef, height.getValue());
}

static const std::array<std::string, 9> sEdgeToString = {
    "Left",       // YGEdgeLeft,
    "Top",        // YGEdgeTop,
    "Right",      // YGEdgeRight,
    "Bottom",     // YGEdgeBottom,
    "Start",      // YGEdgeStart,
    "End",        // YGEdgeEnd,
    "Horizontal", // YGEdgeHorizontal,
    "Vertical",   // YGEdgeVertical,
    "All",        // YGEdgeAll,
};

void setPadding(YGNodeRef nodeRef, YGEdge edge, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX) << sEdgeToString[edge] << "->" << value << " [" << nodeRef << "]";
    Dimension padding = value.asDimension(context);
    if (padding.isRelative())
        YGNodeStyleSetPaddingPercent(nodeRef, edge, padding.getValue());
    else if (padding.isAbsolute())
        YGNodeStyleSetPadding(nodeRef, edge, padding.getValue());
}

void setBorder(YGNodeRef nodeRef, YGEdge edge, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX) << sEdgeToString[edge] << "->" << value << " [" << nodeRef << "]";
    Dimension border = value.asDimension(context);
    if (border.isAbsolute())
        YGNodeStyleSetBorder(nodeRef, edge, border.getValue());
}

void setPosition(YGNodeRef nodeRef, YGEdge edge, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX) << sEdgeToString[edge] << "->" << value << " [" << nodeRef << "]";

    CoreComponent *component = static_cast<CoreComponent*>(nodeRef->getContext());
    if (component && component->getCalculated(kPropertyPosition) == kPositionSticky) {
        YGNodeStyleSetPosition(nodeRef, edge, NAN);
        return;
    }

    Dimension position = value.asDimension(context);
    if (position.isRelative())
        YGNodeStyleSetPositionPercent(nodeRef, edge, position.getValue());
    else if (value.isNull() || value.isAutoDimension())
        YGNodeStyleSetPosition(nodeRef, edge, NAN);
    else if (position.isAbsolute())
        YGNodeStyleSetPosition(nodeRef, edge, position.getValue());
}

void
setFlexDirection(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << sScrollDirectionMap.at(value.asInt()) << " [" << nodeRef << "]";
    auto flexDirection = static_cast<ContainerDirection>(value.asInt());
    switch (flexDirection) {
        case kContainerDirectionColumn:
            YGNodeStyleSetFlexDirection(nodeRef, YGFlexDirectionColumn);
            break;
        case kContainerDirectionRow:
            YGNodeStyleSetFlexDirection(nodeRef, YGFlexDirectionRow);
            break;
        case kContainerDirectionColumnReverse:
            YGNodeStyleSetFlexDirection(nodeRef, YGFlexDirectionColumnReverse);
            break;
        case kContainerDirectionRowReverse:
            YGNodeStyleSetFlexDirection(nodeRef, YGFlexDirectionRowReverse);
            break;
    }
}

// Note: In the future if we allow Container to change layout direction, we'll need to reset all the margins carefully.
void
setSpacing(YGNodeRef nodeRef, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX) << value << " [" << nodeRef << "]";

    CoreComponent *component = static_cast<CoreComponent*>(nodeRef->getContext());
    auto spacing = value.asDimension(context);
    if (spacing.isAbsolute()) {
        YGNodeRef parent = YGNodeGetParent(nodeRef);
        if (!parent || YGNodeGetChild(parent, 0) == nodeRef)
            return;

        auto parentLayoutDirection = component->getParent()->getCalculated(kPropertyLayoutDirection);
        auto dir = YGNodeStyleGetFlexDirection(parent);
        YGEdge edge = YGEdgeLeft;
        switch (dir) {
            case YGFlexDirectionColumn:        edge = YGEdgeTop;    break;
            case YGFlexDirectionColumnReverse: edge = YGEdgeBottom; break;
            case YGFlexDirectionRow:
                edge = (parentLayoutDirection == kLayoutDirectionLTR) ? YGEdgeLeft : YGEdgeRight;
                break;
            case YGFlexDirectionRowReverse:
                edge = (parentLayoutDirection == kLayoutDirectionLTR) ? YGEdgeRight : YGEdgeLeft;
                break;
        }
        YGNodeStyleSetMargin(nodeRef, edge, spacing.getValue());
    }
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
setJustifyContent(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << sFlexboxJustifyContentMap.at(value.asInt()) << " [" << nodeRef << "]";
    auto justify = static_cast<FlexboxJustifyContent>(value.asInt());
    if (justify >= kFlexboxJustifyContentStart && justify <= kFlexboxJustifyContentSpaceAround)
        YGNodeStyleSetJustifyContent(nodeRef, JUSTIFY_LOOKUP[justify]);
}

// This must be kept in synch with componentproperties.h FlexboxWrap
static const YGWrap WRAP_LOOKUP[] = {
    YGWrapNoWrap,
    YGWrapWrap,
    YGWrapWrapReverse,
};

void
setWrap(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << sFlexboxWrapMap.at(value.asInt()) << " [" << nodeRef << "]";
    auto wrap = static_cast<FlexboxWrap>(value.asInt());
    if (wrap >= kFlexboxWrapNoWrap && wrap <= kFlexboxWrapWrapReverse)
        YGNodeStyleSetFlexWrap(nodeRef, WRAP_LOOKUP[wrap]);
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
setAlignSelf(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << sFlexboxAlignMap.at(value.asInt()) << " [" << nodeRef << "]";
    auto alignSelf = static_cast<FlexboxAlign>(value.asInt());
    if (alignSelf >= kFlexboxAlignStretch && alignSelf <= kFlexboxAlignAuto)
        YGNodeStyleSetAlignSelf(nodeRef, ALIGN_LOOKUP[alignSelf]);
}

void
setAlignItems(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << sFlexboxAlignMap.at(value.asInt()) << " [" << nodeRef << "]";
    auto alignItems = static_cast<FlexboxAlign>(value.asInt());
    if (alignItems >= kFlexboxAlignStretch && alignItems <= kFlexboxAlignAuto)
        YGNodeStyleSetAlignItems(nodeRef, ALIGN_LOOKUP[alignItems]);
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
setScrollDirection(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << sScrollDirectionMap.at(value.asInt()) << " [" << nodeRef << "]";
    auto scrollDirection = static_cast<ScrollDirection>(value.asInt());
    YGNodeStyleSetFlexDirection(nodeRef, scrollDirectionLookup(scrollDirection));
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
setGridScrollDirection(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << sScrollDirectionMap.at(value.asInt()) << " [" << nodeRef << "]";
    auto scrollDirection = static_cast<ScrollDirection>(value.asInt());
    YGNodeStyleSetFlexDirection(nodeRef, gridScrollDirectionLookup(scrollDirection));
}

void
setDisplay(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << sDisplayMap.at(value.asInt()) << " [" << nodeRef << "]";
    auto display = static_cast<Display>(value.asInt());
    YGNodeStyleSetDisplay(nodeRef, display == kDisplayNone ? YGDisplayNone : YGDisplayFlex);
}

void
setLayoutDirection(YGNodeRef nodeRef, const Object& value, const Context&) {
    LOG_IF(DEBUG_FLEXBOX) << sLayoutDirectionMap.at(value.asInt()) << " [" << nodeRef << "]";
    auto layoutDirection = static_cast<LayoutDirection>(value.asInt());
    switch (layoutDirection) {
        case kLayoutDirectionLTR:
            YGNodeStyleSetDirection(nodeRef, YGDirection::YGDirectionLTR);
            break;
        case kLayoutDirectionRTL:
            YGNodeStyleSetDirection(nodeRef, YGDirection::YGDirectionRTL);
            break;
        case kLayoutDirectionInherit: // FALL_THROUGH
        default:
            YGNodeStyleSetDirection(nodeRef, YGDirection::YGDirectionInherit);
    }
}

} // namespace yn
} // namespace apl
