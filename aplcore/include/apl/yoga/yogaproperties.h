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

#include "apl/component/componentproperties.h"
#include "apl/engine/propertymap.h"
#include "apl/yoga/yoganode.h"

namespace apl {

class Object;
class Context;

namespace yn {

extern void setPropertyGrow(YogaNode& node, const Object& value, const Context&);
extern void setPropertyShrink(YogaNode& node, const Object& value, const Context&);
extern void setPositionType(YogaNode& node, const Object& value, const Context&);
extern void setWidth(YogaNode& node, const Object& value, const Context& context);
extern void setMinWidth(YogaNode& node, const Object& value, const Context& context);
extern void setMaxWidth(YogaNode& node, const Object& value, const Context& context);
extern void setHeight(YogaNode& node, const Object& value, const Context& context);
extern void setMinHeight(YogaNode& node, const Object& value, const Context& context);
extern void setMaxHeight(YogaNode& node, const Object& value, const Context& context);
extern void setPadding(YogaNode& node, Edge edge, const Object& value, const Context& context);
extern void setBorder(YogaNode& node, Edge edge, const Object& value, const Context& context);
extern void setPosition(YogaNode& node, Edge edge, const Object& value, const Context& context);
extern void setFlexDirection(YogaNode& node, const Object& value, const Context&);
extern void setSpacing(YogaNode& node, const Object& value, const Context& context);
extern void setJustifyContent(YogaNode& node, const Object& value, const Context&);
extern void setWrap(YogaNode& node, const Object& value, const Context&);
extern void setAlignSelf(YogaNode& node, const Object& value, const Context&);
extern void setAlignItems(YogaNode& node, const Object& value, const Context&);
extern void setScrollDirection(YogaNode& node, const Object& value, const Context&);
extern void setGridScrollDirection(YogaNode& node, const Object& value, const Context&);
extern void setDisplay(YogaNode& node, const Object& value, const Context&);
extern void setLayoutDirection(YogaNode& node, const Object& value, const Context&);

template<Edge edge>
void setBorder(YogaNode& node, const Object& value, const Context& context) {
    setBorder(node, edge, value, context);
}

template<Edge edge>
void setPosition(YogaNode& node, const Object& value, const Context& context) {
    setPosition(node, edge, value, context);
}

} // namespace yn
} // namespace apl


#endif // _APL_YOGA_PROPERTIES_H
