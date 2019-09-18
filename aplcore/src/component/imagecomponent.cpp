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
#include "apl/component/imagecomponent.h"
#include "apl/component/yogaproperties.h"

namespace apl {

CoreComponentPtr
ImageComponent::create(const ContextPtr& context,
                       Properties&& properties,
                       const std::string& path)
{
    auto ptr = std::make_shared<ImageComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

ImageComponent::ImageComponent(const ContextPtr& context,
                               Properties&& properties,
                               const std::string& path)
    : CoreComponent(context, std::move(properties), path)
{
}

const ComponentPropDefSet&
ImageComponent::propDefSet() const
{
    static ComponentPropDefSet sImageComponentProperties(CoreComponent::propDefSet(), {
        {kPropertyAlign,           kImageAlignCenter,        sAlignMap,           kPropInOut | kPropStyled}, // Doesn't match 1.0 spec
        {kPropertyBorderRadius,    Object::ZERO_ABS_DIMEN(), asAbsoluteDimension, kPropInOut | kPropStyled},
        {kPropertyFilters,         Object::EMPTY_ARRAY(),    asFilterArray,       kPropInOut },
        {kPropertyOverlayColor,    Color(),                  asColor,             kPropInOut | kPropStyled | kPropDynamic},
        {kPropertyOverlayGradient, Object::NULL_OBJECT(),    asGradient,          kPropInOut | kPropStyled},
        {kPropertyScale,           kImageScaleBestFit,       sScaleMap,           kPropInOut | kPropStyled},
        {kPropertySource,          "",                       asString,            kPropInOut | kPropDynamic}
    });

    return sImageComponentProperties;
}

std::shared_ptr<ObjectMap>
ImageComponent::getEventTargetProperties() const
{
    auto target = CoreComponent::getEventTargetProperties();
    target->emplace("source", mCalculated.get(kPropertySource));
    return target;
}

std::string
ImageComponent::getVisualContextType() {
    return getCalculated(kPropertySource).empty() ? VISUAL_CONTEXT_TYPE_EMPTY : VISUAL_CONTEXT_TYPE_GRAPHIC;
}

} // namespace apl