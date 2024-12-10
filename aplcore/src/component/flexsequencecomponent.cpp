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
#include "apl/component/flexsequencecomponent.h"

#include "apl/component/componentpropdef.h"
#include "apl/content/rootconfig.h"
#include "apl/yoga/yogaproperties.h"

namespace apl {

CoreComponentPtr
FlexSequenceComponent::create(const ContextPtr& context,
                          Properties&& properties,
                          const Path& path)
{
    auto ptr = std::make_shared<FlexSequenceComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

void
FlexSequenceComponent::initialize()
{
    MultiChildScrollableComponent::initialize();
    auto horizontal = isHorizontal();

    // Figure out if we need to adjust cross axis.
    if (mCalculated[horizontal ? kPropertyHeight : kPropertyWidth].isAutoDimension()) {
        if (horizontal) {
            setHeight(getContext()->getRootConfig().getDefaultComponentHeight(getType(), false));
        } else {
            setWidth(getContext()->getRootConfig().getDefaultComponentWidth(getType(), false));
        }
    }
}

const ComponentPropDefSet&
FlexSequenceComponent::propDefSet() const
{
    static ComponentPropDefSet sSequenceComponentProperties(MultiChildScrollableComponent::propDefSet(), {
         {kPropertyAlignItems,       kFlexboxAlignAxisStart,      sFlexboxAlignAxisMap, kPropIn | kPropStyled | kPropDynamic, yn::setJustifyContent },
         {kPropertyScrollDirection,  kScrollDirectionVertical,    sScrollDirectionMap,  kPropInOut | kPropVisualContext | kPropVisibility,
                                                                                                                              yn::setGridScrollDirection},
         {kPropertyWrap,             kFlexboxWrapWrap,            sFlexboxWrapMap,      kPropNone,                            yn::setWrap },
    });

    return sSequenceComponentProperties;
}

const ComponentPropDefSet*
FlexSequenceComponent::layoutPropDefSet() const {
    static ComponentPropDefSet sSequenceChildProperties = ComponentPropDefSet().add( {
         {kPropertyNumbering,    kNumberingNormal,    sNumberingMap,       kPropIn },
         {kPropertyGrow,         0,                   asNonNegativeNumber, kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setPropertyGrow },
         {kPropertyShrink,       0,                   asNonNegativeNumber, kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setPropertyShrink },
         {kPropertySpacing,      Dimension(0),        asAbsoluteDimension, kPropIn | kPropStyled | kPropDynamic | kPropNeedsNode | kPropResetOnRemove, yn::setSpacing }
    });

    return &sSequenceChildProperties;
}

} // namespace apl
