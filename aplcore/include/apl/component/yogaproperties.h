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

#ifndef _APL_YOGA_PROPERTIES_H
#define _APL_YOGA_PROPERTIES_H

#include <yoga/YGNode.h>

#include "componentproperties.h"

namespace apl {

class Object;
class Context;

namespace yn {

extern void setPropertyGrow(YGNodeRef nodeRef, const Object& value, const Context&);
extern void setPropertyShrink(YGNodeRef nodeRef, const Object& value, const Context&);
extern void setPositionType(YGNodeRef nodeRef, const Object& value, const Context&);
extern void setWidth(YGNodeRef nodeRef, const Object& value, const Context& context);
extern void setMinWidth(YGNodeRef nodeRef, const Object& value, const Context& context);
extern void setMaxWidth(YGNodeRef nodeRef, const Object& value, const Context& context);
extern void setHeight(YGNodeRef nodeRef, const Object& value, const Context& context);
extern void setMinHeight(YGNodeRef nodeRef, const Object& value, const Context& context);
extern void setMaxHeight(YGNodeRef nodeRef, const Object& value, const Context& context);
extern void setPadding(YGNodeRef nodeRef, YGEdge edge, const Object& value, const Context& context);
extern void setBorder(YGNodeRef nodeRef, YGEdge edge, const Object& value, const Context& context);
extern void setPosition(YGNodeRef nodeRef, YGEdge edge, const Object& value, const Context& context);
extern void setFlexDirection(YGNodeRef nodeRef, const Object& value, const Context&);
extern void setSpacing(YGNodeRef nodeRef, const Object& value, const Context& context);
extern void setJustifyContent(YGNodeRef nodeRef, const Object& value, const Context&);
extern void setWrap(YGNodeRef nodeRef, const Object& value, const Context&);
extern void setAlignSelf(YGNodeRef nodeRef, const Object& value, const Context&);
extern void setAlignItems(YGNodeRef nodeRef, const Object& value, const Context&);
extern YGFlexDirection scrollDirectionLookup(ScrollDirection direction);
extern void setScrollDirection(YGNodeRef nodeRef, const Object& value, const Context&);
extern void setGridScrollDirection(YGNodeRef nodeRef, const Object& value, const Context&);
extern void setDisplay(YGNodeRef nodeRef, const Object& value, const Context&);
extern void setLayoutDirection(YGNodeRef nodeRef, const Object& value, const Context&);

template<YGEdge edge>
void setBorder(YGNodeRef nodeRef, const Object& value, const Context& context) {
    setBorder(nodeRef, edge, value, context);
}

template<YGEdge edge>
void setPosition(YGNodeRef nodeRef, const Object& value, const Context& context) {
    setPosition(nodeRef, edge, value, context);
}

} // namespace yn
} // namespace apl


#endif // _APL_YOGA_PROPERTIES_H
