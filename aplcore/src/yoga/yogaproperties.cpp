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

#include "apl/yoga/yogaproperties.h"

#include "apl/component/corecomponent.h"
#include "apl/primitives/dimension.h"
#include "apl/primitives/object.h"
#include "apl/utils/bimap.h"
#include "apl/utils/log.h"

namespace apl {
namespace yn {

const static bool DEBUG_FLEXBOX = false;

void
setPropertyGrow(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << value << " [" << node << "]";
    node.setPropertyGrow(value.asNumber());
}

void
setPropertyShrink(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << value << " [" << node << "]";
    node.setPropertyShrink(value.asNumber());
}

void
setPositionType(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << value << " [" << node << "]";
    auto positionType = static_cast<Position>(value.asInt());
    node.setPositionType(positionType);
}

void
setWidth(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << value << " [" << node << "]";
    if (value.isNull())
        return;

    Dimension width = value.asDimension(context);
    if (width.isRelative())
        node.setWidthPercent(width.getValue());
    else if (width.isAuto())
        node.setWidthAuto();
    else if (width.isAbsolute())
        node.setWidth(width.getValue());
}

void
setMinWidth(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << value << " [" << node << "]";
    if (value.isNull())
        return;

    Dimension width = value.asDimension(context);
    if (width.isRelative())
        node.setMinWidthPercent(width.getValue());
    else if (width.isAbsolute())
        node.setMinWidth(width.getValue());
}

void
setMaxWidth(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << value << " [" << node << "]";
    if (value.isNull())
        return;

    Dimension width = value.asDimension(context);
    if (width.isRelative())
        node.setMaxWidthPercent(width.getValue());
    else if (width.isAbsolute())
        node.setMaxWidth(width.getValue());
}

void
setHeight(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << value << " [" << node << "]";
    if (value.isNull())
        return;

    Dimension height = value.asDimension(context);
    if (height.isRelative())
        node.setHeightPercent(height.getValue());
    else if (height.isAuto())
        node.setHeightAuto();
    else if (height.isAbsolute())
        node.setHeight(height.getValue());
}

void
setMinHeight(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << value << " [" << node << "]";
    if (value.isNull())
        return;

    Dimension height = value.asDimension(context);
    if (height.isRelative())
        node.setMinHeightPercent(height.getValue());
    else if (height.isAbsolute())
        node.setMinHeight(height.getValue());
}

void
setMaxHeight(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << value << " [" << node << "]";
    if (value.isNull())
        return;

    Dimension height = value.asDimension(context);
    if (height.isRelative())
        node.setMaxHeightPercent(height.getValue());
    else if (height.isAbsolute())
        node.setMaxHeight(height.getValue());
}

static const std::map<Edge, std::string> sEdgeToString = {
    {Edge::Left, "Left"},
    {Edge::Top, "Top"},
    {Edge::Right, "Right"},
    {Edge::Bottom, "Bottom"},
    {Edge::Start, "Start"},
    {Edge::End, "End"},
    {Edge::Horizontal, "Horizontal"},
    {Edge::Vertical, "Vertical"},
    {Edge::All, "All"},
};

void setPadding(YogaNode& node, Edge edge, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sEdgeToString.at(edge) << "->" << value << " [" << node << "]";
    Dimension padding = value.asDimension(context);
    if (padding.isRelative())
        node.setPaddingPercent(edge, padding.getValue());
    else if (padding.isAbsolute())
        node.setPadding(edge, padding.getValue());
}

void setBorder(YogaNode& node, Edge edge, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sEdgeToString.at(edge) << "->" << value << " [" << node << "]";
    Dimension border = value.asDimension(context);
    if (border.isAbsolute())
        node.setBorder(edge, border.getValue());
}

void setPosition(YogaNode& node, Edge edge, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sEdgeToString.at(edge) << "->" << value << " [" << node << "]";

    CoreComponent *component = node.getComponent();
    if (component && component->getCalculated(kPropertyPosition) == kPositionSticky) {
        node.setPosition(edge, NAN);
        return;
    }

    Dimension position = value.asDimension(context);
    if (position.isRelative())
        node.setPositionPercent(edge, position.getValue());
    else if (value.isNull() || value.isAutoDimension())
        node.setPosition(edge, NAN);
    else if (position.isAbsolute())
        node.setPosition(edge, position.getValue());
}

void
setFlexDirection(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sContainerDirectionMap.at(value.asInt()) << " [" << node << "]";
    auto flexDirection = static_cast<ContainerDirection>(value.asInt());
    if (flexDirection >= kContainerDirectionColumn && flexDirection <= kContainerDirectionRowReverse)
        node.setFlexDirection(flexDirection);
}

// Note: In the future if we allow Container to change layout direction, we'll need to reset all the margins carefully.
void
setSpacing(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << value << " [" << node << "]";
    auto spacing = value.asDimension(context);
    if (!spacing.isAbsolute()) return;
    node.setSpacing(spacing.getValue(), true);
}

void
setJustifyContent(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sFlexboxJustifyContentMap.at(value.asInt()) << " [" << node << "]";
    auto justify = static_cast<FlexboxJustifyContent>(value.asInt());
    if (justify >= kFlexboxJustifyContentStart && justify <= kFlexboxJustifyContentSpaceAround)
        node.setJustifyContent(justify);
}

void
setWrap(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sFlexboxWrapMap.at(value.asInt()) << " [" << node << "]";
    auto wrap = static_cast<FlexboxWrap>(value.asInt());
    if (wrap >= kFlexboxWrapNoWrap && wrap <= kFlexboxWrapWrapReverse)
        node.setWrap(wrap);
}

void
setAlignSelf(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sFlexboxAlignMap.at(value.asInt()) << " [" << node << "]";
    node.setAlignSelf(static_cast<FlexboxAlign>(value.asInt()));
}

void
setAlignItems(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sFlexboxAlignMap.at(value.asInt()) << " [" << node << "]";
    node.setAlignItems(static_cast<FlexboxAlign>(value.asInt()));
}

void
setScrollDirection(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sScrollDirectionMap.at(value.asInt()) << " [" << node << "]";
    node.setScrollDirection(static_cast<ScrollDirection>(value.asInt()));
}

void
setGridScrollDirection(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sScrollDirectionMap.at(value.asInt()) << " [" << node << "]";
    node.setGridScrollDirection(static_cast<ScrollDirection>(value.asInt()));
}

void
setDisplay(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sDisplayMap.at(value.asInt()) << " [" << node << "]";
    node.setDisplay(static_cast<Display>(value.asInt()));
}

void
setLayoutDirection(YogaNode& node, const Object& value, const Context& context) {
    LOG_IF(DEBUG_FLEXBOX).session(context) << sLayoutDirectionMap.at(value.asInt()) << " [" << node << "]";
    node.setLayoutDirection(static_cast<LayoutDirection>(value.asInt()));
}

} // namespace yn
} // namespace apl
