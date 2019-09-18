/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/component/componentpropdef.h"
#include "apl/component/framecomponent.h"
#include "apl/component/yogaproperties.h"

namespace apl {


static void checkBorderRadii(Component& component)
{
    auto& frame = static_cast<FrameComponent&>(component);
    frame.fixBorder(true);
}

CoreComponentPtr
FrameComponent::create(const ContextPtr& context,
                       Properties&& properties,
                       const std::string& path)
{
    auto ptr = std::make_shared<FrameComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

FrameComponent::FrameComponent(const ContextPtr& context,
                               Properties&& properties,
                               const std::string& path)
    : CoreComponent(context, std::move(properties), path)
{
    // TODO: Auto-sized Frame just wraps the children.  Fix this for ScrollView and other containers?
    YGNodeStyleSetAlignItems(mYGNodeRef, YGAlignFlexStart);
}

const ComponentPropDefSet&
FrameComponent::propDefSet() const
{
    static ComponentPropDefSet sFrameComponentProperties(CoreComponent::propDefSet(), {
        {kPropertyBackgroundColor,         Color(),               asColor,             kPropInOut |
                                                                                       kPropStyled |
                                                                                       kPropDynamic},
        {kPropertyBorderRadii,             Object::EMPTY_RADII(), nullptr,             kPropOut},
        {kPropertyBorderColor,             Color(),               asColor,             kPropInOut |
                                                                                       kPropStyled |
                                                                                       kPropDynamic},
        {kPropertyBorderWidth,             Dimension(0),          asAbsoluteDimension, kPropInOut |
                                                                                       kPropStyled, yn::setBorder<YGEdgeAll>},

        // These are input-only properties that trigger the calculation of the output properties
        {kPropertyBorderBottomLeftRadius,  Object::NULL_OBJECT(), asAbsoluteDimension, kPropIn | kPropStyled, checkBorderRadii },
        {kPropertyBorderBottomRightRadius, Object::NULL_OBJECT(), asAbsoluteDimension, kPropIn | kPropStyled, checkBorderRadii },
        {kPropertyBorderRadius,            Dimension(0),          asAbsoluteDimension, kPropIn | kPropStyled, checkBorderRadii },
        {kPropertyBorderTopLeftRadius,     Object::NULL_OBJECT(), asAbsoluteDimension, kPropIn | kPropStyled, checkBorderRadii },
        {kPropertyBorderTopRightRadius,    Object::NULL_OBJECT(), asAbsoluteDimension, kPropIn | kPropStyled, checkBorderRadii },
    });

    return sFrameComponentProperties;
}

void
FrameComponent::fixBorder(bool useDirtyFlag)
{
    static std::vector<std::pair<PropertyKey, Radii::Corner >> BORDER_PAIRS = {
        {kPropertyBorderBottomLeftRadius,  Radii::kBottomLeft},
        {kPropertyBorderBottomRightRadius, Radii::kBottomRight},
        {kPropertyBorderTopLeftRadius,     Radii::kTopLeft},
        {kPropertyBorderTopRightRadius,    Radii::kTopRight}
    };

    float r = mCalculated.get(kPropertyBorderRadius).asAbsoluteDimension(*mContext).getValue();
    std::array<float, 4> result = {r, r, r, r};

    for (const auto& p : BORDER_PAIRS) {
        Object radius = mCalculated.get(p.first);
        if (radius != Object::NULL_OBJECT())
            result[p.second] = radius.asAbsoluteDimension(*mContext).getValue();
    }

    Radii radii(std::move(result));

    if (radii != mCalculated.get(kPropertyBorderRadii).getRadii()) {
        mCalculated.set(kPropertyBorderRadii, Object(std::move(radii)));
        if (useDirtyFlag)
            setDirty(kPropertyBorderRadii);
    }
}

/*
 * Initial assignment of properties.  Don't set any dirty flags here; this
 * all should be running in the constructor.
 *
 * This method initializes the values of the border corners.
*/

void
FrameComponent::assignProperties(const ComponentPropDefSet& propDefSet)
{
    CoreComponent::assignProperties(propDefSet);
    fixBorder(false);
}

} // namespace apl