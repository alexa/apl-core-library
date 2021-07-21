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

#include "apl/component/componentpropdef.h"
#include "apl/component/sequencecomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"

namespace apl {

CoreComponentPtr
SequenceComponent::create(const ContextPtr& context,
                          Properties&& properties,
                          const Path& path)
{
    auto ptr = std::make_shared<SequenceComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

SequenceComponent::SequenceComponent(const ContextPtr& context,
                                     Properties&& properties,
                                     const Path& path)
    : MultiChildScrollableComponent(context, std::move(properties), path) {}

const ComponentPropDefSet&
SequenceComponent::propDefSet() const {
    static ComponentPropDefSet sSequenceComponentProperties(MultiChildScrollableComponent::propDefSet(), {
            {kPropertyFastScrollScale, 1.0,                      asNonNegativeNumber,  kPropInOut | kPropStyled},
            {kPropertyScrollAnimation, kScrollAnimationDefault,  sScrollAnimationMap,  kPropInOut | kPropStyled},
            {kPropertyScrollDirection, kScrollDirectionVertical, sScrollDirectionMap,  kPropInOut | kPropVisualContext,
                                                                                       yn::setScrollDirection}
    });
    return sSequenceComponentProperties;
}

const ComponentPropDefSet*
SequenceComponent::layoutPropDefSet() const {
    static ComponentPropDefSet sSequenceChildProperties = ComponentPropDefSet().add( {
        { kPropertyNumbering, kNumberingNormal, sNumberingMap,       kPropIn },
        { kPropertySpacing,   Dimension(0),     asAbsoluteDimension, kPropIn |
                                                                     kPropNeedsNode |
                                                                     kPropResetOnRemove |
                                                                     kPropDynamic |
                                                                     kPropStyled, yn::setSpacing }
    });

    return &sSequenceChildProperties;
}

size_t
SequenceComponent::estimateChildrenToCover(float distance, size_t baseChild)
{
    auto size = mChildren.at(baseChild)->getCalculated(kPropertyBounds).getRect();
    auto vertical = isVertical();
    return std::ceil(std::abs(distance) / (vertical ? size.getHeight() : size.getWidth()));
}

} // namespace apl
